#!/usr/bin/env python
# -*- coding: UTF-8 -*-

import requests
import json
import os
import sys
import re
reload(sys)
sys.setdefaultencoding( "utf-8" )

url = "http://api-gw.ucloudadmin.com/cmdb/v2/ci/search"
headers = {"api-key":"5dc51cdfb480409f896342d83ca6e4d1"}
params = {
  "category": "Server",
   "q": 'Usage==uhost;SecUsage==rdma-sriov;OperationalStatus==上线',
   "fields": "(IP,AvailableZone,IDC,HostName,OperationalStatus)",
   "order" : "AvailableZone desc"
}
r = requests.get(url, headers=headers , params=params)

server_all_info = json.loads(r.text)
server_ip_data = server_all_info.get('data')

# https://blog.csdn.net/Ls4034/article/details/89161157?utm_medium=distribute.pc_relevant.none-task-blog-2%7Edefault%7EBlogCommendFromMachineLearnPai2%7Edefault-3.vipsorttest&depth_1-utm_source=distribute.pc_relevant.none-task-blog-2%7Edefault%7EBlogCommendFromMachineLearnPai2%7Edefault-3.vipsorttest

# https://blog.csdn.net/hiflower/article/details/7240797?utm_medium=distribute.pc_relevant.none-task-blog-2%7Edefault%7EBlogCommendFromMachineLearnPai2%7Edefault-1.vipsorttest&depth_1-utm_source=distribute.pc_relevant.none-task-blog-2%7Edefault%7EBlogCommendFromMachineLearnPai2%7Edefault-1.vipsorttest
def get_version(module_name, module_ip):
    version = ""
    get_version_cmd = "ls /root/udisk/%s/%s -l" % (module_name, module_name)
    result = os.popen("ssh -q root@" + module_ip + " " +
                      get_version_cmd, "r").read().strip().split()
    if len(result) < 1:
        version = "unknown"
    else:
        version = result[-1]
        version = version.split("/")[-1]

    return version

file = open('uhost_ip_info', 'w')

for ip in server_ip_data:
    version = get_version("block_gate", ip.get('IP'))
    if version != "unknown":
      file.write(ip.get('AvailableZone')+" "+ip.get('IDC')+" "+ip.get('IP')+" "+ip.get('OperationalStatus')+" "+ip.get('HostName') + " " + version)
        # print ip.get('AvailableZone')+" "+ip.get('IDC')+" "+ip.get('IP')+" "+ip.get('OperationalStatus')+" "+ip.get('HostName') + " " + version

    version = get_version("vhost_gate", ip.get('IP'))
    if version != "unknown":
        # print ip.get('AvailableZone')+" "+ip.get('IDC')+" "+ip.get('IP')+" "+ip.get('OperationalStatus')+" "+ip.get('HostName') + " " + version
        file.write(ip.get('AvailableZone')+" "+ip.get('IDC')+" "+ip.get('IP')+" "+ip.get('OperationalStatus')+" "+ip.get('HostName') + " " + version)

    version = get_version("nvmf_gate", ip.get('IP'))
    if version != "unknown":
        # print ip.get('AvailableZone')+" "+ip.get('IDC')+" "+ip.get('IP')+" "+ip.get('OperationalStatus')+" "+ip.get('HostName') + " " + version
        file.write(ip.get('AvailableZone')+" "+ip.get('IDC')+" "+ip.get('IP')+" "+ip.get('OperationalStatus')+" "+ip.get('HostName') + " " + version)

file.close()