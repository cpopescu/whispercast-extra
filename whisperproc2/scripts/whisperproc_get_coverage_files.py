#!/usr/bin/python
#
import sys
from whisperproc_util import *

if __name__ == '__main__':
    if len(sys.argv) != 4:
        sys.exit(
            " Usage: %s <local_dir> <start_date> <end_date>\n" % sys.argv[0])
    # endif

    local_dir = sys.argv[1]
    start_date = ExtractDate(sys.argv[2])
    end_date = ExtractDate(sys.argv[3])

    local_ranges = GetAllRanges(local_dir)
    local_ranges.sort()
    fillers = FindHoleFiller(start_date, end_date, local_ranges)

    for f in fillers:
        if f[0] == "copy":
            print "FULL: %s" % f[1]
        elif f[0] == "crop":
            print "PART: %s [%s - %s]" % (f[1], f[2], f[3])
        # endif
    # endfor
