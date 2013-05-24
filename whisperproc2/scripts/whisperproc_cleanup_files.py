#!/usr/bin/python
#
# This script deletes ranges that are not aliased
#

import sys
import datetime
import glob
import os
import time
import commands
from whisperproc_util import *

from whisperrpc import *
from factory import *
from extra_library import *

def GetTimeRanges(rpc_conn):
    rpc_service = RpcService_MediaElementService(rpc_conn)
    config = rpc_service.GetElementConfig()
    tr = {}
    to_save = {}
    for e in config.elements_():
        if e[u'type_'] == u'time_range':
            tr_spec = TimeRangeElementSpec()
            tr_spec.JsonDecode(e[u'params_'])
            tr[ e[u'name_'] ] = tr_spec
            to_save[tr_spec.home_dir_()] = []
    aliases = rpc_service.GetAllMediaAliases()
    for a in aliases:
        comp = a[u'media_name_'].split('/')
        if len(comp) == 3 and tr.has_key(comp[0]):
            start_date = ExtractDate(comp[1])
            end_date = ExtractDate(comp[2])
            dir = tr[comp[0]].home_dir_()
            to_save[dir].append((start_date, end_date))
        # endif
    # endfor
    return to_save

if __name__ == '__main__':

    """
    params_file should contain a pairs:
    'remote_host': '<remote_host_name>',
    'remote_port': <remoter_port_num>,
    'admin_pass' : '<admin password>'
    'max_date' : '<max_date>'
    """
    params_file = sys.argv[1]
    exec('params = { %s }' %
         ','.join(open(params_file, 'r').readlines()))

    remote_host = params.get('remote_host')
    remote_port = params.get('remote_port')
    admin_pass = params.get('admin_pass')
    max_date = ExtractDate(params.get('max_date'))

    rpc_conn_global = RpcConnection(remote_host, '/rpc-config',
                                    port = remote_port,
                                    user = 'admin',
                                    passwd = admin_pass)

    time_ranges = GetTimeRanges(rpc_conn_global)

    if not time_ranges:
        print " No time range aliases identified - we exit w/o any copy "\
            "just to avoid disaster"
        sys.exit(0)

    to_delete = {}
    for dir in sys.argv[2:]:
        to_keep = {}
        for (sub_dir, ranges) in time_ranges.iteritems():
            local_dir = os.path.normpath(os.path.join(dir,
                                                      './' + sub_dir + '/'))
            print " local dir: %s = %s = %s" % (dir, sub_dir, local_dir)
            local_ranges = GetAllRanges(local_dir)
            local_ranges.sort()
            # print "%s" % local_ranges
            for  (start_date, end_date) in ranges:
                fillers = FindHoleFiller(start_date, end_date, local_ranges)
                for f in fillers:
                    to_keep[f[1]] = 1
            for (sd, ed, f) in local_ranges:
                if sd < max_date and ed < max_date and not to_keep.has_key(f):
                    to_delete[f] = True;
    print "%d Files to delete." % len(to_delete)

    # print "Files to delete: %s" % to_delete
    if params.get('do_delete', False):
        print "... Deleting ..."
        for f in to_delete.keys():
            print " Deleting: %s" % f
            os.unlink(f)
    else:
        print " Files: \n%s" % to_delete
