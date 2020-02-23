#!/usr/bin/python
# -*- coding: utf-8 -*-

import time, sys, os, pymongo , logging
from kill_chunk import kill_chunk
from kill_chunk import kill_chunk_and_update_watch_dog
from kill_chunk import restart_watch_dog
from start_new_chunk import start_new_chunk
#from md5_test_repair import Repair
from prepare_repair import pre_repair
from start_repair import start_repair
from check_repair_done import check_repair_done
from insert_new_chunk_metaserver import SendInsertRequest
from fetch_chunk_id import fetch_chunk_id
from repair_check_mt import RepairCheck
from fetch_primary_chunk import fetch_primary_chunk
sys.path.append(os.path.abspath(os.path.join(__file__, '..', './..')))
import config

chunk_bin = "/root/udisk/chunk/chunk"
config_dir = "/root/udisk/chunk/conf"
#md5_test_count
total = 1
count = 0
bad_chunk_id = 9999
new_chunk_id = 9999
chunk_ip = "" 

def FetchPgInfo(mongo_ip, mongo_port, bad_chunk_id):
    db = pymongo.MongoClient(mongo_ip, mongo_port)
    udisk = db["udisk"]
    t_pg_info = udisk["t_pg_info"]
    bad_pgs = t_pg_info.find({"$or": [{"chunk_0_id": bad_chunk_id}, {"chunk_1_id": bad_chunk_id}, {"chunk_2_id": bad_chunk_id}]})
    bad_pg_list = []
    for pg in bad_pgs:
        bad_pg_list.append(pg["id"])
    print bad_pg_list
    return bad_pg_list

#def InitPG(bad_chunk_ip, bad_chunk_id, chunk_path):
#    cmd = "ssh root@%s rm -rf %s/data/*" % (bad_chunk_ip, chunk_path)
#    os.system(cmd)
#    bad_pg_list = FetchPgInfo(config.mongo_ip, config.mongo_port, bad_chunk_id)
#    for pg in bad_pg_list:
#        cmd = "ssh root@%s mkdir %s/data/%d " % (bad_chunk_ip, chunk_path, pg)
#        os.system(cmd)

#def DD(bad_chunk_ip, config.capacity, chunk_path):
#    cmd = "ssh root@%s sh /root/init_pc_pool.sh %d 33554432 %s/pool" % (bad_chunk_ip, config.capacity, chunk_path)
#    os.system(cmd)

def GetDevName(ip, chunk_id):
    config.test_mode = 1
    if config.test_mode == 1:
        #get nvme addr as devname
        cmd = "ssh root@" + ip + " grep -n 'nvme_disk_addr' /root/udisk/chunk/conf/chunk-" + str(chunk_id) + ".conf" + " | cut -d '=' -f 2"
        addr = os.popen(cmd).read()
        addr = addr.replace("\n", "")
        addr = addr.replace(" ", "")
        return addr

    cmd="ssh root@" + ip + " lsblk | grep -E 'sd|nvme|df' | grep -v sda | awk '{print $1}'"
    print cmd
    dev_list = os.popen(cmd).readlines()
    print dev_list
    for dev in dev_list:
        dev = dev.replace("\n", "")
        print dev
        cmd="ssh root@" + ip + " /root/udisk/chunk/tools/raw_chunk_storage_tool dumpSuperBlock /dev/" + dev + " | grep chunk_id | awk '{print $2}'"
        print cmd
        cur_chunk_id = os.popen(cmd).read()
        cur_chunk_id = cur_chunk_id.replace("\n", "")
        if (cur_chunk_id == ""):
            continue
        print cur_chunk_id
        if (int(cur_chunk_id) == chunk_id):
            return dev
    return "NULL"

def FormatDev(ip, dev_name, chunk_id, bad_chunk_id):
    config.test_mode = 1
    if config.test_mode == 1:
        cmd = "ssh root@" + ip + " /root/udisk/chunk/tools/raw_chunk_storage_tool format " + str(chunk_id) + "-" + dev_name + " " + str(chunk_id) + " true true 4194304 true 0x200"
    else:
        cmd = "ssh root@" + ip + " /root/udisk/chunk/tools/raw_chunk_storage_tool format /dev/" + dev_name + " " + str(chunk_id) + " true true 4194304 true"
    print cmd
    os.system(cmd + "> /dev/null 2>&1")
    time.sleep(5) 
    #更新uuid到新配置文件
    if config.test_mode == 1:
        #update config.py with new chunk_id
        #for item in config.chunkserver_list:
        #    if item["ip"] == ip:
        #        for ele in item["chunk_list"]:
        #            if ele["id"] == bad_chunk_id:
        #                ele["id"] = chunk_id
        cmd = "ssh root@" + ip + " /root/udisk/chunk/tools/raw_chunk_storage_tool dumpSuperBlock " + str(chunk_id) + "-" + dev_name + " | grep 'uuid' | awk '{print $2}'"
    else:
        cmd = "ssh root@" + ip + " /root/udisk/chunk/tools/raw_chunk_storage_tool dumpSuperBlock /dev/" + dev_name + " | grep 'uuid' | awk '{print $2}'"
    uuid = os.popen(cmd).read()
    uuid = uuid.replace("\n", "")
    logging.info(uuid)
    cmd = "ssh root@" + ip + " grep -n 'device_uuid' /root/udisk/chunk/conf/chunk-" + str(chunk_id) + ".conf" + " | cut -d ':' -f 1"
    print cmd
    uuid_line = os.popen(cmd).read()
    uuid_line = uuid_line.replace("\n", "")
    print uuid_line
    config_file = "/root/udisk/chunk/conf/chunk-" + str(chunk_id) + ".conf"
    print config_file
    cmd = "ssh root@" + ip + " \"" + "sed -i " + "'" + uuid_line + "c device_uuid                        = " + uuid + "' " + config_file + "\""
    print cmd
    os.system(cmd)
    return uuid

def FormatNewChunk(chunk_ip, bad_chunk_id, new_chunk_id):
    #format dev
    dev_name = GetDevName(chunk_ip, bad_chunk_id)
    if (dev_name == "NULL"):
        logging.info("get devname failed")
        return False
    info = "dev_name:" + dev_name
    print (info)
    logging.info(dev_name)
    uuid = FormatDev(chunk_ip, dev_name, new_chunk_id, bad_chunk_id)
    info = "uuid:" + uuid
    print (info)
    logging.info(info)
    return True
  
def StartRepairTest(kill_primary, chunk_ip, bad_chunk_id, new_chunk_id, rackid):
    total = 1
    count = 0
    is_primary_killed = False
    repair_rc = {}
    repair_rc["success"] = False
    while count < total:
        count += 1
        print "md5_test ===> %d" % (count)
        
        if is_primary_killed == False:
            #0. 获取初始化bad_chunk_id 和 new_chunk_id
            #rc = fetch_chunk_id(config.mongo_ip, config.mongo_port)
            #bad_chunk_id = rc["bad_chunk_id"]
            #new_chunk_id = rc["new_chunk_id"]
            #chunk_ip = rc["bad_chunk_ip"]
            #rackid = rc["rack"]
            
            # rc = fetch_chunk_id(config.mongo_ip, config.mongo_port)
            # bad_chunk_id = rc["bad_chunk_id"]
            # new_chunk_id = rc["new_chunk_id"]
            # chunk_ip = rc["bad_chunk_ip"]
            # rackid = rc["rack"]

            # # bad_chunk_id = 38
            # # new_chunk_id = 52

            # bad_chunk_id = 39
            # new_chunk_id = 53

            # rackid = "rack-0"
            # chunk_ip = "10.189.149.69"

            info = "Init bad_chunk_id:%d bad_chunk_ip:%s new_chunk_id:%d complete" % (bad_chunk_id, chunk_ip, new_chunk_id)
            logging.info(info)
            repair_rc["new_chunk_id"] = new_chunk_id
        
            #5(a). 测试端指定杀修复主chunk
            if kill_primary == True:
                #设置total为2，使杀掉的主为下次修复的对象
                total = 2
                primary_chunk_info = fetch_primary_chunk(config.mongo_ip, config.mongo_port, bad_chunk_id, count)
                info = "count:%d ===> kill primary chunk id:%d, bad_chunk_id:%d, fixing_chunk_id:%d" % \
                    (count, primary_chunk_info["id"], bad_chunk_id, new_chunk_id)
                logging.info(info)
                logging.info("kill primary chunk complete")
                primary_new_chunk_id = new_chunk_id + 1 
                primary_bad_chunk_id = primary_chunk_info["id"]
                primary_ip = primary_chunk_info["ip"]
                is_primary_killed = True
            
            #1. 模拟磁盘损坏下线
            logging.info("chunk down")
            if kill_chunk_and_update_watch_dog(chunk_ip, bad_chunk_id) != 0:
                logging.error("chunk down faild")
                return repair_rc
            logging.info("chunk down complete")
        else :
            #(a)恢复上轮杀的主chunk
            bad_chunk_id = primary_bad_chunk_id
            new_chunk_id = primary_new_chunk_id
            chunk_ip = primary_ip
            is_primary_killed = False
            info = "primary chunk:%d fix up" % (bad_chunk_id)
            logging.info(info)
        
        time.sleep(120)
        
        #2. 重启新chunk, 修改配置，新chunk信息插入数据库
        logging.info("start chunk up new chunk")
        os.system("scp ./wiwo_python/raw_modify_config_v2.py root@%s:~" % chunk_ip)
        #os.system("scp init_pc_pool.sh root@%s:~" % chunk_ip)
        ipinfo = os.popen("ssh root@%s python raw_modify_config_v2.py %s %d %d" % (chunk_ip, config_dir, bad_chunk_id, new_chunk_id)).readlines()
        if len(ipinfo) != 8:
            print "didn't get ipinfo", ipinfo
            return repair_rc
        ip = ipinfo[0].strip()
        man_port = int(ipinfo[1].strip())
        io_port = int(ipinfo[2].strip())
        repair_port = int(ipinfo[3].strip())
        gate_io_listen_port = int(ipinfo[4].strip())
        chunk_io_listen_port = int(ipinfo[5].strip())
        ark_io_listen_port = int(ipinfo[6].strip())
        hela_io_listen_port = int(ipinfo[7].strip())
        #new_chunk_dir = ipinfo[4].strip()
        
        #先插入数据库chunk信息占位
        #if SendInsertRequest(config.metaserver_ip, config.metaserver_port, new_chunk_id, ip, man_port, io_port, repair_port, config.capacity, rackid) != 0:
        #    logging.error("chunk insert db failed, bad_chunk_id:%d, new_chunk_id:%d" % (bad_chunk_id, new_chunk_id))
        #    return repair_rc
        #info = "chunk insert db complete, bad_chunk_id:%d, new_chunk_id:%d" % (bad_chunk_id, new_chunk_id)
        #logging.info(info)
        
        #init pg
        #InitPG(chunk_ip, bad_chunk_id, new_chunk_dir)
        
        #dd
        #DD(chunk_ip, config.capacity, new_chunk_dir)

        #format dev
        if FormatNewChunk(chunk_ip, bad_chunk_id, new_chunk_id) == False:
          return 0

        #先插入数据库chunk信息占位, after formatdev because uuid is needed.
        if SendInsertRequest(config.metaserver_ip, config.metaserver_port, new_chunk_id, ip, man_port, io_port, gate_io_listen_port, chunk_io_listen_port, ark_io_listen_port, hela_io_listen_port, repair_port, config.capacity, rackid, uuid) != 0:
            logging.error("chunk insert db failed, bad_chunk_id:%d, new_chunk_id:%d" % (bad_chunk_id, new_chunk_id))
            return repair_rc
        info = "chunk insert db complete, bad_chunk_id:%d, new_chunk_id:%d" % (bad_chunk_id, new_chunk_id)
        logging.info(info)
 
        #sleep 5s for dd io buffer sync
        time.sleep(5)
        
        #############break point###########
        #chunk_ip = "172.18.5.93"
        #bad_chunk_id = 54
        #new_chunk_id = 81
        #ip = "172.18.5.93"
        #man_port = 13506
        #io_port = 13606
        #repair_port = 13706
        ##############break point###########

        #start new chunk
        restart_watch_dog(chunk_ip, new_chunk_id)
        time.sleep(60)
        #3. 发起修复准备
        # info = "start pre_repair ===> bad_chunk_id:%d, new_chunk_id:%d" % (bad_chunk_id, new_chunk_id)
        # logging.info(info)
        # loki_rc = pre_repair(bad_chunk_id, new_chunk_id, config.loki_ip, config.loki_port, config.mongo_ip, config.mongo_port)
        # if loki_rc.rc.retcode != 0:
        #     logging.error("start pre_repair failed:" + loki_rc.rc.error_message)
        #     return repair_rc
        # logging.info("start pre_repair complete")
        
        # #4. 启动修复流程
        # logging.info("start repair")
        # start_repair(new_chunk_id, config.loki_ip, config.loki_port)
        # logging.info("start repair complete async")
        # time.sleep(10)
        
        #5(b). 定期杀修复主chunk
        # if is_primary_killed == True:
        #     if kill_chunk(primary_ip, primary_bad_chunk_id) != 0:
        #         logging.error("kill primary chunk faild")
        #         return repair_rc
        #     Info = "primary chunk:%d kill complete" % primary_bad_chunk_id
        #     logging.info(info)
        
        #6. 检查db，修复是否完成
        # logging.info("check repair start")
        # retry = 0
        # while check_repair_done(config.mongo_ip, config.mongo_port, new_chunk_id) == False:
        #     retry += 1
        #     info = "md5_test:%d ===> repair retry:%d " % (count, retry)
        #     start_repair(new_chunk_id, config.loki_ip, config.loki_port)
        #     logging.info(info)
        #     if retry > 100:
        #         return repair_rc
        # info = "chunk-%d repair complete" % new_chunk_id
        # logging.info(info)
        # repair_rc["success"] = True
        # repair_rc["new_chunk_id"] = new_chunk_id
        # return repair_rc
        #time.sleep(3) 
        
        ##停止fio
        ##os.system("scp kill_process.sh root@%s:/root/sean/gate/" % gate_ip)
        #logging.info("kill fio")
        #os.system("ssh root@%s sh /root/md5_test/kill_process.sh md5_test_fio" % gate_ip)
        #time.sleep(10) 
        
        ##7. 通过md5验证修复
        #logging.info("check repair data with md5")
        #if RepairCheck(config.mongo_ip, config.mongo_port, new_chunk_id) != 0:
        #    logging.error("data inconsistency")
        #    sys.exit()
        #logging.info("md5 check complete")
        #time.sleep(3)
        
        ##重新启动fio
        #os.system("ssh root@%s /root/md5_test/gate/md5_test_fio -c /root/md5_test/gate/gate_fio.ini" % gate_ip)
        #logging.info("restart fio")
        
        #info = "md5_test ===> %d Finish\n\n" % count
        #logging.info(info)
        
        #time.sleep(60)
def TemporyRepair():
    new_chunk_id = 37
    bad_chunk_id = 1
    ip = "172.18.139.131"
    man_port = 13501
    io_port = 13601
    repair_port = 13701
    rackid = "rack-131"
    uuid = "743eae9b-fab9-4373-9a0f-178c9ef53ed0"
    chunk_ip = "172.18.139.131"
    repair_rc = {}
    repair_rc["success"] = False
 
    #先插入数据库chunk信息占位, after formatdev because uuid is needed.
    if SendInsertRequest(config.metaserver_ip, config.metaserver_port, new_chunk_id, ip, man_port, io_port, repair_port, config.capacity, rackid, uuid) != 0:
        logging.error("chunk insert db failed, bad_chunk_id:%d, new_chunk_id:%d" % (bad_chunk_id, new_chunk_id))
        return repair_rc
    info = "chunk insert db complete, bad_chunk_id:%d, new_chunk_id:%d" % (bad_chunk_id, new_chunk_id)
    logging.info(info)

    #sleep 5s for dd io buffer sync
    time.sleep(5)
    #start new chunk
    restart_watch_dog(chunk_ip, new_chunk_id)
    time.sleep(60)
    #3. 发起修复准备
    info = "start pre_repair ===> bad_chunk_id:%d, new_chunk_id:%d" % (bad_chunk_id, new_chunk_id)
    logging.info(info)
    loki_rc = pre_repair(bad_chunk_id, new_chunk_id, config.loki_ip, config.loki_port, config.mongo_ip, config.mongo_port)
    if loki_rc.rc.retcode != 0:
        logging.error("start pre_repair failed:" + loki_rc.rc.error_message)
        return repair_rc
    logging.info("start pre_repair complete")

    #4. 启动修复流程
    logging.info("start repair")
    start_repair(new_chunk_id, config.loki_ip, config.loki_port)
    logging.info("start repair complete async")
    time.sleep(10)

    #5(b). 定期杀修复主chunk

    #6. 检查db，修复是否完成
    logging.info("check repair start")
    retry = 0
    while check_repair_done(config.mongo_ip, config.mongo_port, new_chunk_id) == False:
        retry += 1
        info = "md5_test:%d ===> repair retry:%d " % (count, retry)
        start_repair(new_chunk_id, config.loki_ip, config.loki_port)
        logging.info(info)
        if retry > 100:
            return repair_rc
    info = "chunk-%d repair complete" % new_chunk_id
    logging.info(info)
    repair_rc["success"] = True
    repair_rc["new_chunk_id"] = new_chunk_id
    return repair_rc
 
def CheckRepairDone():
    db = pymongo.MongoClient(config.mongo_ip, config.mongo_port)
    udisk = db["udisk"]
    repair_chunk_task_job = udisk["t_chunk_repair_task_job"]
    pc_left = repair_chunk_task_job.find({'state':0}).count()
    info = "上次修复剩余: %d" % pc_left
    logging.info(info)
    
    return pc_left

def RepairTest():
    kill_primary = False
    rc = {}
    rc["success"] = False
    if CheckRepairDone() > 0:
        logging.info("上次修复测试未完成")
        return rc
    AutoTestMutiRssd()
    # return StartRepairTest(kill_primary)

def help():
    print " Usage: " + sys.argv[0] + " <是否启动杀主流程:ture,false>"

def main():
    if len(sys.argv[1:]) < 1:
        help()
        sys.exit()
    if TemporyRepair() == True:
        logging.info("repair success")
    #kill_primary = sys.argv[1]
    ##pc_left = CheckRepairDone()
    #pc_left = 0
    #logging.basicConfig(level = logging.INFO,\
    #                    filename = "raw_md5_test.log",\
    #                    filemode = "a+",\
    #                    format='%(asctime)s - %(levelname)s: %(message)s')
    #if pc_left > 0 :
    #    logging.info("上次修复测试未完成")
    #    sys.exit()
    #else:
    #    logging.info("启动修复测试")
    #    StartRepairTest(kill_primary)

def PreRepair(bad_id, new_id, loki_ip, loki_port, mongo_ip, mongo_port):
    #3. 发起修复准备
    info = "start pre_repair ===> bad_chunk_id:%d, new_chunk_id:%d" % (bad_id, new_id)
    logging.info(info)
    loki_rc = pre_repair(bad_id, new_id, loki_ip, loki_port, mongo_ip, mongo_port)
    if loki_rc.rc.retcode != 0:
        logging.error("start pre_repair failed:" + loki_rc.rc.error_message)
        return False
    logging.info("start pre_repair complete")
    return True

def StartRepair(new_id, loki_ip, loki_port):
    #4. 启动修复流程
    logging.info("start repair")
    start_repair(new_id, loki_ip, loki_port)
    logging.info("start repair complete async")
    time.sleep(10)

def CheckRepairChunkDone(mongo_ip, mongo_port, new_chunk_id, retry):
    if check_repair_done(mongo_ip, mongo_port, new_chunk_id) == False:
        info = "new_chunk_id %d md5_test:%d ===> repair retry:%d " % (new_chunk_id, count, retry)
        start_repair(new_chunk_id, loki_ip, loki_port)
        logging.info(info)
        return False
    else:
      info = "chunk-%d repair complete" % new_chunk_id
      logging.info(info)
      return True

def AutoTestMutiRssd():
    loki_ip = "10.189.151.129"
    loki_port = 32501
    mongo_ip = "10.189.151.126"
    mongo_port = 25001

    bad_chunk_id1 = 38
    new_chunk_id1 = 53

    # bad_chunk_id2 = 39
    # new_chunk_id2 = 54

    rackid = "rack-0"
    chunk_ip = "10.189.149.69"

    repair_rc = {}
    repair_rc["success"] = False

    # 多盘修复,修复同一IP上的chunk能保证三副本不下掉两副本
    StartRepairTest(False, chunk_ip, bad_chunk_id1, new_chunk_id1, rackid)
    success = PreRepair(bad_chunk_id1, new_chunk_id1, loki_ip, loki_port, mongo_ip, mongo_port)
    if success == False:
      sys.exit(1)
      return repair_rc
        #4. 启动修复流程
    StartRepair(new_chunk_id1, loki_ip, loki_port)

    # StartRepairTest(False, chunk_ip, bad_chunk_id2, new_chunk_id2, rackid)
    # success = PreRepair(bad_chunk_id2, new_chunk_id2, loki_ip, loki_port, mongo_ip, mongo_port)
    # if success == False:
    #   sys.exit(1) 
    #     #4. 启动修复流程
    # StartRepair(new_chunk_id2, loki_ip, loki_port)

    # # 重新启动前一次的修复,因为呗第二次的修复中断，修复version+1了
    # StartRepair(new_chunk_id1, loki_ip, loki_port)

    # chunk_ip = "10.189.149.69"
    # metaserver_ip = "10.189.149.69"
    # metaserver_port = 35300
    # man_port = 24006
    # io_port = 24007
    # repair_port = 24206
    # capacity = 6223139176448
    # rackid = "rack-1"
    # uuid = "906cad7d-3f89-41a8-affb-f6d9528fcc84"

    #先插入数据库chunk信息占位, after formatdev because uuid is needed.
    # if SendInsertRequest(metaserver_ip, metaserver_port, new_chunk_id, chunk_ip,
    #     man_port, io_port, repair_port, capacity, rackid, uuid) != 0:
    #     logging.error("chunk insert db failed, bad_chunk_id:%d, new_chunk_id:%d" % (bad_chunk_id, new_chunk_id))
    #     sys.exit(1)
    #     return repair_rc
    # info = "chunk insert db complete, bad_chunk_id:%d, new_chunk_id:%d" % (bad_chunk_id, new_chunk_id)
    # logging.info(info)

    #6. 检查db，修复是否完成
    logging.info("check repair start")
    retry = 0
    new_chunk_repair_finsh1 = False
    new_chunk_repair_finsh2 = False
    while (new_chunk_repair_finsh1 == False or new_chunk_repair_finsh2 == False):
        if new_chunk_repair_finsh1 == False:
          if CheckRepairChunkDone(new_chunk_id1, loki_ip, loki_port, retry) == True:
              new_chunk_repair_finsh1 = True

        if new_chunk_repair_finsh2 == False:
          if CheckRepairChunkDone(new_chunk_id2, loki_ip, loki_port, retry) == True:
              new_chunk_repair_finsh2 = True
        retry += 1
        if retry > 100:
            sys.exit(1)
    logging.info("check repair end")
    repair_rc["success"] = True
    return repair_rc

if __name__ == "__main__":
    # main()
    # AutoTestMutiRssd()
    bad_chunk_id1 = 38
    new_chunk_id1 = 53

    # bad_chunk_id2 = 39
    # new_chunk_id2 = 54

    chunk_ip = "10.189.149.69"

    FormatNewChunk(chunk_ip, bad_chunk_id, new_chunk_id)