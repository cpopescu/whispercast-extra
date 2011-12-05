// Copyright (c) 2009, Whispersoft s.r.l.
// All rights reserved.
//
// Author: Catalin Popescu
//
// We periodically watch directories and find new ready directories to process
//
//////////////////////////////////////////////////////////////////////
#ifndef __DISABLE_FEATURES__

#ifndef UINT64_C
#define UINT64_C(value) value ## ULL
#endif

extern "C" {
#pragma GCC diagnostic ignored "-Wall"
#include __AV_CODEC_INCLUDE_FILE
#pragma GCC diagnostic warning "-Wall"
}
#endif

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

#include "whisperproc_util.h"
#ifndef __DISABLE_FEATURES__
#include "whisperproc_thumbnails.h"
#endif

DECLARE_bool(autogenerate_thumbnails);

//////////////////////////////////////////////////////////////////////

class Whisperproc : public app::App {
 public:
  Whisperproc(int &argc, char **&argv);
  virtual ~Whisperproc() {
  }

 protected:
  int Initialize() {
    common::Init(argc_, argv_);

    selector_ = new net::Selector();
    post_processor_ = new whisperproc::PostProcessor();
#ifndef __DISABLE_FEATURES__
    if ( FLAGS_autogenerate_thumbnails ) {
      thumbnailer_ = new whisperproc::Thumbnailer(selector_);
    }
#endif
    return 0;
  }

  void Run() {
    post_processor_->Start();
    selector_->Loop();
  }
  void Stop() {
    selector_->RunInSelectLoop(NewCallback(selector_,
                                           &net::Selector::MakeLoopExit));
  }
  void Shutdown() {
    delete selector_;
    selector_ = NULL;
#ifndef __DISABLE_FEATURES__
    delete thumbnailer_;
    thumbnailer_ = NULL;
#endif
    delete post_processor_;
    post_processor_ = NULL;
  }

 private:
  net::Selector* selector_;

#ifndef __DISABLE_FEATURES__
  whisperproc::Thumbnailer* thumbnailer_;
#endif
  whisperproc::PostProcessor* post_processor_;
  DISALLOW_EVIL_CONSTRUCTORS(Whisperproc);
};

//////////////////////////////////////////////////////////////////////

int main(int argc, char* argv[]) {
#ifndef __DISABLE_FEATURES__
  /* libavcodec */
  avcodec_init();
  avcodec_register_all();
#endif
  Whisperproc app(argc, argv);
  return app.Main();
}

//////////////////////////////////////////////////////////////////////

Whisperproc::Whisperproc(int &argc, char **&argv)
  : app::App(argc, argv),
    selector_(NULL),
#ifndef __DISABLE_FEATURES__
    thumbnailer_(NULL),
#endif
    post_processor_(NULL) {
}
