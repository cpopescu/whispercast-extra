// Copyright 2009 WhisperSoft s.r.l.
// Author: Catalin Popescu
//
// Library of extra(private) elements
//


#include <whisperlib/net/rpc/lib/codec/json/rpc_json_encoder.h>
#include <whisperlib/net/rpc/lib/codec/json/rpc_json_decoder.h>
#include <whisperlib/common/io/buffer/io_memory_stream.h>

#include "extra_library/extra_library.h"

#include "extra_library/auto/extra_library_invokers.h"

#include "extra_library/feature_detector/feature_detector.h"

#include "extra_library/time_range/time_range_element.h"
#include "extra_library/time_shift/time_shift_element.h"
#include "extra_library/time_delay/time_delay_policy.h"
#include "extra_library/schedule/schedule_policy.h"
#include "extra_library/logic_policy/logic_policy.h"
#include "extra_library/flavouring/flavouring_element.h"

#include "extra_library/http_authorizer/http_authorizer.h"
#include <whisperlib/common/io/ioutil.h>
#include "aux_element_mapper.h"

//////////////////////////////////////////////////////////////////////

// Library Stub

#ifdef __cplusplus
extern "C" {
#endif

  void* GetElementLibrary() {
    return new streaming::ExtraElementLibrary();
  }
  streaming::ElementMapper* CreateAuxElementMapper(
      net::Selector* selector,
      streaming::ElementFactory* factory,
      streaming::ElementMapper* master,
      rpc::HttpServer* rpc_server,
      const char* config_file) {
    return new streaming::AuxElementMapper(
        selector, factory, master, rpc_server, config_file);
  }
#ifdef __cplusplus
}
#endif

//////////////////////////////////////////////////////////////////////

namespace streaming {
ExtraElementLibrary::ExtraElementLibrary()
    : ElementLibrary("extra_library"),
      rpc_impl_(NULL) {
  /* libavcodec */
  #if LIBAVCODEC_VERSION_INT <= AV_VERSION_INT(53,34,0)
    avcodec_init();
  #endif
  avcodec_register_all();
}

ExtraElementLibrary::~ExtraElementLibrary() {
  CHECK(rpc_impl_ == NULL)
      << " Deleting the library before unregistering the rpc";
}

void ExtraElementLibrary::GetExportedElementTypes(vector<string>* element_types) {
  element_types->push_back(FeatureDetectorElement::kElementClassName);
  element_types->push_back(TimeRangeElement::kElementClassName);
  element_types->push_back(TimeShiftElement::kElementClassName);
  element_types->push_back(FlavouringElement::kElementClassName);
}

void ExtraElementLibrary::GetExportedPolicyTypes(vector<string>* policy_types) {
  policy_types->push_back(TimeDelayPolicy::kPolicyClassName);
  policy_types->push_back(SchedulePlaylistPolicy::kPolicyClassName);
  policy_types->push_back(LogicPlaylistPolicy::kPolicyClassName);
}
void ExtraElementLibrary::GetExportedAuthorizerTypes(vector<string>* auth_types) {
  auth_types->push_back(HttpAuthorizer::kAuthorizerClassName);
}

int64 ExtraElementLibrary::GetElementNeeds(const string& element_type) {
  if ( element_type == TimeRangeElement::kElementClassName ) {
    return (NEED_SELECTOR |
            NEED_MEDIA_DIR |
            NEED_STATE_KEEPER |
            NEED_RPC_SERVER);
  }
  if ( element_type == TimeShiftElement::kElementClassName ) {
    return (NEED_SELECTOR |
            NEED_MEDIA_DIR |
            NEED_STATE_KEEPER);
  } else if ( element_type == FlavouringElement::kElementClassName ) {
    return (NEED_SELECTOR |
            NEED_STATE_KEEPER |
            NEED_RPC_SERVER);
  }
  else if ( element_type == FeatureDetectorElement::kElementClassName ) {
    return (NEED_MEDIA_DIR |
            NEED_SELECTOR);
  }
  return 0;
}

int64 ExtraElementLibrary::GetPolicyNeeds(const string& policy_type) {
  if ( policy_type == TimeDelayPolicy::kPolicyClassName ) {
    return (NEED_RPC_SERVER |
            NEED_SELECTOR |
            NEED_MEDIA_DIR |
            NEED_STATE_KEEPER);
  } else if ( policy_type == SchedulePlaylistPolicy::kPolicyClassName ) {
    return (NEED_RPC_SERVER |
            NEED_SELECTOR |
            NEED_STATE_KEEPER);
  } else if ( policy_type == LogicPlaylistPolicy::kPolicyClassName ) {
    return (NEED_RPC_SERVER |
            NEED_SELECTOR |
            NEED_STATE_KEEPER);
  }
  return 0;    // no policies
}
int64 ExtraElementLibrary::GetAuthorizerNeeds(const string& auth_type) {
  if ( auth_type == HttpAuthorizer::kAuthorizerClassName ) {
    return (NEED_SELECTOR);
  }

  return 0;
}


streaming::Element* ExtraElementLibrary::CreateElement(
    const string& element_type,
    const string& name,
    const string& element_params,
    const streaming::Request* req,
    const CreationObjectParams& params,
    bool is_temporary_template,
    // Output:
    vector<string>* needed_policies,
    string* error_description) {
  streaming::Element* ret = NULL;
  if ( element_type == TimeRangeElement::kElementClassName ) {
    CREATE_ELEMENT_HELPER(TimeRange);
    return ret;
  }
  if ( element_type == TimeShiftElement::kElementClassName ) {
    CREATE_ELEMENT_HELPER(TimeShift);
    return ret;
  } else if ( element_type == FlavouringElement::kElementClassName ) {
    CREATE_ELEMENT_HELPER(Flavouring);
    return ret;
  }
  else if ( element_type == FeatureDetectorElement::kElementClassName ) {
    CREATE_ELEMENT_HELPER(FeatureDetector);
    return ret;
  }
  LOG_ERROR << "Dunno to create element of type: '" << element_type << "'";
  return NULL;
}

streaming::Policy* ExtraElementLibrary::CreatePolicy(
      const string& policy_type,
      const string& name,
      const string& policy_params,
      streaming::PolicyDrivenElement* element,
      const streaming::Request* req,
      const CreationObjectParams& params,
      // Output:
      string* error_description) {
  streaming::Policy* ret = NULL;
  if ( policy_type == TimeDelayPolicy::kPolicyClassName ) {
    CREATE_POLICY_HELPER(TimeDelay);
    return ret;
  } else if ( policy_type == SchedulePlaylistPolicy::kPolicyClassName ) {
    CREATE_POLICY_HELPER(SchedulePlaylist);
    return ret;
  } else if ( policy_type == LogicPlaylistPolicy::kPolicyClassName ) {
    CREATE_POLICY_HELPER(LogicPlaylist);
    return ret;
  }
  return ret;
}
streaming::Authorizer* ExtraElementLibrary::CreateAuthorizer(
    const string& authorizer_type,
    const string& name,
    const string& authorizer_params,
    const CreationObjectParams& params,
    // Output:
    string* error_description) {
  streaming::Authorizer* ret = NULL;
  if ( authorizer_type == HttpAuthorizer::kAuthorizerClassName ) {
    CREATE_AUTHORIZER_HELPER(Http);
    return ret;
  }
  return ret;
}

//////////////////////////////////////////////////////////////////////
//
// Element Creation functions:
//

streaming::Element*
ExtraElementLibrary::CreateFeatureDetectorElement(const string& element_name,
                                                  const FeatureDetectorElementSpec& spec,
                                                  const streaming::Request* req,
                                                  const CreationObjectParams& params,
                                                  vector<string>* needed_policies,
                                                  bool is_temporary_template,
                                                  string* error) {
  if ( req != NULL ) {
    LOG_ERROR << " Cannot create temp feature detector - these things consume a lot of CPU";
    return NULL;
  }
  return new FeatureDetectorElement(element_name,
                                    mapper_,
                                    params.selector_,
                                    params.media_dir_,
                                    spec);
}


/////// TimeRange

streaming::Element* ExtraElementLibrary::CreateTimeRangeElement(
    const string& element_name,
    const TimeRangeElementSpec& spec,
    const streaming::Request* req,
    const CreationObjectParams& params,
    vector<string>* needed_policies,
    bool is_temporary_template,
    string* error) {
  string rpc_path = strutil::JoinPaths(name(), element_name);
  rpc::HttpServer* rpc_server = params.rpc_server_;

  return new streaming::TimeRangeElement(
      element_name,
      mapper_,
      params.selector_,
      params.state_keeper_,
      params.local_state_keeper_,
      rpc_path,
      rpc_server,
      spec.internal_media_path_,
      spec.update_media_interval_sec_);
}

/////// TimeShift

streaming::Element* ExtraElementLibrary::CreateTimeShiftElement(
    const string& element_name,
    const TimeShiftElementSpec& spec,
    const streaming::Request* req,
    const CreationObjectParams& params,
    vector<string>* needed_policies,
    bool is_temporary_template,
    string* error) {
  return new streaming::TimeShiftElement(
      element_name,
      mapper_,
      params.selector_,
      spec.media_path_,
      strutil::JoinPaths(params.media_dir_, spec.save_dir_),
      spec.time_range_element_on_save_dir_,
      spec.buffer_size_ms_);
}

/////// Flavouring

streaming::Element* ExtraElementLibrary::CreateFlavouringElement(
    const string& element_name,
    const FlavouringElementSpec& spec,
    const streaming::Request* req,
    const CreationObjectParams& params,
    vector<string>* needed_policies,
    bool is_temporary_template,
    string* error) {
  string rpc_path = strutil::JoinPaths(name(), element_name);

  // sanity check
  uint32 all_flav = 0;
  for ( uint32 i = 0; i < spec.flavours_.size(); ++i ) {
    if ( (all_flav & spec.flavours_[i].flavour_mask_.get()) != 0 ) {
      *error = "Overlapping flavours";
      return NULL;
    }
    all_flav |= spec.flavours_[i].flavour_mask_.get();
  }

  vector< pair<string, uint32> > flavours;
  for ( uint32 i = 0; i < spec.flavours_.size(); ++i ) {
    flavours.push_back(
        make_pair(spec.flavours_[i].media_prefix_.get(),
                  spec.flavours_[i].flavour_mask_.get()));
  }

  return new streaming::FlavouringElement(element_name,
                                          mapper_,
                                          params.selector_,
                                          rpc_path,
                                          params.rpc_server_,
                                          params.state_keeper_,
                                          flavours);
}

/////// TimeDelay

streaming::Policy* ExtraElementLibrary::CreateTimeDelayPolicy(
    const string& policy_name,
    const TimeDelayPolicySpec& spec,
    streaming::PolicyDrivenElement* element,
    const streaming::Request* req,
    const CreationObjectParams& params,
    string* error) {
  const string& home_dir = spec.home_dir_;
  const string full_home_dir(params.media_dir_ + "/" + home_dir);
  if ( !io::IsDir(full_home_dir) ) {
    LOG_ERROR << "Cannot create media element under directory : ["
              << home_dir << "] (looking under: ["
              << full_home_dir << "]";
    delete params.state_keeper_;
    delete params.local_state_keeper_;
    return NULL;
  }
  string rpc_path(name() + "/");
  rpc_path += element->name() + "/" + policy_name + "/" ;
  string local_rpc_path(rpc_path);
  if ( req != NULL ) {
    local_rpc_path += req->info().GetPathId() + "/";
  }
  rpc::HttpServer* rpc_server = params.rpc_server_;
  if ( spec.disable_rpc_.is_set() && spec.disable_rpc_.get() ) {
    rpc_server = NULL;
  }
  return new TimeDelayPolicy(
      policy_name,
      element,
      mapper_,
      params.selector_,
      req != NULL,
      params.state_keeper_,
      params.local_state_keeper_,
      rpc_path,
      local_rpc_path,
      rpc_server,
      full_home_dir,
      spec.root_media_path_,
      spec.file_prefix_,
      spec.time_delay_sec_);
}

/////// SchedulePlaylist

streaming::Policy* ExtraElementLibrary::CreateSchedulePlaylistPolicy(
    const string& policy_name,
    const SchedulePlaylistPolicySpec& spec,
    streaming::PolicyDrivenElement* element,
    const streaming::Request* req,
    const CreationObjectParams& params,
    string* error) {
  string rpc_path(name() + "/");
  rpc_path += element->name() + "/" + policy_name + "/" ;
  string local_rpc_path(rpc_path);
  if ( req != NULL ) {
    local_rpc_path += req->info().GetPathId() + "/";
  }
  const string& default_stream = spec.default_media_;
  const vector<SchedulePlaylistItemSpec>& rpc_playlist =
      spec.playlist_.get();
  vector<const SchedulePlaylistItemSpec*> playlist;
  for ( uint32 i = 0; i < rpc_playlist.size(); ++i ) {
    playlist.push_back(&rpc_playlist[i]);
  }
  rpc::HttpServer* rpc_server = params.rpc_server_;
  if ( spec.disable_rpc_.is_set() && spec.disable_rpc_.get() ) {
    rpc_server = NULL;
  }
  const int32 min_switch_delta_ms =
      spec.min_switch_delta_ms_.is_set()
      ? spec.min_switch_delta_ms_.get() : 2000;
  return new SchedulePlaylistPolicy(policy_name,
                                    element,
                                    params.selector_,
                                    req != NULL,
                                    params.state_keeper_,
                                    params.local_state_keeper_,
                                    rpc_server,
                                    rpc_path,
                                    local_rpc_path,
                                    default_stream,
                                    playlist,
                                    min_switch_delta_ms);
}

/////// LogicPlaylist

streaming::Policy* ExtraElementLibrary::CreateLogicPlaylistPolicy(
    const string& policy_name,
    const LogicPlaylistPolicySpec& spec,
    streaming::PolicyDrivenElement* element,
    const streaming::Request* req,
    const CreationObjectParams& params,
    string* error) {
  string rpc_path(name() + "/");
  rpc_path += element->name() + "/" + policy_name + "/" ;
  string local_rpc_path(rpc_path);
  if ( req != NULL ) {
    local_rpc_path += req->info().GetPathId() + "/";
  }
  rpc::HttpServer* rpc_server = params.rpc_server_;

  return new LogicPlaylistPolicy(policy_name,
                                 element,
                                 mapper_,
                                 params.selector_,
                                 req != NULL,
                                 params.state_keeper_,
                                 params.local_state_keeper_,
                                 rpc_server,
                                 rpc_path,
                                 local_rpc_path,
                                 spec);
}

streaming::Authorizer* ExtraElementLibrary::CreateHttpAuthorizer(
    const string& auth_name,
    const HttpAuthorizerSpec& spec,
    const CreationObjectParams& params,
    string* error) {
  const HttpAuthorizer::Escapement header_escapement =
      spec.header_escapement_.is_set()
      ? static_cast<HttpAuthorizer::Escapement>(
          spec.header_escapement_.get())
      : HttpAuthorizer::ESC_URL;
  const HttpAuthorizer::Escapement body_escapement =
      spec.body_escapement_.is_set()
      ? static_cast<HttpAuthorizer::Escapement>(
          spec.body_escapement_.get())
      : HttpAuthorizer::ESC_URL;
  const int default_allowed_ms =
      spec.default_allowed_ms_.is_set()
      ? spec.default_allowed_ms_.get() : 0;
  const bool parse_json_authorizer_reply =
      spec.parse_json_authorizer_reply_.is_set()
      ? spec.parse_json_authorizer_reply_.get() : false;
  const string force_host_header(
      spec.force_host_header_.is_set()
      ? spec.force_host_header_.c_str() : "");
  const int num_retries =
      spec.num_retries_.is_set()
      ? spec.num_retries_.get() : 3;
  if ( num_retries < 1 ) {
    *error = "num_retries should be at least 1";
    return NULL;
  }
  const int max_concurrent_requests =
      spec.max_concurrent_requests_.is_set()
      ? spec.max_concurrent_requests_.get() : 1;
  if ( max_concurrent_requests < 1 ) {
    *error = "max_concurrent_requests should be at lease 1";
    return NULL;
  }
  const int req_timeout_ms =
      spec.req_timeout_ms_.is_set()
      ? spec.req_timeout_ms_.get() : 10000;
  if ( req_timeout_ms < 100 ) {
    *error = "unrealistic req_timeout_ms";
    return NULL;
  }
  vector<net::HostPort> servers;
  for ( uint32 i = 0; i < spec.servers_.size(); ++i ) {
    servers.push_back(net::HostPort(spec.servers_[i]));
  }
  if ( servers.empty() ) {
    *error = "No valid authorization servers found";
    return NULL;
  }
  vector< pair<string, string> > http_headers;
  for ( uint32 i = 0; i < spec.http_headers_.size(); ++i ) {
    http_headers.push_back(
        make_pair(spec.http_headers_[i].name_.get(),
                  spec.http_headers_[i].value_.get()));
  }
  vector<string> body_lines;
  for ( uint32 i = 0; i < spec.body_lines_.size(); ++i ) {
    body_lines.push_back(spec.body_lines_[i]);
  }
  return new HttpAuthorizer(auth_name,
                            params.selector_,
                            servers,
                            spec.use_https_.get(),
                            spec.query_path_format_,
                            http_headers,
                            body_lines,
                            spec.include_auth_headers_,
                            spec.success_body_.is_set() ?
                                spec.success_body_.get().c_str() : "",
                            header_escapement,
                            body_escapement,
                            default_allowed_ms,
                            parse_json_authorizer_reply,
                            force_host_header,
                            num_retries,
                            max_concurrent_requests,
                            req_timeout_ms);
}


//////////////////////////////////////////////////////////////////////

class ServiceInvokerExtraLibraryServiceImpl
    : public ServiceInvokerExtraLibraryService {
 public:
  ServiceInvokerExtraLibraryServiceImpl(
      streaming::ElementMapper* mapper,
      streaming::ElementLibrary::ElementSpecCreationCallback*
      create_element_spec_callback,
      streaming::ElementLibrary::PolicySpecCreationCallback*
      create_policy_spec_callback,
      streaming::ElementLibrary::AuthorizerSpecCreationCallback*
      create_authorizer_spec_callback)
      : ServiceInvokerExtraLibraryService(
          ServiceInvokerExtraLibraryService::GetClassName()),
        mapper_(mapper),
        create_element_spec_callback_(create_element_spec_callback),
        create_policy_spec_callback_(create_policy_spec_callback),
        create_authorizer_spec_callback_(create_authorizer_spec_callback) {
  }

  virtual void AddFeatureDetectorElementSpec(
      rpc::CallContext< MediaOpResult >* call,
      const string& name,
      bool is_global,
      bool disable_rpc,
      const FeatureDetectorElementSpec& spec) {
    STANDARD_RPC_ELEMENT_ADD(FeatureDetector);
  }
  virtual void AddTimeRangeElementSpec(
      rpc::CallContext< MediaOpResult >* call,
      const string& name,
      bool is_global,
      bool disable_rpc,
      const TimeRangeElementSpec& spec) {
    STANDARD_RPC_ELEMENT_ADD(TimeRange);
  }
  virtual void AddTimeShiftElementSpec(
      rpc::CallContext< MediaOpResult >* call,
      const string& name,
      bool is_global,
      bool disable_rpc,
      const TimeShiftElementSpec& spec) {
    STANDARD_RPC_ELEMENT_ADD(TimeShift);
  }
  virtual void AddFlavouringElementSpec(
      rpc::CallContext< MediaOpResult >* call,
      const string& name,
      bool is_global,
      bool disable_rpc,
      const FlavouringElementSpec& spec) {
    STANDARD_RPC_ELEMENT_ADD(Flavouring);
  }
  virtual void AddSchedulePlaylistPolicySpec(
      rpc::CallContext< MediaOpResult >* call,
      const string& name,
      const SchedulePlaylistPolicySpec& spec) {
    STANDARD_RPC_POLICY_ADD(SchedulePlaylist);
  }
  virtual void AddLogicPlaylistPolicySpec(
      rpc::CallContext< MediaOpResult >* call,
      const string& name,
      const LogicPlaylistPolicySpec& spec) {
    STANDARD_RPC_POLICY_ADD(LogicPlaylist);
  }
  virtual void AddTimeDelayPolicySpec(
      rpc::CallContext< MediaOpResult >* call,
      const string& name,
      const TimeDelayPolicySpec& spec) {
    STANDARD_RPC_POLICY_ADD(TimeDelay);
  }

  virtual void AddHttpAuthorizerSpec(
      rpc::CallContext< MediaOpResult >* call,
      const string& name,
      const HttpAuthorizerSpec& spec) {
    STANDARD_RPC_AUTHORIZER_ADD(Http);
  }
 private:
  streaming::ElementMapper* mapper_;
  streaming::ElementLibrary::ElementSpecCreationCallback*
  create_element_spec_callback_;
  streaming::ElementLibrary::PolicySpecCreationCallback*
  create_policy_spec_callback_;
  streaming::ElementLibrary::AuthorizerSpecCreationCallback*
  create_authorizer_spec_callback_;

  DISALLOW_EVIL_CONSTRUCTORS(ServiceInvokerExtraLibraryServiceImpl);
};

//////////////////////////////////////////////////////////////////////

bool ExtraElementLibrary::RegisterLibraryRpc(rpc::HttpServer* rpc_server,
                                             string* rpc_http_path) {
  CHECK(rpc_impl_ == NULL);
  rpc_impl_ = new ServiceInvokerExtraLibraryServiceImpl(
      mapper_,
      create_element_spec_callback_,
      create_policy_spec_callback_,
      create_authorizer_spec_callback_);
  if ( !rpc_server->RegisterService(name() + "/", rpc_impl_) ) {
    delete rpc_impl_;
    return false;
  }
  *rpc_http_path = strutil::NormalizePath(
      rpc_server->path() + "/" + name() + "/" + rpc_impl_->GetName());
  return true;
}

bool ExtraElementLibrary::UnregisterLibraryRpc(rpc::HttpServer* rpc_server) {
  CHECK(rpc_impl_ != NULL);
  const bool success = rpc_server->UnregisterService(name() + "/", rpc_impl_);
  delete rpc_impl_;
  rpc_impl_ = NULL;
  return success;
}
}
