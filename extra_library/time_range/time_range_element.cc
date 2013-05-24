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
#include <whisperlib/common/base/callback/callback.h>
#include <whisperlib/common/io/ioutil.h>

#define ILOG(level)  LOG(level) << info() << ": "
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

struct RangeFileInfo {
  // internal media name, includes the aio_file_element.
  string media_;
  // tags arriving from the aio_file_element need to be time shifted then output
  int64 offset_ms_;
  // play duration (we can play 3 seconds from a 7 sec file)
  int64 duration_ms_;
  RangeFileInfo(const string media, int64 offset_ms, int64 duration_ms)
    : media_(media), offset_ms_(offset_ms), duration_ms_(duration_ms) {}
  string ToString() const {
    return strutil::StringPrintf("RangeFileInfo{media_: %s"
        ", offset_ms_: %"PRId64", duration_ms_: %"PRId64"}",
        media_.c_str(), offset_ms_, duration_ms_);
  }
};

typedef Callback1<TimeRangeRequest*> SetRangeFilesCallback;

class TimeRangeRequest : public streaming::ElementController {
 public:
  TimeRangeRequest(net::Selector* selector,
                   ElementMapper* mapper,
                   const string& element_name,
                   const string& media_name,
                   int64 start_ts,
                   int64 end_ts,
                   SetRangeFilesCallback* set_range_files_callback)
      : selector_(selector),
        mapper_(mapper),
        element_name_(element_name),
        media_name_(media_name),
        start_ts_(start_ts),
        end_ts_(end_ts),
        files_(),
        first_media_origin_ms_(0),
        last_media_limit_ms_(-1),
        set_range_files_callback_(set_range_files_callback),
        crt_file_index_(-1),
        req_(NULL),
        callback_(NULL),
        crt_req_(NULL),
        processing_callback_(NewPermanentCallback(this,
            &TimeRangeRequest::ProcessTag)),
        play_next_alarm_(*selector),
        pause_count_(0),
        metadata_sent_(false),
        source_started_sent_(false) {
    set_range_files_callback_->Run(this);
    ILOG_INFO << "New TimeRangeRequest"
                 ", start: " << start_ts_ << "(" << timer::Date(start_ts_).ToString() << ")"
              << ", end: " << end_ts_ << "(" << timer::Date(end_ts_).ToString() << ")"
              << ", #" << files_.size() << " files"
                 ", duration: " << DurationMs() << " ms";
  }
  virtual ~TimeRangeRequest() {
    DCHECK(callback_ == NULL) << " Not closed";

    if ( req_ != NULL ) {
      req_->set_controller(NULL);
    }

    delete processing_callback_;
    processing_callback_ = NULL;
  }
  int64 start_ts() const { return start_ts_; }
  int64 end_ts() const { return end_ts_; }
  bool IsEmpty() const { return files_.empty(); }
  void SetFiles(const vector<RangeFileInfo>& files,
                int64 first_media_origin_ms,
                int64 last_media_limit_ms) {
    files_ = files;
    first_media_origin_ms_ = first_media_origin_ms;
    last_media_limit_ms_ = last_media_limit_ms;
  }

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
    if ( seek_pos_ms < 0 ) {
      LOG_ERROR << "Illegal seek position: " << seek_pos_ms;
      return false;
    }

    // Find the file to seek to
    int i = 0;
    // duration of the skipped files
    int64 iduration_ms = 0;
    for (; i < files_.size(); i++ ) {
      if ( files_[i].offset_ms_ <= seek_pos_ms &&
           seek_pos_ms < files_[i].offset_ms_ + files_[i].duration_ms_ ) {
        break;
      }
      iduration_ms += files_[i].duration_ms_;
    }
    if ( i == files_.size() ) {
      LOG_ERROR << "Out of range seek position: " << seek_pos_ms;
      return false;
    }
    // seek in current file
    if ( i == crt_file_index_ && crt_req_ != NULL ) {
      ILOG_INFO << "Seek into current file: " << files_[crt_file_index_].ToString()
                << " to position: " << (seek_pos_ms - iduration_ms);
      crt_req_->controller()->Seek(seek_pos_ms - iduration_ms);
      return true;
    }
    // seek to a different file
    ClearRequest();
    crt_file_index_ = i;
    IssueRequest(seek_pos_ms - iduration_ms);
    return true;
  }

  /////////////////////////////////////////////////////////////////////

  void StartRequest(Request* req, ProcessingCallback* callback) {
    CHECK_NULL(req_);
    CHECK_NULL(callback_);
    CHECK(!files_.empty());

    req_ = req;
    callback_ = callback;

    req_->set_controller(this);
    crt_file_index_ = -1;

    // start with seek
    if ( req->info().seek_pos_ms_ > 0 ) {
      Seek(req->info().seek_pos_ms_);
      return;
    }

    // start with the first file
    play_next_alarm_.Set(NewPermanentCallback(this, &TimeRangeRequest::PlayNext),
        true, 0, false, true);
    return;
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

    crt_file_index_ = -1;
    callback_ = NULL;
  }

 private:
  string info() const {
    return element_name_ + "(" + media_name_ + ")";
  }
  int64 DurationMs() const {
    int64 duration_ms = 0;
    for ( uint32 i = 0; i < files_.size(); i++ ) {
      duration_ms += files_[i].duration_ms_;
    }
    return duration_ms;
  }

  // Clears only the upstream request. The downstream client stays connected,
  // no EOS is sent.
  // NEEDS: separate call context from upstream.
  void ClearRequest() {
    if ( crt_req_ != NULL ) {
      mapper_->RemoveRequest(crt_req_, processing_callback_);
      crt_req_ = NULL;
    }
  }

  // NEEDS: separate call context from both upstream and downstream.
  void PlayNext() {
    ClearRequest();

    // go to next file
    crt_file_index_++;

    // if no more files, then ask for an update of the files
    //    (maybe new files appeared on disk meanwhile)
    // if still no more files, then stop
    if ( crt_file_index_ == files_.size() ) {
      set_range_files_callback_->Run(this);
      if ( crt_file_index_ >= files_.size() ) {
        Close(true);
        return;
      }
    }

    // play current file
    CHECK_LT(crt_file_index_, files_.size());
    IssueRequest(0);
  }
  void IssueRequest(int64 seek_pos_ms) {
    CHECK_NULL(crt_req_);
    crt_req_ = new streaming::Request();
    crt_req_->mutable_caps()->flavour_mask_ = kDefaultFlavourMask;
    if ( crt_file_index_ == 0 ) {
      crt_req_->mutable_info()->media_origin_pos_ms_ =
          first_media_origin_ms_;
    }
    if ( crt_file_index_ == files_.size()-1 ) {
      crt_req_->mutable_info()->limit_ms_ = last_media_limit_ms_;
    }
    if ( seek_pos_ms > 0 ) {
      crt_req_->mutable_info()->seek_pos_ms_ = seek_pos_ms;
    }

    ILOG_INFO << "Sending to play: #" << crt_file_index_ << " => "
              << files_[crt_file_index_].ToString()
              << ", req_info: " << crt_req_->info().ToString();

    if ( !mapper_->AddRequest(files_[crt_file_index_].media_,
                              crt_req_, processing_callback_) ) {
      ILOG_ERROR << "Cannot add to play: " << files_[crt_file_index_].media_;
      delete crt_req_;
      crt_req_ = NULL;
      SendEos();
      return;
    }

    if ( pause_count_ > 0 ) {
      if ( crt_req_->controller() != NULL ) {
        for ( int p = pause_count_; p > 0; --p) {
          crt_req_->controller()->Pause(true);
        }
      }
    }
  }


  void SendEos() {
    // NEEDS: separate call context from downstream.
    if ( source_started_sent_ ) {
      Send(scoped_ref<Tag>(new SourceEndedTag(0,
          streaming::kDefaultFlavourMask,
          strutil::JoinMedia(element_name_, media_name_),
          strutil::JoinMedia(element_name_, media_name_))).get(), 0);
    }
    Send(scoped_ref<Tag>(new streaming::EosTag(
        0, streaming::kDefaultFlavourMask, false)).get(), 0);
    // after EOS make sure nothing follows
    callback_ = NULL;
  }

  // NEEDS: separate call context from downstream.
  void ProcessTag(const Tag* tag, int64 timestamp_ms) {
    CHECK_NOT_NULL(callback_) << "Tag received after Close()"
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

    if ( tag->type() == streaming::Tag::TYPE_SOURCE_STARTED ) {
      if ( !source_started_sent_ ) {
        Send(scoped_ref<Tag>(new SourceStartedTag(0,
            kDefaultFlavourMask,
            strutil::JoinMedia(element_name_, media_name_),
            strutil::JoinMedia(element_name_, media_name_),
            false,
            0)).get(), 0);
        source_started_sent_ = true;
      }
      // eat source started from the files
      return;
    }

    // forward updated media info
    if ( tag->type() == streaming::Tag::TYPE_MEDIA_INFO ) {
      const MediaInfoTag* info_tag = static_cast<const MediaInfoTag*>(tag);
      if ( metadata_sent_ ) {
        return;
      }
      metadata_sent_ = true;
      scoped_ref<MediaInfoTag> new_info_tag(new MediaInfoTag(*info_tag));
      new_info_tag->mutable_info()->set_duration_ms(
          end_ts_ == timer::Date::kFar ? 0 : DurationMs());
      new_info_tag->mutable_info()->set_file_size(0);
      Send(new_info_tag.get(), timestamp_ms + files_[crt_file_index_].offset_ms_);
      return;
    }

    // forward updated metadata
    if ( tag->type() == streaming::Tag::TYPE_FLV ) {
      const FlvTag* flv_tag = static_cast<const FlvTag*>(tag);
      if ( flv_tag->body().type() == streaming::FLV_FRAMETYPE_METADATA ) {
        const FlvTag::Metadata& metadata = flv_tag->metadata_body();
        if ( metadata.name().value() == streaming::kOnMetaData ) {
          if ( metadata_sent_ ) {
            return;
          }
          metadata_sent_ = true;
          scoped_ref<FlvTag> new_meta_tag(new FlvTag(*flv_tag, -1, true));
          FlvTag::Metadata& new_meta = new_meta_tag->mutable_metadata_body();
          if ( end_ts_ == timer::Date::kFar ) {
            new_meta.mutable_values()->Erase("duration");
          } else {
            new_meta.mutable_values()->Set("duration",
                new rtmp::CNumber(DurationMs()/ 1000.0));
          }
          new_meta.mutable_values()->Erase("datasize");
          new_meta.mutable_values()->Erase("offset");
          new_meta.mutable_values()->Erase("filesize");

          ILOG_INFO << "Sending updated metadata: " << new_meta_tag->ToString();
          Send(new_meta_tag.get(), timestamp_ms +
                                   files_[crt_file_index_].offset_ms_);
          return;
        }
      }
    }
    Send(tag, timestamp_ms + files_[crt_file_index_].offset_ms_);
  }

  void Send(const Tag* tag, int64 timestamp_ms) {
    if ( callback_ != NULL ) {
      //LOG_INFO << "### >>>>>>> Send: " << timestamp_ms << " " << tag->ToString();
      callback_->Run(tag, timestamp_ms);
    }
  }

  net::Selector* selector_;
  ElementMapper* mapper_;

  // parent element name; for composing SourceStarted
  const string element_name_;
  // the name of this range; For logging and for SourceStartedTag.
  // e.g.: "20111207-144331/20111207-145323" or: "some_named_time_range"
  const string media_name_;

  // the absolute timestamps(from epoch ms) of this range (start-end)
  const int64 start_ts_;
  const int64 end_ts_;

  // files to play in this range
  vector<RangeFileInfo> files_;
  int64 first_media_origin_ms_;
  int64 last_media_limit_ms_;

  // callback to parent TimeRangeElement::GetMediaFiles
  SetRangeFilesCallback* set_range_files_callback_;

  // the index of the current file being played
  int32 crt_file_index_;

  // downstream client
  streaming::Request* req_;
  streaming::ProcessingCallback* callback_;

  // upstream request
  streaming::Request* crt_req_;
  // permanent callback to ProcessTag
  streaming::ProcessingCallback* processing_callback_;

  ::util::Alarm play_next_alarm_;

  // The only situation this is useful is when a Pause() finds req_ == NULL
  // (i.e.: after a file EOS, and before issuing the request to the next file).
  int pause_count_;

  // flags for sending signaling tags just once
  bool metadata_sent_;
  bool source_started_sent_;

  DISALLOW_EVIL_CONSTRUCTORS(TimeRangeRequest);
};

//////////////////////////////////////////////////////////////////////

const char TimeRangeElement::kElementClassName[] = "time_range";

TimeRangeElement::TimeRangeElement(
    const string& name,
    ElementMapper* mapper,
    net::Selector* selector,
    io::StateKeepUser* global_state_keeper,
    io::StateKeepUser* local_state_keeper,
    const string& rpc_path,
    rpc::HttpServer* rpc_server,
    const string& aio_file_element,
    uint32 update_media_interval_sec)
    : Element(kElementClassName, name, mapper),
      ServiceInvokerTimeRangeElementService(
          ServiceInvokerTimeRangeElementService::GetClassName()),
      selector_(selector),
      global_state_keeper_(global_state_keeper),
      local_state_keeper_(local_state_keeper),
      rpc_path_(rpc_path),
      rpc_server_(rpc_server),
      aio_file_element_(aio_file_element),
      media_(),
      update_media_alarm_(*selector),
      update_media_interval_sec_(update_media_interval_sec),
      requests_(),
      set_range_files_callback_(NewPermanentCallback(this,
          &TimeRangeElement::SetRangeFiles)),
      close_completed_(NULL) {
}

TimeRangeElement::~TimeRangeElement() {
  if ( rpc_server_ != NULL ) {
    rpc_server_->UnregisterService(rpc_path_, this);
  }
  delete set_range_files_callback_;
  set_range_files_callback_ = NULL;

  DCHECK(requests_.empty());
  DCHECK(close_completed_ == NULL);
}

bool TimeRangeElement::SetRange(const string& range_name,
    const string& range_dates, string* out_error) {
  if ( range_dates == "" ) {
    global_state_keeper_->DeleteValue(range_name);
    return true;
  }
  if ( !IsValidDateTimeRange(range_dates) ) {
    *out_error = "Invalid range dates: [" + range_dates + "]";
    return false;
  }
  global_state_keeper_->SetValue(range_name, range_dates);
  return true;
}
string TimeRangeElement::GetRange(const string& range_name) const {
  string range_dates;
  global_state_keeper_->GetValue(range_name, &range_dates);
  return range_dates;
}

bool TimeRangeElement::Initialize() {
  if ( rpc_server_ != NULL &&
       !rpc_server_->RegisterService(rpc_path_, this) ) {
    LOG_ERROR << "rpc_server_->RegisterService failed";
    return false;
  }
  UpdateMedia();
  update_media_alarm_.Set(
      NewPermanentCallback(this,&TimeRangeElement::UpdateMedia),
      true, update_media_interval_sec_*1000, true, true);
  return true;
}

bool TimeRangeElement::HasMedia(const string& media) {
  return global_state_keeper_->HasValue(media) ||
         IsValidDateTimeRange(media);
}

void TimeRangeElement::ListMedia(const string& media_dir,
                                 vector<string>* out) {
  for ( int i = 0; i < media_.size(); i++ ) {
    int64 start_time = 0;
    int64 end_time = 0;
    ParseTimeRangeMedia(media_[i], &start_time, &end_time);
    out->push_back(strutil::StringPrintf("%s/%"PRId64"/%"PRId64"",
        name().c_str(), start_time, end_time));
  }
}

bool TimeRangeElement::DescribeMedia(const string& media,
    MediaInfoCallback* callback) {
  ILOG_ERROR << "TimeRangeElement::DescribeMedia NOT IMPLEMENETED";
  return false;
}

bool TimeRangeElement::AddRequest(const string& media,
    Request* req, ProcessingCallback* callback) {
  if ( requests_.find(req) != requests_.end() ) {
    LOG_FATAL << "Duplicate AddRequest! for req: " << req->ToString();
    return false;
  }
  if ( media_.empty() ) {
    LOG_ERROR << "Nothing to play, no timerange media files found!"
                 " for aio_file_element: " << aio_file_element_;
    return false;
  }
  // find the begin & end time
  string date_range = media;
  if ( global_state_keeper_->GetValue(media, &date_range) ) {
    ILOG_DEBUG << "Making conversion " << media << " => " << date_range;
  }
  int64 start_time = 0;
  int64 end_time = 0;
  if ( !ParseDateTimeRange(date_range, &start_time, &end_time) ) {
    ILOG_ERROR << "Invalid Dates: [" << date_range << "]";
    return false;
  }

  TimeRangeRequest* trr = new TimeRangeRequest(selector_, mapper_,
      name(), media, start_time, end_time, set_range_files_callback_);
  if ( trr->IsEmpty() ) {
    ILOG_ERROR << "Empty TimeRangeRequest, failed AddRequest()"
                  " on media: [" << media << "]";
    delete trr;
    return false;
  }
  trr->StartRequest(req, callback);
  requests_[req] = trr;
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
    selector_->RunInSelectLoop(close_completed_);
    close_completed_ = NULL;

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
  for ( ReqMap::iterator it = requests_.begin(); it != requests_.end(); ++it ) {
    TimeRangeRequest* tr = it->second;
    tr->Close(true);
  }
}

void TimeRangeElement::GetAllRangeNames(vector<string>* out_range_names) {
  global_state_keeper_->GetKeys("", out_range_names);
}
void TimeRangeElement::GetAllTimeSpans(vector<TimeSpan>* out_spans) {
  int64 span_start_ts = 0;
  int64 span_end_ts = 0;
  for ( uint32 i = 0; i < media_.size(); i++ ) {
    int64 start_ts, end_ts;
    if ( !ParseTimeRangeMedia(media_[i], &start_ts, &end_ts) ) {
      LOG_ERROR << "Ignoring invalid time range media: [" << media_[i] << "]";
      continue;
    }
    if ( span_start_ts == 0 ) {
      // initialize current span
      span_start_ts = start_ts;
      span_end_ts = end_ts;
      continue;
    }
    if ( start_ts == span_end_ts ) {
      // enlarge current span
      span_end_ts = end_ts;
      continue;
    }
    // close current span, and start a new one
    out_spans->push_back(TimeSpan(span_start_ts, span_end_ts));
    span_start_ts = start_ts;
    span_end_ts = end_ts;
  }
}

void TimeRangeElement::GetAllTimeSpans(
    rpc::CallContext< vector<TimeSpan> >* call) {
  vector<TimeSpan> time_spans;
  GetAllTimeSpans(&time_spans);
  call->Complete(time_spans);
}
void TimeRangeElement::GetRange(rpc::CallContext<string>* call,
                                const string& range_name) {
  call->Complete(GetRange(range_name));
}
void TimeRangeElement::GetAllRangeNames(
    rpc::CallContext<vector<string> >* call) {
  vector<string> range_names;
  GetAllRangeNames(&range_names);
  call->Complete(range_names);
}
void TimeRangeElement::SetRange(rpc::CallContext<MediaOpResult>* call,
                                const string& range_name,
                                const string& range_dates) {
  string err;
  const bool success = SetRange(range_name, range_dates, &err);
  call->Complete(MediaOpResult(success, err));
}
bool TimeRangeElement::ParseDateTime(const string& comp, int64* out_ts) {
  if ( comp.find('-') != string::npos ) {
    // ShortString format: "20111207-144331"
    timer::Date d;
    if ( !d.FromShortString(comp, true) ) {
      LOG_ERROR << "Invalid time specified: [" << comp << "]";
      return false;
    }
    *out_ts = d.GetTime();
    return true;
  }
  // Unix timestamp format: "1329415828" (seconds)
  errno = 0;
  char* endp = NULL;
  const long long int secs = ::strtoll(comp.c_str(), &endp, 10);
  if ( secs == 0 || errno != 0 || *endp != '\0' ) {
    LOG_ERROR << "Invalid time specified: [" << comp << "]";
    return false;
  }
  *out_ts = static_cast<int64>(secs) * 1000;
  return true;
}

bool TimeRangeElement::ParseDateTimeRange(const string& s,
                                          int64* out_start_ts,
                                          int64* out_end_ts) {
  if ( s.empty() ) {
    LOG_ERROR << "Empty DateTimeRange";
    return false;
  }
  vector<string> comp;
  strutil::SplitString(s, "/", &comp);
  if ( comp.size() > 2 ) {
    LOG_ERROR << "Invalid range specified: [" << s << "]";
    return false;
  }
  if ( !ParseDateTime(comp[0], out_start_ts) ) {
    return false;
  }
  if ( comp.size() == 2 ) {
    if ( !ParseDateTime(comp[1], out_end_ts) ) {
      return false;
    }
  } else {
    *out_end_ts = timer::Date::kFar;
  }
  return true;
}
bool TimeRangeElement::IsValidDateTimeRange(const string& s) {
  int64 a = 0;
  return ParseDateTimeRange(s, &a, &a);
}

void TimeRangeElement::UpdateMedia() {
  vector<string> info;
  mapper_->ListMedia(aio_file_element_, &info);
  // filter
  media_.clear();
  for ( uint32 i = 0; i < info.size(); i++ ) {
    if ( !IsValidTimeRangeMedia(info[i]) ) {
      continue;
    }
    media_.push_back(info[i]);
  }
  // Luckily the timerange files can be lexicographically sorted.
  ::sort(media_.begin(), media_.end());
  LOG_INFO << "Updated media ranges, found: #" << info.size()
           << " files in: " << aio_file_element_
           << endl << "    first: "
           << (media_.empty() ? "NULL" : media_[0].c_str())
           << endl << "    last: "
           << (media_.empty() ? "NULL" : media_[media_.size()-1].c_str());
}
void TimeRangeElement::SetRangeFiles(TimeRangeRequest* t) {
  // Create the vector of files to play. Attach to each file timing info
  // so that a continuous sane file is obtained on output.
  // e.g.
  //   file_a: 7 seconds on disk
  //     Play: offset: 0, duration: 6, first file media_origin_pos_ms_: 1
  //   file_b: 3 seconds on disk
  //     Play: offset: 6, duration: 3
  //   -- gap between 9 -> 13, missing media --
  //   file_c: 8 seconds on disk
  //     Play: offset: 13, duration: 5, last file limit_ms_: 5
  // Tags from the first file are offset by 0 seconds: 0,1,2,3,4,5,6
  // Tags from the seconds file are offset by 6 seconds: 6,7,8,9
  // Tags from the third file are offset by 13 seconds: 13,14,15,16,17
  // Total range duration: 18 seconds (including the gap).
  // NOTE: Redundancy info: the first file has always offset == 0
  //                        the intermediate files have always duration == full
  //                        the last file has always duration == limit_ms_
  int64 start_time = t->start_ts();
  int64 end_time = t->end_ts();
  int32 i = GetTimeRangeMediaIndex(media_, start_time);
  if ( i == -1 ) {
    LOG_ERROR << "Nothing to play for"
                 " start_time: " << timer::Date(start_time, true).ToString()
              << ", end_time: " << timer::Date(end_time, true).ToString()
              << ", #" << media_.size() << " media files"
              << ", first media: "
              << (media_.empty() ? "NULL" : media_[0].c_str())
              << ", last_media: "
              << (media_.empty() ? "NULL" : media_[media_.size()-1].c_str());
    return;
  }
  vector<RangeFileInfo> to_play;
  int64 first_file_media_origin_ms = 0;
  int64 last_file_limit_ms = -1;
  for ( ; i < media_.size(); i++ ) {
    int64 file_begin, file_end;
    ParseTimeRangeMedia(media_[i], &file_begin, &file_end);
    const string media = strutil::JoinMedia(aio_file_element_, media_[i]);
    if ( to_play.empty() ) {
      // first file
      if ( end_time < file_begin ) {
        // the range is inside a media gap, nothing to play
        return;
      }
      int64 duration_ms = end_time < file_end ?
                            end_time - start_time : file_end - start_time;
      to_play.push_back(RangeFileInfo(media, 0, duration_ms));
      // NOTE: if the range starts in a gap then: start_time < file_begin
      first_file_media_origin_ms = start_time > file_begin ?
                                   start_time - file_begin : 0;
      if ( end_time < file_end ) {
        // the whole range is contained in the first file (which is also
        // the last)
        last_file_limit_ms = duration_ms;
        LOG_INFO << "RANGE Compact: " << media
                 << ", media_origin_ms: " << first_file_media_origin_ms
                 << ", duration: " << last_file_limit_ms;
        break;
      }
      // the range is longer than the first file
      LOG_INFO << "RANGE First: " << media << ", duration: " << duration_ms
               << ", media_origin_ms: " << first_file_media_origin_ms;
      continue;
    }
    if ( file_end < end_time ) {
      // intermediate file, fully contained inside the range
      int64 offset_ms = file_begin - start_time;
      int64 duration_ms = file_end - file_begin;
      to_play.push_back(RangeFileInfo(media, offset_ms, duration_ms));
      LOG_INFO << "RANGE Inter: " << media << ", offset: " << offset_ms
               << ", duration: " << duration_ms;
      continue;
    }
    if ( end_time < file_begin ) {
      // this file is completely outside the range
      LOG_INFO << "RANGE Last : the range ends in a gap; previous file is"
                  " also the last";
      break;
    }
    // last file, partially used
    int64 offset_ms = file_begin - start_time;
    int64 duration_ms = end_time - file_begin;
    to_play.push_back(RangeFileInfo(media, offset_ms, duration_ms));
    LOG_INFO << "RANGE Last : " << media << ", offset: " << offset_ms
             << ", duration: " << duration_ms;
    last_file_limit_ms = duration_ms;
    break;
  }
  CHECK_GE(first_file_media_origin_ms, 0);
  CHECK_GE(last_file_limit_ms, -1);

  t->SetFiles(to_play, first_file_media_origin_ms, last_file_limit_ms);
}

}
