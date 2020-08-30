#!/bin/bash

# tick range
# 0 一天前
# 1 24-12小时
# 2 12-6小时
if [[ $# -eq 0 ]]; then
  echo "Usage $0 tick_range"
  exit 100
fi

#模块名 [需要根据机器上的模块进行修改]
modules="udataark_module"

#提取压缩文件时间范围
tick_range=$1
if [[ $tick_range -eq 0 ]]; then
  Date_Begin=`date +"%Y%m%d" -d "-1 days"`
  Date_End=`date +"%Y%m%d"`
  Date=`date +"%Y%m%d" -d "-1 days"`
elif [[ $tick_range -eq 1 ]]; then
  Date_Begin=`date +"%Y-%m-%d %H:%M:%S" -d "-24 hour"`
  Date_End=`date +"%Y-%m-%d %H:%M:%S" -d "-12 hour"`
  Date=`date +"%Y%m%d_%H"`
elif [[ $tick_range -eq 2 ]]; then
  Date_Begin=`date +"%Y-%m-%d %H:%M:%S" -d "-12 hour"`
  Date_End=`date +"%Y-%m-%d %H:%M:%S" -d "-6 hour"`
  Date=`date +"%Y%m%d_%H"`
elif [[ $tick_range -eq 3 ]]; then
  Date_Begin=`date +"%Y-%m-%d %H:%M:%S" -d "-6 hour"`
  Date_End=`date +"%Y-%m-%d %H:%M:%S" -d "-3 hour"`
  Date=`date +"%Y%m%d_%H"`
else
  echo "tick range illegal"
  exit 100
fi

echo $Date_Begin $Date_End

#find . -type f -newermt '2019-06-18 11:00:00' ! -newermt '2019-06-18 23:59:59'


for module in $modules
do
  if [[ $module == "uvale" || $module == "gateway" ]]; then
    if [[ -d ~/$module/log/ ]]; then
      cd ~/$module/log/
    else
      continue
    fi
  elif [[ $module == "new_front" ]]; then
    if [[ -d ~/udataark/front/log/ ]]; then
      cd ~/udataark/front/log/
    else
      continue
    fi
  else 
    if [[ -d ~/utimemachine/$module/log/ ]]; then
      cd ~/utimemachine/$module/log/
    else
      continue
    fi
  fi
  
  files=`find . -name "*.log" -type f -newermt "$Date_Begin" ! -newermt "$Date_End"`
  if [[ $files == "" ]]; then
    echo "$module none log need compress"
    continue
  fi
  #cnt=`ls |grep -c ${Date}`

  #if [[ $cnt -eq 0 ]]; then
  #  echo “$module $cnt”
  #  continue
  #fi
  
  #压缩日志
  #tar -czf ${module}_${Date}.tar.gz ${module}*${Date}*.log
  tar -czf ${module}_${Date}_${tick_range}.tar.gz $files
done