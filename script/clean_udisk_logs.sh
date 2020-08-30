#!/bin/bash

# 按照修改时间排序
# https://www.cnblogs.com/sien6/p/8056315.html

udisk_log_dir="/var/log/udisk"
udisk_core_dir="/home/core"

# 保留200个日志
udisk_logs_max=200
udisk_core_max=50

# 可以根据模块定制保留的策略

echo -e "\r"
echo "===========> Logs max num: $udisk_logs_max"

function clean_dir_logs(){
    curr_dir=$1
    files_list=`ls -rt ${curr_dir}`
    files_count=`ls -rt ${curr_dir}|wc -l`
    echo "===========> Find dir: ${curr_dir}, logs num: $files_count"
    # 将文件名称的最大值取出
    for filename in $files_list;do
        echo ">>>>>>>>>> Log item: $filename"
        if [ $files_count -gt $udisk_logs_max ];then
            rm ${curr_dir}/${filename}
            let files_count--
            echo "!!!!!!!!!! Remove log: $curr_dir/$filename"
        fi
    done
}

function clean_udisk_logs(){
    if [ ! -d $udisk_log_dir ];then
        echo "$udisk_log_dir not exist"
        return
    fi

    object_list=`ls -rt ${udisk_log_dir}`
    for object in $object_list;do
        curr_object=${udisk_log_dir}/${object}
        if [ -d ${curr_object} ];then
            echo -e "\r"
            clean_dir_logs ${curr_object}
            echo -e "\r"
        else
            echo "-----------> Found file: ${curr_object}"
        fi  
    done
}

function clean_udisk_core(){
    if [ ! -d $udisk_core_dir ];then
        echo "$udisk_core_dir not exist"
        return
    fi

    object_list=`ls -rt ${udisk_core_dir}`
    for object in $object_list;do
        curr_object=${udisk_core_dir}/${object}
        core_count=`ls -rt ${udisk_core_dir}|wc -l`
        if [ -d ${curr_object} ];then
            echo "-----------> Found dir: ${curr_object}"
        else
            echo "-----------> Found file: ${curr_object}"
            if [ $core_count -gt $udisk_core_max ];then
                rm ${curr_object}
                let core_count--
                echo "!!!!!!!!!! Remove core: ${curr_object}"
            fi
        fi  
    done
}

clean_udisk_core
# clean_udisk_logs
echo -e "\r"