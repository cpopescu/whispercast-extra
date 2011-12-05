// Copyright (c) 2009, Whispersoft s.r.l.
// All rights reserved.
//
// Authors: Catalin Popescu
//
#include <whisperlib/common/base/types.h>
#include <whisperlib/common/base/log.h>
#include <whisperlib/common/base/system.h>
#include <whisperlib/common/base/gflags.h>
#include <whisperlib/common/base/errno.h>
#include <whisperlib/common/io/ioutil.h>

#include  "features/util/flv_thumbnailer.h"
#include  "features/util/rtmp_thumbnailer.h"

//////////////////////////////////////////////////////////////////////

DEFINE_string(input,
              "",
              "It can be either a FLV file, or a RTMP stream.");

DEFINE_int32(start_ms,
             1000,
             "The first snapshot is taken at this time.");

DEFINE_int32(repeat_ms,
             3000,
             "Take snapshot every so often.");

DEFINE_int32(end_ms,
             kMaxInt32,
             "Stop taking snapshots at this time.");

DEFINE_string(output_dir,
              "",
              "Where to write output thumbnails");
DEFINE_string(output_basename,
              "",
              "Basename for ouput thumbnails. e.g. jpeg_name='abc' =>"
              " output images are: abc-0000000000.jpg, abc-0000001000.jpg,"
              " ..., where the big 00 number is the snapshot ms timestamp.");
DEFINE_int32(jpeg_quality,
             75,
             "In range: 1-100");
DEFINE_int32(jpeg_width,
             0,
             "Width of the destination (0 - use original if height not set, "
             "or use corresponding width to keep aspect ratio - if set)");
DEFINE_int32(jpeg_height,
             0,
             "Height of the destination (0 - use original, if width not set, "
             "or use corresponding height to keep aspect ratio - if set)");

//////////////////////////////////////////////////////////////////////

int main(int argc, char* argv[]) {
  common::Init(argc, argv);
  CHECK(!FLAGS_input.empty()) << " Please specify an input file/stream !";
  CHECK(!FLAGS_output_dir.empty()) << " Please specify an output dir";
  CHECK_GE(FLAGS_end_ms, FLAGS_start_ms);
  CHECK_GE(FLAGS_jpeg_quality, 1);
  CHECK_LE(FLAGS_jpeg_quality, 100);

  /* libavcodec */
  avcodec_init();
  avcodec_register_all();

  // Maybe Process a RTMP stream
  if ( strutil::StrStartsWith(FLAGS_input, "rtmp://") ) {
    return util::RtmpExtractThumbnails(FLAGS_input,
                                       FLAGS_start_ms,
                                       FLAGS_repeat_ms,
                                       FLAGS_end_ms,
                                       FLAGS_jpeg_width,
                                       FLAGS_jpeg_height,
                                       FLAGS_jpeg_quality,
                                       FLAGS_output_dir,
                                       FLAGS_output_basename) ? 0 : 1;
  }
  // Process a FLV file
  return util::FlvExtractThumbnails(FLAGS_input,
                                    FLAGS_start_ms,
                                    FLAGS_repeat_ms,
                                    FLAGS_end_ms,
                                    FLAGS_jpeg_width,
                                    FLAGS_jpeg_height,
                                    FLAGS_jpeg_quality,
                                    FLAGS_output_dir) ? 0 : 1;

}
