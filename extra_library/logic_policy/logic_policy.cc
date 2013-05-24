// Copyright 2009 WhisperSoft s.r.l.
// Author: Catalin Popescu


#include "logic_policy.h"

namespace streaming {
const char LogicPlaylistPolicy::kPolicyClassName[] =
    "logic_playlist_policy";

LogicPlaylistPolicy::LogicPlaylistPolicy(const string& name,
                                         PolicyDrivenElement* element,
                                         ElementMapper* mapper,
                                         net::Selector* selector,
                                         bool is_temp_policy,
                                         io::StateKeepUser* global_state_keeper,
                                         io::StateKeepUser* local_state_keeper,
                                         rpc::HttpServer* rpc_http_server,
                                         const string& rpc_path,
                                         const string& local_rpc_path,
                                         const LogicPlaylistPolicySpec& spec)
    : Policy(kPolicyClassName, name, element),
      ServiceInvokerLogicPlaylistPolicyService(
          ServiceInvokerLogicPlaylistPolicyService::GetClassName()),
      mapper_(mapper),
      selector_(selector),
      is_temp_policy_(is_temp_policy),
      global_state_keeper_(global_state_keeper),
      local_state_keeper_(local_state_keeper),
      rpc_http_server_(rpc_http_server),
      rpc_path_(rpc_path),
      local_rpc_path_(local_rpc_path),
      is_registered_(false),
      crt_state_(NULL),
      crt_action_map_(kDefaultActionMapSize),
      timeout_closure_(NewPermanentCallback(
                           this, &LogicPlaylistPolicy::NotifyTimeout)) {
  SetPlaylist(spec.playlist_.get());
  for ( int i = 0; i < spec.extra_media_.get().size(); ++i ) {
    extra_media_.push_back(spec.extra_media_.get()[i]);
  }
}


LogicPlaylistPolicy::~LogicPlaylistPolicy() {
  if ( is_registered_ ) {
    rpc_http_server_->UnregisterService(local_rpc_path_, this);
  }
  for ( int i = 0; i < extra_reqs_.size(); ++i ) {
    if ( extra_reqs_[i] != NULL ) {
      streaming::Request* const req = extra_reqs_[i];
      streaming::ProcessingCallback* const cb = extra_callbacks_[i];

      extra_reqs_[i] = NULL;
      extra_callbacks_[i] = NULL;

      mapper_->RemoveRequest(req, cb);
      delete cb;
    }
  }
  Clear();
  delete timeout_closure_;
  if ( is_temp_policy_ ) {
    ClearState();
  }
  delete global_state_keeper_;
  delete local_state_keeper_;
}

//////////////////////////////////////////////////////////////////////

//static
int LogicPlaylistPolicy::ActionID(Tag::Type tag_type) {
  return (int)tag_type;
}
//static
bool LogicPlaylistPolicy::IsActionTag(Tag::Type tag_type) {
  return tag_type == Tag::TYPE_FEATURE_FOUND ||
         tag_type == Tag::TYPE_OSD ||
         tag_type == Tag::TYPE_SEEK_PERFORMED ||
         tag_type == Tag::TYPE_SOURCE_STARTED ||
         tag_type == Tag::TYPE_SOURCE_ENDED ||
         tag_type == Tag::TYPE_CUE_POINT;
}

void LogicPlaylistPolicy::Clear() {
  for ( StateMap::const_iterator it = state_.begin();
        it != state_.end(); ++it ) {
    delete it->second;
  }
  default_state_name_.clear();
  state_.clear();
  crt_state_ = NULL;
  for ( ActionMap::const_iterator it = crt_action_map_.begin();
        it != crt_action_map_.end(); ++it ) {
    delete it->second;
  }
  crt_action_map_.clear();
  crt_state_name_.clear();
  selector_->UnregisterAlarm(timeout_closure_);
}

void LogicPlaylistPolicy::LoadState() {
  if ( global_state_keeper_ != NULL ) {
    string state;
    if ( global_state_keeper_->GetValue("state", &state) ) {
      LogicPlaylistSpec playlist;
      if ( rpc::JsonDecoder::DecodeObject(state, &playlist) ) {
        SetPlaylist(playlist);
        LOG_INFO << name() << " Loaded playlist from state.";
        LOG_DEBUG << name() <<  " DETAIL for loaded playlist: "
                  << playlist.ToString();
      }
    }
    if ( global_state_keeper_->GetValue("crt_state", &state) ) {
      crt_state_name_ = state;
    } else {
      crt_state_name_.clear();
    }
  }
}

bool LogicPlaylistPolicy::SaveState() {
  // VERY IMPORTANT  !! we save in local state the playlist. In this way
  // the temps do not mess the global..
  if ( is_temp_policy_ ) {
    return true;
  }
  bool success = true;
  if ( global_state_keeper_ != NULL ) {
    LogicPlaylistSpec playlist;
    string state;
    GetPlaylist(&playlist);
    rpc::JsonEncoder::EncodeToString(playlist, &state);
    success = global_state_keeper_->SetValue("state",
                                             state) && success;
    success = global_state_keeper_->SetValue("crt_state",
                                             crt_state_name_) && success;
  }
  return success;
}
void LogicPlaylistPolicy::ClearState() {
  if ( global_state_keeper_ != NULL ) {
    global_state_keeper_->DeleteValue("state");
    global_state_keeper_->DeleteValue("crt_state");
  }
}

//////////////////////////////////////////////////////////////////////

bool LogicPlaylistPolicy::Initialize() {
  if ( rpc_http_server_ != NULL ) {
    is_registered_ = rpc_http_server_->RegisterService(local_rpc_path_, this);
    LOG_INFO << "==================> " << local_rpc_path_
             << " -- " << rpc_path_;
    if ( is_registered_ && local_rpc_path_ != rpc_path_ ) {
      rpc_http_server_->RegisterServiceMirror(rpc_path_, local_rpc_path_);
    }
  }

  LoadState();
  for ( int i = 0; i < extra_media_.size(); ++i ) {
    DLOG_DEBUG << name() << " -> " << i << " Registering to: "
               << extra_media_[i];
    URL crt_url(string("http://x/") + extra_media_[i]);
    streaming::Request* req = new streaming::Request(crt_url);
    streaming::ProcessingCallback* process_tag_callback = NewPermanentCallback(
        this, &LogicPlaylistPolicy::ProcessTag, i);
    if ( !mapper_->AddRequest(req->info().path_,
                              req,
                              process_tag_callback) ) {
      LOG_ERROR << name() << ": Failed to register to media_name: "
                << extra_media_[i];
      delete req;
      delete process_tag_callback;
      extra_reqs_.push_back(NULL);
      extra_callbacks_.push_back(NULL);
      return false;
    } else {
      extra_reqs_.push_back(req);
      extra_callbacks_.push_back(process_tag_callback);
    }
  }
  return SwitchToState(crt_state_name_);
}


string LogicPlaylistPolicy::GetPolicyConfig() {
  LogicPlaylistPolicySpec playlist;
  GetPlaylist(&playlist.playlist_.ref());
  string state;
  rpc::JsonEncoder::EncodeToString(playlist, &state);
  return state;
}

bool LogicPlaylistPolicy::NotifyEos() {
  ActionMap::const_iterator it = crt_action_map_.find(
      ActionID(Tag::TYPE_EOS));
   LOG_INFO << " Logic Policy " << name() << " EOS in state: ["
            << crt_state_name_ << "]";
  if ( it != crt_action_map_.end() ) {
    it->second->num_encountered_++;
    if ( it->second->num_encountered_ >= it->second->num_to_switch_ ) {
      SwitchToState(it->second->new_state_);
    }
  } else if ( !default_state_name_.empty() ) {
    SwitchToState(default_state_name_);
  } else {
    return false;
  }
  return true;
}


bool LogicPlaylistPolicy::NotifyTag(const Tag* tag, int64 timestamp_ms) {
  if ( IsActionTag(tag->type()) ) {
    const int key = ActionID(tag->type());
    ProcessAction(key, 0, tag, timestamp_ms);
  }
  return true;
}

void LogicPlaylistPolicy::NotifyTimeout() {
  LOG_INFO << " Logic Policy: " << name() << " in state: "
           << crt_state_name_  << " Timeout";
  if ( crt_state_ != NULL ) {
    if ( crt_state_->timeout_state_name_.is_set() ) {
      SwitchToState(crt_state_->timeout_state_name_.get());
    } else {
      SwitchToState(default_state_name_);
    }
  } else {
    SwitchToState(default_state_name_);
  }
}


void LogicPlaylistPolicy::ProcessTag(int id,
                                     const Tag* tag,
                                     int64 timestamp_ms) {
  if ( tag->type() == streaming::Tag::TYPE_EOS ) {
    LOG_INFO << " =======>>> " << name() << " EOS on extra id: " << id;
    streaming::Request* const req = extra_reqs_[id];
    extra_reqs_[id] = NULL;
    streaming::ProcessingCallback* const cb = extra_callbacks_[id];
    extra_callbacks_[id] = NULL;

    mapper_->RemoveRequest(req, cb);
    delete cb;
  }

  if ( IsActionTag(tag->type()) ) {
    const int key = ActionID(tag->type());
    // LOG_INFO << " =======>>> " << name() << " Processing EXTRA TAG : " << key
    //         << " extra_id: " << id << " tag: " << tag->ToString()
    //         << " [[" << tag->ToString() << "]]";
    ProcessAction(key, id + 1, tag, timestamp_ms);
  }
}

void LogicPlaylistPolicy::ProcessAction(int key,
                                        int extra_id,
                                        const Tag* tag,
                                        int64 timestamp_ms) {
  // LOG_INFO << " =======>>> " << name() << " Processing action for key: "
  //          << key
  //         << " extra_id: " << extra_id << " tag: " << tag->ToString()
  //         << " [[" << tag->ToString() << "]]";
  ActionMap::const_iterator it = crt_action_map_.find(
      kExtraIdBase * extra_id + key);
  if ( it != crt_action_map_.end() ) {
    if ( it->second->param_.empty() ||
         it->second->param_ == tag->ToString() ) {
      it->second->num_encountered_++;
      if ( it->second->num_encountered_ >= it->second->num_to_switch_ ) {
        // LOG_INFO << " Logic Policy: " << name() << " in state: "
        //         << crt_state_name_  << " action for key: " << key
        //         << " extra id: " << extra_id
        //         << " tag: " << tag->ToString() << " [["
        //          << tag->ToString() << "]]";
        SwitchToState(it->second->new_state_);
      } else {
        DLOG_DEBUG << " Logic Policy: " << name() << " in state: "
                   << crt_state_name_
                   << " extra id: " << extra_id
                   << " tag encounterd: " << tag->ToString()
                   << "  but not yet ready.";
      }
    }
  }
}

//////////////////////////////////////////////////////////////////////

void LogicPlaylistPolicy::GetPlaylist(LogicPlaylistSpec* playlist) const {
  playlist->default_state_name_.set(default_state_name_);
  for ( StateMap::const_iterator it = state_.begin();
        it != state_.end(); ++it ) {
    playlist->states_.ref().push_back(*it->second);
  }
}

void LogicPlaylistPolicy::SetPlaylist(const LogicPlaylistSpec& spec) {
  StateMap new_state;
  for ( int i = 0; i < spec.states_.get().size(); ++i ) {
    new_state.insert(
        make_pair(spec.states_.get()[i].name_.get(),
                  new LogicState(spec.states_.get()[i])));
  }
  bool do_switch = true;
  if ( crt_state_ != NULL ) {
    StateMap::const_iterator
        it = new_state.find(crt_state_->name_.get());
    if ( *it->second == *crt_state_ ) {
      do_switch = false;
    }
  }
  for ( StateMap::iterator it = state_.begin(); it != state_.end();  ) {
    StateMap::iterator it2 = new_state.find(it->first);
    if ( it2 == new_state.end() ) {
      it2 = it;
      ++it2;
      delete it->second;
      state_.erase(it);
      it = it2;
    } else {
      if ( *(it2->second) == *(it->second) ) {
        delete it2->second;
      } else {
        delete it->second;
        it->second = it2->second;
      }
      new_state.erase(it2);
      ++it;
    }
  }
  for ( StateMap::iterator it = new_state.begin();
        it != new_state.end(); ++it ) {
    DCHECK(state_.find(it->first) == state_.end());
    state_.insert(make_pair(it->first, it->second));
  }
  default_state_name_ = spec.default_state_name_.get();
  if ( do_switch ) {
    crt_state_ = NULL;   // already deleted ..
  }
  SwitchToState(crt_state_name_);

}

bool LogicPlaylistPolicy::SwitchToState(const string& state_name) {
  selector_->UnregisterAlarm(timeout_closure_);

  StateMap::const_iterator it = state_.find(state_name);
  if ( it == state_.end() ) {
    LOG_ERROR << name() << " - cannot switch to state - " << state_name
              << " inexistent - trying default: " << default_state_name_;
    it = state_.find(default_state_name_);
    if ( it == state_.end() ) {
      LOG_ERROR << name() << " - cannot find even the default state: "
                << default_state_name_;
      return false;
    }
  }

  // Warining - this may invalidate state_name itself !
  for ( ActionMap::const_iterator it = crt_action_map_.begin();
        it != crt_action_map_.end(); ++it ) {
    delete it->second;
  }
  crt_action_map_.clear();

  LOG_INFO << " Logic Policy: " << name() << " in state: "
           << crt_state_name_  << " switch to: "
           << it->second->name_;

  crt_state_name_ = it->first;
  crt_state_ = it->second;
  for ( int i = 0; i < crt_state_->actions_.size(); ++i ) {
    const LogicAction& action = crt_state_->actions_[i];
    const int key = action.tag_type_;
    const int extra_id = action.extra_media_id_.is_set()
                         ? 1 + action.extra_media_id_.get()
                         : 0;
    LOG_INFO << " Adding action : " << action.ToString()
             << " @@@ " << extra_id * kExtraIdBase + key;
    crt_action_map_.insert(
        make_pair(extra_id * kExtraIdBase + key,
                  new ActionStruct(
                      action.new_state_name_,
                      action.tag_param_.is_set()
                      ? action.tag_param_.c_str()
                      : "",
                      action.num_to_switch_.is_set()
                      ? action.num_to_switch_.get()
                      : 1)));
  }
  if ( it->second->max_state_time_ms_.is_set() ) {
    selector_->RegisterAlarm(timeout_closure_,
                               it->second->max_state_time_ms_);
  }
  if ( crt_state_->media_.is_set() ) {
    LOG_INFO << name() << " In state: " << crt_state_name_
             << " switching to media "
             << crt_state_->media_.get();
    element_->SwitchCurrentMedia(crt_state_->media_, NULL, true);
  } else {
    LOG_INFO << name() << " In state: " << crt_state_name_
             << " no new media to switch. Keeping "
             << element_->current_media();
  }
  ///// TODO Switch
  return true;
}

//////////////////////////////////////////////////////////////////////

void LogicPlaylistPolicy::GetPlaylist(
    rpc::CallContext<LogicPlaylistSpec>* call) {
  LogicPlaylistSpec spec;
  GetPlaylist(&spec);
  call->Complete(spec);
}

void LogicPlaylistPolicy::SetPlaylist(
    rpc::CallContext<MediaOpResult>* call,
    const LogicPlaylistSpec& spec) {
  SetPlaylist(spec);
  call->Complete(MediaOpResult(true, ""));
}

void LogicPlaylistPolicy::GetCurrentState(
    rpc::CallContext<string>* call) {
  call->Complete(crt_state_name_);
}
void LogicPlaylistPolicy::SetCurrentState(
    rpc::CallContext<bool>* call,
    const string& state_name) {
  call->Complete(SwitchToState(state_name));
}

}
