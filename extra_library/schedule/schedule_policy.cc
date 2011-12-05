// Copyright (c) 2009, Whispersoft s.r.l.
// All rights reserved.
//
// Author: Cosmin Tudorache & Catalin Popescu

#include <whisperlib/net/rpc/lib/codec/json/rpc_json_encoder.h>
#include <whisperlib/net/rpc/lib/codec/json/rpc_json_decoder.h>
#include "extra_library/schedule/schedule_policy.h"

namespace streaming {

const char SchedulePlaylistPolicy::kPolicyClassName[] =
    "schedule_playlist_policy";
const int64 SchedulePlaylistPolicy::kInvalidItemId = -1;
const int32 SchedulePlaylistPolicy::kMinScheduleError = 100;
const int32 SchedulePlaylistPolicy::kMinEosInterval = 500;
const int32 SchedulePlaylistPolicy::kMinEosRetry = 2000;
const int64 SchedulePlaylistPolicy::kMaxEventDurationSec = 365 * 86000LL;
const int64 SchedulePlaylistPolicy::kUndefinedEventDurationSec = 20; // use a
// small value, within our wake time precision. If the value is too small,
// on wake we may find that the item+duration already expired (i.e. never play)
// If the value is larger than 1 week, and the event repeats (say every sunday
// at 10:00), on SetPlaylist: it will immediately switch to the event
// (because it's in progress).

//////////////////////////////////////////////////////////////////////

SchedulePlaylistPolicy::SchedulePlaylistPolicy(
    const char* name,
    PolicyDrivenElement* element,
    net::Selector* selector,
    bool is_temp_policy,
    io::StateKeepUser* global_state_keeper,
    io::StateKeepUser* local_state_keeper,
    rpc::HttpServer* rpc_http_server,
    const string& rpc_path,
    const string& local_rpc_path,
    const string& default_media,
    const vector<const SchedulePlaylistItemSpec*>& playlist,
    int32 min_switch_delta_ms)
    : Policy(kPolicyClassName, name, element),
      ServiceInvokerSchedulePlaylistPolicyService(
          ServiceInvokerSchedulePlaylistPolicyService::GetClassName()),
      selector_(selector),
      is_temp_policy_(is_temp_policy),
      global_state_keeper_(global_state_keeper),
      local_state_keeper_(local_state_keeper),
      rpc_http_server_(rpc_http_server),
      rpc_path_(rpc_path),
      local_rpc_path_(local_rpc_path),
      default_media_(default_media),
      min_switch_delta_ms_(min_switch_delta_ms),
      current_id_(kInvalidItemId),
      current_begin_ts_(0),
      next_callback_(
          NewPermanentCallback(this,
                               &SchedulePlaylistPolicy::GoToNext, false)),
      is_registered_(false) {
  for ( int i = 0; i < playlist.size(); ++i ) {
    const int64 crt_id = playlist[i]->id_.get();
    if ( specs_.find(crt_id) != specs_.end() ) {
      LOG_WARNING << name_ << ": Duplicate spec for id: " << crt_id
                  << " ignoring: " << playlist[i]->ToString();
    } else {
      specs_.insert(make_pair(crt_id,
                              new SchedulePlaylistItemSpec(*playlist[i])));
    }
  }
}
SchedulePlaylistPolicy::~SchedulePlaylistPolicy() {
  if ( is_temp_policy_ ) {
    ClearState();
  }
  delete global_state_keeper_;
  delete local_state_keeper_;
  selector_->UnregisterAlarm(next_callback_);
  delete next_callback_;
  if ( is_registered_ ) {
    rpc_http_server_->UnregisterService(local_rpc_path_, this);
  }
  ClearPlaylist();
}

bool SchedulePlaylistPolicy::Initialize() {
  if ( rpc_http_server_ != NULL ) {
    if ( !rpc_http_server_->RegisterService(local_rpc_path_, this) ) {
      LOG_ERROR << "Failed to register rpc SchedulePlaylistPolicy service"
                   " on path: " << local_rpc_path_;
      return false;
    }
    LOG_INFO << " ========================>> Scheduler registered on path: ["
             << local_rpc_path_ << "]";
    if ( local_rpc_path_ != rpc_path_ ) {
      rpc_http_server_->RegisterServiceMirror(rpc_path_, local_rpc_path_);
    }
    is_registered_ = true;
  } else {
    LOG_ERROR << "No rpc_http_server_ provided, SchedulePlaylistPolicy"
                 " won't be available on RPC";
  }
  LoadState();
  return true;
}

bool SchedulePlaylistPolicy::NotifyEos() {
  // Update the duration of the current playing item, if unknown.
  // If this unknown duration is not updated, then it will re-start
  // playing the same media forever.
  if ( current_id_ != kInvalidItemId ) {
    SpecMap::iterator it = specs_.find(current_id_);
    if ( it != specs_.end() ) {
      SchedulePlaylistItemSpec* spec = it->second;
      if ( !spec->start_time_.get().duration_in_seconds_.is_set() ||
           spec->start_time_.get().duration_in_seconds_.get() <= 0 ) {
        int64 now = timer::Date::Now();
        CHECK(now > current_begin_ts_) << " Current item in the future?"
            " current_begin_ts_: " << current_begin_ts_ << ", now: " << now;
        spec->start_time_.ref().duration_in_seconds_ =
            (now - current_begin_ts_) / 1000;
        SaveState();
      }
    }
  }

  // We impose a minimum switch delay to prevent crappy media to loop us
  // forever..
  if ( timer::Date::Now() - current_begin_ts_ < kMinEosInterval ) {
    selector_->RegisterAlarm(next_callback_, kMinEosRetry);
  } else {
    GoToNext(true);
  }
  return true;
}

void SchedulePlaylistPolicy::GetPlaylist(
    rpc::CallContext< SchedulePlaylistPolicySpec >* call) {
  SchedulePlaylistPolicySpec ret;
  GetPlaylist(&ret);
  call->Complete(ret);
}

void SchedulePlaylistPolicy::GetPlaylist(
    SchedulePlaylistPolicySpec* playlist) const {
  playlist->default_media_.set(default_media_);
  playlist->playlist_.ref();
  for ( SpecMap::const_iterator it = specs_.begin();
        it != specs_.end(); ++it ) {
    playlist->playlist_.ref().push_back(*it->second);
  }
}

void SchedulePlaylistPolicy::SetPlaylist(
    rpc::CallContext< MediaOperationErrorData >* call,
    const SetSchedulePlaylistPolicySpec& playlist) {
  if ( !SetPlaylist(playlist) ) {
    call->Complete(MediaOperationErrorData(1, "Failed to SetPlaylist"));
    return;
  }
  if ( !SaveState() ) {
    call->Complete(MediaOperationErrorData(1, "Error saving config.."));
    return;
  }
  call->Complete(MediaOperationErrorData(0, ""));
}

bool SchedulePlaylistPolicy::SetPlaylist(
    const SetSchedulePlaylistPolicySpec& playlist) {
  bool changed = false;
  if ( playlist.default_media_.is_set() ) {
    default_media_ = playlist.default_media_.get();
    if ( current_id_ == kInvalidItemId ) {
      current_begin_ts_ = timer::Date::Now();
      element_->SwitchCurrentMedia(default_media_,
                                   NULL,
                                   playlist.switch_now_.is_set() &&
                                   playlist.switch_now_.get());
      SaveState();
    }
    changed = true;
  }
  if ( playlist.playlist_.is_set() ) {
    ClearPlaylist();
    for ( int i = 0; i < playlist.playlist_.size(); ++i ) {
      const int64 crt_id = playlist.playlist_[i].id_;
      if ( specs_.find(crt_id) != specs_.end() ) {
        LOG_WARNING << name() << ": Duplicate spec for id: " << crt_id;
      } else {
        specs_.insert(make_pair(crt_id,
                                new SchedulePlaylistItemSpec(
                                    playlist.playlist_[i])));
      }
    }
    GoToNext(false);
    changed = true;
  }
  return changed;
}

void SchedulePlaylistPolicy::GetDefaultMedia(
    rpc::CallContext<string>* call) {
  call->Complete(default_media_);
}

void SchedulePlaylistPolicy::SetDefaultMedia(
    rpc::CallContext< rpc::Void >* call,
    const string& default_media,
    bool switch_now) {
  if ( default_media_ != default_media ) {
    default_media_ = default_media;
    if ( current_id_ == kInvalidItemId ) {
      current_begin_ts_ = timer::Date::Now();
      element_->SwitchCurrentMedia(default_media_, NULL, switch_now);
      SaveState();
    }
  }
  call->Complete();   // TODO cpopescu: we can fail here
}

void SchedulePlaylistPolicy::GetProgram(
    rpc::CallContext<ScheduleProgram>* call,
    const string& date,
    int32 num_schedules) {
  ScheduleProgram ret;
  timer::Date query_date;
  if ( !query_date.SetFromShortString(date, false) ) {
    ret.error_ = 1;
    ret.description_.ref() = "Invalid date format";
    call->Complete(ret);
    return;
  }

  const int num = num_schedules;
  if ( num < 0 || num > 50 ) {
    ret.error_ = 1;
    ret.description_.ref() = "Bad number of schedules.";
    call->Complete(ret);
    return;
  }

  ret.error_ = 0;
  vector< ScheduleItem > schedule;
  GetScheduleAtTime(num, &schedule, query_date);
  for ( int i = 0; i < schedule.size(); ++i ) {
    ScheduleProgramElement crt(
        schedule[i].id_, default_media_, schedule[i].switch_ts_,
        timer::Date(schedule[i].switch_ts_).ToString());
    const SpecMap::const_iterator it = specs_.find(schedule[i].id_);
    if ( it != specs_.end() ) {
      crt.media_.ref() = it->second->media_;
    }
    ret.program_.ref().push_back(crt);
  }
  call->Complete(ret);
}

void SchedulePlaylistPolicy::GetNextPlay(
    rpc::CallContext<SchedulePlayInfo>* call) {
  SchedulePlayInfo ret;
  timer::Date crt_time(timer::Date::Now());

  vector< ScheduleItem > schedule;
  GetScheduleAtTime(2, &schedule, crt_time.GetTime());
  SetPlayInfo(schedule[0].id_, schedule[1].switch_ts_, &ret);
  call->Complete(ret);
}

void SchedulePlaylistPolicy::GetCurrentPlay(
    rpc::CallContext<SchedulePlayInfo>* call) {
  SchedulePlayInfo ret;
  SetPlayInfo(current_id_, current_begin_ts_, &ret);
  call->Complete(ret);
}

void SchedulePlaylistPolicy::LoadState() {
  bool changed = false;
  if ( global_state_keeper_ != NULL ) {
    // First get the values out of the state.
    string schedule;
    bool has_schedule = global_state_keeper_->GetValue("schedule", &schedule);
    string str_current_id;
    bool has_current_id =
        global_state_keeper_->GetValue("current_id", &str_current_id);

    // Then change internal state. These changes will most likely trigger
    // a SaveState() which will overwrite the state! That's why we took all
    // values out in the first place.
    if ( has_schedule ) {
      SetSchedulePlaylistPolicySpec playlist;
      if ( rpc::JsonDecoder::DecodeObject(schedule, &playlist) ) {
        SetPlaylist(playlist);
        LOG_INFO << name() << " Loaded playlist from state.";
        LOG_DEBUG << name() <<  " DETAIL for loaded playlist: "
                  << playlist.ToString();
      }
    }
    if ( has_current_id ) {
      current_id_ = ::atoll(str_current_id.c_str());
      const SpecMap::const_iterator it = specs_.find(current_id_);
      if ( it != specs_.end() ) {
        const string& media_name = it->second->media_;
        LOG_INFO << "LoadState: switching to media: [" << media_name << "]";
        element_->SwitchCurrentMedia(media_name, NULL, true);
        SaveState();
        changed = true;
      } else {
        LOG_ERROR << "LoadState: current_id: " << current_id_
                  << " not in specs";
      }
    } else {
      LOG_ERROR << "LoadState: cannot find value 'current_id'";
    }
  }
  if ( !changed ) {
    GoToNext(false);
  }
}
bool SchedulePlaylistPolicy::SaveState() {
  // VERY IMPORTANT  !! we save in local state the playlist. In this way
  // the temps do not mess the global..
  if ( is_temp_policy_ ) {
    return true;
  }
  if ( global_state_keeper_ != NULL ) {
    SchedulePlaylistPolicySpec playlist;
    string schedule;
    GetPlaylist(&playlist);
    rpc::JsonEncoder::EncodeToString(playlist, &schedule);
    global_state_keeper_->SetValue("schedule", schedule);
    global_state_keeper_->SetValue("current_id",
        strutil::StringPrintf("%d", current_id_).c_str());
  }
  return true;
}
void SchedulePlaylistPolicy::ClearState() {
  if ( global_state_keeper_ != NULL ) {
    global_state_keeper_->DeleteValue("schedule");
    global_state_keeper_->DeleteValue("current_id");
  }
}
void SchedulePlaylistPolicy::GetCurrentItemAtTime(const timer::Date& now,
                                                  int64* out_item_id,
                                                  int64* out_begin_ts,
                                                  int64* out_switch_ts,
                                                  int64* out_end_ts) const {
  const int64 now_ts = now.GetTime();
  // by default, choose default media, with start time: 1 year behind now.
  int64 best_id = kInvalidItemId;
  int64 best_begin = now_ts  - kMaxEventDurationSec * 1000LL;
  int64 best_switch = best_begin;
  int64 best_end = now_ts  + kMaxEventDurationSec * 1000LL;
  for ( SpecMap::const_iterator it = specs_.begin();
        it != specs_.end(); ++it ) {
    const SchedulePlaylistItemSpec& item_spec = *it->second;
    const int64 item_id = item_spec.id_;
    const TimeSpec& item_time = item_spec.start_time_.get();
    const int64 item_duration = 1000LL *
      ((item_time.duration_in_seconds_.is_set() &&
        item_time.duration_in_seconds_.get() > 0)
       ? item_time.duration_in_seconds_.get()
       : kUndefinedEventDurationSec);

    const int64 past_delay = PrevHappening(item_time, now);
    CHECK_GE(past_delay, 0); // PrevHappening returns old or in progress ev
    const int64 past_begin = now_ts - past_delay;
    const int64 past_end = past_begin + item_duration;
    // LOG_WARNING << "GetPrevItemAtTime looking at item id: " << item_id
    //        << ", item_time: " << item_time
    //        << ", found PrevHappening => begin: " << timer::Date(past_begin)
    //        << ", end: " << timer::Date(past_end);
    CHECK_LE(past_begin, now_ts);
    if ( //past_begin <= now_ts &&   // past begin // CHECKED above
        past_end >= now_ts &&       // future end - i.e. current playing item
        past_begin > best_begin ) { // better than 'best' current
      best_id = item_id;
      best_begin = past_begin;
      if ( best_switch < best_begin ) {
        best_switch = best_begin;    // switch is always >= begin
      }
      best_end = past_end;
    }
    if ( //past_begin <= now_ts &&   // past begin // CHECKED above
        past_end <= now_ts &&       // past end
        past_begin > best_begin &&  // overlaps 'best'
        past_end > best_switch ) {  // better than 'best' switch
      best_switch = past_end;
    }
  }
  LOG_DEBUG << "GetCurrentItemAtTime(" << now << ") \n"
               "=> id: " << best_id << "\n"
            << ", begin: " << timer::Date(best_begin) << "\n"
            << ", switch: " << timer::Date(best_switch) << "\n"
            << ", end: " << timer::Date(best_end);
  *out_item_id = best_id;
  *out_begin_ts = best_begin;
  *out_switch_ts = best_switch;
  *out_end_ts = best_end;
}

void SchedulePlaylistPolicy::GetFutureItemAtTime(const timer::Date& now,
                                                 int64* out_item_id,
                                                 int64* out_begin_ts) const {
  const int64 now_ts = now.GetTime();
  // by default, choose default media, with start time: 1 year after now.
  int64 best_id = kInvalidItemId;
  int64 best_begin = now_ts + kMaxEventDurationSec * 1000LL;
  for ( SpecMap::const_iterator it = specs_.begin();
        it != specs_.end(); ++it ) {
    const SchedulePlaylistItemSpec& item_spec = *it->second;
    const int64 item_id = item_spec.id_;
    const TimeSpec& item_time = item_spec.start_time_.get();
    const int64 item_duration = 1000LL *
                                ((item_time.duration_in_seconds_.is_set() &&
                                  item_time.duration_in_seconds_.get() > 0)
                                 ? item_time.duration_in_seconds_.get()
                                 : kUndefinedEventDurationSec);

    const int64 next_delay = NextFutureHappening(item_time, now);
    CHECK_GT(next_delay, 0); // NextFutureHappening returns strictly future ev
    const int64 next_begin = now_ts + next_delay;
    const int64 next_end = next_begin + item_duration;
    (void)next_end;
    //  LOG_WARNING << "GetFutureItemAtTime looking at item id: " << item_id
    //        << ", item_time: " << item_time
    //        << ", found NextHappening => begin: " << timer::Date(next_begin)
    //        << ", end: " << timer::Date(next_end);

    CHECK_GT(next_begin, now_ts);
    if ( //next_begin > now_ts // CHECKED above
        next_begin < best_begin ) {
      // current item begin is better than 'best'
      best_id = item_id;
      best_begin = next_begin;
    }

  }
  LOG_DEBUG << "GetFutureItemAtTime(" << now << ") \n"
               "=> id: " << best_id << "\n"
            << ", begin: " << timer::Date(best_begin);
  *out_item_id = best_id;
  *out_begin_ts = best_begin;
}
void SchedulePlaylistPolicy::GetItemAtTime(const timer::Date& now,
                                           int64* out_item_id,
                                           int64* out_begin_ts,
                                           int64* out_switch_ts,
                                           int64* out_end_ts) const {
  int64 item_id = 0;
  int64 item_begin = 0;
  int64 item_switch = 0;
  int64 item_end = 0;
  GetCurrentItemAtTime(now, &item_id, &item_begin, &item_switch, &item_end);
  CHECK_LE(item_begin, now.GetTime());

  int64 future_item_id = 0;
  int64 future_item_begin = 0;
  GetFutureItemAtTime(now, &future_item_id, &future_item_begin);
  CHECK_GT(future_item_begin, now.GetTime());

  if ( future_item_begin < item_end ) {
    item_end = future_item_begin;
  }

  LOG_DEBUG << "GetItemAtTime(" << now << ") \n"
               "=> id: " << item_id << "\n"
            << ", begin on: " << timer::Date(item_begin) << "\n"
            << ", switch on: " << timer::Date(item_switch) << "\n"
            << ", end on: " << timer::Date(item_end);

  *out_item_id = item_id;
  *out_begin_ts = item_begin;
  *out_switch_ts = item_switch;
  *out_end_ts = item_end;
}

void SchedulePlaylistPolicy::GetScheduleAtTime(
    int num_to_schedule,
    vector< ScheduleItem >* schedule,
    const timer::Date& now) const {
  const int64 now_ts = now.GetTime();
  int64 ts = now_ts;
  for ( int i = 0; i < num_to_schedule; i++ ) {
    int64 item_id = 0;
    int64 begin_ts = 0;
    int64 switch_ts = 0;
    int64 end_ts = 0;
    GetItemAtTime(ts, &item_id, &begin_ts, &switch_ts, &end_ts);
    if ( i > 0 &&
         item_id == -1 &&
         ts - begin_ts == kMaxEventDurationSec * 1000 ) {
      begin_ts = ts - 100;
      switch_ts = ts - 100;
    }
    schedule->push_back(ScheduleItem(item_id, begin_ts, switch_ts, end_ts));
    ts = end_ts + 100;
  }
}

void SchedulePlaylistPolicy::GoToNext(bool is_eos) {
  const timer::Date crt_time;
  vector< ScheduleItem > schedule;
  // We need at least two ..
  // The +100 reason: the time is very close to the moment next item follows.
  //                  If we find schedule exactly on item begin,
  //                  we get 0 seconds delay => Reregister alarm.
  GetScheduleAtTime(2, &schedule, crt_time.GetTime() + 100);
  CHECK_EQ(schedule.size(), 2);
  // See when we need to schedule next stuff
  // (we may allow some miliseconds +/i )
  if ( schedule[0].switch_ts_ > crt_time.GetTime() + kMinScheduleError ) {
    LOG_INFO << "GoToNext: next items coming up in "
             << (schedule[0].switch_ts_ - crt_time.GetTime())/1000
             << " seconds";
    selector_->RegisterAlarm(next_callback_,
                               schedule[0].switch_ts_ - crt_time.GetTime());
    return;
  }

  LOG_INFO << "GoToNext: current_id_=" << current_id_ << "\n"
           << ", crt_time=" << crt_time << "\n"
           << ", schedule[0]=" << schedule[0].ToString() << "\n"
      ", schedule[1]=" << schedule[1].ToString();

  // We have to play something at this time ..
  if ( ( current_id_ != schedule[0].id_ &&
         schedule[0].id_ != kInvalidItemId ) || is_eos ) {
    current_begin_ts_ = crt_time.GetTime();
    if ( schedule[0].id_ == kInvalidItemId ) {
      current_id_ = kInvalidItemId;
      if ( !default_media_.empty() ) {
        LOG_INFO << "GoToNext: switching to"
            " default_media_: [" << default_media_ << "]";
        element_->SwitchCurrentMedia(default_media_, NULL, false);
        SaveState();
      } else {
        LOG_ERROR << "GoToNext: cannot switch to"
                    " default_media_: [" << default_media_ << "]";
      }
    } else {
      current_id_ = schedule[0].id_;
      const SpecMap::const_iterator it = specs_.find(current_id_);
      CHECK(it != specs_.end());
      const string& media_name = it->second->media_;
      LOG_INFO << "GoToNext: switching to media: [" << media_name << "]";
      element_->SwitchCurrentMedia(media_name, NULL, true);
      SaveState();
    }
  }

  if ( specs_.empty() ) {
    LOG_WARNING << "GoToNext: nothing is next";
    selector_->UnregisterAlarm(next_callback_);
    return;
  }

  const int64 delay = schedule[1].switch_ts_ - crt_time.GetTime();
  LOG_INFO << "GoToNext: next switch in " << delay/1000 << " seconds";
  CHECK(delay > 0);
  selector_->RegisterAlarm(next_callback_, delay);
}

}
