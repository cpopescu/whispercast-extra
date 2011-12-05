// Copyright (c) 2009, Whispersoft s.r.l.
// All rights reserved.
//
// Author: Cosmin Tudorache
//
//////////////////////////////////////////////////////////////////////

#ifndef DIRECTORY_MONITOR_H_
#define DIRECTORY_MONITOR_H_

#include <string>
#include <set>
#include <whisperlib/common/base/re.h>

namespace manager {

// A scan for files utility. It scans a given directory and returns all
// files found (it also looks into subdirectories), and all recent files
// newer than last scan.
// This class deals only with files. An empty directory is not reported.
//

class DirectoryMonitor {
public:
  // dir_path: the directory to be monitored. It must exist.
  // regexp: regular expression to match reported files.
  //         Only files& directories matching this regexp are reported.
  //         If NULL all entries are reported.
  //         We won't delete it on destructor.
  DirectoryMonitor(const std::string& dir_path,
                   re::RE* regexp = NULL);
  ~DirectoryMonitor();

  const std::string& dir_path() const;
  const re::RE* regexp() const;

  // Makes a new scan, updates internal state, possibly returns all
  //  entries found + recent entries from the last scan.
  // On the first scan the 'recent' are identical with 'existing'.
  //input:
  // [OUT] existing: if not NULL, filled with current entries.
  // [OUT] recent: if not NULL, filled with files created after the last scan.
  //                On the first scan this is filled with all current
  //                entries (identical with 'existing').
  //returns:
  //  true - no error.
  //  false - some error encountered. Check GetLastSystemErrorDescription().
  bool Scan(std::set<std::string>* existing, std::set<std::string>* recent);

  // Scans 'path' which should be a directory path and fills 'out' with the
  // entries found. If regexp is provided only mathing entries are reported.
  static bool ScanNow(const std::string& path,
                      std::set<std::string>& out, re::RE* regexp = NULL);

  // makes: diff = a - b
  static void Diff(const std::set<std::string>& a,
                   const std::set<std::string>& b,
                   std::set<std::string>& diff);

  string ToString() const;

private:
  const std::string dir_path_;
  re::RE* regexp_;

  // entries list on the last scan
  std::set<std::string> entries_;
};
}

#endif  // DIRECTORY_MONITOR_H_
