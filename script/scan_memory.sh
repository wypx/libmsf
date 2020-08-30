#!/bin/bash

IP_DIR="../../machine_ip/"

if [ $# -lt 1 ]; then
  echo "Usage: $0 <module>"
  exit 1
fi

module=$1

for IP_LIST in `find $IP_DIR -name ${module}.txt`
do
  region=`echo $IP_LIST | awk -F\/ '{print \$4}'`
  set_id=`echo $IP_LIST | awk -F\/ '{print \$5}'`
  for ip in `cat $IP_LIST`
  do
    ip=`echo $ip | awk -F "#" '{print $1}'`
    if [ "$ip" == "" ]; then
      continue
    fi

    centos_version=`ssh -n root@$ip "uname -r | awk -F. '{print \\$4}'"`

    if [ $centos_version == "el7" ]; then
      mem_usage=`ssh -n root@$ip "free -m | sed -n '2p' | awk '{print ""100-(\\$6+\\$4)/\\$2*100""}' | awk -F. '{print \\$1}'"`
    else
      mem_usage=`ssh -n root@$ip "free -m | sed -n '2p' | awk '{print ""100-(\\$7+\\$4)/\\$2*100""}' | awk -F. '{print \\$1}'"`
    fi
    
    if [ $mem_usage -gt 50 ]; then
      echo "$region $set_id $ip: $mem_usage%"
    fi
  done
done
