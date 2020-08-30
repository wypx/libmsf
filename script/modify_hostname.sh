#!/bin/bash

if [ $# -lt 2 ]; then
  echo "Usage: $0 ip hostname"
  exit 1
fi

dest_ip=$1
hostname="$2"

ssh -q -n root@$dest_ip "[ -f /proc/sys/kernel/hostname ] && echo \"${hostname}\" > /proc/sys/kernel/hostname"
ssh -q -n root@$dest_ip "[ -f /etc/sysconfig/network ] && echo \"NETWORKING=yes\nHOSTNAME=${hostname}\nNOZEROCONF=yes\" > /etc/sysconfig/network"
ssh -q -n root@$dest_ip "[ -f /etc/hostname ] && echo \"${hostname}\" > /etc/hostname"