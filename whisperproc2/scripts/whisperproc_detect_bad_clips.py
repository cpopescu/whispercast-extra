#!/usr/bin/python
#
import sys
import os
from whisperproc_util import *

if __name__ == '__main__':
    if len(sys.argv) != 3:
        sys.exit(
            " Usage: %s <local_dir> <max_bps>\n" % sys.argv[0])
    # endif

    local_dir = sys.argv[1]
    max_bps = int(sys.argv[2])
    files = GetFiles(local_dir)
    for f in files:
        # print f
        (start, end) = ParseFileName(f)
        if start is None or end is None:
            continue
        duration = (end - start)
        duration_in_sec = duration.days * 86400 + duration.seconds
        size = os.stat(f).st_size

        bps = size * 8L / duration_in_sec
        if bps > max_bps:
            print "%s %s %s %s" % (f, size, duration_in_sec, bps)
        # endif
    # endfor
