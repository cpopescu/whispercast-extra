// (c) 2009, Whispersoft s.r.l.
// All rights reserved.
//
// Author: Catalin Popescu
//

#include <whisperlib/common/io/ioutil.h>
#include <whisperstreamlib/base/saver.h>
#include "extra_library/time_delay/time_delay_policy.h"

namespace streaming {
const char TimeDelayPolicy::kPolicyClassName[] = "time_delay_policy";
static const int64 kDefaultWaitOnError = 30000;

TimeDelayPolicy::TimeDelayPolicy(const char* name,
                                 PolicyDrivenElement* element,
                                 ElementMapper* mapper,
                                 net::Selector* selector,
                                 bool is_temp_policy,
                                 io::StateKeepUser* global_state_keeper,
                                 io::StateKeepUser* local_state_keeper,
                                 const char* rpc_path,
                                 const char* local_rpc_path,
                                 rpc::HttpServer* rpc_server,
                                 const char* home_dir,
                                 const char* root_media_path,
                                 const char* file_prefix,
                                 int time_delay_sec)
    : Policy(kPolicyClassName, name, element),
      ServiceInvokerTimeDelayPolicyService(
          ServiceInvokerTimeDelayPolicyService::GetClassName()),
      mapper_(mapper),
      selector_(selector),
      is_temp_policy_(is_temp_policy),
      global_state_keeper_(global_state_keeper),
      local_state_keeper_(local_state_keeper),
      rpc_path_(rpc_path),
      local_rpc_path_(local_rpc_path),
      rpc_server_(rpc_server),
      home_dir_(home_dir),
      root_media_path_(root_media_path),
      file_prefix_(file_prefix),
      time_delay_ms_(time_delay_sec * 1000),
      is_registered_(false),
      begin_file_timestamp_(0),
      playref_time_(0),
      play_next_callback_(NewPermanentCallback(this,
                                               &TimeDelayPolicy::PlayNext)) {
}

TimeDelayPolicy::~TimeDelayPolicy() {
  selector_->UnregisterAlarm(play_next_callback_);
  if ( is_registered_ ) {
    rpc_server_->UnregisterService(local_rpc_path_, this);
  }
  if ( is_temp_policy_ ) {
    ClearState();
  }
  delete global_state_keeper_;
  delete local_state_keeper_;
}

//////////////////////////////////////////////////////////////////////

bool TimeDelayPolicy::LoadState() {
  if ( global_state_keeper_ == NULL ) {
    LOG_WARNING << " No state keeper to Load State !!";
    return true;
  }
  string value;
  if ( !global_state_keeper_->GetValue("num_skips", &value) )  {
    return false;
  }
  int32 num_skips = ::strtol(value.c_str(), NULL, 10);
  for ( int i = 0; i < num_skips; ++i ) {
    string key = strutil::StringPrintf("ts-%d", i);
    if ( !global_state_keeper_->GetValue(key, &value) ) {
      LOG_ERROR << "Cannot find key: [" << key << "]";
      continue;
    }
    int64 timestamp = ::strtoll(value.c_str(), NULL, 0);
    key = strutil::StringPrintf("d-%d", i);
    if ( !global_state_keeper_->GetValue(key, &value) ) {
      LOG_ERROR << "Cannot find key: [" << key << "]";
      continue;
    }
    int32 duration = ::strtol(value.c_str(), NULL, 0);
    skip_map_.insert(make_pair(timestamp, duration));
  }
  return true;
}

bool TimeDelayPolicy::SaveState() {
  if ( is_temp_policy_ )  {
    return true;
  }
  if ( global_state_keeper_ == NULL ) {
    LOG_ERROR << "No state keeper to Load State !!";
    return true;
  }
  global_state_keeper_->BeginTransaction();
  int i = 0;
  global_state_keeper_->SetValue("num_skips",
      strutil::StringPrintf("%d", skip_map_.size()));
  for ( SkipMap::const_iterator it = skip_map_.begin();
        it != skip_map_.end(); ++it ) {
    global_state_keeper_->SetValue(strutil::StringPrintf("ts-%d", i),
        strutil::StringPrintf("%"PRId64, it->first));
    global_state_keeper_->SetValue(strutil::StringPrintf("d-%d", i),
        strutil::StringPrintf("%d", it->second));
    ++i;
  }
  global_state_keeper_->CommitTransaction();
  return true;
}
void TimeDelayPolicy::ClearState() {
  if ( global_state_keeper_ != NULL ) {
    global_state_keeper_->DeleteAllValues();
  }
}

//////////////////////////////////////////////////////////////////////

bool TimeDelayPolicy::Initialize() {
  if ( rpc_server_ != NULL ) {
    is_registered_ = rpc_server_->RegisterService(local_rpc_path_, this);
    if ( is_registered_ && local_rpc_path_ != rpc_path_ ) {
      rpc_server_->RegisterServiceMirror(rpc_path_, local_rpc_path_);
    }
  }
  LoadState();
  PlayNext();
  return (is_registered_ || rpc_server_ == NULL);
}

bool TimeDelayPolicy::NotifyEos() {
  crt_media_.clear();
  PlayNext();
  return true;
}

bool TimeDelayPolicy::NotifyTag(const Tag* tag, int64 timestamp_ms) {
  // TODO [cpopescu]: skip
  return true;
}

void TimeDelayPolicy::PlayNext() {
  if ( !crt_media_.empty() ) {
    RequestInfo* req_info = NULL;
    if ( begin_file_timestamp_ > 0 ) {
      req_info = new RequestInfo();
      req_info->seek_pos_ms_ = begin_file_timestamp_;
    }
    LOG_INFO << name() << " Switching to: " << crt_media_
              << " @" << begin_file_timestamp_;
    Capabilities caps;
    if ( !mapper_->HasMedia(crt_media_.c_str(), &caps) ) {
      selector_->RegisterAlarm(play_next_callback_, kDefaultWaitOnError);
      LOG_ERROR << "Unknown media: " << crt_media_;
      delete req_info;
      return;
    }
    if ( !element_->SwitchCurrentMedia(crt_media_, req_info, true) ) {
      selector_->RegisterAlarm(play_next_callback_, kDefaultWaitOnError);
      LOG_ERROR << "Cannot start playing: " << crt_media_;
      crt_media_.clear();
    } else {
      selector_->UnregisterAlarm(play_next_callback_);
      LOG_DEBUG << " Started time delay play of: "
                << crt_media_ << " @" << begin_file_timestamp_;
    }
    delete req_info;
    return;
  }
  timer::Date play_time(timer::Date::Now() - time_delay_ms_, true);   // UTC always
  int64 duration = 0;
  vector<string> files;
  int last_choice = -1;
  const int64 delay = streaming::GetNextStreamMediaFile(
      play_time,
      home_dir_, file_prefix_, root_media_path_,
      &files,
      &last_choice,
      &crt_media_,
      &begin_file_timestamp_,
      &playref_time_,
      &duration);
  if ( delay == 0 ) {
    CHECK(!crt_media_.empty());
    PlayNext();
  } else {
    selector_->RegisterAlarm(play_next_callback_, delay);
  }
}

}
