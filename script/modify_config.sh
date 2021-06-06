#!/bin/bash

_dir_=$(cd `dirname $0`; pwd)

if [ $# -eq 4 ]; then
  module="$1"
  conf_key="$2"
  conf_value="$3"
  ip_file="$4"
else
  echo "Usage: $0 [module] [key] [value] [ip_file] "
  exit 1
fi

IFS=$'\n'
for destip in `cat $ip_file`
  do
    destip=`echo $destip | awk -F "#" '{print $1}'`
    if [ "$destip" == "" ]; then  
      continue
    fi
    echo "@@@@@@@@@@@ modify config on:" $destip
    ssh -q -n root@$destip "cd /root/udisk/${module}/conf/; sed -i \"/^$conf_key /{s/=.*/= $conf_value/}\" ${module}*.ini; exit"

  done

exit 0