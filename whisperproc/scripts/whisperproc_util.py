#!/usr/bin/python
#
import datetime
import glob
import os
import time

FILE_PARM = "[0-9][0-9][0-9][0-9][0-9][0-9][0-9][0-9]-[0-9][0-9][0-9][0-9][0-9][0-9]-[0-9][0-9][0-9]"
def miliseconds(td):
    return td.days * 86400000 + td.seconds * 1000 + td.microseconds / 1000

def ExtractDate(s):
    if s.find('-') > 0:
        return datetime.datetime(int(s[0:4]),    # year
                                 int(s[4:6]),    # month ( 1->12 )
                                 int(s[6:8]),    # month
                                 int(s[9:11]),   # hour
                                 int(s[11:13]),  # minute
                                 int(s[13:15]))  # second
    return datetime.datetime.utcfromtimestamp(int(s))
def ParseFileName(fn):
    f = os.path.basename(fn)
    pos2 = f.rfind('_')
    if pos2 < 0:
        return (None, None)
    # endif
    pos1 = f.rfind('_', 0, pos2)

    try:
        return (ExtractDate(f[pos1 + 1:pos1 + 20]),
                ExtractDate(f[pos2 + 1:pos2 + 20]))
    except ValueError:
        return (None, None)
    # endtry

def GetAllRanges(dir):
    files = glob.glob(os.path.join(dir,
                                   "media_" +
                                   FILE_PARM + "_" + FILE_PARM + ".flv"))
    ranges = []
    for f in files:
        (start_date, end_date) = ParseFileName(f)
        ranges.append((start_date, end_date, f))
    # endfor
    return ranges
# enddef

def GetFiles(dir):
    files = glob.glob(os.path.join(dir,
                                   "media_" +
                                   FILE_PARM + "_" + FILE_PARM + ".flv"))
    return files

def GetFiller(filename,
              range_begin, range_end,
              hole_begin, hole_end):
    min_diff = datetime.timedelta(0, 2)   # 2 seconds
    portion_begin = max(range_begin, hole_begin)
    portion_end = min(range_end, hole_end)
    delta_begin = abs(portion_begin - range_begin)
    delta_end = abs(range_end - portion_end)
    if delta_begin < min_diff and delta_end < min_diff:
        return ("copy", filename, 0, 0, "")

    if delta_begin < min_diff:
        start_sec = 0
    else:
        start_sec = delta_begin.days * 86400 + delta_begin.seconds
    # endif
    end_abs = abs(portion_end - range_begin)
    end_sec = end_abs.days * 86400 + end_abs.seconds
    out_filename = "media_%s_%s.flv" % (
        portion_begin.strftime("%Y%m%d-%H%M%S-000"),
        portion_end.strftime("%Y%m%d-%H%M%S-000"))
    return ("crop", filename, start_sec, end_sec, out_filename);

def FindHoleFiller(hole_begin, hole_end,
                   ranges):
    """ ranges as from GetAllRanges """
    fillers = []
    for r in ranges:
        range_begin = r[0]
        range_end = r[1]
        if range_end <= hole_begin:
            continue     # not yet there
        if range_begin >= hole_end:
            # print("breaking : %s %s -- %s - %s" % (hole_begin, hole_end,
            #                                  r, fillers))
            break        # already passed
        portion_begin = max(range_begin, hole_begin)
        portion_end = min(range_end, hole_end)
        fillers.append(GetFiller(r[2], range_begin, range_end,
                                 portion_begin, portion_end));
    # endfor
    return fillers
