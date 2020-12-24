
#!/usr/bin/python3
# -*- coding: utf-8 -*-
import os
import sys
import pymongo

def main(db_host, db_port, udisk_id):
  print("=====> Start to get clone task info")
  client = pymongo.MongoClient(db_host, db_port)
  db = client['udisk']
  coll_list = db.list_collection_names(session=None)
  print("===================== Start to debug coll info ==========================")
  for coll_item in coll_list:
    print(coll_item)
  coll = db['t_clone_task']
  coll_lc_info = db['t_lc_info']
  start_clone_task = coll.find_one({"dst_extern_id":udisk_id})
  print("\n")
  print("First clone task status: %d" % start_clone_task['status'])
  start_tick = start_clone_task['hela_info']['start_tick']
  # 1594973761
  # start_tick = 1594796471 
  # start_tick = 1594973761 - 100
  print("First clone task start_tick: %d" % start_tick)
  # print("First clone task done_tick: %d" % start_clone_task['hela_info']['done_tick'])
  print("First clone task end_tick: %d" % start_clone_task['hela_info']['end_tick'])
  print("Total success clone task count: " + str(coll.count_documents({"status": 2})))
  print("Total current all clone task count: " + 
  str(coll.count_documents({'hela_info.start_tick':{'$gte':start_tick}})))
  clone_success_count = coll.count_documents({"status": 2, 'hela_info.start_tick':{'$gte':start_tick}})
  print("Total current success clone task count: " + str(clone_success_count))
  print("\n")
  # { "_id": 0, "status": 1, "start_tick": 1, "done_tick": 1, "end_tick": 1}
  results = coll.find({"status": 2, 'hela_info.start_tick':{'$gte':start_tick}})
  clone_success_times_t1 = 0
  clone_success_times_t2 = 0
  print("=====> Start to debug clone task info")
  for result in results:
    udisk_id = result['dst_extern_id']
    lc_info = coll_lc_info.find_one({"extern_id":udisk_id})
    print("udisk_id: %s mount status: %d" % (udisk_id, lc_info['mount_status']))

    start = result['hela_info']['start_tick']
    done = result['hela_info']['done_tick']
    end = result['hela_info']['end_tick']
    # print("Current clone task udisk_id:%s start_tick: %d done_tick: %d end_tick: %d" % 
    # (udisk_id, start, done, end))
    clone_success_times_t1 = clone_success_times_t1 + done-start
    clone_success_times_t2= clone_success_times_t2 + end-start
    print("Cost time udisk_id:%s t1:%d t2:%d" % (udisk_id, done-start, end-start))
    print("\n")
    print("Cost time total t1:%d t2:%d" % (clone_success_times_t1, clone_success_times_t2))
    print("Cost time average t1:%d t2:%d" % 
    (clone_success_times_t1/clone_success_count, clone_success_times_t2/clone_success_count))
  print("========================= End to debug clone task info =============================")
  print("\n")
if __name__ == "__main__":
  # 第一个盘
  # main("172.27.24.98", 27019, "bsi-5xg22swl")
  # main("10.66.128.13", 20731, "bsi-dvvpumx5")
  # main("172.27.24.98", 27019, "bsi-uwx0t55m")
  main("172.27.39.115", 27035, "windows_hn02_bk_820")
