#!/usr/bin/python3
# -*- coding: utf-8 -*-
import os
import sys
import time
import os.path
import subprocess
from subprocess import Popen, PIPE, STDOUT
import threading
import uuid
import errno
import io

# https://www.jianshu.com/p/ae888700bad0

# https://www.cnblogs.com/stono/p/9095063.html

file_res = open('write_res.log', 'w')

def FindKeywords(line):
  udisk_offset_start = 4145709056
  udisk_offset_end = 4145709056 + 16384
  # print("udisk_offset_start: %d" %  udisk_offset_start)
  # print("udisk_offset_end: %d" %  udisk_offset_end)
  index = udisk_offset_start
  while index < udisk_offset_end:
    if line.find("%d" % index) != -1:
      print(line)
      file_res.write(line)

    index = index + 1

if __name__ == "__main__":
  path = "/Users/wypx/Downloads/write.log"
  file = open(path) 
  for line in file.readlines():
    line = line.strip('\n') 
    if FindKeywords(line):
      break
  file.close()
  file_res.close
 
