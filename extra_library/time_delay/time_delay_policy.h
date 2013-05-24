// (c) 2009, Whispersoft s.r.l.
// All rights reserved.
//
// Author: Catalin Popescu
//

#include <string>

#include <whisperlib/net/base/selector.h>
#include <whisperstreamlib/base/element.h>
#include <whisperlib/net/rpc/lib/server/rpc_http_server.h>
#include <whisperlib/common/base/date.h>
#include <whisperlib/common/io/checkpoint/state_keeper.h>

#include "extra_library/auto/extra_library_invokers.h"

//
// VERY IMPORTNANT:  set tag_timeout_sec == 0 (no tag timeout) to the
//                   associated switching source
//
namespace streaming {

class TimeDelayPolicy : public Policy,
                        public ServiceInvokerTimeDelayPolicyService {
 public:
  TimeDelayPolicy(const string& name,
                  PolicyDrivenElement* element,
                  ElementMapper* mapper,
                  net::Selector* selector,
                  bool is_temp_policy,
                  io::StateKeepUser* global_state_keeper,
                  io::StateKeepUser* local_state_keeper,
                  const string& rpc_path,
                  const string& local_rpc_path_,
                  rpc::HttpServer* rpc_server,
                  const string& home_dir,
                  const string& root_media_path,
                  const string& file_prefix,
                  int time_delay_sec);
  virtual ~TimeDelayPolicy();

  virtual bool Initialize();
  virtual bool NotifyEos();
  virtual bool NotifyTag(const Tag* tag, int64 timestamp_ms);

  virtual void Reset() {}
  virtual string GetPolicyConfig() { return ""; }

  static const char kPolicyClassName[];

 private:
  bool LoadState();
  bool SaveState();
  void ClearState();

  void PlayNext();

  ElementMapper* const mapper_;
  net::Selector* const selector_;
  const bool is_temp_policy_;
  io::StateKeepUser* const global_state_keeper_;
  io::StateKeepUser* const local_state_keeper_;
  const string rpc_path_;
  const string local_rpc_path_;
  rpc::HttpServer* const rpc_server_;
  const string home_dir_;
  const string root_media_path_;
  const string file_prefix_;
  const int time_delay_ms_;
  bool is_registered_;

  string crt_media_;
  int64 begin_file_timestamp_;
  int64 playref_time_;   // the original time of the next(current) tag

  Closure* play_next_callback_;

  typedef map<int64, int32> SkipMap;   // UTC based time -> duration
  SkipMap skip_map_;

  DISALLOW_EVIL_CONSTRUCTORS(TimeDelayPolicy);
};
}
