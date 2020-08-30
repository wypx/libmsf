#!/bin/bash

cd /root
crontab -l > tmp.conf

# 清理以前的清理方式
sed -i "/clean_hela_logs.sh/d" tmp.conf
sed -i "/clean_idun_logs.sh/d" tmp.conf
sed -i "/clean_ymer_logs.sh/d" tmp.conf

# 采用新的方式
echo "0 * * * * /root/udisk/clean_udisk_logs.sh >/dev/null 2>&1" >> tmp.conf

# 重复发布去重
# https://blog.csdn.net/weixin_33724059/article/details/91993797
awk '!x[$0]++' tmp.conf > tmp_new.conf

# 应用新的配置文件
crontab tmp_new.conf

rm -f /root/clean_hela_logs.sh
rm -f /root/clean_idun_logs.sh
rm -f /root/clean_ymer_logs.sh
rm -f /root/clean_udisk_logs.sh
rm -f tmp.conf
rm -f tmp_new.conf