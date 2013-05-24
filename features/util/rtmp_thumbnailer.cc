// Copyright (c) 2011, Whispersoft s.r.l.
// All rights reserved.
//
// Author: Cosmin Tudorache
//

#include <whisperlib/common/base/types.h>
#include <whisperlib/common/base/log.h>
#include <whisperlib/common/base/system.h>
#include <whisperlib/common/base/gflags.h>
#include <whisperlib/common/base/errno.h>
#include <whisperlib/common/io/ioutil.h>

#include <whisperstreamlib/rtmp/rtmp_client.h>

#include "features/util/rtmp_thumbnailer.h"
#include "features/util/jpeg_util.h"

namespace util {

RtmpThumbnailer::RtmpThumbnailer(net::Selector* selector, const string& url,
    int64 start_ms, int64 repeat_ms, int64 end_ms,
    int32 jpeg_width, int32 jpeg_height, int32 jpeg_quality,
    const string& output_dir, const string output_basename,
    Closure* finish_callback)
    : selector_(selector),
      url_(url),
      start_ms_(start_ms),
      repeat_ms_(repeat_ms),
      end_ms_(end_ms),
      jpeg_quality_(jpeg_quality),
      jpeg_width_(jpeg_width),
      jpeg_height_(jpeg_height),
      output_dir_(output_dir),
      output_basename_(output_basename),
      finish_callback_(finish_callback),
      extractor_(jpeg_width, jpeg_height, PIX_FMT_RGB24),
      client_(selector),
      next_snapshot_ms_(start_ms) {
  CHECK_NOT_NULL(finish_callback_);
  client_.set_read_handler(
      NewPermanentCallback(this, &RtmpThumbnailer::ReadHandler));
  client_.set_close_handler(
      NewPermanentCallback(this, &RtmpThumbnailer::CloseHandler));
}
RtmpThumbnailer::~RtmpThumbnailer() {
}

bool RtmpThumbnailer::Start() {
  io::Mkdir(output_dir_, true);
  return client_.Open(url_);
}

void RtmpThumbnailer::ReadHandler() {
  while ( true ) {
    scoped_ref<streaming::FlvTag> tag = client_.PopTag();
    if ( tag.get() == NULL ) {
      break;
    }
    vector<scoped_ref<const feature::BitmapExtractor::PictureStruct> > pics;
    extractor_.GetNextPictures(tag.get(), tag->timestamp_ms(), &pics);
    for ( uint32 i = 0; i < pics.size(); i++ ) {
      const feature::BitmapExtractor::PictureStruct* pic = pics[i].get();
      if ( pic->timestamp_ < next_snapshot_ms_ ) {
        continue;
      }

      string basename = output_basename_;
      if ( basename == "" ) {
        // create a name based on stream name
        vector<string> v;
        strutil::SplitString(client_.stream_name(), "/", &v);
        basename = strutil::JoinStrings(v, "-");
      }
      const string filename = strutil::JoinPaths(
          output_dir_,
          strutil::StringPrintf("%s-%010"PRId64".jpg",
              basename.c_str(),
              pic->timestamp_));
      if ( !JpegSave(pic, jpeg_quality_, filename) ) {
        client_.Close();
        return;
      }
      LOG_INFO << "Snapshot at: " << next_snapshot_ms_
               << ", Output File: [" << filename << "], next snapshot at: "
               << (next_snapshot_ms_ + repeat_ms_) << " ms";
      next_snapshot_ms_ += repeat_ms_;
      if ( next_snapshot_ms_ > end_ms_ ) {
        client_.Close();
        return;
      }
    }
  }
}
void RtmpThumbnailer::CloseHandler() {
  finish_callback_->Run();
}

namespace {
void RtmpThumbnailerFinish(net::Selector* selector) {
  selector->MakeLoopExit();
}
}

bool RtmpExtractThumbnails(const string& url,
                          int64 start_ms,
                          int64 repeat_ms,
                          int64 end_ms,
                          int jpeg_width, int jpeg_height, int jpeg_quality,
                          const string& output_dir,
                          const string& output_basename) {
  net::Selector selector;
  RtmpThumbnailer thumbnailer(&selector, url,
      start_ms, repeat_ms, end_ms,
      jpeg_width, jpeg_height, jpeg_quality,
      output_dir, output_basename,
      NewCallback(&RtmpThumbnailerFinish, &selector));
  if ( !thumbnailer.Start() ) {
    LOG_ERROR << "Failed to start thumbnailer";
    return false;
  }
  selector.Loop();
  return true;
}

}
