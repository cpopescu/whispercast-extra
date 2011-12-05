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

#ifndef __DISABLE_FEATURES__
#include "extra_library/feature_detector/feature_detector.h"
#endif

#include "extra_library/time_range/time_range_element.h"
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
#ifndef __DISABLE_FEATURES__
  /* libavcodec */
  avcodec_init();
  avcodec_register_all();
#endif
}

ExtraElementLibrary::~ExtraElementLibrary() {
  CHECK(rpc_impl_ == NULL)
      << " Deleting the library before unregistering the rpc";
}

void ExtraElementLibrary::GetExportedElementTypes(vector<string>* element_types) {
#ifndef __DISABLE_FEATURES__
  element_types->push_back(FeatureDetectorElement::kElementClassName);
#endif
  element_types->push_back(TimeRangeElement::kElementClassName);
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
  } else if ( element_type == FlavouringElement::kElementClassName ) {
    return (NEED_SELECTOR |
            NEED_STATE_KEEPER |
            NEED_RPC_SERVER);
  }
#ifndef __DISABLE_FEATURES__
  else if ( element_type == FeatureDetectorElement::kElementClassName ) {
    return (NEED_MEDIA_DIR |
            NEED_SELECTOR);
  }
#endif
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
    // Output:
    vector<string>* needed_policies,
    string* error_description) {
  streaming::Element* ret = NULL;
  if ( element_type == TimeRangeElement::kElementClassName ) {
    CREATE_ELEMENT_HELPER(TimeRange);
    return ret;
  } else if ( element_type == FlavouringElement::kElementClassName ) {
    CREATE_ELEMENT_HELPER(Flavouring);
    return ret;
  }
#ifndef __DISABLE_FEATURES__
  else if ( element_type == FeatureDetectorElement::kElementClassName ) {
    CREATE_ELEMENT_HELPER(FeatureDetector);
    return ret;
  }
#endif
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
                                                  string* error) {
  if ( req != NULL ) {
    LOG_ERROR << " Cannot create temp feature detector - these things consume a lot of CPU";
    return NULL;
  }
#ifdef __DISABLE_FEATURES__
  return NULL;
#else
  return new FeatureDetectorElement(element_name.c_str(),
                                    element_name.c_str(),
                                    mapper_,
                                    params.selector_,
                                    params.media_dir_,
                                    spec);
#endif
}


/////// TimeRange

streaming::Element* ExtraElementLibrary::CreateTimeRangeElement(
    const string& element_name,
    const TimeRangeElementSpec& spec,
    const streaming::Request* req,
    const CreationObjectParams& params,
    vector<string>* needed_policies,
    string* error) {
  const string home_dir(spec.home_dir_.c_str());
  const string full_home_dir(params.media_dir_ + "/" + home_dir);
  // We disable this check ...
  //
  // if ( !io::IsDir(full_home_dir.c_str()) ) {
  //   LOG_ERROR << "Cannot create media element under directory : ["
  //             << home_dir << "] (looking under: ["
  //             << full_home_dir << "]";
  //   return NULL;
  // }
  const bool utc_requests =
      spec.utc_requests_.is_set()
      ? spec.utc_requests_.get() : false;

  string rpc_path(name() + "/" + element_name + "/");
  rpc::HttpServer* rpc_server = params.rpc_server_;


  const string id(req != NULL
                  ? element_name + "-" + req->GetUrlId(): element_name);
  return new streaming::TimeRangeElement(
      element_name.c_str(),
      id.c_str(),
      mapper_,
      params.selector_,
      params.state_keeper_,
      params.local_state_keeper_,
      rpc_path.c_str(),
      rpc_server,
      full_home_dir.c_str(),
      spec.root_media_path_.c_str(),
      spec.file_prefix_.c_str(),
      utc_requests);
}

/////// Flavouring

streaming::Element* ExtraElementLibrary::CreateFlavouringElement(
    const string& element_name,
    const FlavouringElementSpec& spec,
    const streaming::Request* req,
    const CreationObjectParams& params,
    vector<string>* needed_policies,
    string* error) {
  string rpc_path(name() + "/" + element_name + "/");
  const string id(req != NULL
                  ? element_name + "-" + req->GetUrlId(): element_name);

  // sanity check
  uint32 all_flav = 0;
  for ( int i = 0; i < spec.flavours_.size(); ++i ) {
    if ( (all_flav & spec.flavours_[i].flavour_mask_.get()) != 0 ) {
      *error = "Overlapping flavours";
      return NULL;
    }
    all_flav |= spec.flavours_[i].flavour_mask_.get();
  }

  vector< pair<string, uint32> > flavours;
  for ( int i = 0; i < spec.flavours_.size(); ++i ) {
    flavours.push_back(
        make_pair(spec.flavours_[i].media_prefix_.get(),
                  spec.flavours_[i].flavour_mask_.get()));
  }

  return new streaming::FlavouringElement(element_name,
                                          id,
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
  const string home_dir(spec.home_dir_.c_str());
  const string full_home_dir(params.media_dir_ + "/" + home_dir);
  if ( !io::IsDir(full_home_dir.c_str()) ) {
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
      policy_name.c_str(),
      element,
      mapper_,
      params.selector_,
      req != NULL,
      params.state_keeper_,
      params.local_state_keeper_,
      rpc_path.c_str(),
      local_rpc_path.c_str(),
      rpc_server,
      full_home_dir.c_str(),
      spec.root_media_path_.c_str(),
      spec.file_prefix_.c_str(),
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
  for ( int i = 0; i < rpc_playlist.size(); ++i ) {
    playlist.push_back(&rpc_playlist[i]);
  }
  rpc::HttpServer* rpc_server = params.rpc_server_;
  if ( spec.disable_rpc_.is_set() && spec.disable_rpc_.get() ) {
    rpc_server = NULL;
  }
  const int32 min_switch_delta_ms =
      spec.min_switch_delta_ms_.is_set()
      ? spec.min_switch_delta_ms_.get() : 2000;
  return new SchedulePlaylistPolicy(policy_name.c_str(),
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

  return new LogicPlaylistPolicy(policy_name.c_str(),
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
  for ( int i = 0; i < spec.servers_.size(); ++i ) {
    net::HostPort hp(spec.servers_[i].c_str());
    if ( !hp.IsInvalid() ) {
      servers.push_back(hp);
    }
  }
  if ( servers.empty() ) {
    *error = "No valid servers_ host-port found";
    return NULL;
  }
  vector< pair<string, string> > http_headers;
  for ( int i = 0; i < spec.http_headers_.size(); ++i ) {
    http_headers.push_back(
        make_pair(spec.http_headers_[i].name_.get(),
                  spec.http_headers_[i].value_.get()));
  }
  vector<string> body_lines;
  for ( int i = 0; i < spec.body_lines_.size(); ++i ) {
    body_lines.push_back(spec.body_lines_[i]);
  }
  return new HttpAuthorizer(auth_name.c_str(),
                            params.selector_,
                            servers,
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
      rpc::CallContext< MediaOperationErrorData >* call,
      const string& name,
      bool is_global,
      bool disable_rpc,
      const FeatureDetectorElementSpec& spec) {
#ifndef __DISABLE_FEATURES__
    STANDARD_RPC_ELEMENT_ADD(FeatureDetector);
#endif
  }
  virtual void AddTimeRangeElementSpec(
      rpc::CallContext< MediaOperationErrorData >* call,
      const string& name,
      bool is_global,
      bool disable_rpc,
      const TimeRangeElementSpec& spec) {
    STANDARD_RPC_ELEMENT_ADD(TimeRange);
  }
  virtual void AddFlavouringElementSpec(
      rpc::CallContext< MediaOperationErrorData >* call,
      const string& name,
      bool is_global,
      bool disable_rpc,
      const FlavouringElementSpec& spec) {
    STANDARD_RPC_ELEMENT_ADD(Flavouring);
  }
  virtual void AddSchedulePlaylistPolicySpec(
      rpc::CallContext< MediaOperationErrorData >* call,
      const string& name,
      const SchedulePlaylistPolicySpec& spec) {
    STANDARD_RPC_POLICY_ADD(SchedulePlaylist);
  }
  virtual void AddLogicPlaylistPolicySpec(
      rpc::CallContext< MediaOperationErrorData >* call,
      const string& name,
      const LogicPlaylistPolicySpec& spec) {
    STANDARD_RPC_POLICY_ADD(LogicPlaylist);
  }
  virtual void AddTimeDelayPolicySpec(
      rpc::CallContext< MediaOperationErrorData >* call,
      const string& name,
      const TimeDelayPolicySpec& spec) {
    STANDARD_RPC_POLICY_ADD(TimeDelay);
  }

  virtual void AddHttpAuthorizerSpec(
      rpc::CallContext< MediaOperationErrorData >* call,
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
