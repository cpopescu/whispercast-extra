// Copyright (c) 2009, Whispersoft s.r.l.
// All rights reserved.
//
// Author: Catalin Popescu
//

#include <stdio.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/types.h>

#include <whisperlib/common/io/file/file_input_stream.h>
#include <whisperlib/common/io/buffer/memory_stream.h>
#include <whisperstreamlib/base/media_file_reader.h>

#include "feature_extractor.h"
#include "../util/jpeg_util.h"

namespace feature {

FlvFeatureExtractor::FlvFeatureExtractor(const string& flv_file,
                                         const string& jpeg_dir,
                                         int64 start_timestamp_ms,
                                         int64 end_timestamp_ms,
                                         int feature_width,
                                         int feature_height,
                                         int x0, int x1, int y0, int y1)
    : flv_file_(flv_file),
      jpeg_dir_(jpeg_dir),
      start_timestamp_ms_(start_timestamp_ms),
      end_timestamp_ms_(end_timestamp_ms),
      x0_(x0), x1_(x1), y0_(y0), y1_(y1),
      close_file_(false),
      processed_(false),
      extractor_(new feature::BitmapExtractor(feature_width, feature_height,
                                              PIX_FMT_RGB24)),
      feature_type_(FeatureData::FEATURE_RAW),
      feature_use_yuv_(false),
      feature_size_(0) {
}

FlvFeatureExtractor::~FlvFeatureExtractor() {
  delete extractor_;
}

void FlvFeatureExtractor::ProcessImage(
    const feature::BitmapExtractor::PictureStruct* picture) {
  if ( picture->timestamp_ < start_timestamp_ms_ ) {
    return;
  }
  FeatureData* feature = new FeatureData();
  if ( feature->Extract(feature_type_, feature_use_yuv_,
                        x0_, x1_, y0_, y1_,
                        feature_size_,
                        picture) ) {
    if ( features_ != NULL ) {
      if ( !jpeg_dir_.empty() ) {
        util::JpegSave(picture, 80, strutil::StringPrintf(
            "%s/feature-%"PRId64"-%s.jpg", jpeg_dir_.c_str(),
            picture->timestamp_, strutil::Basename(flv_file_).c_str()).c_str());
      }
      features_->set_feature_width(picture->width_);
      features_->set_feature_height(picture->height_);
      features_->AddFeature(picture->timestamp_ - start_timestamp_ms_,
                            feature);
    }
    if ( feature_callback_ != NULL ) {
      feature_callback_->Run(picture, feature);
    }
  } else {
    LOG_ERROR << " Error extracting frame feature data @: "
              <<  picture->timestamp_
              << " for: " << flv_file_;
  }
  if ( picture->timestamp_ >= end_timestamp_ms_ ) {
    close_file_ = true;
  }
}

bool FlvFeatureExtractor::ProcessFile(
    FeatureData::FeatureType feature_type,
    bool feature_use_yuv,
    int feature_size,
    FeatureSet* features,
    FeatureCallback* feature_callback) {
  CHECK(features == NULL ||
        features->feature_size() == 0);
  CHECK(!processed_);
  processed_ = true;

  feature_type_ = feature_type;
  feature_use_yuv_ = feature_use_yuv;
  feature_size_ = feature_size;
  features_ = features;
  feature_callback_ = feature_callback;

  streaming::MediaFileReader reader;
  if ( !reader.Open(flv_file_) ) {
    LOG_ERROR << "Cannot open for thumbnailing: " << flv_file_;
    return false;
  }
  if ( reader.splitter()->media_format() != streaming::MFORMAT_FLV ) {
    LOG_ERROR << "Not a FLV file: " << flv_file_;
    return false;
  }

  while ( true ) {
    int64 timestamp_ms;
    scoped_ref<streaming::Tag> tag;
    streaming::TagReadStatus err = reader.Read(&tag, &timestamp_ms);
    if ( err == streaming::READ_EOF ) {
      break;
    }
    if ( err == streaming::READ_SKIP ) {
      continue;
    }
    if ( err != streaming::READ_OK ) {
      LOG_ERROR << "Failed to read next tag, result: "
                << streaming::TagReadStatusName(err);
      break;
    }

    vector<scoped_ref<const BitmapExtractor::PictureStruct> > pictures;
    extractor_->GetNextPictures(tag.get(), timestamp_ms, &pictures);
    for ( uint32 i = 0; i < pictures.size(); i++ ) {
      ProcessImage(pictures[i].get());
    }
  };

  features_ = NULL;
  return true;
}
}
