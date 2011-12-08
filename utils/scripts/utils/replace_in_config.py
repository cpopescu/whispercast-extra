#!/usr/bin/python

import sys
import json

def main():
  if len(sys.argv) != 3:
    print "This script replaces something in whispercast config file"
    print "Usage: %s <whispercast .config file> <output .config file>" % sys.argv[0]
    exit(1)

  # Decode input config file
  #
  s = open(sys.argv[1]).read()
  config,end = json.JSONDecoder().raw_decode(s)
  exports,new_end = json.JSONDecoder().raw_decode(s[end:])
  end += new_end
  saves,new_end = json.JSONDecoder().raw_decode(s[end:])
  end += new_end
  assert end == len(s), "Unknown data at file end, index: %d, data: [%s]" % (end, s[end:])

  # Modify this part to suit your replacement needs
  #
  count = 0
  for e in exports:
    print "Export: %s" % e['path_']
    if e['path_'].find("/download/") != -1:
      #print 'Replacing in %r' % e
      e['flow_control_total_ms_'] = 0
      e['flow_control_video_ms_'] = 0
      count += 1
  print "Replaced: %d items" % count
  
  # Encode & write output config file
  s = ""
  s += json.JSONEncoder().encode(config)
  s += json.JSONEncoder().encode(exports)
  s += json.JSONEncoder().encode(saves)
  f = open(sys.argv[2],"w")
  f.write(s)
  f.close()
  print "Done"

if __name__ == "__main__":
  main()
