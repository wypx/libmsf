#在ini配置文件中某个域中加入键值对。
#!/bin/bash

if [ $# -lt 4 ]
then
    echo "$0 region(not include []) key value ini_filename"
    exit 1
fi

REGION=$1
KEY=$2
VALUE=$3
INI_FILENAME=$4

sed -in "/^\[${REGION}\]/a\\${KEY}\t\t\t\t\t= ${VALUE}" ${INI_FILENAME}