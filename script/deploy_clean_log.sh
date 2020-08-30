#!/bin/bash

# 只适用于测试和临时清理,线上使用minitor的脚本库
# example:   ./deploy_clean_log.sh hn02 3103        ---  后端机器
# example:   ./deploy_clean_log.sh ip_list_file     ---  宿主机器

# https://www.cnblogs.com/aquester/archive/2013/01/19/9891742.html

# 3个参数,第三个指定外部IP列表
if [ $# == 1 ];then
  ip_list=$1
  echo "use extern host ip: " `cat $ip_list`
elif [ $# == 2 ];then
  region=$1
  set_id=$2
  echo "region: $1"
  echo "set_id: $2"
  ip_list=${HOME}/limax/udisk/region/$region/$set_id/host_info
  echo "use limax host  ip: " `cat $ip_list`
else
  echo "unknown argument num: $#"
  exit 1
fi

pscp -h $ip_list -l root -r clean_udisk_logs.sh /root/udisk/
pscp -h $ip_list -l root -r install_clean_logs.sh /root/udisk/

pssh -h $ip_list -l root -i "cd /root/udisk/;sh install_clean_logs.sh"
pssh -h $ip_list -l root -i "chmod -R 777 /root/udisk/clean_udisk_logs.sh"
pssh -h $ip_list -l root -i "cd /root/udisk/;rm -f install_clean_logs.sh"
pssh -h $ip_list -l root -i "crontab -l"