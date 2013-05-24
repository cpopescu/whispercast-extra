// Copyright (c) 2009, Whispersoft s.r.l.
// All rights reserved.
//
// Authors: Catalin Popescu
//

#include <whisperlib/common/base/log.h>
#include <whisperlib/common/base/gflags.h>
#include <whisperlib/common/base/system.h>
#include  "features/decoding/feature_extractor.h"
#include <whisperlib/common/io/file/file_output_stream.h>

DEFINE_string(flv_file,
              "",
              "The file to process");
DEFINE_string(feature_file,
              "",
              "We output the features to this file");
DEFINE_string(jpeg_dir,
              "",
              "We output the images from which we extracted features "
              "in this dir");
DEFINE_int64(start_timestamp_ms,
             0,
             "We start extracting features from this time");

DEFINE_int64(end_timestamp_ms,
             kMaxInt64,
             "We stop extracting features at this time");

DEFINE_int32(feature_width,
             0,
             "We scale to this width for feature extraction");
DEFINE_int32(feature_height,
             0,
             "We scale to this height for feature extraction");
DEFINE_int32(crop_x0,
             0,
             "Crop on x axis begins here");
DEFINE_int32(crop_x1,
             0,
             "Crop on x axis ends here");
DEFINE_int32(crop_y0,
             0,
             "Crop on y axis begins here");
DEFINE_int32(crop_y1,
             0,
             "Crop on y axis ends here");

DEFINE_bool(use_yuv,
            true,
            "Use YUV instead of RGB ?");
DEFINE_int32(feature_type,
             1,
             "0 - Raw/ 1 - Haar / 2 - Daubechies ?");
DEFINE_int32(feature_size,
             256,
             "Use these many coefficients in the feature");

int main(int argc, char* argv[]) {
  common::Init(argc, argv);
  CHECK(!FLAGS_flv_file.empty()) << " Please specify a flv file !";

  /* libavcodec */
#if LIBAVCODEC_VERSION_INT <= AV_VERSION_INT(53,34,0)
  avcodec_init();
#endif
  avcodec_register_all();

  feature::FlvFeatureExtractor extractor(
      FLAGS_flv_file,
      FLAGS_jpeg_dir,
      FLAGS_start_timestamp_ms, FLAGS_end_timestamp_ms,
      FLAGS_feature_width, FLAGS_feature_height,
      FLAGS_crop_x0, FLAGS_crop_x1,
      FLAGS_crop_y0, FLAGS_crop_y1);
  feature::FeatureSet result;
  if ( !extractor.ProcessFile(
           feature::FeatureData::FeatureType(FLAGS_feature_type),
           FLAGS_use_yuv,
           FLAGS_feature_size,
           &result,
           NULL) ) {
    LOG_FATAL << " Error extracting features... ";
  }

  if ( FLAGS_feature_file.empty() ) {
    LOG_INFO << " Obtained feaures: \n"
             << result.ToString();
  } else {
    io::MemoryStream out;
    result.Save(&out);
    string s;
    out.ReadString(&s);
    io::FileOutputStream::WriteFileOrDie(FLAGS_feature_file.c_str(), s);
  }
  return 0;
}
