// Copyright (c) 2009, Whispersoft s.r.l.
// All rights reserved.
//
// Author: Catalin Popescu
//
// We periodically watch directories and find new ready directories to process
//
//////////////////////////////////////////////////////////////////////

#include <whisperlib/common/app/app.h>

#include <whisperlib/common/base/types.h>
#include <whisperlib/common/base/log.h>
#include <whisperlib/common/base/system.h>
#include <whisperlib/common/base/date.h>
#include <whisperlib/common/base/gflags.h>
#include <whisperlib/common/base/errno.h>
#include <whisperlib/common/base/strutil.h>
#include <whisperlib/common/sync/thread.h>
#include <whisperlib/common/sync/thread_pool.h>
#include <whisperlib/common/base/callback.h>
#include <whisperlib/common/base/re.h>
#include <whisperlib/common/io/ioutil.h>
#include <whisperlib/common/io/file/file.h>
#include <whisperlib/common/io/file/file_input_stream.h>
#include <whisperlib/common/io/file/file_output_stream.h>

#include <whisperlib/net/base/selector.h>

#include "processor.h"

DEFINE_string(media_dir,
              "",
              "Media to be processed is under this directory");
DEFINE_string(output_format,
              "",
              "Output files type. e.g.: flv, f4v");

DEFINE_int32(output_media_minutes,
             20,
             "Build media files of approximately these many minutes");

DEFINE_int32(max_delay_minutes,
             20,
             "If there are not enough files for a full 'output_media_minutes'"
             " but the last saved file is older than these minutes, then process"
             " them anyway into a smaller output. Useful at the end of save.");

DEFINE_int32(cue_ms,
             5000,
             "In joined files we insert cuepoints every so many milliseconds");
DEFINE_bool(delete_part_files,
            false,
            "If enabled, we delete the files that we joined.");

DEFINE_int32(periodic_scan_sec,
             60,
             "We look for new materials every these many seconds.");

//////////////////////////////////////////////////////////////////////

class Whisperproc : public app::App {
 public:
  Whisperproc(int &argc, char **&argv)
    : app::App(argc, argv),
      selector_(NULL),
      processor_(NULL) {
  }
  virtual ~Whisperproc() {
    CHECK_NULL(selector_);
    CHECK_NULL(processor_);
  }

 protected:
  int Initialize() {
    common::Init(argc_, argv_);

    streaming::MediaFormat output_format;
    if ( !streaming::MediaFormatFromSmallType(FLAGS_output_format,
                                              &output_format) ) {
      LOG_ERROR << "Invalid output format: [" << FLAGS_output_format << "]";
      return 1;
    }

    selector_ = new net::Selector();
    processor_ = new whisperproc::Processor(FLAGS_media_dir,
                                            output_format,
                                            FLAGS_output_media_minutes*60000LL,
                                            FLAGS_max_delay_minutes*60000LL,
                                            FLAGS_cue_ms,
                                            FLAGS_periodic_scan_sec*1000LL,
                                            FLAGS_delete_part_files);
    if ( !processor_->Start() ) {
      LOG_ERROR << "Failed to start processor_";
      return 1;
    }
    return 0;
  }

  void Run() {
    selector_->Loop();
  }
  void Stop() {
    processor_->Stop();
    selector_->RunInSelectLoop(NewCallback(selector_,
        &net::Selector::MakeLoopExit));
  }
  void Shutdown() {
    delete processor_;
    processor_ = NULL;
    delete selector_;
    selector_ = NULL;
  }

 private:
  net::Selector* selector_;
  whisperproc::Processor* processor_;

  DISALLOW_EVIL_CONSTRUCTORS(Whisperproc);
};

//////////////////////////////////////////////////////////////////////

int main(int argc, char* argv[]) {
  Whisperproc app(argc, argv);
  return app.Main();
}
