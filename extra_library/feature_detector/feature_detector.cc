// Copyright 2009 WhisperSoft s.r.l.
// Author: Catalin Popescu

// @@@ NEEDS REFACTORING - CLOSE/TAG DISTRIBUTION

#include <whisperlib/common/io/file/file_input_stream.h>
#include "feature_detector.h"

namespace streaming {

const char FeatureDetectorElement::kElementClassName[] = "feature_detector";

FeatureDetectorElement::FeatureDetectorElement(
    const string& name,
    ElementMapper* mapper,
    net::Selector* selector,
    const string& media_dir,
    const FeatureDetectorElementSpec& spec)
    : Element(kElementClassName, name, mapper),
      selector_(selector),
      media_dir_(media_dir),
      media_name_(spec.media_name_.get()),
      internal_req_(NULL),
      process_tag_callback_(NULL),
      open_media_callback_(NULL),
      unpause_controller_callback_(
          NewPermanentCallback(
              this, &FeatureDetectorElement::UnpauseController)),
      processing_queue_(kMaxPendingTags),
      paused_controller_(false),
      distributor_(streaming::kDefaultFlavourMask, name),
      processing_thread_(NULL),
      feature_width_(0),
      feature_height_(0),
      feature_x0_(0),
      feature_x1_(0),
      feature_y0_(0),
      feature_y1_(0),
      feature_type_(feature::FeatureData::FEATURE_RAW),
      feature_size_(0),
      feature_use_yuv_(false),
      crt_match_(feature::NO_MATCH),
      crt_match_id_(-1),
      crt_match_timestamp_(-1),
      crt_callback_(NULL) {
  for ( uint32 i = 0; i < spec.features_.get().size(); ++i ) {
    features_.push_back(new FeatureStruct(media_dir_,
                                          spec.features_.get()[i]));
  }
}

FeatureDetectorElement::~FeatureDetectorElement() {
  CHECK(processing_thread_ == NULL) << " You need to close before deleting.. ";

  // Well we just have to hope that hmm.. we have no prepared
  distributor_.CloseAllCallbacks(true);
  if ( process_tag_callback_ != NULL ) {
    CHECK_NOT_NULL(internal_req_);

    mapper_->RemoveRequest(internal_req_, process_tag_callback_);
    internal_req_ = NULL;

    delete process_tag_callback_;
    process_tag_callback_ = NULL;
  }
  if ( open_media_callback_ != NULL ) {
    selector_->UnregisterAlarm(open_media_callback_);
    delete open_media_callback_;
    open_media_callback_ = NULL;
  }
  delete unpause_controller_callback_;
}

bool FeatureDetectorElement::Initialize() {
  CHECK(processing_thread_ == NULL);
  processing_thread_ = new thread::Thread(
      NewCallback(this, &FeatureDetectorElement::ProcessingThread));
  processing_thread_->SetJoinable();
  processing_thread_->Start();
  uint32 failed_features = 0;
  for ( uint32 i = 0; i < features_.size(); ++i ) {
    if ( !features_[i]->Open() ) {
      ++failed_features;
    } else if ( feature_width_ > 0 ) {
      if ( feature_width_ != features_[i]->feature_->feature_width() ||
           feature_height_ != features_[i]->feature_->feature_height() ||
           feature_x0_ !=  features_[i]->feature_->features()[0].second->x0() ||
           feature_x1_ != features_[i]->feature_->features()[0].second->x1() ||
           feature_y0_ != features_[i]->feature_->features()[0].second->y0() ||
           feature_y1_ != features_[i]->feature_->features()[0].second->y1() ||
           feature_type_ != features_[i]->feature_->features()[0].second->feature_type() ||
           feature_size_ != features_[i]->feature_->feature_size() ||
           feature_use_yuv_ != features_[i]->feature_->features()[0].second->use_yuv() ) {
        LOG_ERROR << name() << " incompatible feature set at position : " << i
                  << "  - discarding";
        delete features_[i]->feature_;
        features_[i]->feature_ = NULL;
        ++failed_features;
      }
    } else {
      feature_width_ = features_[i]->feature_->feature_width();
      feature_height_ = features_[i]->feature_->feature_height();
      feature_x0_ =  features_[i]->feature_->features()[0].second->x0();
      feature_x1_ = features_[i]->feature_->features()[0].second->x1();
      feature_y0_ = features_[i]->feature_->features()[0].second->y0();
      feature_y1_ = features_[i]->feature_->features()[0].second->y1();
      feature_type_ = features_[i]->feature_->features()[0].second->feature_type();
      feature_size_ = features_[i]->feature_->features()[0].second->feature_size();
      feature_use_yuv_ = features_[i]->feature_->features()[0].second->use_yuv();
    }
  }
  if ( failed_features == features_.size() ) {
    LOG_ERROR << name() << " No valid features to detect.. ";
    return false;
  } else if ( failed_features ) {
    LOG_ERROR << name() << " Some Invalid features found ...";
  }
  // TODO - start thread
  OpenMedia();
  return true;
}

bool FeatureDetectorElement::AddRequest(const string& media_name, Request* req,
                                        ProcessingCallback* callback) {
  if ( media_name != "" ) {
    LOG_ERROR << name() << ": Cannot AddRequest media_name=" << media_name;
    return false;
  }
  callbacks_to_bootstrap_[req] = callback;
  return true;
}
void FeatureDetectorElement::RemoveRequest(streaming::Request* req) {
  distributor_.remove_callback(req);

  map<Request*, ProcessingCallback*>::iterator it = callbacks_to_bootstrap_.find(req);
  if (it != callbacks_to_bootstrap_.end()) {
    if ( crt_callback_ == it->second ) {
      crt_callback_ = NULL;
    }
  }
  callbacks_to_bootstrap_.erase(req);
}

bool FeatureDetectorElement::HasMedia(const string& media) {
  return media == "";
}

void FeatureDetectorElement::ListMedia(const string& media_dir,
                                       vector<string>* out) {
  out->push_back(name());
}
bool FeatureDetectorElement::DescribeMedia(const string& media,
                                           MediaInfoCallback* callback) {
  // FeatureDetector does not create any stream
  return false;
}

void FeatureDetectorElement::Close(Closure* call_on_close) {
  processing_queue_.Clear();
  processing_queue_.Put(ProcessedTag(NULL, 0));

  processing_thread_->Join();
  processing_thread_ = NULL;

  // Essential to do this after the possible scheduled functions in
  // the processing thread
  selector_->RunInSelectLoop(
      NewCallback(&distributor_, &TagDistributor::CloseAllCallbacks, true));
  selector_->RunInSelectLoop(call_on_close);
}

//////////////////////////////////////////////////////////////////////


void FeatureDetectorElement::OpenMedia() {
  CHECK(process_tag_callback_ == NULL);
  CHECK(internal_req_ == NULL);
  process_tag_callback_ =
      NewPermanentCallback(this, &FeatureDetectorElement::ProcessTag);
  internal_req_ = new streaming::Request();
  if ( open_media_callback_ != NULL ) {
    open_media_callback_ = NULL;
  }
  if ( !mapper_->AddRequest(media_name_.c_str(),
                            internal_req_,
                            process_tag_callback_) ) {
    LOG_ERROR << name() << " faild to add itself to: " << media_name_;
    delete process_tag_callback_;
    process_tag_callback_ = NULL;

    delete internal_req_;
    internal_req_ = NULL;
    open_media_callback_ = NewCallback(this,
                                       &FeatureDetectorElement::OpenMedia);
    selector_->RegisterAlarm(open_media_callback_, 1000);
  }
}

void FeatureDetectorElement::ProcessTag(const Tag* tag, int64 timestamp_ms) {
  while ( !callbacks_to_bootstrap_.empty() ) {
    Request* req = callbacks_to_bootstrap_.begin()->first;
    ProcessingCallback* callback = callbacks_to_bootstrap_.begin()->second;
    callbacks_to_bootstrap_.erase(callbacks_to_bootstrap_.begin());

    crt_callback_ = callback;
    crt_callback_->Run(scoped_ref<Tag>(new SourceStartedTag(
        0, kDefaultFlavourMask, name(), name(), false, timestamp_ms)).get(),
        timestamp_ms);

    if ( crt_callback_ != NULL ) {
      distributor_.add_callback(req, callback);
    }
    crt_callback_ = NULL;
  }

  if ( tag->type() == streaming::Tag::TYPE_EOS ) {
    CHECK_NOT_NULL(process_tag_callback_);
    CHECK_NOT_NULL(internal_req_);

    mapper_->RemoveRequest(internal_req_, process_tag_callback_);
    internal_req_ = NULL;

    selector_->DeleteInSelectLoop(process_tag_callback_);  // we are in it ..
    process_tag_callback_ = NULL;

    open_media_callback_ = NewCallback(this,
                                       &FeatureDetectorElement::OpenMedia);
    selector_->RegisterAlarm(open_media_callback_, 1000);
  } else {
    ElementController* controller = internal_req_->controller();
    if ( controller != NULL && controller->SupportsPause() &&
         processing_queue_.Size() > kMaxPausePendingTags ) {
      LOG_INFO << name() << " Processing speed too low - pausing controller..";
      {
        synch::MutexLocker l(&pause_mutex_);
        paused_controller_ = true;
      }
      controller->Pause(true);
    }
  }
  if ( !processing_queue_.Put(ProcessedTag(tag, timestamp_ms), 0) ) {
    LOG_WARNING << name() << " Full processing queue ..." << tag->ToString();
  }
}

void FeatureDetectorElement::UnpauseController() {
  DCHECK(internal_req_ != NULL);
  DCHECK(internal_req_->controller() != NULL);
  internal_req_->controller()->Pause(false);
}

void FeatureDetectorElement::ProcessImage(
    const feature::BitmapExtractor::PictureStruct* picture) {
  feature::FeatureData* feature = new feature::FeatureData();
  if ( feature->Extract(feature_type_, feature_use_yuv_,
                        feature_x0_, feature_x1_, feature_y0_, feature_y1_,
                        feature_size_,
                        picture) ) {
    if ( crt_match_id_ >= 0 ) {
      feature::MatchResult result = features_[crt_match_id_]->feature_->MatchNext(
          picture->timestamp_,
          feature,
          features_[crt_match_id_]->max_distance_,
          feature::SHORTEST_PATH);
      crt_match_ = result;
      if ( result >= feature::MATCH_BEGIN ) {
        delete feature;
        return;
      }
      crt_match_id_ = -1;
      crt_match_timestamp_ = -1;
    }
    if ( crt_match_id_ < 0 ) {
      crt_match_ = feature::MATCH_NO_DATA;
      crt_match_id_ = -1;
      for ( uint32 i = 0; i < features_.size(); ++i ) {
        feature::MatchResult result = features_[i]->feature_->MatchNext(
            picture->timestamp_,
            feature,
            features_[i]->max_distance_,
            feature::SHORTEST_PATH);
        if ( result >= feature::MATCH_BEGIN ) {
          crt_match_ = result;
          if ( result == feature::MATCH_BEGIN ) {
            crt_match_id_ = i;
            crt_match_timestamp_ = picture->timestamp_;
          }
          break;
        }
      }
    }
  } else {
    LOG_ERROR << " Error extracting feature...  @" << picture->timestamp_;
    crt_match_ = feature::MATCH_NO_DATA;
    crt_match_id_ = -1;
    crt_match_timestamp_ = -1;
  }
  delete feature;
}

void FeatureDetectorElement::ProcessingThread() {
  feature::BitmapExtractor extractor(feature_width_, feature_height_,
                                     PIX_FMT_RGB24);
  while ( true ) {
    ProcessedTag ptag = processing_queue_.Get();
    if ( ptag.tag_.get() == NULL ) {
      break;
    }

    vector<scoped_ref<const feature::BitmapExtractor::PictureStruct> > pictures;
    extractor.GetNextPictures(ptag.tag_.get(), ptag.timestamp_ms_, &pictures);
    for ( uint32 i = 0; i < pictures.size(); i++ ) {
      ProcessImage(pictures[i].get());
    }

    if ( crt_match_ == feature::MATCH_COMPLETED ) {
      selector_->RunInSelectLoop(
          NewCallback(this,
                      &FeatureDetectorElement::SubmitCompleteTag,
                      ptag.tag_,
                      ptag.timestamp_ms_,
                      crt_match_timestamp_,
                      crt_match_id_));
      crt_match_ = feature::NO_MATCH;
      crt_match_id_ = -1;
      crt_match_timestamp_ = -1;
    } else {
      selector_->RunInSelectLoop(
          NewCallback(this,
                      &FeatureDetectorElement::SubmitTag,
                      ptag.tag_,
                      ptag.timestamp_ms_));
    }

    {
      synch::MutexLocker l(&pause_mutex_);
      if ( paused_controller_ &&
           processing_queue_.Size() < kMaxUnPausePendingTags ) {
        paused_controller_ = false;
        selector_->RunInSelectLoop(unpause_controller_callback_);
      }
    }
  }
}

void FeatureDetectorElement::SubmitCompleteTag(scoped_ref<const Tag> tag,
                                               int64 timestamp_ms,
                                               int64 match_timestamp,
                                               int match_id) {
  scoped_ref<Tag> feature(new FeatureFoundTag(0, tag->flavour_mask(),
          strutil::StringPrintf("%s/%s", name().c_str(),
                                features_[match_id]->name_.c_str()),
          features_[match_id]->length_ms_));
  LOG_INFO << name() << " Feature detected: "
           << " @: " << match_timestamp << " reported @:" << timestamp_ms
           << " - feature: " << feature->ToString();
  distributor_.DistributeTag(feature.get(), match_timestamp);
}

void FeatureDetectorElement::SubmitTag(scoped_ref<const Tag> tag,
                                       int64 timestamp_ms) {
  distributor_.DistributeTag(tag.get(), timestamp_ms);
}

//////////////////////////////////////////////////////////////////////

FeatureDetectorElement::FeatureStruct::FeatureStruct(const string& media_dir,
                                                     const FeatureSpec& spec)
    : name_(spec.feature_name_.get()),
      file_name_(strutil::JoinPaths(media_dir,
                                    spec.feature_file_.get())),
      length_ms_(spec.length_ms_.get()),
      feature_(NULL) {
  if ( spec.max_distance_C0_.is_set() ) {
    max_distance_[0] = spec.max_distance_C0_.get();
  } else {
    max_distance_[0] = 1.2;
  }
  if ( spec.max_distance_C1_.is_set() ) {
    max_distance_[1] = spec.max_distance_C1_.get();
  } else {
    max_distance_[1] = .8;
  }
  if ( spec.max_distance_C2_.is_set() ) {
    max_distance_[2] = spec.max_distance_C2_.get();
  } else {
    max_distance_[2] = .8;
  }
  if ( spec.max_distance_C3_.is_set() ) {
    max_distance_[3] = spec.max_distance_C3_.get();
  } else {
    max_distance_[3] = 1.35;
  }
}

FeatureDetectorElement::FeatureStruct::~FeatureStruct() {
  delete feature_;
  feature_ = NULL;
}

bool FeatureDetectorElement::FeatureStruct::Open() {
  string content;
  CHECK(feature_ == NULL);
  if ( !io::FileInputStream::TryReadFile(file_name_.c_str(), &content) ) {
    LOG_ERROR << " Error opening feature file: " << file_name_;
    return false;
  }
  feature_ = new feature::FeatureSet();
  io::MemoryStream ms;
  ms.Write(content);
  if ( feature_->Load(&ms) != feature::READ_OK ) {
    LOG_ERROR << " Error reading feature file: " << file_name_;
    delete feature_;
    feature_ = NULL;
    return false;
  }
  if ( feature_->feature_size() == 0 ) {
    LOG_ERROR << " No feature in feature set obtained from file: " << file_name_;
    delete feature_;
    feature_ = NULL;
    return false;
  }
  return true;
}
}
