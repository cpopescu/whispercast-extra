// Copyright (c) 2009, Whispersoft s.r.l.
// All rights reserved.
//
// Author: Catalin Popescu & Cosmin Tudorache
//

#ifndef __WHISPERPROC_UTIL_H__
#define __WHISPERPROC_UTIL_H__

#include <whisperlib/common/base/types.h>
#include <whisperlib/common/base/gflags.h>
#include <whisperlib/common/base/re.h>
#include <whisperlib/common/base/alarm.h>
#include <whisperlib/common/sync/thread.h>
#include <whisperlib/common/sync/event.h>
#include <whisperlib/net/base/selector.h>
#include <whisperstreamlib/base/tag_splitter.h>

namespace whisperproc {

class Processor {
 public:
  Processor(const string& media_dir,
            streaming::MediaFormat output_format,
            int64 output_media_ms,
            int64 max_delay_ms,
            int64 cue_ms,
            int64 periodic_scan_ms,
            bool delete_part_files);
  virtual ~Processor();

  // start monitoring the media_dir_
  bool Start();
  // stop the monitoring
  void Stop();

 private:
  // internal thread procedure
  // Scan the media_dir_ for processable directories, from time to time.
  void ThreadRun();

  // Returns the necessary files for a single composition.
  // All the returned files should be composed into a single flv file.
  //
  // out_next_file_ts: the timestamp of the file following the current selection
  //                   Useful when building the timerange:
  //                   [timestamp of out.begin(), out_next_file_ts]
  void GetProcessableFiles(const string& dir, vector<string>* out,
      int64* out_next_file_ts);

  //  Consume part files for a single composition, outputs a single
  //  "media_.._.." file. Moves or deletes the consumed parts.
  // dir: relative to media_dir_
  // returns: true = enough files were found and a composition was made
  //          false = not enough files
  bool ProcessDirectoryOnce(string dir);

  // Does multiple consecutive media compositions, until there are not enough
  // files to process.
  void ProcessDirectory(string dir);

 private:
  // the base of the dir tree we're going to process
  const string media_dir_;
  // output file format
  const streaming::MediaFormat output_format_;
  // output .flv files are approximatively this long
  const int64 output_media_ms_;
  // if the last saved file is older than this, then process the last bunch of
  // saved files even if there are not enough for a complete 'output_media_ms_'
  const int64 max_delay_ms_;
  // in the output files, cues are inserted at this interval
  const int64 cue_ms_;
  // interval between consecutive scans of the 'media_dir_'
  const int64 periodic_scan_ms_;
  // true = delete processed files
  // false = move them into 'done_files/'
  const bool delete_part_files_;

  // regular expression matching the "media.info" file
  re::RE info_file_re_;
  // regular expression matching "*.part" files
  re::RE part_file_re_;

  // The processing happens on this separate thread.
  thread::Thread thread_;

  // signals the internal 'thread_' it should stop
  synch::Event stop_event_;

  DISALLOW_EVIL_CONSTRUCTORS(Processor);
};

}

#endif  // __WHISPERPROC_UTIL_H__
