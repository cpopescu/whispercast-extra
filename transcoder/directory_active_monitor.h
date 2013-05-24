#ifndef DIRECTORY_ACTIVE_MONITOR_H_
#define DIRECTORY_ACTIVE_MONITOR_H_

#include <string>
#include <whisperlib/common/base/re.h>
#include <whisperlib/common/base/callback.h>
#include <whisperlib/common/base/alarm.h>
#include <whisperlib/net/base/selector.h>
#include "directory_monitor.h"

namespace manager {

class DirectoryActiveMonitor {
public:
  typedef Callback1<const string &> HandleFile;
public:
  // selector: selector
  // dirname: path to directory to be monitored
  // regexp: regular expression. Only files matching this expression
  //         are reported. If NULL, all files are reported.
  // ms_between_scans: milliseconds between consecutive scans of the monitored
  //                   directory
  // handle_file: a permanent callback which is called with every new file found.
  DirectoryActiveMonitor(net::Selector & selector,
                         const string & dirname,
                         re::RE * regexp,
                         int64 ms_between_scans,
                         HandleFile * handle_file,
                         bool delete_handle_file);
  virtual ~DirectoryActiveMonitor();

  //  Starts listening on the given dirname_.
  // Returns success. Returns false if dirname_ does not exists,
  //  it's not a directory, or permission denied.
  bool Start();

  // Test if we're running.
  bool IsRunning() const;

  // Stop listening.
  void Stop();

private:
  // Callback to be periodically run in selector thread.
  // It reports new files on handle_file.
  void DoScan();

public:
  string ToString() const;

private:
  net::Selector& selector_;

  // callback called with every reported file
  HandleFile* handle_file_;
  bool delete_handle_file_;

  // this guy is actually doing the scans
  DirectoryMonitor monitor_;

  // calls DoScan() from time to time
  util::Alarm scan_alarm_;
};

};

#endif /*DIRECTORY_ACTIVE_MONITOR_H_*/
