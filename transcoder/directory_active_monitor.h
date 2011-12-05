#ifndef DIRECTORY_ACTIVE_MONITOR_H_
#define DIRECTORY_ACTIVE_MONITOR_H_

#include <string>
#include "common/base/re.h"
#include "common/base/callback.h"
#include "net/base/selector.h"
#include "directory_monitor.h"

namespace manager {

class DirectoryActiveMonitor {
public:
  typedef Callback1<const std::string &> HandleFile;
public:
  // selector: selector
  // dirname: path to directory to be monitored
  // regexp: regular expression. Only files matching this expression
  //         are reported. If NULL, all files are reported.
  // ms_between_scans: miliseconds between consecutive scans of the monitored
  //                   directory
  // handle_file: a permanent callback which is called with every new file found.
  DirectoryActiveMonitor(net::Selector & selector,
                         const std::string & dirname,
                         re::RE * regexp,
                         int64 ms_between_scans,
                         HandleFile * handle_file);
  virtual ~DirectoryActiveMonitor();

  //  Starts listening on the given dirname_.
  // Returns success. Returns false if dirname_ does not exists,
  //  it's not a directory, or permission was denied.
  bool Start();

  // Test if we're running.
  bool IsRunning();

  // Stop listening.
  void Stop();

  // Makes a scan right now, in the caller thread context.
  //input:
  //  [OUT] existing: all files found
  //  [OUT] recent: new files, created after the last scan.
  //returns:
  // success. Call GetLastSystemError() for more info.
  bool Scan(std::set<std::string> * existing, std::set<std::string> * recent);

private:
  // Callback to be periodically run in selector thread.
  // It reports new files on handle_file.
  void DoScan();

public:
  string ToString() const;

private:
  net::Selector & selector_;
  // miliseconds between consecutive scans
  const int64 ms_between_scans_;
  // callback called with every reported file
  HandleFile * handle_file_;

  // this guy is actually doing the scans
  DirectoryMonitor monitor_;

  // callback to DoScan
  Closure * do_scan_callback_;
  // true if we're running
  bool is_running_;
};

};

#endif /*DIRECTORY_ACTIVE_MONITOR_H_*/
