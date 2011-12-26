// (c) 2009, Whispersoft s.r.l.
// All rights reserved.
//
// Author: Catalin Popescu
//

#include "extra_library/time_range/time_range_element.h"
#include <whisperstreamlib/flv/flv_tag_splitter.h>
#include <whisperstreamlib/flv/flv_coder.h>
#include <whisperstreamlib/base/saver.h>
#include <whisperstreamlib/base/media_info_util.h>

#include <whisperstreamlib/rtmp/objects/rtmp_objects.h>
#include <whisperstreamlib/rtmp/objects/amf/amf_util.h>
#include <whisperlib/common/base/re.h>
#include <whisperlib/common/base/alarm.h>
#include <whisperlib/common/io/ioutil.h>

#define ILOG(level)  LOG(level) << info() << " " << this << ": "
#ifdef _DEBUG
#define ILOG_DEBUG   ILOG(INFO)
#else
#define ILOG_DEBUG   if (false) ILOG(INFO)
#endif
#define ILOG_INFO    ILOG(INFO)
#define ILOG_WARNING ILOG(WARNING)
#define ILOG_ERROR   ILOG(ERROR)
#define ILOG_FATAL   ILOG(FATAL)

namespace streaming {
class TimeRangeRequest : public streaming::ElementController {
 public:
  TimeRangeRequest(ElementMapper* mapper,
                   net::Selector* selector,
                   const string& home_dir,
                   const string& root_media_path,
                   const string& file_prefix,
                   const string& media_name,
                   const string& element_name)
      : mapper_(mapper),
        selector_(selector),
        home_dir_(home_dir),
        root_media_path_(root_media_path),
        file_prefix_(file_prefix),
        media_name_(media_name),
        element_name_(element_name),
        req_(NULL),
        callback_(NULL),
        total_duration_ms_(0),
        metadata_sent_(false),
        start_time_(true),    // UTC
        end_time_(true),      // UTC
        crt_req_(NULL),
        processing_callback_(NewPermanentCallback(
                                   this, &TimeRangeRequest::ProcessTag)),
        pause_count_(0),
        crt_file_(-1),
        start_tag_sent_(false),
        offset_ms_(0),
        last_timestamp_ms_(0),
        seeking_(false),
        seek_alarm_(*selector),
        play_next_alarm_(*selector) {
  }
  virtual ~TimeRangeRequest() {
    DCHECK(callback_ == NULL) << " Not closed";

    if ( req_ != NULL ) {
      req_->set_controller(NULL);
    }

    delete processing_callback_;
    processing_callback_ = NULL;
  }

  string info() const {
    return element_name_ + " [" + media_name_ + "]";
  }

  streaming::ProcessingCallback* const callback() const {
    return callback_;
  }
  int64 OffsetMs(int file_no) {
    CHECK(file_no >= 0 && file_no < start_times_ms_.size())
        << "Illegal file_no: " << file_no;
    // first file is strange
    const int64 first_file_ms_ = skip_times_ms_[0] + durations_ms_[0];
    if ( file_no == 0 ) {
      return 0;
    }
    if ( file_no == 1 ) {
      return first_file_ms_;
    }
    // file_no is >= 2
    return start_times_ms_[file_no] - start_times_ms_[1] + first_file_ms_;
  }

  // Our interface from the top class
  bool StartRequest(const timer::Date& start_time,
                   const timer::Date& end_time,
                   streaming::Request* req,
                   streaming::ProcessingCallback* callback);

  // Clears only the upstream request. The downstream client stays connected,
  // no EOS is sent.
  // NEEDS: separate call context from upstream.
  void ClearRequest() {
    seek_alarm_.Clear();
    if ( crt_req_ != NULL ) {
      mapper_->RemoveRequest(crt_req_, processing_callback_);
      crt_req_ = NULL;
    }
  }

  // Clears the upstream request, may send EOS to client, and nullifies the
  // client callback.
  // NEEDS: separate call context from upstream. If send_eos then separate
  //        from downstream too.
  void Close(bool send_eos) {
    play_next_alarm_.Clear();

    ClearRequest();
    if ( send_eos ) {
      SendEos();
    }

    crt_file_ = -1;
    callback_ = NULL;
  }

  // Controller Interface

  virtual bool SupportsPause() const {
    return crt_req_ != NULL && crt_req_->controller() != NULL &&
        crt_req_->controller()->SupportsPause();
  }
  virtual bool DisabledPause() const {
    return crt_req_ != NULL && crt_req_->controller() != NULL &&
        crt_req_->controller()->DisabledPause();
  }
  virtual bool SupportsSeek() const {
    return crt_req_ != NULL && crt_req_->controller() != NULL &&
        crt_req_->controller()->SupportsSeek();
  }
  virtual bool DisabledSeek() const {
    return crt_req_ != NULL && crt_req_->controller() != NULL &&
        crt_req_->controller()->DisabledSeek();
  }
  virtual bool Pause(bool pause) {
    if ( pause ) {
      ++pause_count_;
    } else {
      CHECK_GT(pause_count_, 0);
      --pause_count_;
    }

    if ( crt_req_ != NULL && crt_req_->controller() != NULL ) {
      crt_req_->controller()->Pause(pause);
      return true;
    }
    return false;
  }
  virtual bool Seek(int64 seek_pos_ms) {
    CHECK_NOT_NULL(callback_) << " Seek before start request";
    seek_alarm_.Set(NewPermanentCallback(this,
        &TimeRangeRequest::SeekInternal, seek_pos_ms), true, 0, false, true);
    return true;
  }

 private:
  bool IssueRequest(int64 seek_pos_ms, int64 adjustment_ms);

  void PlayNext();
  void SeekInternal(int64 seek_pos_ms);

  void SendEos() {
    // NEEDS: separate call context from downstream.
    if ( start_tag_sent_ ) {
      Send(scoped_ref<Tag>(new SourceEndedTag(0,
          streaming::kDefaultFlavourMask,
          media_name_, media_name_)).get(), 0);
    }
    Send(scoped_ref<Tag>(new streaming::EosTag(
        0, streaming::kDefaultFlavourMask, false)).get(), 0);
    // after EOS make sure nothing follows
    callback_ = NULL;
  }
  void ProcessTag(const streaming::Tag* tag, int64 timestamp_ms);
  bool ProcessMetadataTag(const streaming::FlvTag* flv_tag, int64 timestamp_ms);

  void Send(const Tag* tag, int64 timestamp_ms) {
    if ( callback_ != NULL ) {
      //LOG_INFO << "### >>>>>>> Send: " << timestamp_ms << " " << tag->ToString();
      callback_->Run(tag, timestamp_ms);
    }
  }

  ElementMapper* const mapper_;
  net::Selector* const selector_;

  const string home_dir_;
  const string root_media_path_;
  const string file_prefix_;
  const string media_name_;
  const string element_name_;
  streaming::Request* req_;
  streaming::ProcessingCallback* callback_;

  vector<string> files_;
  vector<int64> start_times_ms_;
  vector<int64> skip_times_ms_;
  vector<int64> durations_ms_;
  int64 total_duration_ms_;
  bool metadata_sent_;

  timer::Date start_time_;
  timer::Date end_time_;

  streaming::Request* crt_req_;
  streaming::ProcessingCallback* processing_callback_;

  int pause_count_;

  int crt_file_;

  bool start_tag_sent_;

  // the current timestamp offset (incremented on each file step)
  int64 offset_ms_;
  // the timestamp of the last tag received
  int64 last_timestamp_ms_;

  // we are currently seeking
  bool seeking_;

  ::util::Alarm seek_alarm_;
  ::util::Alarm play_next_alarm_;

  DISALLOW_EVIL_CONSTRUCTORS(TimeRangeRequest);
};

bool TimeRangeRequest::StartRequest(
    const timer::Date& start_time,
    const timer::Date& end_time,
    streaming::Request* req,
    streaming::ProcessingCallback* callback) {
  if ( req_ != NULL ) {
    ILOG_ERROR << "Cannot start a new request - already running for: [ "
               << start_time << " ] -> [ "
               << end_time << " ]";
    return false;
  }
  if ( end_time.GetTime() <= start_time.GetTime() ) {
    ILOG_ERROR << "Cannot start a new request - invalid times: [ "
               << start_time << " ] -> [ "
               << end_time << " ]";
    return false;
  }

  start_time_ = start_time;
  end_time_ = end_time;

  total_duration_ms_ = end_time.GetTime() - start_time.GetTime();

  timer::Date play_time(start_time);

  ILOG_DEBUG << "start_time: " << start_time.ToShortString()
             << ", end_time: " << end_time.ToShortString()
             << ", duration: " << total_duration_ms_
             << ", seek_pos_ms_: " << req->info().seek_pos_ms_;

  vector<string> files;
  int last_selected = -1;
  while ( play_time.GetTime() < end_time.GetTime() ) {
    string crt_media;
    int64 begin_file_timestamp, playref_time, duration;
    if ( GetNextStreamMediaFile(play_time,
                                home_dir_,
                                file_prefix_,
                                root_media_path_,
                                &files,
                                &last_selected,
                                &crt_media,
                                &begin_file_timestamp, // timestamp in file
                                &playref_time, // absolute time
                                &duration) < 0 ) {
      break;
    }
    if ( crt_media.empty() ) {
      break;
    }

    const int64 crt_duration =
        std::min(duration, end_time.GetTime() - playref_time);

    ILOG_DEBUG << "Obtained: [" << crt_media << "] w/ len: " << duration
               << ", start_times_ms: " << playref_time
               << "(" << timer::Date(playref_time) << ")"
               << ", skip_times_ms_: " << begin_file_timestamp
               << ", durations_ms_: " << crt_duration;

    if ( crt_duration <= 0 ) {
      break;
    }

    files_.push_back(crt_media);

    start_times_ms_.push_back(playref_time);
    skip_times_ms_.push_back(begin_file_timestamp);
    durations_ms_.push_back(crt_duration);

    play_time.SetTime(playref_time + crt_duration);
  }

  if ( files_.empty() ) {
    ILOG_ERROR << "Cannot start a new request - cannot find media for: [ "
               << start_time << " ] -> [ "
               << end_time << " ]";
    return false;
  }

  req_ = req;
  callback_ = callback;

  req_->set_controller(this);
  crt_file_ = -1;

  // start with seek
  if ( req->info().seek_pos_ms_ > 0 ) {
    SeekInternal(req->info().seek_pos_ms_);
    return crt_file_ >= 0;
  }

  // start with first file
  play_next_alarm_.Set(NewPermanentCallback(this, &TimeRangeRequest::PlayNext),
      true, 0, false, true);
  return true;
}

bool TimeRangeRequest::IssueRequest(int64 seek_pos_ms, int64 adjustment_ms) {
  offset_ms_ =
    start_times_ms_[crt_file_] - start_time_.GetTime() + adjustment_ms;
  //LOG_ERROR << "### Change offset_: " << offset_ms_;

  crt_req_ = new streaming::Request();
  crt_req_->mutable_caps()->flavour_mask_ =
      streaming::kDefaultFlavourMask;
  crt_req_->mutable_info()->media_origin_pos_ms_ = skip_times_ms_[crt_file_];
  crt_req_->mutable_info()->seek_pos_ms_ = seek_pos_ms;
  if (crt_file_ == files_.size()-1) {
    crt_req_->mutable_info()->limit_ms_ = durations_ms_[crt_file_];
  } else {
    crt_req_->mutable_info()->limit_ms_ = -1;
  }
  crt_req_->mutable_info()->internal_id_ =
      strutil::StringPrintf("%p", this);

  ILOG_DEBUG << "Sending to play: "
             << files_[crt_file_] << " @ "
             << crt_req_->mutable_info()->media_origin_pos_ms_ << "->"
             << crt_req_->mutable_info()->seek_pos_ms_
             << " : " << crt_req_->mutable_info()->limit_ms_;
  if ( !mapper_->AddRequest(files_[crt_file_].c_str(),
                            crt_req_, processing_callback_) ) {
    ILOG_ERROR << "Cannot add to play: " << files_[crt_file_];
    delete crt_req_;
    crt_req_ = NULL;
    SendEos();
    return false;
  }

  if ( pause_count_ > 0 ) {
    if ( crt_req_->controller() != NULL ) {
      for ( int p = pause_count_; p > 0; --p) {
        crt_req_->controller()->Pause(true);
      }
    }
  }
  return true;
}

// NEEDS: separate call context from both upstream and downstream.
void TimeRangeRequest::PlayNext() {
  ClearRequest();

  int64 adjustment_ms = 0;
  if (crt_file_ >= 0) {
    adjustment_ms = last_timestamp_ms_ - durations_ms_[crt_file_];
  }
  crt_file_++;

  ILOG_DEBUG << "PlayNext: crt_file_: " << crt_file_;
  if ( crt_file_ >= files_.size() ) {
    ILOG_DEBUG << "PlayNext: crt_file_: " << crt_file_
               << " out of range: " << files_.size() << ", stop";
    SendEos();
    return;
  }
  IssueRequest(0, adjustment_ms);
}

// NEEDS: separate call context from both upstream and downstream.
void TimeRangeRequest::SeekInternal(int64 seek_pos_ms) {
  if ( seek_pos_ms > total_duration_ms_ || seek_pos_ms < 0 ) {
    return;
  }

  seeking_ = true;

  seek_pos_ms += start_time_.GetTime();

  int l = 0;
  int r = start_times_ms_.size();
  while ( l + 1 < r ) {
    const int mid = (r + l) / 2;
    if ( start_times_ms_[mid] <= seek_pos_ms ) {
      l = mid;
    } else {
      r = mid;
    }
  }

  if ( l == crt_file_ && crt_req_ != NULL && crt_req_->controller() != NULL &&
      crt_req_->controller()->SupportsSeek() ) {
    crt_req_->controller()->Seek(seek_pos_ms - start_times_ms_[l]);
  } else {
    if ( crt_req_ != NULL ) {
      mapper_->RemoveRequest(crt_req_, processing_callback_);
      crt_req_ = NULL;
    }

    crt_file_ = l;
    IssueRequest(seek_pos_ms - start_times_ms_[l], 0);
  }
}

// NEEDS: separate call context from downstream.
void TimeRangeRequest::ProcessTag(const Tag* tag, int64 timestamp_ms) {
  CHECK_NOT_NULL(callback_) << " " << this << " Tag received after Close()"
                               ", tag: " << tag->ToString();

  if ( tag->type() == streaming::Tag::TYPE_SOURCE_ENDED ) {
    return;
  }
  if ( tag->type() == streaming::Tag::TYPE_EOS ) {
    ILOG_DEBUG << "EOS received";
    play_next_alarm_.Set(NewPermanentCallback(this,
        &TimeRangeRequest::PlayNext), true, 0, false, true);
    return;
  }

  if ( tag->type() == streaming::Tag::TYPE_CUE_POINT ||
       tag->type() == streaming::Tag::TYPE_FLV_HEADER ) {
    // We eat these things as they are of no interest downstream
    return;
  }

  // check if reached the end of the range
  if (crt_file_ == (files_.size()-1)) {
    if ( timestamp_ms >
         (skip_times_ms_[crt_file_]+durations_ms_[crt_file_]) ) {
      play_next_alarm_.Set(NewPermanentCallback(this,
          &TimeRangeRequest::PlayNext), true, 0, false, true);
      return;
    }
  }

  last_timestamp_ms_ = timestamp_ms;

  if ( tag->type() == streaming::Tag::TYPE_SOURCE_STARTED ) {
    if ( !start_tag_sent_ ) {
      Send(scoped_ref<Tag>(new SourceStartedTag(0,
          kDefaultFlavourMask,
          media_name_,
          media_name_,
          false,
          0)).get(), 0);
      start_tag_sent_ = true;
    }
    // eat source started from the files
    return;
  }

  if ( tag->type() == streaming::Tag::TYPE_FLV ) {
    const FlvTag* flv_tag = static_cast<const FlvTag*>(tag);

    if ( flv_tag->body().type() == streaming::FLV_FRAMETYPE_METADATA ) {
      if ( ProcessMetadataTag(flv_tag, timestamp_ms + offset_ms_) ) {
        return;
      }
    }
  }
  //LOG_ERROR << "### SHIFT timestamp: " << timestamp_ms << " + " << offset_ms_;
  Send(tag, timestamp_ms + offset_ms_);
}

bool TimeRangeRequest::ProcessMetadataTag(const FlvTag* flv_tag,
    int64 timestamp_ms) {
  CHECK_EQ(flv_tag->body().type(), streaming::FLV_FRAMETYPE_METADATA);
  const FlvTag::Metadata& metadata = flv_tag->metadata_body();

  if ( metadata.name().value() == streaming::kOnMetaData ) {
    if ( metadata_sent_ ) {
      return true;
    }
    scoped_ref<FlvTag> new_meta_tag(new FlvTag(*flv_tag, -1, true));
    FlvTag::Metadata& new_meta = new_meta_tag->mutable_metadata_body();
    new_meta.mutable_values()->Set("duration",
        new rtmp::CNumber(total_duration_ms_/ 1000.0));
    new_meta.mutable_values()->Erase("datasize");
    new_meta.mutable_values()->Erase("offset");
    new_meta.mutable_values()->Erase("filesize");

    Send(new_meta_tag.get(), timestamp_ms);
    metadata_sent_ = true;
    return true;
  }
  // forward flv_tag
  return false;
}
//////////////////////////////////////////////////////////////////////

const char TimeRangeElement::kElementClassName[] = "time_range";

TimeRangeElement::TimeRangeElement(
    const char* name,
    const char* id,
    ElementMapper* mapper,
    net::Selector* selector,
    io::StateKeepUser* global_state_keeper,
    io::StateKeepUser* local_state_keeper,
    const char* rpc_path,
    rpc::HttpServer* rpc_server,
    const char* home_dir,
    const char* root_media_path,
    const char* file_prefix,
    bool utc_requests)
    : Element(kElementClassName, name, id, mapper),
      ServiceInvokerTimeRangeElementService(
          ServiceInvokerTimeRangeElementService::GetClassName()),
      selector_(selector),
      global_state_keeper_(global_state_keeper),
      local_state_keeper_(local_state_keeper),
      rpc_path_(rpc_path),
      rpc_server_(rpc_server),
      home_dir_(home_dir),
      root_media_path_(root_media_path),
      file_prefix_(file_prefix),
      utc_requests_(utc_requests),
      is_registered_(false),
      cache_compute_time_(0),
      default_caps_(streaming::Tag::kAnyType,
                    streaming::kDefaultFlavourMask),
      close_completed_(NULL) {
}

TimeRangeElement::~TimeRangeElement() {
  if ( is_registered_ ) {
    rpc_server_->UnregisterService(rpc_path_, this);
  }

  DCHECK(requests_.empty());
  DCHECK(close_completed_ == NULL);
}

bool TimeRangeElement::Initialize() {
  if ( rpc_server_ != NULL ) {
    is_registered_ = rpc_server_->RegisterService(rpc_path_,
                                                  this);
  }
  return (is_registered_ || rpc_server_ == NULL);
}

bool TimeRangeElement::SetDateFromUrlComponent(const string& comp,
                                               timer::Date* t) {
  if ( comp.find('-') != string::npos ) {
    if ( !t->SetFromShortString(comp + "-000", utc_requests_) ) {
      ILOG_ERROR << "Invalid start time specified: " <<  comp;
      return false;
    }
  } else {
    errno = 0;
    const double secs = strtod(comp.c_str(), NULL);
    if ( secs <= 0.0 || errno ) {
      ILOG_ERROR << "Invalid start time specified: " <<  comp;
      return false;
    }
    t->SetTime(static_cast<int64>(secs * 1000));
  }
  // internally we use UTC independently of what they give us
  t->SetUTC(true);
  return true;
}
bool TimeRangeElement::GetDates(const string& str_dates,
                                timer::Date* start_time,
                                timer::Date* end_time) {
  if ( str_dates.empty() ) {
    ILOG_ERROR << "Empty media specified.";
    return false;
  }
  vector<string> comp;
  strutil::SplitString(str_dates, "/", &comp);
  if ( comp.size() > 2 ) {
    ILOG_ERROR << "Bad range specified: " << str_dates;
    return false;
  }
  if ( !SetDateFromUrlComponent(comp[0], start_time) ) {
    return false;
  }
  if ( comp.size() == 2 ) {
    if ( !SetDateFromUrlComponent(comp[1], end_time) ) {
      return false;
    }
  } else {
    end_time->SetTime(0);
  }
  return true;
}

bool TimeRangeElement::HasMedia(const char* media, Capabilities* out) {
  pair<string, string> split(strutil::SplitFirst(media, '/'));
  if ( name() != split.first ) {
    return false;
  }
  if ( global_state_keeper_->GetValue(split.second,
                                      &split.first) ) {
    *out = default_caps_;
    return true;
  }
  timer::Date test(true);
  if ( !GetDates(split.second.c_str(), &test, &test) ) {
    return false;
  }
  *out = default_caps_;
  return true;
}

void TimeRangeElement::ListMedia(const char* media_dir,
                                 streaming::ElementDescriptions* medias) {
  if ( selector_->now() - cache_compute_time_ > kCacheExpirationMs ) {
    medias_cache_.clear();
    vector<string> files;
    re::RE regex_files(string("^") + file_prefix_ +
                       kWhisperProcFileTermination);
    if ( !io::DirList(home_dir_ + "/", &files, false, &regex_files) ) {
      ILOG_ERROR << "Error listing home_dir_: [" << home_dir_ << "]";
      return ;
    }
    // this should be cheap as they come already sorted (most of times)
    sort(files.begin(), files.end());
    if ( files.empty() ) {
      return;
    }

    size_t pos_uscore1 = files[0].rfind('_');
    DCHECK(pos_uscore1 != string::npos) << "Bad file: " << files[0];
    size_t pos_uscore2 = files[0].rfind('_', pos_uscore1 - 1);
    DCHECK(pos_uscore2 != string::npos) << "Bad file: " << files[0];
    int64 start_time = 0;
    int64 end_time = 0;
    timer::Date utc_start_time(true);
    timer::Date utc_end_time(true);

    for ( int i = 0; i < files.size(); i++ ) {
      if ( utc_start_time.SetFromShortString(
               files[i].substr(pos_uscore2 + 1, pos_uscore1 - pos_uscore2 - 1),
               true) &&
           utc_end_time.SetFromShortString(
               files[i].substr(pos_uscore1 + 1, pos_uscore1 - pos_uscore2 - 1),
               true) ) {
        if ( utc_start_time.GetTime() - end_time < 5000 ) {
          end_time = utc_end_time.GetTime();
        } else {
          medias_cache_.push_back(
              make_pair(strutil::StringPrintf(
                            "%s/%"PRId64"/%"PRId64"",
                            name_.c_str(),
                            (start_time),
                            (end_time)),
                        default_caps_));
          start_time = utc_start_time.GetTime();
          end_time = utc_end_time.GetTime();
        }
      }
    }
    if ( start_time > 0 ) {
      medias_cache_.push_back(
          make_pair(strutil::StringPrintf(
                        "%s/%"PRId64"/%"PRId64"",
                        name_.c_str(),
                        (start_time),
                        (end_time)),
                    default_caps_));
    }
    cache_compute_time_ = selector_->now();
  }
  for ( int i = 0; i < medias_cache_.size(); ++i ) {
    medias->push_back(medias_cache_[i]);
  }
}

bool TimeRangeElement::DescribeMedia(const string& media,
    MediaInfoCallback* callback) {
  ElementDescriptions medias;
  ListMedia(media.c_str(), &medias);
  if ( medias.empty() ) {
    return false;
  }
  string filename = strutil::JoinPaths(home_dir_, medias[0].first);
  ILOG_ERROR << "TimeRangeElement::DescribeMedia is BLOCKING!"
                " Implement an asynchronous method!";
  MediaInfo media_info;
  if ( !util::ExtractMediaInfoFromFile(filename, &media_info) ) {
    return false;
  }
  callback->Run(&media_info);
  return true;
}

bool TimeRangeElement::AddRequest(
    const char* media_name,
    streaming::Request* req,
    streaming::ProcessingCallback* callback) {
  pair<string, string> split(strutil::SplitFirst(media_name, '/'));
  if ( name() != split.first ) {
    ILOG_ERROR << "AddRequest, missmatch prefix: [" << media_name << "]";
    return false;
  }
  if ( requests_.find(req) != requests_.end() ) {
    return false;
  }
  string date_range = split.second;
  if ( global_state_keeper_->GetValue(split.second,
                                      &date_range) ) {
    ILOG_DEBUG << "Making conversion " << split.second << " => " << date_range;
  }
  timer::Date start_time(true);
  timer::Date end_time(true);
  if ( !GetDates(date_range.c_str(), &start_time, &end_time) ) {
    ILOG_ERROR << "Invalid Dates: [" << media_name << "]";
    return false;
  }
  TimeRangeRequest* trr = new TimeRangeRequest(mapper_,
                                               selector_,
                                               home_dir_,
                                               root_media_path_,
                                               file_prefix_,
                                               media_name,
                                               name());
  if ( !trr->StartRequest(start_time, end_time, req, callback) ) {
    ILOG_ERROR << "StartRequest failed for: [" << media_name << "] - "
               << " start_time: " << start_time.ToShortString()
               << " end_time: " << end_time.ToShortString();
    delete trr;
    return false;
  }
  requests_.insert(make_pair(req, trr));
  return true;
}

void TimeRangeElement::RemoveRequest(streaming::Request* req) {
  ReqMap::iterator it = requests_.find(req);
  if ( it != requests_.end() ) {
    it->second->Close(false);

    delete it->second;
    requests_.erase(it);
  }
  // maybe complete close
  if ( close_completed_ != NULL && requests_.empty() ) {
    Closure* call_on_close = close_completed_;
    close_completed_ = NULL;

    selector_->RunInSelectLoop(call_on_close);
  }
}

void TimeRangeElement::Close(Closure* call_on_close) {
  CHECK_NULL(close_completed_);
  if ( requests_.empty() ) {
    selector_->RunInSelectLoop(call_on_close);
    return;
  }
  close_completed_ = call_on_close;

  // Send EOS to all clients.
  vector<pair<streaming::Request*,
    streaming::ProcessingCallback*> > callbacks;
  for ( ReqMap::iterator it = requests_.begin(); it != requests_.end(); ++it ) {
    if ( it->second->callback() != NULL) {
      callbacks.push_back(make_pair(it->first, it->second->callback()));
    }
  }

  for ( int i = 0; i < callbacks.size(); ++i ) {
    callbacks[i].second->Run(scoped_ref<Tag>(new streaming::EosTag(
            0, streaming::kDefaultFlavourMask, true)).get(), 0);
  }
}
void TimeRangeElement::GetRange(
    rpc::CallContext<string>* call,
    const string& range_name) {
  string result;
  global_state_keeper_->GetValue(range_name, &result);
  call->Complete(result);
}
void TimeRangeElement::GetAllRangeNames(
    rpc::CallContext<vector<string> >* call) {
  map<string, string>::const_iterator begin, end, it;
  global_state_keeper_->GetBounds("", &begin, &end);
  vector<string> result;
  for ( it = begin; it != end; ++it ) {
    result.push_back(it->first.substr(
                         global_state_keeper_->prefix().size()));
  }
  call->Complete(result);
}
void TimeRangeElement::SetRange(
    rpc::CallContext<bool>* call,
    const string& range_name,
    const string& range_dates) {
  bool result = false;
  if ( range_dates.empty() ) {
    global_state_keeper_->DeleteValue(range_name);
    result = true;
  } else {
    timer::Date test(true);
    if ( GetDates(range_dates, &test, &test) ) {
      global_state_keeper_->SetValue(range_name, range_dates);
      result = true;
    }
  }
  call->Complete(result);
}

}
