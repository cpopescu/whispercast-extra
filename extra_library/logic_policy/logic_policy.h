// Copyright 2009 WhisperSoft s.r.l.
// Author: Catalin Popescu

#ifndef __STREAMING_ELEMENTS_LOGIC_POLICY_H__
#define __STREAMING_ELEMENTS_LOGIC_POLICY_H__

#include <string>
#include <map>

#include <whisperlib/common/base/types.h>

#include "extra_library/auto/extra_library_types.h"
#include "extra_library/auto/extra_library_invokers.h"

#include <whisperlib/common/io/checkpoint/state_keeper.h>
#include <whisperlib/net/base/selector.h>
#include <whisperstreamlib/base/element.h>
#include <whisperlib/net/rpc/lib/server/rpc_http_server.h>

namespace streaming {

class LogicPlaylistPolicy
    : public Policy,
      public ServiceInvokerLogicPlaylistPolicyService {
 public:
  LogicPlaylistPolicy(const string& name,
                      PolicyDrivenElement* element,
                      ElementMapper* mapper,
                      net::Selector* selector,
                      bool is_temp_policy,
                      io::StateKeepUser* global_state_keeper,
                      io::StateKeepUser* local_state_keeper,
                      rpc::HttpServer* rpc_http_server,
                      const string& rpc_path,
                      const string& local_rpc_path,
                      const LogicPlaylistPolicySpec& spec);
  ~LogicPlaylistPolicy();


  static const char kPolicyClassName[];

  virtual bool Initialize();
  virtual void Refresh() {
    Clear();
    LoadState();
    SwitchToState(crt_state_name_);
  }
  virtual void Reset() {
    SwitchToState(default_state_name_);
  }
  virtual bool NotifyEos();
  virtual bool NotifyTag(const Tag* tag, int64 timestamp_ms);

  virtual string GetPolicyConfig();

  void LoadState();
  bool SaveState();
  void ClearState();

  virtual void GetPlaylist(rpc::CallContext<LogicPlaylistSpec>* call);
  virtual void SetPlaylist(rpc::CallContext<MediaOpResult>* call,
                           const LogicPlaylistSpec& spec);
  virtual void GetCurrentState(rpc::CallContext<string>* call);
  virtual void SetCurrentState(rpc::CallContext<bool>* call,
                               const string& state_name);

 private:
  static int ActionID(Tag::Type tag_type);
  static bool IsActionTag(Tag::Type tag_type);
  void Clear();
  void GetPlaylist(LogicPlaylistSpec* playlist) const;
  void SetPlaylist(const LogicPlaylistSpec& spec);
  bool SwitchToState(const string& state_name);

  void NotifyTimeout();

  void ProcessAction(int key, int extra_id, const Tag* tag, int64 timestamp_ms);
  void ProcessTag(int id, const Tag* tag, int64 timestamp_ms);

  ElementMapper* const mapper_;
  net::Selector* const selector_;
  const bool is_temp_policy_;
  io::StateKeepUser* const global_state_keeper_;
  io::StateKeepUser* const local_state_keeper_;
  rpc::HttpServer* const rpc_http_server_;
  const string rpc_path_;
  const string local_rpc_path_;
  int is_registered_;

  string default_state_name_;
  typedef map<string, LogicState*> StateMap;
  StateMap state_;

  string crt_state_name_;
  LogicState* crt_state_;
  struct ActionStruct {
    const string new_state_;
    const string param_;
    const int num_to_switch_;
    int num_encountered_;
    ActionStruct(const string& new_state,
                 const string& param,
                 int num_to_switch)
        : new_state_(new_state),
          param_(param),
          num_to_switch_(num_to_switch),
          num_encountered_(0) {
    }
  };
  typedef hash_map< int, ActionStruct* > ActionMap;
  ActionMap crt_action_map_;
  static const int kDefaultActionMapSize = 10;

  Closure* timeout_closure_;

  vector<streaming::Request*> extra_reqs_;
  vector<streaming::ProcessingCallback*> extra_callbacks_;
  vector<string> extra_media_;

  static const int kExtraIdBase = 1000;
};

}

#endif // __STREAMING_ELEMENTS_LOGIC_POLICY_H__
