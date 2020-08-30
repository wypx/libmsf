#!/bin/bash
LIMIT=75

#提取磁盘使用量
disks_usage=`df -lh |grep -v "Filesystem" |awk '{print $5,$6}' |awk -F\% '{print $1,$2}'`
#修改for间隔符号为换行
IFS=$'\n'

for disk_usage in $disks_usage
do
  #30天
  GZ_DAYS=30
  #2天 2880分钟
  LOG_MINS=2880

  usage=`echo $disk_usage |awk '{print $1}'`
  drive=`echo $disk_usage |awk '{print $2}'`
  if [[ $usage -ge $LIMIT ]]; then
    if [[ $drive == "/" ]]; then
      #系统盘先删除6小时前core文件
      find /home/core/ -mmin +360 -name "*core.*" |xargs rm -f
      usage=`df -lh |grep -w $drive |awk '{print $5}' |awk -F\% '{print $1}'`
    else
      #删除压缩文件
      while ([[ $usage -ge $LIMIT ]] && [[ $GZ_DAYS -ge 20 ]])
      do
        find $drive -mtime +$GZ_DAYS -name "*.tar.gz" |xargs rm -f
        ((GZ_DAYS=$GZ_DAYS-2))
        usage=`df -lh |grep -w $drive |awk '{print $5}' |awk -F\% '{print $1}'`
      done
    fi

    #删除日志文件
    while ([[ $usage -ge $LIMIT ]] && [[ $LOG_MINS -ge 360 ]])
    do
      find $drive -mmin +$LOG_MINS -name "*.log" |xargs rm -f
      ((LOG_MINS=$LOG_MINS-360))
      usage=`df -lh |grep -w $drive |awk '{print $5}' |awk -F\% '{print $1}'`
    done
  fi
done