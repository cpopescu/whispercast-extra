// (c) 2012, Whispersoft s.r.l.
// All rights reserved.
//
// Author: Cosmin Tudorache
//

#include "extra_library/time_shift/time_shift_element.h"
#include <whisperlib/common/base/alarm.h>
#include <whisperlib/common/base/date.h>
#include <whisperstreamlib/base/media_info_util.h>
#include <whisperstreamlib/base/tag_splitter.h>
#include <whisperstreamlib/base/time_range.h>
#include <whisperstreamlib/flv/flv_file_writer.h>

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
class BufferedSaver;
class RequestController : public ElementController {
 public:
  RequestController(BufferedSaver* buf,
                    Request* req,
                    ProcessingCallback* callback,
                    uint32 tag_index,
                    const string& description);
  virtual ~RequestController() {
  }

  const string& info() const { return description_;}
  void ReleaseRequest(Request** out_req, ProcessingCallback** out_callback) {
    req_->set_controller(NULL);
    *out_req = req_;
    *out_callback = callback_;
    req_ = NULL;
    callback_ = NULL;
  }
  void NotifyNewTag() {
    if ( pause_count_ == 0 ) {
      run_.Start();
    }
  }
  void SendEos() {
    run_.Stop();
    callback_->Run(scoped_ref<Tag>(new EosTag(
        0, kDefaultFlavourMask, false)).get(), 0);
  }

  ////////////////////////////////////////////////////////////////////////////
  // ElementController methods

  // Return true if an element controller supports pausing / unpausing
  // of the element
  virtual bool SupportsPause() const { return true; }

  // Pause the element (pause true - > stop, false -> restart)
  virtual bool Pause(bool pause) {
    if ( pause ) {
      pause_count_++;
    } else {
      CHECK_GT(pause_count_, 0);
      pause_count_--;
    }
    if ( pause_count_ == 0 ) {
      ILOG_DEBUG << "Pause: Start";
      run_.Start();
    } else {
      ILOG_DEBUG << "Pause: Stop";
      run_.Stop();
    }
    return true;
  }

 private:
  void Run();

  void SendBootstrap(const string& element_name, const MediaInfo& media_info,
      int64 first_tag_ts) {
    CHECK(!is_bootstrapped_);
    first_tag_ts_ = first_tag_ts;
    SendTag(scoped_ref<SourceStartedTag>(new SourceStartedTag(
        0, kDefaultFlavourMask,
        element_name, element_name, false, 0)).get(), first_tag_ts);
    SendTag(scoped_ref<Tag>(new MediaInfoTag(
        0, kDefaultFlavourMask, media_info)).get(), first_tag_ts);
    is_bootstrapped_ = true;
  }
  void SendTag(const Tag* tag, int64 ts) {
    callback_->Run(tag, ts - first_tag_ts_);
  }

 private:
  // parrent buffer
  BufferedSaver* buf_;
  // the client Request
  Request* req_;
  // the client link
  ProcessingCallback* callback_;
  // the start timestamp (the original ts of the first buffered tag sent to this request)
  int64 first_tag_ts_;
  // next tag (this is the index inside buffer_->media_)
  uint32 tag_index_;
  // counts the Pause() calls (flow control + user pause)
  uint32 pause_count_;

  // motorizes the request. This alarm sends tags downstream; reacts to Pause().
  ::util::Alarm run_;

  // true = SourceStartedTag + MediaInfoTag + .. were sent
  bool is_bootstrapped_;

  // just for logging
  const string description_;
};

class BufferedSaver {
 public:
  typedef Callback3<BufferedSaver*, Request*, ProcessingCallback*> HandlePlayFinished;
  typedef map<Request*, RequestController*> RequestMap;
 public:
  BufferedSaver(net::Selector* selector,
                HandlePlayFinished* handle_play_finished,
                const string& element_name,
                int64 begin_ts,
                const string& description)
    : selector_(selector),
      begin_ts_(begin_ts),
      element_name_(element_name),
      media_info_(),
      media_(),
      handle_play_finished_(handle_play_finished),
      requests_(),
      first_tag_ts_(-1),
      is_active_(true),
      is_closed_(false),
      disk_writer_(NULL),
      description_(description) {
  }

  virtual ~BufferedSaver() {
    CHECK(requests_.empty());
    CHECK(media_.empty());
    CHECK_NULL(disk_writer_);
  }

  net::Selector* selector() { return selector_; }
  const string& element_name() const { return element_name_; }
  const MediaInfo& media_info() const { return media_info_; }
  void set_media_info(const MediaInfo& inf) { media_info_ = inf; }
  string info() const {
    return description_ + strutil::StringPrintf("[size: %"PRId64"]", SizeMs());
  }
  bool is_active() const { return is_active_; }
  void set_active(bool is_active) { is_active_ = is_active; }

  bool is_closed() const { return is_closed_; }

  // returns buffered media length in milliseconds
  int64 SizeMs() const {
    if ( media_.empty() ) { return 0;}
    return media_[media_.size()-1].second;
  }
  // the number of requests feeding on this buffer
  uint32 RequestCount() const { return requests_.size(); }
  // used by each RequestController to retrieve next tag
  const Tag* GetTag(uint32 index, int64* out_ts) {
    if ( index >= media_.size() ) {
      return NULL;
    }
    *out_ts = media_[index].second;
    return media_[index].first.get();
  }
  void NotifyPlayFinished(RequestController* r) {
    // if we're active, then ignore this "play finished",
    // because new tags will come.
    if ( is_active_ ) {
      return;
    }
    // remove the finished request,
    Request* req = NULL;
    ProcessingCallback* callback = NULL;
    r->ReleaseRequest(&req, &callback);
    requests_.erase(req);
    selector_->DeleteInSelectLoop(r);
    // forward the notification to parent
    handle_play_finished_->Run(this, req, callback);
  }

  void ProcessTag(const Tag* new_tag, int64 new_ts) {
    CHECK_GE(SizeMs(), 0);
    // append new_tag to media buffer
    if ( first_tag_ts_ == -1 ) {
      first_tag_ts_ = new_ts;
    }
    if ( new_tag->type() == Tag::TYPE_MEDIA_INFO ) {
      // set current media_info
      media_info_ = static_cast<const MediaInfoTag*>(new_tag)->info();
      // update media info, according to timeshift buffer
      media_info_.set_pausable(true);
      media_info_.set_seekable(false);
      media_info_.set_duration_ms(0);
      return;
    }
    if ( media_.empty() ) {
      CHECK_EQ(new_ts, first_tag_ts_);
    } else {
      int64 diff = new_ts - first_tag_ts_ - media_[media_.size()-1].second;
      CHECK(diff >= 0 && diff < 1000) << " diff: " << diff;
    }
    media_.push_back(make_pair(new_tag, new_ts - first_tag_ts_));

    // notify all the requests about the new tag
    NotifyAllRequests();
  }
  void NotifyAllRequests() {
    for ( RequestMap::iterator it = requests_.begin();
          it != requests_.end(); ++it) {
      RequestController* r = it->second;
      r->NotifyNewTag();
    }
  }

  void SaveToFile(string dir, int64 end_ts) {
    if ( SizeMs() == 0 ) {
      return; // empty buffer, nothing to save
    }

    // Update some properties of the media, suitable for file,
    // use a copy of the media info
    MediaInfo inf(media_info_);
    inf.set_duration_ms(SizeMs());
    inf.set_seekable(true);

    // write the whole buffer to disk
    FlvFileWriter writer(true, true);
    const string filename = strutil::JoinPaths(dir,
          MakeTimeRangeFile(begin_ts_, end_ts, MFORMAT_FLV));
    if ( !writer.Open(filename) ) {
      ILOG_ERROR << "Failed to open file: " << filename;
      return;
    }
    scoped_ref<FlvTag> metadata;
    if ( !util::ComposeFlv(inf, &metadata) ) {
      ILOG_ERROR << "Failed to compose Metadata from inf: " << inf.ToString();
      return;
    }
    writer.Write(metadata, 0);
    for ( uint32 i = 0; i < media_.size(); i++ ) {
      writer.Write(*static_cast<const FlvTag*>(media_[i].first.get()),
          media_[i].second);
    }
    writer.Close();
    ILOG_INFO << "Saved File: [" << filename << "]";
  }
  void AsyncSaveToFile(string dir, int64 end_ts) {
    CHECK_NULL(disk_writer_) << " Multiple AsyncSaveToFile() ???";
    disk_writer_ = new thread::Thread(NewCallback(this,
        &BufferedSaver::SaveToFile, dir, end_ts));
    disk_writer_->SetJoinable();
    disk_writer_->Start();
  }

  // delay: ms
  // from_begin: true = the delay is from buffer start
  //             false = the delay is from buffer end (i.e. current position)
  // on_keyframe: start request on a video keyframe
  bool AddRequest(Request* req, ProcessingCallback* callback,
                  int64 delay_ms, bool from_begin, bool on_keyframe) {
    ILOG_INFO << "AddRequest delay_ms: " << delay_ms
              << ", from begin: " << strutil::BoolToString(from_begin);
    if ( delay_ms > SizeMs() ) {
      ILOG_ERROR << "Delay too long: " << delay_ms
                 << ", buffer size: " << SizeMs() << " ms";
      return false;
    }
    int64 first_tag_ms = from_begin ? delay_ms : (SizeMs() - delay_ms);
    uint32 first_tag_index = 0;
    for ( ; first_tag_index < media_.size(); first_tag_index++ ) {
      if ( media_[first_tag_index].second >= first_tag_ms ) {
        if ( !on_keyframe ) {
          break;
        }
        if ( media_[first_tag_index].first->is_video_tag() &&
             media_[first_tag_index].first->can_resync() ) {
          break;
        }
      }
    }
    RequestController* r = new RequestController(
        this, req, callback, first_tag_index, info());
    requests_[req] = r;
    r->NotifyNewTag();

    return true;
  }
  void RemoveRequest(Request* req) {
    RequestMap::iterator it = requests_.find(req);
    CHECK(it != requests_.end()) << " Cannot find req: " << req->ToString();
    RequestController* r = it->second;
    requests_.erase(it);
    delete r;
  }
  void Close() {
    for ( RequestMap::iterator it = requests_.begin();
          it != requests_.end(); ++it ) {
      RequestController* r= it->second;
      r->SendEos();
    }
    media_.clear();
    first_tag_ts_ = -1;
    media_info_ = MediaInfo();
    is_closed_ = true;
    if ( disk_writer_ != NULL ) {
      disk_writer_->Join();
      delete disk_writer_;
      disk_writer_ = NULL;
    }
  }

 private:
  net::Selector* selector_;
  // absolute timestamp (from epoch, in ms). Useful when saving to file.
  const int64 begin_ts_;

  // The name of the element owning this saver.
  // This name is sent in SourceStartedTags to newly added requests.
  const string element_name_;

  MediaInfo media_info_;
  vector<pair<scoped_ref<const Tag>, int64> > media_;
  HandlePlayFinished* handle_play_finished_;

  // requests map
  RequestMap requests_;

  // the timestamp of the first ProcessTag's tag
  // (because the media buffer stores tags with timestamps starting on 0)
  int64 first_tag_ts_;

  // true = indicates that this is an active buffer, accumulating tags.
  //        Keep alive requests that have reached the end of buffer.
  // false = indicates a passive Buffer which no longer receives new tags
  //         but still serves requests.
  bool is_active_;

  // true = Close() was called, and this buffer is marked for deletion
  bool is_closed_;

  // a separate thread which does the disk saving
  thread::Thread* disk_writer_;

  // just for logging
  const string description_;
};


RequestController::RequestController(BufferedSaver* buf,
                  Request* req,
                  ProcessingCallback* callback,
                  uint32 tag_index,
                  const string& description)
  : buf_(buf), req_(req), callback_(callback),
    first_tag_ts_(-1), tag_index_(tag_index),
    pause_count_(0), run_(*buf->selector()), is_bootstrapped_(false),
    description_(description) {
  req_->set_controller(this);
  run_.Set(NewPermanentCallback(this, &RequestController::Run),
      true, 0, false, true);
}

void RequestController::Run() {
  while ( pause_count_ == 0 ) {
    int64 ts = 0;
    const Tag* tag = buf_->GetTag(tag_index_, &ts);
    if ( tag == NULL ) {
      buf_->NotifyPlayFinished(this);
      return;
    }
    if ( !is_bootstrapped_ ) {
      SendBootstrap(buf_->element_name(), buf_->media_info(), ts);
    }
    SendTag(tag, ts);
    tag_index_++;
  }
}

const char TimeShiftElement::kElementClassName[] = "time_shift";

TimeShiftElement::TimeShiftElement(const string& name,
                                   ElementMapper* mapper,
                                   net::Selector* selector,
                                   const string& media_path,
                                   const string& save_path,
                                   const string& time_range_element,
                                   int64 buffer_size_ms)
  : Element(kElementClassName, name, mapper),
    selector_(selector),
    media_path_(media_path),
    save_dir_(save_path),
    time_range_element_(time_range_element),
    buffer_size_ms_(buffer_size_ms),
    internal_req_(NULL),
    process_tag_callback_(NewPermanentCallback(this,
        &TimeShiftElement::ProcessTag)),
    calculator_(),
    buffers_(),
    play_finished_callback_(NewPermanentCallback(this,
        &TimeShiftElement::PlayFinished)),
    close_completed_(NULL) {
}
TimeShiftElement::~TimeShiftElement() {
  CHECK_NULL(internal_req_);
  DCHECK(close_completed_ == NULL);
  delete process_tag_callback_;
  process_tag_callback_ = NULL;
  delete play_finished_callback_;
  play_finished_callback_ = NULL;
  CHECK(buffers_.empty());
}

bool TimeShiftElement::Initialize() {
  buffers_.push_back(new BufferedSaver(
      selector_, play_finished_callback_, name(), timer::Date::Now(), info()));
  if ( !io::IsDir(save_dir_) &&
       !io::Mkdir(save_dir_, true) ) {
    ILOG_ERROR << "Failed to create save dir: [" << save_dir_ << "]";
    return false;
  }
  return Register();
}
bool TimeShiftElement::AddRequest(const string& media, Request* req,
                                  ProcessingCallback* callback) {
  if ( media != "" ) {
    ILOG_ERROR << "Invalid submedia: [" << media << "]";
    return false;
  }

  // try to insert the request into a memory buffer
  int64 delay_ms = req->info().seek_pos_ms_;
  CHECK_GE(delay_ms, 0);
  for ( list<BufferedSaver*>::reverse_iterator it = buffers_.rbegin();
        it != buffers_.rend(); ++it ) {
    BufferedSaver* buf = *it;
    if ( buf->SizeMs() > delay_ms ) {
      if ( !buf->AddRequest(req, callback, delay_ms, false, true) ) {
        ILOG_ERROR << "Failed to AddRequest() on buffer, delay_ms: " << delay_ms;
        return false;
      }
      buff_requests_[req] = buf;
      return true;
    }
    delay_ms -= buf->SizeMs();
  }
  // reset the delay_ms (it was modified when trying to add on buffer)
  delay_ms = req->info().seek_pos_ms_;

  // the delay must be longer than the media buffer, so add the request on disk
  string trr_media = strutil::JoinPaths(time_range_element_,
      strutil::StringPrintf("%"PRId64, (timer::Date::Now() - delay_ms)/1000));
  // remove seek_pos_ms_ because the timerange reacts to it
  req->mutable_info()->seek_pos_ms_ = 0;
  // forward the request to the associated timerange element
  // (RemoveRequest() will happen on the timerange element, not on us)
  if ( !mapper_->AddRequest(trr_media, req, callback) ) {
    ILOG_ERROR << "Failed to AddRequest() on disk, on path: " << trr_media;
    return false;
  }
  return true;
}
void TimeShiftElement::RemoveRequest(Request* req) {
  // NOTE: disk request are removed from the timerange element directly

  // remove request from buffer
  BuffReqMap::iterator bit = buff_requests_.find(req);
  if ( bit == buff_requests_.end() ) {
    LOG_FATAL << "RemoveRequest() cannot find req: " << req->ToString();
  }
  BufferedSaver* buf = bit->second;
  buff_requests_.erase(bit);
  buf->RemoveRequest(req);
  if ( buf->RequestCount() == 0 && buf->is_closed() ) {
    list<BufferedSaver*>::iterator buf_it =
        std::find(buffers_.begin(), buffers_.end(), buf);
    CHECK(buf_it != buffers_.end()) << " cannot find buffer: " << buf;
    buffers_.erase(buf_it);
    delete buf;
  }
  if ( close_completed_ != NULL && buff_requests_.empty() ) {
    // no more buffer requests
    selector_->RunInSelectLoop(close_completed_);
    close_completed_ = NULL;
  }
}
bool TimeShiftElement::HasMedia(const string& media) {
  return media == "";
}
void TimeShiftElement::ListMedia(const string& media_dir, vector<string>* out) {
  if ( media_dir != "" ) {
    return;
  }
  out->push_back("");
}
bool TimeShiftElement::DescribeMedia(const string& media,
    MediaInfoCallback* callback) {
  if ( media != "" ) {
    ILOG_ERROR << "Invalid submedia: [" << media << "]";
    return false;
  }
  return mapper_->DescribeMedia(media_path_, callback);
}
void TimeShiftElement::Close(Closure* call_on_close) {
  Unregister();
  for ( list<BufferedSaver*>::iterator it = buffers_.begin();
        it != buffers_.end(); ) {
    BufferedSaver* buf = *it;
    buf->Close();
    if ( buf->RequestCount() == 0 ) {
      delete buf;
      it = buffers_.erase(it);
      continue;
    }
    ++it;
  }
  if ( buff_requests_.empty() ) {
    // no more buffer requests
    CHECK(buffers_.empty()) << " all buffers should be closed and deleted";
    selector_->RunInSelectLoop(call_on_close);
    return;
  }
  close_completed_ = call_on_close;
}

int64 TimeShiftElement::BufferedMs() const {
  int64 ms = 0;
  for ( list<BufferedSaver*>::const_iterator it = buffers_.begin();
        it != buffers_.end(); ++it ) {
    ms += (*it)->SizeMs();
  }
  return ms;
}

void TimeShiftElement::ProcessTag(const Tag* tag, int64 timestamp) {
  calculator_.ProcessTag(tag, timestamp);

  if ( tag->type() == Tag::TYPE_EOS ) {
    Unregister();
    return;
  }

  // TODO(cosmin): add support for signaling tags, like SourceStartedTag, ...
  if ( tag->type() != Tag::TYPE_FLV &&
       tag->type() != Tag::TYPE_MEDIA_INFO ) {
    return;
  }

  // if the crt buffer has grown too large, close it and start a new one
  if ( buffers_.back()->SizeMs() > buffer_size_ms_ ||
       tag->type() == Tag::TYPE_MEDIA_INFO ) {
    // finalize current buffer
    int64 now = timer::Date::Now();
    buffers_.back()->set_active(false);
    buffers_.back()->AsyncSaveToFile(save_dir_, now);

    // allocate a new crt buffer
    BufferedSaver* buf = new BufferedSaver(selector_, play_finished_callback_,
        name(), now, info());
    buf->set_media_info(buffers_.back()->media_info());
    buffers_.push_back(buf);

    // compute the time length of all buffers
    int64 len_ms = BufferedMs();

    // check if we should remove any of the old buffers
    if ( len_ms - buffers_.front()->SizeMs() > 2*buffer_size_ms_ ) {
      buffers_.front()->Close(); // sends EOS to any lazy client
      if ( buffers_.front()->RequestCount() == 0 ) {
        delete buffers_.front();
        buffers_.pop_front();
      }
    }
    // resuscitate the previous crt_buffer_. The request that are waiting on
    // the buffer end for new tags should move out, to the new crt buffer.
    (*(++buffers_.rbegin()))->NotifyAllRequests();
  }

  // append tag to current buffer
  // NOTE: It's important to first create/switch the new buffer,
  //       then send the tag (this tag may be a new MediaInfo).
  buffers_.back()->ProcessTag(tag, calculator_.stream_time_ms());
}

void TimeShiftElement::PlayFinished(BufferedSaver* buf,
    Request* req, ProcessingCallback* c) {
  // add 'req' to the next buffer, after "buf"
  list<BufferedSaver*>::iterator it = buffers_.begin();
  while ( it != buffers_.end() && (*it) != buf ) { ++it; }
  CHECK(it != buffers_.end()) << " buf not found";
  ++it;
  CHECK(it != buffers_.end()) << " Req finished playing on crt buffer";
  (*it)->AddRequest(req, c, 0, true, false);
  // update the mapping: req -> buf
  buff_requests_[req] = (*it);
}

bool TimeShiftElement::Register() {
  CHECK_NULL(internal_req_);
  internal_req_ = new Request();
  if ( !mapper_->AddRequest(media_path_, internal_req_, process_tag_callback_) ) {
    ILOG_ERROR << "AddRequest failed for media_path: [" << media_path_ << "]";
    delete internal_req_;
    internal_req_ = NULL;
    return false;
  }
  return true;
}
void TimeShiftElement::Unregister() {
  if ( internal_req_ != NULL ) {
    mapper_->RemoveRequest(internal_req_, process_tag_callback_);
    internal_req_ = NULL;
  }
}

}
