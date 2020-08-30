#!/bin/bash

remote_host=172.27.40.102
tomcat_name=idun
pid=$(ssh root@$remote_host "ps -ef | grep ${tomcat_name} | grep -v grep | awk  '{print $2}'"| awk '{print $2}')
echo "process pid: $pid"

process_cnt=$(ssh root@$remote_host "ps -aux | grep -w ${tomcat_name} | egrep -v \"grep|vim|vi|less|bash\" | wc -l")
echo "process cnt: $process_cnt"