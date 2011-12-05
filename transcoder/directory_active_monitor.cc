// Copyright (c) 2009, Whispersoft s.r.l.
// All rights reserved.
//
// Author: Cosmin Tudorache
//
//////////////////////////////////////////////////////////////////////

#include <sys/stat.h>
#include <whisperlib/common/base/errno.h>
#include <whisperlib/common/base/strutil.h>
#include "directory_active_monitor.h"

namespace manager {

DirectoryActiveMonitor::DirectoryActiveMonitor(net::Selector& selector,
                                               const std::string& dirname,
                                               re::RE* regexp,
                                               int64 ms_between_scans,
                                               HandleFile* handle_file)
  : selector_(selector),
    ms_between_scans_(ms_between_scans),
    handle_file_(handle_file),
    monitor_(dirname, regexp),
    do_scan_callback_(
        NewPermanentCallback(this, &DirectoryActiveMonitor::DoScan)),
    is_running_(false) {
  CHECK_NOT_NULL(handle_file);
  CHECK(handle_file->is_permanent());
}
DirectoryActiveMonitor::~DirectoryActiveMonitor() {
  Stop();
}

bool DirectoryActiveMonitor::Start() {
  CHECK(!IsRunning());

  struct stat st;
  int result = ::stat(monitor_.dir_path().c_str(), &st);
  if(result != 0) {
    LOG_ERROR << "::stat failed for dirname: [" << monitor_.dir_path()
              << "] error: " << GetLastSystemErrorDescription();
    return false;
  }
  if(!S_ISDIR(st.st_mode)) {
    LOG_ERROR << "Not a directory: [" << monitor_.dir_path() << "]";
    return false;
  }
  selector_.RunInSelectLoop(do_scan_callback_);
  is_running_ = true;
  LOG_INFO << "Started monitoring directory: [" << monitor_.dir_path()
           << "] for files of regexp type: ["
           << (monitor_.regexp() ? monitor_.regexp()->regex() : "null") << "]";
  return true;
}

bool DirectoryActiveMonitor::IsRunning() {
  return is_running_;
}

void DirectoryActiveMonitor::Stop() {
  if(!IsRunning()) {
    return;
  }
  selector_.UnregisterAlarm(do_scan_callback_);
  is_running_ = false;
}

bool DirectoryActiveMonitor::Scan(std::set<std::string>* existing,
                                  std::set<std::string>* recent) {
  return monitor_.Scan(existing, recent);
}

void DirectoryActiveMonitor::DoScan() {
  std::set<std::string> files;
  bool success = Scan(NULL, &files);
  if(!success) {
    LOG_ERROR << "Scan failed: " << GetLastSystemErrorDescription();
    return;
  }
  LOG_DEBUG << "Scan: " << strutil::ToString(files)
            << " new files found. Next scan in "
            << ms_between_scans_/1000 << " seconds";
  for(std::set<std::string>::const_iterator it = files.begin();
      it != files.end(); ++it) {
    const std::string& filename = *it;
    handle_file_->Run(filename);
  }
  selector_.RegisterAlarm(do_scan_callback_, ms_between_scans_);
}

string DirectoryActiveMonitor::ToString() const {
  return strutil::StringPrintf(
      "DirectoryActiveMonitor{"
      "ms_between_scans_: %"PRId64", monitor_: %s, is_running_: %s}",
      ms_between_scans_,
      monitor_.ToString().c_str(),
      strutil::BoolToString(is_running_).c_str());
}
}  // namespace manager
