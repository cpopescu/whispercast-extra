// Copyright (c) 2009, Whispersoft s.r.l.
// All rights reserved.
//
// Author: Catalin Popescu
//
#include <math.h>

#include "feature_data.h"
#include <whisperlib/common/math/wavelets.h>
#include <whisperlib/common/base/gflags.h>

DEFINE_double(path_penalty,
              1.0,
              "Penalty for skipping the normal path in shortest distance "
              "computation");
namespace feature {

FeatureData::FeatureData()
    : initialized_(false),
      x0_(0), x1_(0), y0_(0), y1_(0),
      feature_size_(0),
      use_yuv_(false),
      feature_type_(FEATURE_RAW) {
}


bool FeatureData::Initialize(int x0, int x1, int y0, int y1,
                             int feature_size,
                             FeatureType type, bool use_yuv) {
  CHECK(features_.empty());
  x0_ = x0;
  x1_ = x1;
  y0_ = y0;
  y1_ = y1;
  const int size = (y1 - y0) * (x1 - x0);
  if ( size <= 0 ) {
    LOG_ERROR << " Bad size found for FeatureData: "
              << "  x0: " << x0 << " y0: " << y0
              << "  x1: " << x1 << " y1: " << y1;
    return false;
  }
  if ( (feature_size & 1) == 1 ) {
    --feature_size;   // Insure an even feature size..
  }
  CHECK_LE(feature_size, size);
  for ( int i = 0; i < kNumFeatureChannels; ++i ) {
    double* p = new double[feature_size];
    features_.push_back(p);
  }
  feature_size_ = feature_size;
  feature_type_ = type;
  use_yuv_ = use_yuv;
  return true;
}

void FeatureData::Clear() {
  x0_ = x1_ = y0_ = y1_ = 0;
  feature_type_ = FEATURE_RAW;
  for ( int i = 0; i < features_.size(); ++i ) {
    delete features_[i];
  }
  features_.clear();
}

FeatureData* FeatureData::Clone() const {
  FeatureData* d = new FeatureData();
  d->Initialize(x0_, x1_, y0_, y1_, feature_size_, feature_type_, use_yuv_);
  for ( int i = 0; i < kNumFeatureChannels; ++i ) {
    memcpy(d->features_[i], features_[i], sizeof(double) * feature_size_);
  }
  return d;
}

// YPbPr (ITU-R BT.601)
// ========================================================
// Y' =   + 0.299    * R' + 0.587    * G' + 0.114    * B'
// Pb =   - 0.168736 * R' - 0.331264 * G' + 0.5      * B'
// Pr =   + 0.5      * R' - 0.418688 * G' - 0.081312 * B'
// ........................................................
// R', G', B' in [0; 1]
// Y' in [0; 1]
// Pb in [-0.5; 0.5]
// Pr in [-0.5; 0.5]

inline double rgb_to_Y(double r, double g, double b) {
  return (.299 * r + .587 * g + .114 * b) / 255.0;          // Y'
}
inline double rgb_to_U(double r, double g, double b) {
  return (-.14713 * r  -.28886 * g + .436 * b) / 255.0;     // U
}
inline double rgb_to_V(double r, double g, double b) {
  return (.615 * r - .51499 * g + -.10001 * b) / 255.0;      // V
}

inline double rgb_to_Pb(double r, double g, double b) {
  return (-.164713 * r  -.331264 * g + .5 * b) / 255.0;      // Pb
}
inline double rgb_to_Pr(double r, double g, double b) {
  return (.5 * r - .418688 * g + -.081312 * b) / 255.0;      // Pr
}

bool FeatureData::Extract(
    FeatureType type, bool use_yuv,
    int x0, int x1, int y0, int y1,
    int feature_size,
    const feature::BitmapExtractor::PictureStruct* pic) {
  Clear();
  CHECK_EQ(pic->pix_format_, PIX_FMT_RGB24);
  if ( !Initialize(x0 > 0 ? min(x0, pic->width_) : 0,
                   x1 > 0 ? min(x1, pic->width_) : pic->width_,
                   y0 > 0 ? min(y0, pic->height_) : 0,
                   y1 > 0 ? min(y1, pic->height_) : pic->height_,
                   feature_size,
                   type, use_yuv) ) {
    return false;
  }

  const int size = (y1_ - y0_) * (x1_ - x0_);

  double* d = new double[size];
  double* d_arranged = new double[size];
  const int mid_size = feature_size_ / 2;
  double* mins = new double[mid_size + 1];
  double* maxs = new double[mid_size + 1];

  for ( int c = 0; c < kNumFeatureChannels; ++c ) {
    int i = 0;
    double (*rgb_to_comp)(double, double, double) = NULL;
    switch ( c ) {
      case 0 : rgb_to_comp = &rgb_to_Y; break;
      case 1 : rgb_to_comp = &rgb_to_Pb; break;
      case 2 : rgb_to_comp = &rgb_to_Pr; break;
    }
    for ( int y = y0_; y < y1_; ++y ) {
      const uint8* p = pic->frame_.data[0] +
                       (y * kNumFeatureChannels * pic->width_ + x0_);
      for ( int x = x0_; x1_ > x; ++x ) {
        if ( use_yuv ) {
          d[i] = (*rgb_to_comp)(static_cast<double>(int(*(p))),
                                static_cast<double>(int(*(p + 1))),
                                static_cast<double>(int(*(p + 2))));
        } else {
          d[i] = static_cast<double>(int(*(p + c))) / 255.0;
        }
        ++i;
        p += kNumFeatureChannels;
      }
    }
    if ( feature_type_ == FEATURE_WAVELET_HAAR ||
         feature_type_ == FEATURE_WAVELET_DAUBECHIES ) {
      if ( feature_type_ == FEATURE_WAVELET_HAAR ) {
        Haar2Wavelet(d, x1_ - x0_, y1_ - y0_);
      } else {
        Daub2Wavelet(d, x1_ - x0_, y1_ - y0_);
      }
      WaveletRearrangeCoef(d, d_arranged, x1_ - x0_, y1_ - y0_);

      memset(features_[c], 0, sizeof(double) * feature_size_);
      memset(mins, 0, sizeof(double) * (mid_size + 1));
      memset(maxs, 0, sizeof(double) * (mid_size + 1));

      for ( int i = 0; i < size; ++i ) {
        // Well.. a heap can be used, but in practice is way slower than this
        // (since feature_size is relatively small )
        double* pmin = mins;
        double c_min = d[i];
        if ( c_min < mins[mid_size] ) {
          for ( int j = 0; j <= mid_size; ++j ) {
            if ( c_min < *pmin ) {
              const double tmp = *pmin;
              *pmin = c_min;
              c_min = tmp;
            }
            ++pmin;
          }
        }
        double* pmax = maxs;
        double c_max = d[i];
        if ( c_max > maxs[mid_size] ) {
          for ( int j = 0; j <= mid_size; ++ j ) {
            if ( c_max > *pmax ) {
              const double tmp = *pmax;
              *pmax = c_max;
              c_max = tmp;
            }
            ++pmax;
          }
        }
      }

      int crt = 0;
      for ( int i = 0; i < size; ++i ) {
        if ( (d_arranged[i] <= mins[mid_size] ||
              d_arranged[i] >= maxs[mid_size]) && crt < feature_size_ ) {
          features_[c][crt++] = d_arranged[i];
        }
      }
    } else {
      const int size = (y1_ - y0_) * (x1_ - x0_);
      const int delta = size / feature_size_;
      int crt = 0;
      for ( int i = 0; i < feature_size_; ++i ) {
        features_[c][i] = d[crt];
        crt += delta;
      }
    }
  }
  delete [] mins;
  delete [] maxs;
  delete [] d;
  delete [] d_arranged;
  return true;
}

string FeatureData::ToString() const {
  string s = strutil::StringPrintf(
      "Use YUV: %d, Feature Type: %d, X0: %d, X1: %d, "
      "Y0: %d, Y1: %d, Channels: %zd",
      use_yuv_, feature_type_, x0_, x1_, y0_, y1_, features_.size());
  for ( int i = 0; i < features_.size(); ++i ) {
    double* p = features_[i];
    s += strutil::StringPrintf(
        "\n====================== CHANNEL %d ==================\n", i);
    for ( int k = 0; k < feature_size_; ++k ) {
      s += strutil::StringPrintf(" %.2f, ", *p++);
    }
  }
  return s;
}


bool FeatureData::EuclidianDistance(
    const FeatureData* d,
    double distance[kNumFeatureChannels + 1]) const {
  if ( use_yuv_ != d->use_yuv_ ||
       feature_size_ != d->feature_size_ ||
       feature_type_ != d->feature_type_ ) {
    return false;
  }
  for ( int c = 0; c < kNumFeatureChannels; ++c ) {
    distance[c] = 0.0;
  }
  for ( int c = 0; c < features_.size(); ++c ) {
    double dist = 0;
    const double* p1 = features_[c];
    const double* p2 = d->features_[c];
    for ( int i = 0; i < feature_size_; ++i ) {
      dist += (*p1 - *p2) * (*p1 - *p2);
      ++p1;
      ++p2;
    }
    distance[kNumFeatureChannels] += dist;
    distance[c] = sqrt(dist);
  }
  distance[kNumFeatureChannels] = sqrt(distance[kNumFeatureChannels]);
  return true;
}

bool FeatureData::ShortestPathDistance(
    const FeatureData* dest,
    double distance[kNumFeatureChannels + 1]) const {
  if ( use_yuv_ != dest->use_yuv_ ||
       feature_size_ != dest->feature_size_ ||
       feature_type_ != dest->feature_type_ ) {
    return false;
  }
  for ( int c = 0; c < kNumFeatureChannels + 1; ++c ) {
    distance[c] = 0.0;
  }
  const int size = (feature_size_ + 1) * (feature_size_ + 1);
  double* coef = new double[size];
  for ( int c = 0; c < features_.size(); ++c ) {
    double* d = coef;
    const double* p1 = features_[c];
    const double* p2 = dest->features_[c];
    *d++ = 0.00;                                              // coef[0,0] = 0
    for ( int j = 1; j <= feature_size_; ++j ) {
      *d = *(d - 1) + fabs(p1[j - 1]) + FLAGS_path_penalty;   // coef[0, j]
      ++d;
    }
    double min_dist = *(d - 1);
    for ( int i = 1; i <= feature_size_; ++i ) {
      for ( int j = 0; j <= feature_size_; ++j ) {
        if ( j == 0 ) {
          *d = *(d - feature_size_ - 1) + fabs(p2[i - 1] +
                                               FLAGS_path_penalty);
                                                               // coef[i, 0]
        } else {
          *d = min(*(d - feature_size_ - 2) +                  // coef[i, j]
                   fabs(p2[i - 1] - p1[j - 1]),
                   min(*(d - feature_size_ - 1) + fabs(p2[i - 1]) +
                       FLAGS_path_penalty,
                       *(d - 1) + fabs(p1[j - 1]) +
                       FLAGS_path_penalty));
        }
        if ( (j == feature_size_ || i == feature_size_) && *d < min_dist ) {
          min_dist = *d;
        }
        ++d;
      }
    }
    distance[c] = min_dist / feature_size_;
    distance[kNumFeatureChannels] += distance[c] * distance[c];
  }
  distance[kNumFeatureChannels] = sqrt(distance[kNumFeatureChannels]);
  delete [] coef;
  return true;
}

bool FeatureData::IsMatching(const FeatureData* d,
                             double max_distance[kNumFeatureChannels + 1],
                             DistanceType distance_type,
                             bool large_match) const {
  if ( use_yuv_ != d->use_yuv_ ||
       feature_size_ != d->feature_size_ ||
       feature_type_ != d->feature_type_ ) {
    return false;
  }
  double distance[kNumFeatureChannels + 1];
  switch ( distance_type ) {
    case EUCLIDIAN:
      if ( ! EuclidianDistance(d, distance) )
        return false;
      break;
    case SHORTEST_PATH:
      if ( ! ShortestPathDistance(d, distance) )
        return false;
      break;
  }
  if ( large_match ) {
    for ( int c = 0; c < kNumFeatureChannels + 1; ++c ) {
      if ( distance[c] > 2 * max_distance[c] ) {
        return false;
      }
    }
  } else {
    for ( int c = 0; c < kNumFeatureChannels + 1; ++c ) {
      if ( distance[c] > max_distance[c] ) {
        return false;
      }
    }
  }
  return true;
}

ReadStatus FeatureData::Load(io::MemoryStream* ms) {
  Clear();
  if ( ms->Size() < 1) {
    return READ_NO_DATA;
  }
  ms->MarkerSet();
  if ( !io::NumStreamer::ReadByte(ms) ) {
    ms->MarkerClear();
    return READ_OK;
  }
  if ( ms->Size() < 6 * sizeof(int32)) {
    return READ_NO_DATA;
  }
  x0_ = io::NumStreamer::ReadInt32(ms, common::BIGENDIAN);
  x1_ = io::NumStreamer::ReadInt32(ms, common::BIGENDIAN);
  y0_ = io::NumStreamer::ReadInt32(ms, common::BIGENDIAN);
  y1_ = io::NumStreamer::ReadInt32(ms, common::BIGENDIAN);
  use_yuv_ = io::NumStreamer::ReadInt32(ms, common::BIGENDIAN);
  feature_type_ = static_cast<FeatureType>(io::NumStreamer::ReadInt32(
                                               ms, common::BIGENDIAN));
  feature_size_ = io::NumStreamer::ReadInt32(ms, common::BIGENDIAN);

  const int size = (y1_ - y0_) * (x1_ - x0_);
  if ( size <= 0 ) {
    ms->MarkerRestore();
    return READ_INVALID_DATA;
  }
  if ( ms->Size() < kNumFeatureChannels * feature_size_ * sizeof(double) ) {
    ms->MarkerRestore();
    return READ_NO_DATA;
  }
  for ( int c = 0; c < kNumFeatureChannels; ++c ) {
    double* crt = new double[feature_size_];
    features_.push_back(crt);

    int s = feature_size_;
    while ( s-- ) {
      *crt++ = io::NumStreamer::ReadDouble(ms, common::BIGENDIAN);
    }
  }
  ms->MarkerClear();
  return READ_OK;
}

void FeatureData::Save(io::MemoryStream* ms) const {
  const int size = (y1_ - y0_) * (x1_ - x0_);
  if ( size <= 0 ) {
    io::NumStreamer::WriteByte(ms, false);
    return;
  }
  io::NumStreamer::WriteByte(ms, true);
  io::NumStreamer::WriteInt32(ms, x0_, common::BIGENDIAN);
  io::NumStreamer::WriteInt32(ms, x1_, common::BIGENDIAN);
  io::NumStreamer::WriteInt32(ms, y0_, common::BIGENDIAN);
  io::NumStreamer::WriteInt32(ms, y1_, common::BIGENDIAN);
  io::NumStreamer::WriteInt32(ms, use_yuv_, common::BIGENDIAN);
  io::NumStreamer::WriteInt32(ms, static_cast<int32>(feature_type_),
                              common::BIGENDIAN);
  io::NumStreamer::WriteInt32(ms, feature_size_, common::BIGENDIAN);
  for ( int i = 0; i < features_.size(); ++i ) {
    const double* crt = features_[i];

    int s = feature_size_;
    while ( s-- ) {
      io::NumStreamer::WriteDouble(ms, *crt++, common::BIGENDIAN);
    }
  }
}

//////////////////////////////////////////////////////////////////////

FeatureSet::FeatureSet()
    : feature_width_(0),
      feature_height_(0),
      last_match_(-1),
      first_match_timestamp_(-1),
      last_match_timestamp_(-1) {
}

FeatureSet::~FeatureSet() {
  for ( int i = 0; i < features_.size(); ++i ) {
    delete features_[i].second;
  }
  features_.clear();
}

MatchResult FeatureSet::MatchNext(int64 timestamp, const FeatureData* data,
                                  double max_distance[kNumFeatureChannels + 1],
                                  DistanceType distance_type) {
  if ( features_.empty() ) {
    return MATCH_NO_DATA;
  }
  if ( last_match_ >= features_.size() - 1 ) {
    ResetMatch();
  }
  if (last_match_ >= 0 && last_match_timestamp_ > timestamp) {
    LOG_INFO << " Reseting match @: " << last_match_
             << "  per: " << last_match_timestamp_ << " / " << timestamp;
    ResetMatch();
  }

  if ( last_match_ == -1 ) {
    if ( !features_[0].second->IsMatching(data, max_distance,
                                          distance_type, false) ) {
      return NO_MATCH;
    }
    last_match_ = 0;
    last_match_timestamp_ = timestamp;
    first_match_timestamp_ = timestamp;
    return (features_.size() == 1
            ? MATCH_COMPLETED : MATCH_BEGIN);
  }
  int crt_check = last_match_ + 1;
  while ( crt_check < features_.size() &&
          features_[crt_check].first - features_[last_match_].first <
          timestamp - last_match_timestamp_ ) {
    ++crt_check;
  }
  if ( crt_check >= features_.size() ) {
    last_match_ = crt_check;
    return MATCH_OVER_TIME;
  }
  if ( !features_[crt_check].second->IsMatching(data, max_distance,
                                                distance_type, true) ) {
    ResetMatch();
    return MATCH_BROKEN;
  }
  last_match_ = crt_check;
  last_match_timestamp_ = timestamp;
  return ((crt_check == features_.size() - 1)
          ? MATCH_COMPLETED : MATCH_CONT);
}

ReadStatus FeatureSet::Load(io::MemoryStream* ms) {
  if ( ms->Size() < sizeof(int32) ) {
    return READ_NO_DATA;
  }
  const int32 total_size = io::NumStreamer::ReadInt32(ms, common::BIGENDIAN);
  ms->MarkerSet();
  if ( ms->Size() < total_size ) {
    ms->MarkerRestore();
    return READ_NO_DATA;
  }
  feature_width_ = io::NumStreamer::ReadInt32(ms, common::BIGENDIAN);
  feature_height_ = io::NumStreamer::ReadInt32(ms, common::BIGENDIAN);
  const int num_features = io::NumStreamer::ReadInt32(ms, common::BIGENDIAN);
  for ( int i = 0; i < num_features; ++i ) {
    const int64 timestamp = io::NumStreamer::ReadInt64(ms, common::BIGENDIAN);
    FeatureData* data = new FeatureData();
    const ReadStatus stat = data->Load(ms);
    CHECK_NE(stat, READ_NO_DATA);
    if ( stat != READ_OK ) {
      return stat;
    }
    AddFeature(timestamp, data);
  }
  return READ_OK;
}

void FeatureSet::Save(io::MemoryStream* ms) const {
  io::MemoryStream tmp;
  io::NumStreamer::WriteInt32(&tmp, feature_width_, common::BIGENDIAN);
  io::NumStreamer::WriteInt32(&tmp, feature_height_, common::BIGENDIAN);
  io::NumStreamer::WriteInt32(&tmp, features_.size(), common::BIGENDIAN);
  for ( int i = 0; i < features_.size(); ++i ) {
    io::NumStreamer::WriteInt64(&tmp, features_[i].first, common::BIGENDIAN);
    features_[i].second->Save(&tmp);
  }
  io::NumStreamer::WriteInt32(ms, tmp.Size(), common::BIGENDIAN);
  ms->AppendStream(&tmp);
}

string FeatureSet::ToString() const {
  string s = strutil::StringPrintf("Feature width: %d / height: %d",
                                   feature_width_, feature_height_);
  for ( int i = 0; i < features_.size(); ++i ) {
    s += strutil::StringPrintf("\n @%"PRId64"\n",
                               (features_[i].first));
    s += features_[i].second->ToString();
    if ( i > 0 ) {
      double dist[kNumFeatureChannels + 1];
      features_[i - 1].second->EuclidianDistance(features_[i].second, dist);
      s += strutil::StringPrintf(
          "\n@%"PRId64" >> Euclidian Interscene distance: "
          "%.5f / %.5f / %.5f / %.5f ",
          (features_[i].first),
          dist[0], dist[1], dist[2], dist[3]);
      features_[i - 1].second->ShortestPathDistance(features_[i].second, dist);
      s += strutil::StringPrintf(
          "\n%"PRId64" >> Shortest Path Interscene distance: "
          "%.5f / %.5f / %.5f / %.5f\n",
          (features_[i].first),
          dist[0], dist[1], dist[2], dist[3]);
    }
  }
  return s;
}

}
