#!/usr/bin/python
#
import sys
import datetime
import glob
import os
import time
import commands
from whisperproc_util import *


if __name__ == '__main__':
    if len(sys.argv) != 5:
        sys.exit(
            " Usage: %s <local_dir> <start_date> <end_date> <output_file> <command_path>\n"
            "we make a full file from the givven range")
    local_dir = sys.argv[1]
    start_date = ExtractDate(sys.argv[2])
    end_date = ExtractDate(sys.argv[3])
    out_file = sys.argv[4]
    cmd_dir = sys.argv[5]

    local_ranges = GetAllRanges(local_dir)
    local_ranges.sort()
    fillers = FindHoleFiller(start_date, end_date, local_ranges)

    commands = []
    in_files = []
    to_del = []
    for f in fillers:
        if f[0] == "copy":
            in_files.append(f[1])
        elif f[0] == "crop":
            commands.append("%s/flv_split_file "
                            "--in_path=%s "
                            "--out_path=/tmp/%s "
                            "--start_sec=%d "
                            "--end_sec=%d" % (
                    cmd_dir, f[1], f[4], f[2], f[3]))
            in_files.append("/tmp/%s" % f[4])
            to_del.append("/tmp/%s" % f[4])

    for c in commands:
        if os.system(c):
            sys.exit("ERROR in: %s" % c)
    # Now join
    join_cmd = "%s/flv_join_files --files=%s --out=%s --cue_ms_time=2000" % (
        cmd_dir, ','.join(in_files), out_file)
    print " Executing JOIN: %s" % join_cmd
    if os.system(join_cmd):
        sys.exit("ERROR in: %s" % join_cmd)
    print " Deleting temp files %s" % (', '.join(to_del))
    for f in to_del:
        os.unlink(to_del[f])
    print "You have your file at %s" % out_file
