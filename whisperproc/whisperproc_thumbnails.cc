// Copyright (c) 2009, Whispersoft s.r.l.
// All rights reserved.
//
// Author: Catalin Popescu
//
//////////////////////////////////////////////////////////////////////

#include <whisperlib/common/base/log.h>
#include <whisperlib/common/base/gflags.h>

#include <whisperlib/net/rpc/lib/codec/json/rpc_json_decoder.h>
#include <whisperlib/common/io/ioutil.h>
#include <whisperlib/common/io/file/file_input_stream.h>
#include "private/features/util/flv_thumbnailer.h"
#include "whisperproc_thumbnails.h"

//////////////////////////////////////////////////////////////////////

DECLARE_string(thumbnail_processing_dir);
DECLARE_string(thumbnail_output_dir);
DECLARE_int32(thumbnail_processing_threads);
DECLARE_int32(thumbnail_default_quality);

//////////////////////////////////////////////////////////////////////

namespace whisperproc {

Thumbnailer::Thumbnailer(net::Selector* selector)
    : selector_(selector),
      pool_(FLAGS_thumbnail_processing_threads,
            FLAGS_thumbnail_processing_threads + 1),
      processing_callback_(
          NewPermanentCallback(this,
                               &Thumbnailer::StartThumbnailProcessing,
                               true)) {
  CHECK(!FLAGS_thumbnail_processing_dir.empty());
  selector_->RegisterAlarm(processing_callback_, kProcessingIntervalMs);
}

Thumbnailer::~Thumbnailer() {
  selector_->UnregisterAlarm(processing_callback_);
  delete processing_callback_;
}

void Thumbnailer::StartThumbnailProcessing(bool periodic_callback) {
  if ( periodic_callback ) {
    selector_->RegisterAlarm(processing_callback_,
                               kProcessingIntervalMs);
  }
  vector<string> files;
  if ( !io::DirList(FLAGS_thumbnail_processing_dir + "/", &files,
                    false, NULL) ) {
    LOG_ERROR << " Cannot list directory: " << FLAGS_thumbnail_processing_dir;
    return;
  }
  if ( files.empty() ) {
    return;
  }
  set<string> crt_chosen;
  while ( current_processing_.size() < FLAGS_thumbnail_processing_threads ) {
    string* chosen_file = new string();
    for ( int i = 0; i < files.size(); ++i ) {
      const string crt(strutil::JoinPaths(FLAGS_thumbnail_processing_dir,
                                          files[i]));
      if ( crt_chosen.find(crt) == crt_chosen.end() &&
           current_processing_.find(crt) == current_processing_.end() &&
           bad_files_.find(crt) == bad_files_.end() ) {
        *chosen_file = crt;
        break;
      }
    }
    if ( chosen_file->empty() ) {
      delete chosen_file;
      return;
    }
    crt_chosen.insert(*chosen_file);
    ProcessFile(chosen_file);
  }
}

void Thumbnailer::ClearFile(const string& f) {
  LOG_INFO << " Deleting: " << f;
  if ( unlink(f.c_str()) ) {
    bad_files_.insert(f);
    LOG_ERROR << " Error deleting file: " << f
              << " - declaring bad";
  }
}

void Thumbnailer::ProcessFile(const string* chosen_file) {
  string content;
  ThumbnailCommand* tc = new ThumbnailCommand();
  if ( !io::FileInputStream::TryReadFile(chosen_file->c_str(),
                                         &content) ||
       !rpc::JsonDecoder::DecodeObject(content, tc) ) {
    LOG_ERROR << " Cannot read thumbnail command file: " << *chosen_file;
    ClearFile(*chosen_file);
    delete tc;
    delete chosen_file;
    return;
  }
  current_processing_.insert(*chosen_file);
  pool_.jobs()->Put(NewCallback(this,
                                &Thumbnailer::CreateThumbnails,
                                chosen_file,
                                reinterpret_cast<const ThumbnailCommand*>(tc)));
}

void Thumbnailer::CreateThumbnails(const string* chosen_file,
                                   const ThumbnailCommand* tc) {
  io::Mkdir(tc->output_dir_.get());

  LOG_INFO << " Generating thumbnail(s) for " << tc->file_name_;
  if ( !util::FlvExtractThumbnails(
          tc->file_name_,
          tc->start_ms_.get(),
          tc->repeat_ms_.get(),
          tc->end_ms_.get(),
          tc->width_.is_set() ? tc->width_.get() : 0,
          tc->height_.is_set() ? tc->height_.get() : 0,
          tc->quality_.is_set() ? tc->quality_.get()
                                : FLAGS_thumbnail_default_quality,
          tc->output_dir_) ) {
    LOG_ERROR << "Error processing thumbnails for file: ["
              << tc->file_name_
              << "] per command file: " << *chosen_file;
  }
  selector_->RunInSelectLoop(NewCallback(this,
                                         &Thumbnailer::CompleteThumbnails,
                                         chosen_file, tc));
}

void Thumbnailer::CompleteThumbnails(const string* chosen_file,
                                     const ThumbnailCommand* tc) {
  current_processing_.erase(*chosen_file);
  ClearFile(*chosen_file);
  delete chosen_file;
  delete tc;
}

}
