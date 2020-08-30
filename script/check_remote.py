#!/usr/bin/python
# -*- coding: utf-8 -*-

import paramiko
import sys
import os
# import pymongo
import subprocess
import re
import commands


def help():
    print("Usage: " + sys.argv[0] + " <region> <module[loki/buddy/metaserver/odin/umongo]>")

def write_result(f, set_id, ip, cnt):
  f.write(set_id + " " + ip + " " + str(cnt) + "\n")


def check_process_exist(self, name):
  cmd = 'ps -aux | grep -v grep | grep -w %s | egrep -v \"grep|vim|vi|less|bash\" | wc -l' % name
  status, output = commands.getstatusoutput(cmd)
  # 获取失败不算
  if status != 0:
    return  True
  process_num = int(output.split()[0].strip())
  if process_num > 0:
    return True
  else:
    return False

def get_process_cnt(ip):
  cmd = 'ssh root@%s \"ps -aux | grep -v grep | grep -w metaserver | grep -v grep | wc -l\"' % ip
  return int(os.popen(cmd).readlines()[0].strip())
  # try:
  #   ssh = paramiko.SSHClient()
  #   ssh.load_system_host_keys()
  #   # 指定本地的RSA私钥文件,如果建立密钥对时设置的有密码，password为设定的密码，如无不用指定password参数
  #   # pkey = paramiko.RSAKey.from_private_key_file('/home/super/.ssh/id_rsa', password='12345')
  #   private_key = paramiko.RSAKey.from_private_key_file('/data/tang.luo/.ssh/id_rsa')
  #   ssh.set_missing_host_key_policy(paramiko.AutoAddPolicy())
  #   # ssh.connect(hostname=ip, port=22, username='root', password='tang')
  #   ssh.connect(hostname=ip, port=22, username='root', pkey=private_key, timeout=1800)
  # except paramiko.ssh_exception.NoValidConnectionsError as e:
  #   print('...连接失败...')
  # except paramiko.ssh_exception.AuthenticationException as e:
  #   print('...密码错误...')
  # else:
  #   cmd = 'ps aux | grep -v grep | grep -w metaserver | wc -l'
  #   stdin, stdout, stderr = ssh.exec_command(cmd)
  #   return stdout.read().decode('utf-8')
  #   # print(result)
  # finally:
  #   ssh.close()


  # def exec(self, shell, timeout=1800):
  #     stdin, stdout, stderr = self.client.exec_command(command=shell, bufsize=1, timeout=timeout)
  #     while True:
  #         line = stdout.readline()
  #         if not line:
  #             break
  #         print(line)
  #     print(stderr.read())
  #     code = stdout.channel.recv_exit_status()
  #     return code

def main():
  if len(sys.argv[1:]) < 2:
      help()
      sys.exit()

  region = sys.argv[1]
  module_name = sys.argv[2]
  file_name = module_name + "_ips"
  module_ips = []
  region_dir = os.path.join("/data/tang.luo", "limax", "udisk",
                            "region", region)

  print(region_dir,module_name,file_name)

  f = open(file_name, 'w')

  for root, dirs, files in os.walk(region_dir):
    for set_id in dirs:
      vm_file = os.path.join(region_dir, set_id, "vm_info")
      print(vm_file)
      vm_info = open(vm_file).read().split("\n")
      for line in vm_info:
        line_map = line.split()
        if len(line_map) == 0:
          continue
        if line_map[0] == module_name:
          print(set_id + " " + line_map[1] + " ")
          write_result(f, set_id, line_map[1], get_process_cnt(line_map[1]));
          module_ips.append(line_map[1])
    break

  f.close()

if __name__ == "__main__":
  main()