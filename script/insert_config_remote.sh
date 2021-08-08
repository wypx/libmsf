#!/bin/bash

_dir_=$(cd `dirname $0`; pwd)

#在ini配置文件中某个域中加入键值对。
#!/bin/bash

# if [ $# -lt 5 ]
# then
#     echo "$0 region(not include []) key value ini_filename"
#     echo "Usage: $0 [iplist] [module] [section] [key] [value]"
#     exit 1
# fi

iplist="$1"
# module="$2"
# section="$3"
# conf_key="$4"
# conf_value="$5"

echo "-------- deploy $module -----------"



for dest in `cat $iplist`
do
  echo "-------- deploy $dest -----------"
  # 跳过标记#的ip
  dest=`echo $dest | awk -F "#" '{print $1}'`
  if [ "$dest" == "" ]; then
    continue
  fi
  # ssh -q -n root@$dest "cd /root/udisk/$module/conf/; sed -i \"/^$conf_key /{s/=.*/= $conf_value/}\" ${module}.ini; exit"
  # scp insert_config_local.sh root@$dest:/root/udisk/
  # ssh -n root@$dest "cd /root/udisk/vhost_gate/conf/;sed -i '/net_type/a\inactive_conn_interval             = 30' vhost_gate.ini"
  # ssh -n root@$dest "cd /root/udisk/vhost_gate/conf/;sed -i '/net_type/a\check_inactive_conn_duration       = 5' vhost_gate.ini"
  # ssh -n root@$dest "cd /root/udisk/vhost_gate/conf/;sed -i '/net_type/a\enanle_close_inactive_conn         = 1' vhost_gate.ini"

  # ssh -n root@$dest "cd /root/udisk/block_gate/conf/;sed -i '/net_type/a\inactive_conn_interval             = 30' block_gate.ini"
  # ssh -n root@$dest "cd /root/udisk/block_gate/conf/;sed -i '/net_type/a\check_inactive_conn_duration       = 5' block_gate.ini"
  # ssh -n root@$dest "cd /root/udisk/block_gate/conf/;sed -i '/net_type/a\enanle_close_inactive_conn         = 1' block_gate.ini"

  # ./PbBlockGateTerminateRequest $dest 13800

  ssh -n root@$dest "cd /root/udisk/chunk/conf/;sed -i '/hela_io_listen_port/a\inactive_conn_interval             = 30' *.conf"
  ssh -n root@$dest "cd /root/udisk/chunk/conf/;sed -i '/hela_io_listen_port/a\check_inactive_conn_duration       = 5' *.conf"
  ssh -n root@$dest "cd /root/udisk/chunk/conf/;sed -i '/hela_io_listen_port/a\enanle_close_inactive_conn         = 1' *.conf"

done
exit 0


