#!/usr/bin/python
#
# (c) Whispersoft
#

import sys
import datetime
import feedparser

from whisperrpc import *
from factory import *
from osd_library import *

import osd

def PrepareCrawlerItemsFromRss(assoc, rss_url, header, limit):
    f = feedparser.parse(rss_url)

    crawler_id = f.feed.title
    num_entries = len(f.entries)
    if header:
       item  = osd.CrawlerItem(content = header)
       assoc.crawlers().append(osd.AddCrawlerItemParams(crawler_id,
                                                        "0000_head", item));
    for i in range(min(limit, num_entries)):
        e = f.entries[i]
        item  = osd.CrawlerItem(content = '<a href="%s" target="_blank">%s</a>' % (
          e.link, e.title))
        assoc.crawlers().append(osd.AddCrawlerItemParams(crawler_id,
                                                         "%04d_%s" % (i + 1, e.title), item));
    return crawler_id

if __name__ == '__main__':
    if len(sys.argv) != 2:
        print 'Usage: rss2osd.py <config_file>'
        sys.exit(1)

    """
    params_file should contain a pairs:
    remote_host: '<remote_host_name>',
    remote_port: <remoter_port_num>,
    admin_pass : '<admin password>',
    rss_url' : '<where the rss to publish is>',
    associator_name : '<the associator that will inject osds for us>',
    stream_name : '<the media (stream) name where to publish the rss>',

    Example config file:
    ----
    remote_host = 'ds01.itsol.eu'
    remote_port = 8080
    admin_pass = 'somepass'
    rss_url = 'http://www.techmeme.com/index.xml'
    associator_name = 'osda_35'
    stream_name = 'rtmp_source_35/hotnews'
    ----
    """

    params = {
        'remote_host' : '',
        'remote_port' : 8080,
        'admin_pass' : '',
        'rss_url' : '',
        'associator_name' : None,
        'stream_name' : None,
        'speed' : .2,
        'header' : None,
        'item_limit' : 12,
        }
    execfile(sys.argv[1], params)

    # Prepare the RPC connection to the remote server
    rpc_conn = RpcConnection(
        params.get('remote_host'),
        '/rpc-config/osd_library/osd_associator/%s' %
        params.get('associator_name'),
        port = params.get('remote_port'),
        user = 'admin',
        passwd = params.get('admin_pass'))

    rpc_service = osd.RpcService_OsdAssociator(rpc_conn)


    # Prepare associated crawlers with the media ..
    assoc = osd.AssociatedOsds(media = params.get('stream_name'),
                               crawlers = [],
                               eos_clear_crawlers = True,
                               eos_clear_crawl_items = True)
    rss_url = params.get('rss_url')
    crawler_id = PrepareCrawlerItemsFromRss(assoc, rss_url, 
                         params['header'], params['item_limit'])

    # Prepare the parameters for crawler creation / update
    crawler_params = osd.CreateCrawlerParams(
        id = crawler_id,
        speed = .2,
        fore_color = osd.PlayerColor(color="FFF0D0", alpha=1.0),
        back_color = osd.PlayerColor(color="402010", alpha=.7),
        link_color = osd.PlayerColor(color="F0FFD0", alpha=1.0),
        y_location = 1,   # bottom
        show = True)


    # Add / reconfigure crawler params
    ret1 = rpc_service.CreateCrawlerType(crawler_params)
    ret2 = rpc_service.SetAssociatedOsds(assoc)
