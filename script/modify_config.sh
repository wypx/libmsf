#!/bin/bash

_dir_=$(cd `dirname $0`; pwd)

if [ $# -eq 4 ]; then
  idcset="$1"
  module="$2"
  conf_key="$3"
  conf_value="$4"
elif [ $# -eq 3 ]; then
  module="$1"
  conf_key="$2"
  conf_value="$3"
else
  echo "Usage: $0 [set] [module] [key] [value] or $0 [module] [key] [value]"
  exit 1
fi

source "$_dir_/../common.sh"

MODULE_IP_FILE="`GetModuleMachineIpFile $idcset $module`"
AssertFile $MODULE_IP_FILE

while read dest; do
  # 跳过标记#的ip
  dest=`echo $dest | awk -F "#" '{print $1}'`
  if [ "$dest" == "" ]; then
    continue
  fi

  LOG_PRINT "===> $dest"
  ssh -q -n root@$dest "cd ~/udataark/conf/; sed -i \"/^$conf_key /{s/=.*/= $conf_value/}\" ${module}*.conf; exit"

done < $MODULE_IP_FILE
exit 0