#!/usr/bin/python
#
import sys
import datetime
import glob
import os
import time
import commands

from whisperproc_util import *

def GetRemoteFiles(machine, dir):
    cmd = "ssh %s %s" % (
        commands.mkarg(machine),
        commands.mkarg("ls -1 %s/media_%s_%s.flv" % (
                dir, FILE_PARM, FILE_PARM)))
    (err, out) = commands.getstatusoutput(cmd)
    if err:
        sys.exit("Cannot list remotely: %s" % cmd)
    return out.split('\n')

def GetRanges(files):
    files.sort()
    intervals = []
    last_interval = None
    min_diff = datetime.timedelta(0, 5)   # 5 seconds
    for f in files:
        (start_date, end_date) = ParseFileName(f)
        if start_date is not None and end_date is not None:
            if last_interval is None:
                last_interval = [start_date, end_date]
            else:
                diff = start_date - last_interval[1]
                if diff > min_diff:
                    intervals.append(last_interval)
                    last_interval = [start_date, end_date]
                else:
                    last_interval[1] = end_date
                # endif
            # endif
        else:
            print "error parsing: %s" % f
        # endif
    # endfor
    if last_interval is not None:
        intervals.append(last_interval)
    return intervals

def GetHoles(ranges):
    last_end = None
    holes = []
    for [s, e] in ranges:
        if last_end is not None:
            holes.append([last_end, s])
        # endif
        last_end = e
    # endfor
    return holes

def ToTimestamps(pairs):
    ret = []
    for [s, e] in pairs:
        ret.append([time.mktime(s.timetuple()), time.mktime(e.timetuple())])
    return ret

def FromTimestamps(pairs):
    ret = []
    for [s, e] in pairs:
        ret.append([datetime.datetime.fromtimestamp(s),
                    datetime.datetime.fromtimestamp(e)])
    return ret


if __name__ == '__main__':
    if len(sys.argv) != 5:
        sys.exit(
            " Usage: %s <remote user@host> "
            "<remote_dir> <local_dir> <command_path>\n"
            "we try to fill-up remote missing saved files with local ones")
    remote_host = sys.argv[1]
    remote_dir = sys.argv[2]
    local_dir = sys.argv[3]
    cmd_dir = sys.argv[4]
    remote_holes = GetHoles(GetRanges(GetRemoteFiles(remote_host,
                                                     remote_dir)))
    local_ranges = GetAllRanges(local_dir)
    local_ranges.sort()

    print "%s" % remote_holes
    fillers = []
    for (hb, he) in remote_holes:
        fillers.extend(FindHoleFiller(hb, he, local_ranges))
    commands = []
    for f in fillers:
        if f[0] == "copy":
            commands.append("scp %s %s:%s/%s" %
                            (f[1],
                             remote_host, remote_dir, os.path.basename(f[1])))
        elif f[0] == "crop":
            commands.append("%s/flv_split_file "
                            "--in_path=%s "
                            "--out_path=/tmp/%s "
                            "--start_sec=%d "
                            "--end_sec=%d && "
                            "scp /tmp/%s %s:%s/%s && "
                            "rm -f /tmp/%s" % (
                    cmd_dir, f[1], f[4], f[2], f[3],
                    f[4], remote_host, remote_dir, f[4],
                    f[4]))
    print "%s" % commands
    for c in commands:
        if os.system(c):
            sys.exit("ERROR in: %s" % c)
