#!/usr/bin/python

import sys
import os
import os.path
import time
import subprocess

def isRemoteFile(filename):
  return filename.find('@') != -1

def createDir(path):
  if not os.path.exists(path):
    os.makedirs(path)

def reportMessage(message):
  sys.stderr.write('{"message_": "%s"}\n' % message.replace('\\', '\\\\').replace('"', '\\"'))
def reportProgress(progress):
  sys.stderr.write('{"progress_": %f}\n' % (progress))

def copyRemoteFile(remote_src, local_dst):
  reportMessage("Copying")
  print "Executing: " + "scp " + remote_src + " " + local_dst
  # TODO(cosmin): maybe report progress while copying
  os.system("scp " + remote_src + " " + local_dst)
  reportMessage("Copy Successful")

def processFile(filename, output_dir):
  reportMessage("Transcoding")
  # execute avconv
  for i in range(0,100):
    reportProgress(0.01 * i)
    time.sleep(0.3)
  print "Output fake results"
  os.system("touch " + os.path.join(output_dir, "240p.mp4"))
  os.system("touch " + os.path.join(output_dir, "360p.mp4"))
  os.system("touch " + os.path.join(output_dir, "480p.mp4"))
  os.system("touch " + os.path.join(output_dir, "720p.mp4"))
  os.system("touch " + os.path.join(output_dir, "1080p.mp4"))
  reportMessage("Transcode Successful")
  reportProgress(1)
  pass

def runProcess(exe):
  print "Executing:", exe
  p = subprocess.Popen(exe, bufsize=0, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
  while True:
    retcode = p.poll() #returns None while subprocess is running
    print "Retcode:", retcode
    line = p.stdout.readline()
    #yield line
    print "Read line:", line
    #if retcode is not None:
    #  break
    if line == "": break
  print "Subprocess Done!"

def main():
  if len(sys.argv) < 3: 
    print "Usage: %s [params] <input_file> <output_dir>" % sys.argv[0]
    return 1
  filename = sys.argv[-2]
  output_dir = sys.argv[-1]

  createDir(output_dir)

  if isRemoteFile(filename):
    tmp_filename = os.path.join("/tmp", os.path.basename(filename))
    copyRemoteFile(filename, tmp_filename)
    filename = tmp_filename

  if not os.path.exists(filename):
    reportMessage("No such file: " + filename)
    return 1
  
  processFile(filename, output_dir)
  #runProcess("./avconv.py")
  
  pass

if __name__ == "__main__":
  sys.exit(main())
