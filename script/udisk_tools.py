#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import fcntl
import os
import sys
import errno
import logging
from logging.handlers import RotatingFileHandler

OK  = 0
ERR = 1

class UDiskLogger:
    def __init__(self, name, logname):
        self.logger = logging.getLogger(name)
        self.logger.setLevel(level = logging.DEBUG)
        log_dir = "/var/log/udisk/"
        ''' 
        定义一个RotatingFileHandler,最多备份10个日志文件,每个日志文件最大50M 
        '''
        if not os.path.exists(log_dir):
            os.makedirs(log_dir)
        rHandler = RotatingFileHandler(log_dir+logname,maxBytes = 50*1024*1024,backupCount = 10)
        rHandler.setLevel(logging.DEBUG)
        formatter = logging.Formatter('%(asctime)s - %(name)s - %(levelname)s - %(message)s')
        rHandler.setFormatter(formatter)
        self.logger.addHandler(rHandler)

logger = UDiskLogger("udisk_lockfile", "add_remove_udisk.log").logger


# 为了保持主机组使用习惯 保留IQN软连接
# IQN="iqn.$UBS_ID.$SNAPSHOT_ID.ucloud.cn"
UDISK_SNAPSHOT_ID="0"
UDISK_DEVLINK_FORMAT="/dev/disk/by-path/iqn.%s.%s.ucloud.cn"

def GenUDiskDevLink(udisk_id):
    UDISK_DEVLINK = UDISK_DEVLINK_FORMAT % (udisk_id, UDISK_SNAPSHOT_ID)
    return UDISK_DEVLINK

GLOBAL_LOCK_DIR = "/var/lock/"

# nbd添加此锁是因为方舟镜像NBD, QEMU-NBD, SPDK-NBD可能共享
# 保持兼容使用统一路径: /var/lock/login_nbd.lock
GLOBAL_NBD_LOCK_NAME="login_nbd"
GLOBAL_UDISK_LOCK_NAME="udisk_spdk"

class UDiskLock(object):
    def __init__(self, udisk_dir, udsik_id):
        if not os.path.exists(udisk_dir):
            os.makedirs(udisk_dir)
        try:
            self.file_name = udisk_dir + udsik_id + ".lock"
            self.handle = open(self.file_name, 'wb')
        except Exception:
            logger.error("'%s' cannot open: %s " % (os.getpid(), self.file_name))
            exit(ERR)

        if self.lock() == False:
            exit(ERR)
        # logger.info("'%s' __init__ ", self.file_name)

    def __del__(self):
        try:
            self.unlock()
            self.handle.close()
            # lockfile是否需要删除?
        except:
            pass
        # logger.info("'%s' __del__ ", self.file_name)

    def lock(self):
        '''同步锁,其他进程获取不到需等待 | fcntl.LOCK_NB 函数不能获得文件锁就立即返回 '''
        try:
            fcntl.flock(self.handle, fcntl.LOCK_EX)
            logger.info("'%s' locked successful: %s " % (self.file_name, os.getpid()))
            return True
        except:
            logger.info("'%s' unlocked failed: %s " % (self.file_name, os.getpid()))
            return False

    def unlock(self):
        try:
            fcntl.flock(self.handle, fcntl.LOCK_UN)
            logger.info("'%s' unlocked successful: %s " % (self.file_name, os.getpid()))
        except:
            logger.info("'%s' unlocked failed: %s " % (self.file_name, os.getpid()))
            return False

def ForceRemove(dstFile):
    # 如果是已经存在的软连接,尝试删除然后重新软连接
    # 理论上udisk_id冲突之前已经检查过了,不会出现这种情况
    # 第一次检查删除软连接
    if os.path.lexists(dstFile):
        logger.info("'%s' is exist, unlink link" % dstFile)
        os.unlink(dstFile)
    else:
        return True
    
    # 第二次检查强制删除文件连接
    if os.path.lexists(dstFile):
        logger.info("'%s' also exist, remove link" % dstFile)
        os.remove(dstFile)
    else:
        return True
    
    # 最后一次检查
    if os.path.lexists(dstFile):
        logger.info("'%s' also exist, remove failed" % dstFile)
        return False
    else:
        return True

''' like system cmd: ln -sf xxx xxx '''
def ForceSymlink(srcFile, dstFile):
    if ForceRemove(dstFile) == False:
        return False

    try:
        os.symlink(srcFile, dstFile)
    except OSError as e:
        logger.info("'%s' symlink errno: %d" % e.errno)
        return False

    if os.path.lexists(dstFile):
        logger.info("symlink to %s, add nbd success", dstFile)
        return True
    else:
        logger.info("'%s' not exist, add nbd failed" % dstFile)
        return False

''' like system cmd: unlink/remove xxx '''
def ForceUnlink(dstFile):
    if ForceRemove(dstFile) == False:
        return False
    
    logger.info("unlink %s, remove nbd success", dstFile)
    return True

if  __name__ == "__main__":
    udisk_logger = UDiskLogger(__name__, __name__)
    udisk_logger.logger.info("udisk logger test")
