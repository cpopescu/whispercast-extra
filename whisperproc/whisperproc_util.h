// Copyright (c) 2009, Whispersoft s.r.l.
// All rights reserved.
//
// Author: Catalin Popescu
//

#ifndef __WHISPERPROC_UTIL_H__
#define __WHISPERPROC_UTIL_H__

#include <whisperlib/common/base/types.h>
#include <whisperlib/common/base/gflags.h>
#include <whisperlib/common/base/re.h>
#include <whisperlib/net/base/selector.h>


//////////////////////////////////////////////////////////////////////

DECLARE_string(content_type_file);
DECLARE_string(default_content_type);
DECLARE_int32(defult_buildup_period_min);
DECLARE_int32(defult_buildup_delay_min);
DECLARE_int32(min_buildup_period_min);
DECLARE_int32(max_buildup_period_min);
DECLARE_int32(min_buildup_delay_min);
DECLARE_int32(max_buildup_delay_min);
DECLARE_int32(cue_ms_time);
DECLARE_bool(delete_part_files);
DECLARE_string(files_to_process);
DECLARE_string(done_message_file);
DECLARE_string(processed_media_file);
DECLARE_string(files_suffix);
DECLARE_string(done_file);
DECLARE_string(begin_file);
DECLARE_string(processing_file);
DECLARE_string(base_media_dir);

//////////////////////////////////////////////////////////////////////

namespace whisperproc {

bool StartProcessing(thread::ThreadPool* pool,
                     const string& path);
string PrepareRegex(const string& s);

class PostProcessor {
 public:
  PostProcessor();
  ~PostProcessor();

  void Start();

 private:
  bool FindProcessing(vector<string>* dirs);
  bool FindFresh(vector<string>* dirs);

  void ProcessProcessing();
  void ProcessFresh(bool look_again);

  net::SelectorThread selector_thread_;

  re::RE begin_file_re_;
  re::RE processing_re_;

  thread::ThreadPool pool_;

  DISALLOW_EVIL_CONSTRUCTORS(PostProcessor);
};
};

#endif  // __WHISPERPROC_UTIL_H__
