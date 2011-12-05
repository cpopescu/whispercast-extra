// Copyright (c) 2011, Whispersoft s.r.l.
// All rights reserved.
//
// Author: Cosmin Tudorache
//

#ifndef __FEATURE_RTMP_THUMBNAILER_H__
#define __FEATURE_RTMP_THUMBNAILER_H__

#include <whisperlib/common/base/types.h>
#include <whisperstreamlib/rtmp/rtmp_client.h>

#include "features/decoding/bitmap_extractor.h"

namespace util {

class RtmpThumbnailer {
 public:
  // finish_callback: is called when the task is completed (either all
  //                  thumbnails were processed, or the rtmp stream has ended)
  // output_basename: e.g. "abc" => output files names are:
  //                         - abc-0002500.jpg
  //                         - abc-0005000.jpg
  //                         ... where '0005000' is the ms timestamp in stream
  //                  e.g. "" , and url is rtmp://../ceva/files/spiderman.flv =>
  //                         - ceva_files_spiderman.flv-0002500.jpg
  //                         - ceva_files_spiderman.flv-0005000.jpg
  //                         ...
  RtmpThumbnailer(net::Selector* selector, const string& url,
      int64 start_ms, int64 repeat_ms, int64 end_ms,
      int32 jpeg_width, int32 jpeg_height, int32 jpeg_quality,
      const string& output_dir, const string output_basename,
      Closure* finish_callback);
  virtual ~RtmpThumbnailer();

  bool Start();

 private:
  void ReadHandler();
  void CloseHandler();

 private:
  net::Selector* selector_;
  const string url_;
  const int64 start_ms_;
  const int64 repeat_ms_;
  const int64 end_ms_;
  const int32 jpeg_quality_;
  const int32 jpeg_width_;
  const int32 jpeg_height_;
  const string output_dir_;
  const string output_basename_;

  // called when processing terminates, or on error
  Closure* finish_callback_;

  // extracts images
  feature::BitmapExtractor extractor_;

  // receives rtmp stream
  rtmp::SimpleClient client_;

  // timestamp (relative to stream begin) of the next snapshot
  int64 next_snapshot_ms_;
};

//
// Utility for dumping thumbnails from a rtmp stream.
// Blocks until all pictures are extracted, or the rtmp stream gets
// closed (by server or by error).
//
bool RtmpExtractThumbnails(const string& url,
                          int64 start_ms,
                          int64 repeat_ms,
                          int64 end_ms,
                          int jpeg_width, int jpeg_height, int jpeg_quality,
                          const string& output_dir,
                          const string& output_basename);

} // namespace util

#endif // __FEATURE_RTMP_THUMBNAILER_H__
