// Copyright (c) 2009, Whispersoft s.r.l.
// All rights reserved.
//
// Author: Catalin Popescu
//

#ifndef __FEATURE_FEATURE_DATA_H__
#define __FEATURE_FEATURE_DATA_H__

#include  "features/decoding/bitmap_extractor.h"
#include <whisperlib/common/io/buffer/memory_stream.h>

namespace feature {

enum ReadStatus {
  READ_OK,
  READ_NO_DATA,
  READ_INVALID_DATA,
};

enum MatchResult {
  MATCH_NO_DATA,
  NO_MATCH,
  MATCH_BROKEN,
  MATCH_BEGIN,
  MATCH_CONT,
  MATCH_COMPLETED,
  MATCH_OVER_TIME,
};

enum DistanceType {
  EUCLIDIAN,
  SHORTEST_PATH,
};
static const int kNumFeatureChannels = 3;
class FeatureData {
 public:
  enum FeatureType {
    FEATURE_RAW                = 0,
    FEATURE_WAVELET_HAAR       = 1,
    FEATURE_WAVELET_DAUBECHIES = 2,
  };
  FeatureData();
  ~FeatureData() {
    Clear();
  }
  bool Initialize(int x0, int x1, int y0, int y1,
                  int feature_size,
                  FeatureType type, bool use_yuv);
  void Clear();

  FeatureData* Clone() const;

  bool Extract(FeatureType type, bool use_yuv,
               int x0, int x1, int y0, int y1,  // crop limits
               int feature_size,
               const feature::BitmapExtractor::PictureStruct* pic);

  bool EuclidianDistance(const FeatureData* d,
                         double dist[kNumFeatureChannels + 1]) const;
  bool ShortestPathDistance(const FeatureData* d,
                            double dist[kNumFeatureChannels + 1]) const;
  bool IsMatching(const FeatureData* d,
                  double max_distance[kNumFeatureChannels + 1],
                  DistanceType distance_type,
                  bool large_match) const;

  ReadStatus Load(io::MemoryStream* ms);
  void Save(io::MemoryStream* ms) const;

  string ToString() const;

  int x0() const  { return x0_; }
  int x1() const  { return x1_; }
  int y0() const  { return y0_; }
  int y1() const  { return y1_; }
  int feature_size() const         { return feature_size_; }
  bool use_yuv() const             { return use_yuv_; }
  FeatureType feature_type() const { return feature_type_; }

  const vector<double*>& features() const { return features_; }

 private:
  bool initialized_;

  int x0_, x1_, y0_, y1_;
  int feature_size_;
  bool use_yuv_;
  FeatureType feature_type_;

  vector<double*> features_;   // one vector of features for each channel

  DISALLOW_EVIL_CONSTRUCTORS(FeatureData);
};

class FeatureSet {
 public:
  FeatureSet();
  ~FeatureSet();

  int feature_size() const {
    return features_.size();
  }
  const vector< pair< int64, FeatureData* > >& features() const {
    return features_;
  }
  void AddFeature(int64 timestamp, FeatureData* data) {
    features_.push_back(make_pair(timestamp, data));
  }

  void ResetMatch() {
    last_match_ = -1;
    first_match_timestamp_ = -1;
    last_match_timestamp_ = -1;
  }
  MatchResult MatchNext(int64 timestamp, const FeatureData* data,
                        double max_distance[kNumFeatureChannels + 1],
                        DistanceType distance_type);

  ReadStatus Load(io::MemoryStream* ms);
  void Save(io::MemoryStream* ms) const;

  string ToString() const;

  int feature_width() const {
    return feature_width_;
  }
  int feature_height() const {
    return feature_height_;
  }
  void set_feature_width(int feature_width) {
    feature_width_ = feature_width;
  }
  void set_feature_height(int feature_height) {
    feature_height_ = feature_height;
  }
  int last_match() const {
    return last_match_;
  }
 private:
  int feature_width_;
  int feature_height_;
  vector< pair< int64, FeatureData* > > features_;
  int last_match_;
  int64 first_match_timestamp_;
  int64 last_match_timestamp_;

  DISALLOW_EVIL_CONSTRUCTORS(FeatureSet);
};

}

#endif   // __FEATURE_FEATURE_DATA_H__
