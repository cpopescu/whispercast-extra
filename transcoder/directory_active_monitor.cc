// Copyright (c) 2009, Whispersoft s.r.l.
// All rights reserved.
//
// Author: Cosmin Tudorache
//
//////////////////////////////////////////////////////////////////////

#include <sys/stat.h>
#include <whisperlib/common/base/errno.h>
#include <whisperlib/common/base/strutil.h>
#include <whisperlib/common/io/ioutil.h>
#include "directory_active_monitor.h"

namespace manager {

DirectoryActiveMonitor::DirectoryActiveMonitor(net::Selector& selector,
                                               const string& dirname,
                                               re::RE* regexp,
                                               int64 ms_between_scans,
                                               HandleFile* handle_file,
                                               bool delete_handle_file)
  : selector_(selector),
    handle_file_(handle_file),
    delete_handle_file_(delete_handle_file),
    monitor_(dirname, regexp),
    scan_alarm_(selector) {
  CHECK_NOT_NULL(handle_file);
  CHECK(handle_file->is_permanent());
  scan_alarm_.Set(NewPermanentCallback(this, &DirectoryActiveMonitor::DoScan),
      true, ms_between_scans, true, false);
}
DirectoryActiveMonitor::~DirectoryActiveMonitor() {
  Stop();
  if ( delete_handle_file_ ) {
    delete handle_file_;
    handle_file_ = NULL;
  }
}

bool DirectoryActiveMonitor::Start() {
  CHECK(!IsRunning());

  // check that monitored directory exists
  if ( !io::IsDir(monitor_.dir_path()) ) {
    LOG_ERROR << "Not a directory: [" << monitor_.dir_path() << "]";
    return false;
  }

  scan_alarm_.Start();

  LOG_INFO << "Started monitoring directory: [" << monitor_.dir_path()
           << "] for files of regexp type: ["
           << (monitor_.regexp() ? monitor_.regexp()->regex() : "null") << "]";
  return true;
}

bool DirectoryActiveMonitor::IsRunning() const {
  return scan_alarm_.IsStarted();
}

void DirectoryActiveMonitor::Stop() {
  scan_alarm_.Stop();
}

void DirectoryActiveMonitor::DoScan() {
  set<string> new_files;
  bool success = monitor_.Scan(NULL, &new_files);
  if( !success ) {
    LOG_ERROR << "Scan failed: " << GetLastSystemErrorDescription();
    return;
  }
  LOG_DEBUG << "Scan: " << strutil::ToString(new_files)
            << " new files found. Next scan in "
            << scan_alarm_.timeout()/1000 << " seconds";
  for ( set<string>::const_iterator it = new_files.begin();
        it != new_files.end(); ++it ) {
    const string& filename = *it;
    handle_file_->Run(filename);
  }
}

string DirectoryActiveMonitor::ToString() const {
  return strutil::StringPrintf(
      "DirectoryActiveMonitor{ms_between_scans: %"PRId64""
      ", monitor_: %s, is_running: %s}",
      scan_alarm_.timeout(),
      monitor_.ToString().c_str(),
      strutil::BoolToString(IsRunning()).c_str());
}
}  // namespace manager
