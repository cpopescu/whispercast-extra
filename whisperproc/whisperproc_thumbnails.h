// Copyright (c) 2009, Whispersoft s.r.l.
// All rights reserved.
//
// Author: Catalin Popescu
//
//////////////////////////////////////////////////////////////////////

#ifndef __WHISPERPROC_THUMBNAILS_H__
#define __WHISPERPROC_THUMBNAILS_H__

#include <set>

#include <whisperlib/common/base/types.h>
#include <whisperlib/net/base/selector.h>
#include <whisperlib/common/sync/thread_pool.h>
#include <whisperlib/common/base/callback.h>

#include "auto/whisperproc_types.h"

namespace whisperproc {

class Thumbnailer {
 public:
  Thumbnailer(net::Selector* selector);
  ~Thumbnailer();

 private:
  void StartThumbnailProcessing(bool periodic_callback);
  void ClearFile(const string& f);
  void ProcessFile(const string* chosen_file);
  void CreateThumbnails(const string* chosen_file,
                        const ThumbnailCommand* tc);
  void CompleteThumbnails(const string* chosen_file,
                          const ThumbnailCommand* tc);

 private:
  net::Selector* const selector_;
  set<string> current_processing_;
  set<string> bad_files_;
  thread::ThreadPool pool_;
  Closure* processing_callback_;

  static const int kProcessingIntervalMs = 5000;

  DISALLOW_EVIL_CONSTRUCTORS(Thumbnailer);
};

}

#endif
