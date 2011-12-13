// Copyright 2009 WhisperSoft s.r.l.
// Author: Catalin Popescu

// @@@ NEEDS REFACTORING - CLOSE/TAG DISTRIBUTION

#ifndef __STREAMING_ELEMENTS_EXTRA_LIBRARY_FEATURE_DETECTOR_H__
#define __STREAMING_ELEMENTS_EXTRA_LIBRARY_FEATURE_DETECTOR_H__

#include <whisperstreamlib/flv/flv_tag.h>
#include <whisperlib/common/sync/mutex.h>
#include <whisperstreamlib/base/element.h>
#include <whisperstreamlib/base/bootstrapper.h>
#include <whisperstreamlib/base/callbacks_manager.h>
#include <whisperstreamlib/base/tag_distributor.h>
#include <whisperlib/common/sync/producer_consumer_queue.h>
#include <whisperlib/common/sync/thread.h>
#include <whisperlib/net/base/selector.h>

#include "extra_library/auto/extra_library_types.h"
#include "features/decoding/bitmap_extractor.h"
#include "features/decoding/feature_data.h"

namespace streaming {

class FeatureDetectorElement :
      public Element {
 public:
  FeatureDetectorElement(const char* name,
                         const char* id,
                         ElementMapper* mapper,
                         net::Selector* selector,
                         const string& media_dir,
                         const FeatureDetectorElementSpec& spec);
  ~FeatureDetectorElement();

  static const char kElementClassName[];

  virtual bool Initialize();
  virtual bool AddRequest(const char* media,
                          streaming::Request* req,
                          streaming::ProcessingCallback* callback);
  virtual void RemoveRequest(streaming::Request* req);
  virtual bool HasMedia(const char* media, Capabilities* out);
  virtual void ListMedia(const char* media_dir, ElementDescriptions* medias);
  virtual bool DescribeMedia(const string& media, MediaInfoCallback* callback);
  virtual void Close(Closure* call_on_close);

 private:
  void UnpauseController();
  void ProcessTag(const Tag* tag, int64 timestamp_ms);

  void SubmitTag(scoped_ref<const Tag> tag, int64 timestamp_ms);
  void SubmitCompleteTag(scoped_ref<const Tag> tag,
                         int64 timestamp_ms,
                         int64 match_timestamp,
                         int match_id);

  void OpenMedia();
  void ProcessingThread();
  void ProcessImage(const feature::BitmapExtractor::PictureStruct* picture);

  struct FeatureStruct {
   public:
    FeatureStruct(const string& media_dir, const FeatureSpec& spec);
    ~FeatureStruct();

    bool Open();

    const string name_;
    const string file_name_;
    const int32 length_ms_;
    feature::FeatureSet* feature_;
    double max_distance_[feature::kNumFeatureChannels + 1];
  };

  net::Selector* const selector_;
  const string media_dir_;
  const string media_name_;
  streaming::Request* internal_req_;
  ProcessingCallback* process_tag_callback_;
  Closure* open_media_callback_;
  Closure* unpause_controller_callback_;

  struct ProcessedTag {
    ProcessedTag(const Tag* tag, int64 timestamp_ms) :
      tag_(tag),
      timestamp_ms_(timestamp_ms) {
    }
    scoped_ref<const Tag> tag_;
    int64 timestamp_ms_;
  };
  synch::ProducerConsumerQueue<ProcessedTag> processing_queue_;
  static const int kMaxUnPausePendingTags = 200;
  static const int kMaxPausePendingTags = 400;
  static const int kMaxPendingTags = 500;
  bool paused_controller_;
  synch::Mutex pause_mutex_;

  // the downstream clients
  streaming::TagDistributor distributor_;

  thread::Thread* processing_thread_;
  vector<FeatureStruct*> features_;
  int feature_width_;
  int feature_height_;
  int feature_x0_;
  int feature_x1_;
  int feature_y0_;
  int feature_y1_;
  feature::FeatureData::FeatureType feature_type_;
  int feature_size_;
  bool feature_use_yuv_;

  feature::MatchResult crt_match_;
  int crt_match_id_;
  int64 crt_match_timestamp_;

  const streaming::Capabilities caps_;

  // Things we did not bootstrap yet
  map<Request*, ProcessingCallback*> callbacks_to_bootstrap_;
  ProcessingCallback* crt_callback_;  // used to pass callback to bootstrap

  DISALLOW_EVIL_CONSTRUCTORS(FeatureDetectorElement);
};
}


#endif  // __STREAMING_ELEMENTS_EXTRA_LIBRARY_FEATURE_DETECTOR_H__
