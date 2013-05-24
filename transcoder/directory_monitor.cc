// Copyright (c) 2009, Whispersoft s.r.l.
// All rights reserved.
//
// Author: Cosmin Tudorache
//
//////////////////////////////////////////////////////////////////////

#include <algorithm>
#include <whisperlib/common/base/errno.h>
#include <whisperlib/common/base/log.h>
#include <whisperlib/common/base/strutil.h>
#include <whisperlib/common/io/ioutil.h>
#include "directory_monitor.h"

namespace manager {

DirectoryMonitor::DirectoryMonitor(const std::string& dir_path,
                                   re::RE* regexp)
  : dir_path_(dir_path), regexp_(regexp), entries_() {
}
DirectoryMonitor::~DirectoryMonitor() {
}

const std::string& DirectoryMonitor::dir_path() const {
  return dir_path_;
}
const re::RE* DirectoryMonitor::regexp() const {
  return regexp_;
}

bool DirectoryMonitor::Scan(std::set<std::string>* existing,
                            std::set<std::string>* changes) {
  std::set<std::string> now;
  bool success = ScanNow(dir_path_, now, regexp_);
  if(!success) {
    LOG_ERROR << "Failed to scan: " << GetLastSystemErrorDescription();
    return false;
  }
  if(existing) {
    existing->clear();
    *existing = now;
  }
  if(changes) {
    changes->clear();
    Diff(now, entries_, *changes);
  }
  entries_ = now;
  return true;
}

// static
bool DirectoryMonitor::ScanNow(const std::string& path,
                               std::set<std::string>& out, re::RE* regexp) {
  std::vector<std::string> filenames;
  if ( !io::DirList(path,
                    io::LIST_FILES | io::LIST_SYMLINKS | io::LIST_RECURSIVE,
                    regexp, &filenames) ) {
    LOG_ERROR << "ioutil::DirList failed: "
              << GetLastSystemErrorDescription();
    return false;
  }
  LOG_DEBUG << "ScanNow path: [" << path << "] regexp: "
            << (regexp ? regexp->regex() : std::string("NULL"))
            << " found files: " << strutil::ToString(filenames);
  out.clear();
  out.insert(filenames.begin(), filenames.end());
  return true;
}

// static
void DirectoryMonitor::Diff(const std::set<std::string>& a,
                            const std::set<std::string>& b,
                            std::set<std::string>& diff) {
  diff.clear();
  set_difference(a.begin(), a.end(),
                 b.begin(), b.end(),
                 std::inserter(diff, diff.begin()));
}

string DirectoryMonitor::ToString() const {
  return strutil::StringPrintf(
      "DirectoryMonitor{dir_path_: %s, regexp_: %s, entries_: %s}",
      dir_path_.c_str(),
      regexp_ == NULL ? "NULL" : regexp_->regex().c_str(),
      strutil::ToString(entries_).c_str());
}
};
