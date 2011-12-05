// Copyright (c) 2009, Whispersoft s.r.l.
// All rights reserved.
//
// Authors: Catalin Popescu

#include <whisperlib/common/io/file/file_input_stream.h>
#include <whisperlib/common/io/ioutil.h>

#include "aux_element_mapper.h"
#include "auto/aux_element_mapper_types.h"

namespace {
http::BaseClientConnection* CreateConnection(net::Selector* selector,
                                             net::NetFactory* net_factory,
                                             net::PROTOCOL net_protocol) {
  return new http::SimpleClientConnection(selector, *net_factory, net_protocol);
}
}

namespace streaming {

// Use
//  ${RESOURCE} in your strings for requested media
//  ${REQ_QUERY} the request query part (e.g. wsp (seek_pos) and so on)
//  ${AUTH_QUERY} the authorization request query part (e.g. wuname and so on)

AuxElementMapper::AuxElementMapper(
    net::Selector* selector,
    ElementFactory* factory,
    ElementMapper* master,
    rpc::HttpServer* rpc_server,
    const char* config_file)
    : ElementMapper(selector),
      factory_(factory),
      factory_mapper_(NULL),
      rpc_server_(rpc_server),
      processing_lr_(NULL),
      net_factory_(selector_),
      connection_factory_(NULL),
      last_cache_expired_(0),
      serving_info_cache_(util::CacheBase::LRU,
                          kServingInfoCacheSize,
                          kCacheExpirationTimeMs,
                          &util::DefaultValueDestructor,
                          NULL) {  // TODO- parametrize
  master_mapper_ = master;
  connection_factory_ = NewPermanentCallback(&CreateConnection,
                                             selector, &net_factory_,
                                             net::PROTOCOL_TCP);
  string config_str;
  if ( !io::FileInputStream::TryReadFile(config_file,
                                         &config_str) ) {
    LOG_FATAL << " Cannot open AuxMapper config file: " << config_file;
  }
  AuxElementConfig config;
  if ( !rpc::JsonDecoder::DecodeObject(config_str, &config) ) {
    LOG_FATAL << " Cannot decode an AuxElementConfig from:\n" << config;
  }
  client_params_.default_request_timeout_ms_ =
      config.lookup_req_timeout_ms_.is_set()
      ? config.lookup_req_timeout_ms_.get() : 5000;
  client_params_.max_concurrent_requests_ =
      config.lookup_max_concurrent_requests_.is_set()
      ? config.lookup_max_concurrent_requests_.get() : 1;

  for ( int i = 0; i < config.servers_.size(); ++i ) {
    vector<net::HostPort> lookup_servers;
    const AuxRemoteConfigServer& crt = config.servers_[i];
    for ( int j = 0; j < crt.lookup_servers_.size(); ++j ) {
      net::HostPort hp(crt.lookup_servers_[j].c_str());
      if ( !hp.IsInvalid() ) {
        lookup_servers.push_back(hp);
      }
    }
    if ( lookup_servers.empty() ) {
      LOG_ERROR << " AuxElementConfig: No valid servers in specification: "
                << crt;
      continue;
    }
    http::FailSafeClient* failsafe_client =
        new http::FailSafeClient(
            selector,
            &client_params_,
            lookup_servers,
            connection_factory_,
            crt.lookup_num_retries_.is_set()
            ? crt.lookup_num_retries_.get() : 5,
            client_params_.default_request_timeout_ms_,
            kReopenHttpConnectionIntervalMs,
            crt.lookup_force_host_header_.is_set()
            ? crt.lookup_force_host_header_.c_str() : "");
    rpc::FailsafeClientConnectionHTTP* rpc_connection =
        new rpc::FailsafeClientConnectionHTTP(
            selector,
            crt.lookup_json_encoding_.is_set()
            && crt.lookup_json_encoding_.get()
            ? rpc::CID_JSON : rpc::CID_BINARY,
            failsafe_client,
            crt.lookup_path_,
            crt.auth_user_,
            crt.auth_pass_);
    ServiceWrapperElementConfigService* service =
        new ServiceWrapperElementConfigService(
            *rpc_connection, "ElementConfigService");
    bool used = false;
    if ( crt.element_config_prefix_.is_set() ) {
      element_config_services_.insert(
          make_pair(crt.element_config_prefix_, service));
      used = true;
    }
    if ( crt.media_detail_prefix_.is_set() ) {
      media_detail_services_.insert(
          make_pair(crt.media_detail_prefix_, service));
      used = true;
    }
    if ( !used ) {
      LOG_ERROR << " No prefix set in Aux Service => not used: " << crt;
      delete service;
    }
  }

  factory_mapper_ = new FactoryBasedElementMapper(selector, factory, NULL);
  fallback_mapper_ = factory_mapper_;
  factory_mapper_->set_extra_element_spec_map(&extra_element_spec_map_);
  factory_mapper_->set_extra_policy_spec_map(&extra_policy_spec_map_);
  LOG_INFO << " AuxElementMapper Created !!!!!!!!!!! ";
}

AuxElementMapper::~AuxElementMapper() {
  CHECK(lookup_ops_.empty()) << " Perform a Close first (and wait for it)";
  delete connection_factory_;
  hash_set<ServiceWrapperElementConfigService*> services;
  for ( ServiceMap::const_iterator it = element_config_services_.begin();
        it != element_config_services_.end(); ++it ) {
    services.insert(it->second);
  }
  for ( ServiceMap::const_iterator it = media_detail_services_.begin();
        it != media_detail_services_.end(); ++it ) {
    services.insert(it->second);
  }
  for ( hash_set<ServiceWrapperElementConfigService*>::iterator
            it = services.begin(); it != services.end(); ++it ) {
    delete *it;
  }
  delete factory_mapper_;
}

void AuxElementMapper::GetMediaDetails(
    const string& protocol,
    const string& path,
    streaming::Request* req,
    Callback1<bool>* completion_callback) {
  string normal_path = strutil::NormalizeUrlPath("/" + path);
  const ElementExportSpec* spec = serving_info_cache_.Get(
      protocol + ":" + normal_path);
  if ( spec != NULL ) {
    SetDataFromServingInfo(normal_path, req, spec);
    completion_callback->Run(true);
    return;
  }
  string key = normal_path;
  ServiceWrapperElementConfigService* service =
      io::FindPathBased(&media_detail_services_, key);
  if ( service == NULL ) {
    completion_callback->Run(false);
    return;
  }
  MediaLookupStruct* const ls = new MediaLookupStruct(
      protocol, normal_path,
      req,
      completion_callback);
  LOG_INFO << "START =============> Remote GetMediaDetails start: "
           << ls->protocol_ << " / " << ls->path_;
  ls->rpc_id_ = service->GetMediaDetails(
      NewCallback(this, &AuxElementMapper::MediaRpcLookupCompleted, ls),
      ls->protocol_, ls->path_);
}

void AuxElementMapper::MediaRpcLookupCompleted(
    MediaLookupStruct* ls,
    const rpc::CallResult<ElementExportSpec>& response) {
  if ( !response.success_ ||
       response.result_.media_name_.empty() ) {
    ls->completion_callback_->Run(false);
  } else {
    LOG_INFO << "END =============> Remote GetMediaDetails COMPLETE: "
             << response.success_ << " \n " << response.result_.ToString();
    SetDataFromServingInfo(ls->path_,
                           ls->req_,
                           &response.result_);
    serving_info_cache_.Add(ls->protocol_ + ":" + ls->path_,
                            new ElementExportSpec(response.result_));
    ls->completion_callback_->Run(true);
  }
  delete ls;
}

void AuxElementMapper::SetDataFromServingInfo(const string& norm_path,
                                              streaming::Request* req,
                                              const ElementExportSpec* spec) {
  req->mutable_serving_info()->FromElementExportSpec(*spec);
  req->mutable_info()->write_ahead_ms_ =
      (spec->flow_control_total_ms_ > 1) ? spec->flow_control_total_ms_/2 : 0;
  const string returned_key = strutil::NormalizeUrlPath(
      "/" + spec->path_.get());
  const string element_path = norm_path.substr(returned_key.size());
  if ( !element_path.empty() && element_path[0] != '/' ) {
    req->mutable_serving_info()->media_name_ =
        strutil::NormalizeUrlPath(spec->media_name_.get() +
                                  "/" + element_path);
  } else {
    req->mutable_serving_info()->media_name_ = (
        spec->media_name_.get() + element_path);
  }
}

bool AuxElementMapper::AddRequest(
    const char* media_name,
    streaming::Request* req,
    streaming::ProcessingCallback* callback,
    bool do_lookup) {
  if ( *media_name == '\0' ) {
    return false;
  }
  if ( *media_name == '/' ) {
    ++media_name;
  }
  pair<string, string> media_pair = strutil::SplitFirst(media_name, '/');
  if ( extra_element_spec_map_.find(media_pair.first) !=
       extra_element_spec_map_.end() ) {
    return fallback_mapper_->AddRequest(media_name, req, callback);
  }
  if ( !IsAuxiliaryKnownElement(media_pair.first) ) {
    return false;
  }
  if ( !AddRequestWithPerRequestElements(media_name, req, callback) ) {
    if ( do_lookup ) {
      // Proceed to a lookup..
      return StartLookupStruct(media_pair.first.c_str(),
                               media_name, req, callback);
    }
    return false;
  }
  return true;  // good so far..
}

bool AuxElementMapper::AddRequest(
    const char* media_name,
    streaming::Request* req,
    streaming::ProcessingCallback* callback) {
  return AddRequest(media_name, req, callback, true);
}

void AuxElementMapper::RemoveRequest(streaming::Request* req,
                                     ProcessingCallback* callback) {
  if ( processing_lr_ != NULL && processing_lr_->req_ == req ) {
    processing_lr_ = NULL;
  }
  LookupOpsMap::iterator it = lookup_ops_.find(req);
  if ( it != lookup_ops_.end() ) {
    it->second->is_cancelled_ = true;
    lookup_ops_.erase(it);
  }

  typedef hash_map<streaming::Request*, ElementSpecMap*> ReqElementSpecMap;
  typedef hash_map<streaming::Request*, PolicySpecMap*> ReqPolicySpecMap;

  ReqElementSpecMap::iterator it_elem = request_element_spec_map_.find(req);
  if ( it_elem != request_element_spec_map_.end() ) {
    for ( ElementSpecMap::const_iterator it = it_elem->second->begin();
          it != it_elem->second->end(); ++it ) {
      delete it->second;
    }
    delete it_elem->second;
    request_element_spec_map_.erase(it_elem);
  }
  ReqPolicySpecMap::iterator it_policy = request_policy_spec_map_.find(req);
  if ( it_policy != request_policy_spec_map_.end() ) {
    for ( PolicySpecMap::iterator it = it_policy->second->begin();
          it != it_policy->second->end(); ++it ) {
      delete it->second;
    }
    delete it_policy->second;
    request_policy_spec_map_.erase(it_policy);
  }
  fallback_mapper_->RemoveRequest(req, callback);
}

bool AuxElementMapper::HasMedia(const char* media, Capabilities* out) {
  if ( *media == '\0' ) {
    return false;
  }
  if ( *media == '/' ) {
    ++media;
  }
  if ( fallback_mapper_->HasMedia(media, out) ) {
    return true;
  }
  pair<string, string> media_pair = strutil::SplitFirst(media, '/');
  if ( !IsAuxiliaryKnownElement(media_pair.first) ) {
    return false;
  }
  *out = Capabilities();
  return true;
}

void AuxElementMapper::ListMedia(const char* media_dir,
               streaming::ElementDescriptions* medias) {
  fallback_mapper_->ListMedia(media_dir, medias);
}

bool AuxElementMapper::GetElementByName(
    const string& name,
    streaming::Element** element,
    vector<streaming::Policy*>** policies) {
  return fallback_mapper_->GetElementByName(name, element, policies);
}

//////////////////////////////////////////////////////////////////////

bool AuxElementMapper::AddRequestWithPerRequestElements(
    const char* media_name,
    streaming::Request* req,
    streaming::ProcessingCallback* callback) {
  ReqElementSpecMap::iterator
      itreq_elem = request_element_spec_map_.find(req);
  if ( itreq_elem == request_element_spec_map_.end() ) {
    return false;
  }

  DCHECK_NE(*media_name, '\0');
  if ( *media_name == '/' ) {
    ++media_name;
  }
  pair<string, string> media_pair = strutil::SplitFirst(media_name, '/');

  ElementSpecMap* element_spec_map = itreq_elem->second;
  PolicySpecMap* policy_spec_map = NULL;
  ReqPolicySpecMap::iterator
      itreq_policy = request_policy_spec_map_.find(req);
  if ( itreq_policy != request_policy_spec_map_.end() ) {
    policy_spec_map = itreq_policy->second;
  }

  Element* const element = factory_->CreateElement(
      media_pair.first,
      element_spec_map, policy_spec_map,
      req,  &req->mutable_temp_struct()->policies_);
  if ( element == NULL ) {
    return false;
  }
  ElementMap::const_iterator
      root_it = req->temp_struct().elements_.find(media_pair.first);
  if ( !element->Initialize() ) {
    delete element;
    return false;
  }
  req->mutable_temp_struct()->elements_.insert(
      make_pair(media_pair.first, element)).first;
  PolicyMap::const_iterator policy_it =
      req->temp_struct().policies_.find(element);
  if ( policy_it != req->temp_struct().policies_.end() ) {
    for ( int i = 0; i < policy_it->second->size(); ++i ) {
      Policy& policy = *(*policy_it->second)[i];
      if ( !policy.Initialize() ) {
        return false;
      }
    }
  }
  return true;
}

bool AuxElementMapper::StartLookupStruct(
    const char* element_name,
    const char* media_name,
    streaming::Request* req,
    streaming::ProcessingCallback* callback) {
  string key = strutil::NormalizeUrlPath(strutil::StringPrintf(
                                             "/%s", media_name));
  ServiceWrapperElementConfigService* service =
      io::FindPathBased(&element_config_services_, key);
  if ( service == NULL ) {
    return false;
  }
  LookupReqStruct* lr = new LookupReqStruct(media_name, req, callback);

  MediaRequestInfoSpec spec;
  req->info().ToSpec(&spec, false);   // TODO : do we want auth data
  lookup_ops_.insert(make_pair(req, lr));

  DLOG_DEBUG << "Media Element start lookup " << media_name;

  lr->rpc_id_ = service->GetElementConfig(
      NewCallback(this, &AuxElementMapper::RpcLookupCompleted, lr),
      MediaRequestSpec(element_name, spec));
  return true;
}

void AuxElementMapper::RpcLookupCompleted(
    LookupReqStruct* lr,
    const rpc::CallResult<MediaRequestConfigSpec>& response) {
  if ( !lr->is_cancelled_ ) {
    CHECK(lookup_ops_.erase(lr->req_));
    if ( response.success_ ) {
      LookupCompleted(lr, response.result_);
    } else {
      LOG_ERROR << "RPC error looking for media: " << lr->media_name_;
      ClosePendingRequest(lr);
    }
  }
  delete lr;
}

void AuxElementMapper::LookupCompleted(
    LookupReqStruct* lr,
    const MediaRequestConfigSpec& aux_spec) {
  DLOG_DEBUG << " Media Lookup completed: " << lr->req_->ToString();
  processing_lr_ = lr;
  if ( aux_spec.persistent_element_specs_.is_set() ) {
    for ( int i = 0;
          i < aux_spec.persistent_element_specs_.size();
          ++i ) {
      UpdateElementSpec(aux_spec.persistent_element_specs_[i]);
    }
  }
  if ( aux_spec.persistent_policy_specs_.is_set() ) {
    for ( int i = 0;
          i < aux_spec.persistent_policy_specs_.size();
          ++i ) {
      UpdatePolicySpec(aux_spec.persistent_policy_specs_[i]);
    }
  }
  if ( processing_lr_ == NULL ) {
    LOG_INFO << " Processing Lookup Request gone while processing answer";
    return;  // request gone while processing
  }

  if ( aux_spec.request_element_specs_.is_set() &&
       aux_spec.request_element_specs_.size() > 0 ) {
    ReqElementSpecMap::iterator itreq_elem =
        request_element_spec_map_.find(lr->req_);
    ElementSpecMap* spec_map = NULL;
    if ( itreq_elem == request_element_spec_map_.end() ) {
      spec_map = new ElementSpecMap();
      request_element_spec_map_.insert(make_pair(lr->req_, spec_map));
    } else {
      spec_map = itreq_elem->second;
    }
    for ( int i = 0;
          i < aux_spec.request_element_specs_.size();
          ++i ) {
      const MediaElementSpecs& spec =
          aux_spec.request_element_specs_[i];
      const string& name = spec.name_;
      ElementSpecMap::iterator it_spec = spec_map->find(name);
      if ( it_spec != spec_map->end() ) {
        delete it_spec->second;
        it_spec->second = new MediaElementSpecs(spec);
      } else {
        spec_map->insert(make_pair(name, new MediaElementSpecs(spec)));
      }
    }
  }

  if ( aux_spec.request_policy_specs_.is_set() &&
       aux_spec.request_policy_specs_.size() > 0 ) {
    ReqPolicySpecMap::iterator itreq_policy =
        request_policy_spec_map_.find(lr->req_);
    PolicySpecMap* spec_map = NULL;
    if ( itreq_policy == request_policy_spec_map_.end() ) {
      spec_map = new PolicySpecMap();
      request_policy_spec_map_.insert(make_pair(lr->req_, spec_map));
    } else {
      spec_map = itreq_policy->second;
    }
    for ( int i = 0;
          i < aux_spec.request_policy_specs_.size();
          ++i ) {
      const PolicySpecs& spec = aux_spec.request_policy_specs_[i];
      const string& name = spec.name_;
      PolicySpecMap::iterator it_spec = spec_map->find(name);
      if ( it_spec != spec_map->end() ) {
        delete it_spec->second;
        it_spec->second = new PolicySpecs(spec);
      } else {
        spec_map->insert(make_pair(name, new PolicySpecs(spec)));
      }
    }
  }

  if ( !AddRequest(lr->media_name_.c_str(),
                   lr->req_, lr->callback_, false) ) {
    LOG_INFO << " Error adding request after lookup to: "
             << lr->media_name_;
    ClosePendingRequest(lr);
  }
  processing_lr_ = NULL;
}

void AuxElementMapper::UpdateElementSpec(const MediaElementSpecs& spec) {
  string json_enc;
  rpc::JsonEncoder::EncodeToString(spec.params_,
                                   &json_enc);
  const string& name = spec.name_;
  ElementSpecMap::iterator it_spec =
      extra_element_spec_map_.find(name);
  if ( it_spec == extra_element_spec_map_.end() ) {
    MediaElementSpecs* new_spec = new MediaElementSpecs(spec);
    extra_element_spec_map_.insert(make_pair(name, new_spec));
    if ( !factory_mapper_->AddElement(name, spec.is_global_) ) {
      delete new_spec;
      extra_element_spec_map_.erase(name);
    } else {
      extra_element_spec_data_.insert(make_pair(name, json_enc));
    }
  } else {
    map<string, string>::iterator it_enc =
        extra_element_spec_data_.find(name);
    if ( it_enc != extra_element_spec_data_.end() &&
         it_enc->second != json_enc ) {
      // Need to update the element spec
      factory_mapper_->RemoveElement(name);
      delete it_spec->second;
      it_spec->second = new MediaElementSpecs(spec);
      if ( !factory_mapper_->AddElement(name, spec.is_global_) ) {
        delete it_spec->second;
        extra_element_spec_map_.erase(it_spec);
        extra_element_spec_data_.erase(it_enc);
      } else {
        it_enc->second = json_enc;
      }
    } // else we are cool
  }
}

void AuxElementMapper::UpdatePolicySpec(const PolicySpecs& spec) {
  string json_enc;
  rpc::JsonEncoder::EncodeToString(spec.params_.get(),
                                   &json_enc);
  const string& name = spec.name_;
  PolicySpecMap::iterator it_spec = extra_policy_spec_map_.find(name);
  if ( it_spec == extra_policy_spec_map_.end() ) {
    PolicySpecs* new_spec = new PolicySpecs(spec);
    extra_policy_spec_map_.insert(make_pair(name, new_spec));
    extra_element_spec_data_.insert(make_pair(name, json_enc));
  } else {
    map<string, string>::iterator it_enc =
        extra_element_spec_data_.find(name);
    if ( it_enc != extra_element_spec_data_.end() &&
         it_enc->second != json_enc ) {
      delete it_spec->second;
      it_spec->second = new PolicySpecs(spec);
      it_enc->second = json_enc;
    } // else we are cool
  }
}

void AuxElementMapper::ClosePendingRequest(LookupReqStruct* lr) {
  // Lookup failed, no tags were sent to this client, use EOS with timestamp 0
  lr->callback_->Run(scoped_ref<Tag>(new EosTag(
      0, lr->req_->caps().flavour_mask_, 0)).get());
}

}
