#!/usr/bin/env python

import subprocess
import re
import sys

if __name__ == "__main__":
    cmd = 'git ls-files | grep -E "\.(c|hpp|cc|cpp|h)$" | grep -v test | xargs ci/cpplint.py'
    result = subprocess.Popen(cmd, shell=True, stdout=subprocess.PIPE)
    for line in result.stdout.readlines():
        m = re.match("Total errors found: (\d+)", line)
        if m != None:
            error_count = int(m.group(1))
            print "error count:", error_count
            sys.exit(1 if error_count > 100 else 0)
    print "unexpected result, error count not found"
    sys.exit(-1)

