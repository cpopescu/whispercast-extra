// Copyright (c) 2009, Whispersoft s.r.l.
// All rights reserved.
//
// Author: Catalin Popescu
//

#ifndef __FEATURE_FEATURE_EXTRACTOR_H__
#define __FEATURE_FEATURE_EXTRACTOR_H__

#include <whisperstreamlib/flv/flv_tag.h>
#include <whisperstreamlib/flv/flv_tag_splitter.h>

#include "features/decoding/bitmap_extractor.h"
#include "features/decoding/feature_data.h"

namespace feature {

class FlvFeatureExtractor {
 public:
  FlvFeatureExtractor(
      const string& flv_file,
      const string& jpeg_dir,
      // Time cropping
      int64 start_timestamp_ms, int64 end_timestamp_ms,
      // Image scaling
      int feature_width, int feature_height,
      // Image crop
      int x0, int x1, int y0, int y1);
  virtual ~FlvFeatureExtractor();

  typedef Callback2<const feature::BitmapExtractor::PictureStruct*,
                    const FeatureData*> FeatureCallback;
  bool ProcessFile(FeatureData::FeatureType feature_type,
                   bool feature_use_yuv,
                   int feature_size,
                   FeatureSet* features,                // can be null
                   FeatureCallback* feature_callback);  // can be null

 private:
  void ProcessImage(const feature::BitmapExtractor::PictureStruct* picture);

  const string flv_file_;
  const string jpeg_dir_;
  const int64 start_timestamp_ms_;
  const int64 end_timestamp_ms_;
  const int x0_, x1_, y0_, y1_;

  bool close_file_;
  bool processed_;

  feature::BitmapExtractor* extractor_;

  FeatureData::FeatureType feature_type_;
  bool feature_use_yuv_;
  int feature_size_;
  FeatureSet* features_;
  FeatureCallback* feature_callback_;

  static const int kDefaultBufferSize = 65536;

  DISALLOW_EVIL_CONSTRUCTORS(FlvFeatureExtractor);
};

}

#endif  // __FEATURE_FEATURE_EXTRACTOR_H__
