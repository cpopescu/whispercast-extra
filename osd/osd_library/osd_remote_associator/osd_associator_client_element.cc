// Copyright 2009 WhisperSoft s.r.l.
// Author: Cosmin Tudorache

#include <whisperlib/common/io/ioutil.h>
#include "osd_associator_client_element.h"

namespace streaming {

///////////////////////////////////////////////////////////////////////////////

OsdAssociatorClientElement::ClientCallbackData::ClientCallbackData(
    const string& creation_path)
    : FilteringCallbackData(),
      creation_path_(creation_path),
      pending_osd_tags_(),
      osd_(NULL) {
}
OsdAssociatorClientElement::ClientCallbackData::~ClientCallbackData() {
  delete osd_;
  osd_ = NULL;
}
const string& OsdAssociatorClientElement::ClientCallbackData::creation_path()
const {
  return creation_path_;
}
void OsdAssociatorClientElement::ClientCallbackData::Inject(OsdTag* tag) {
  pending_osd_tags_.push_back(tag);
}
void OsdAssociatorClientElement::ClientCallbackData::InjectCreateAll(
    const OsdAssociatorCompleteOsd& osd) {
  for ( uint32 i = 0; i < osd.overlays.get().size(); i++ ) {
    const CreateOverlayParams& overlay = osd.overlays.get()[i];
    Inject(new OsdTag(0, streaming::kDefaultFlavourMask, 0, overlay));
  }
  for ( uint32 i = 0; i < osd.crawlers.get().size(); i++ ) {
    const CreateCrawlerParams& crawler = osd.crawlers.get()[i];
    Inject(new OsdTag(0, streaming::kDefaultFlavourMask, 0, crawler));
  }
  for ( uint32 i = 0; i < osd.crawl_items.get().size(); i++ ) {
    const AddCrawlerItemParams& crawler_item = osd.crawl_items.get()[i];
    Inject(new OsdTag(0, streaming::kDefaultFlavourMask, 0, crawler_item));
  }
  if ( osd.pip.is_set() ) {
    Inject(new OsdTag(0, streaming::kDefaultFlavourMask, 0, osd.pip.get()));
  }
  if ( osd.click_url.is_set() ) {
    Inject(new OsdTag(0, streaming::kDefaultFlavourMask, 0, osd.click_url.get()));
  }
}
void OsdAssociatorClientElement::ClientCallbackData::InjectDestroyAll(
    const OsdAssociatorCompleteOsd& osd) {
  for ( uint32 i = 0; i < osd.overlays.get().size(); i++ ) {
    const CreateOverlayParams& overlay = osd.overlays.get()[i];
    Inject(new OsdTag(0, streaming::kDefaultFlavourMask, 0,
        DestroyOverlayParams(overlay.id.get())));
  }
  for ( uint32 i = 0; i < osd.crawlers.get().size(); i++ ) {
    const CreateCrawlerParams& crawler = osd.crawlers.get()[i];
    Inject(new OsdTag(0, streaming::kDefaultFlavourMask, 0,
        DestroyCrawlerParams(crawler.id.get())));
  }
  for ( uint32 i = 0; i < osd.crawl_items.get().size(); i++ ) {
    const AddCrawlerItemParams& crawler_item = osd.crawl_items.get()[i];
    Inject(new OsdTag(0, streaming::kDefaultFlavourMask, 0,
        RemoveCrawlerItemParams(
            crawler_item.crawler_id.get(),
            crawler_item.item_id.get())));
  }
  if ( osd.pip.is_set() ) {
    Inject(new OsdTag(0, streaming::kDefaultFlavourMask, 0,
        SetPictureInPictureParams("", 0, 0, 0, 0, false)));
  }
  if ( osd.click_url.is_set() ) {
    Inject(new OsdTag(0, streaming::kDefaultFlavourMask, 0,
        SetClickUrlParams("")));
  }
}
void OsdAssociatorClientElement::ClientCallbackData::SetOsd(
    const OsdAssociatorCompleteOsd& osd) {
  // clear previous state
  ClearOsd();
  CHECK_NULL(osd_);
  // set new state
  InjectCreateAll(osd);
  osd_ = new OsdAssociatorCompleteOsd(osd);
}
void OsdAssociatorClientElement::ClientCallbackData::ClearOsd() {
  if ( osd_ == NULL ) {
    // nothing to do, we have no OSDs set
    return;
  }
  InjectDestroyAll(*osd_);
  delete osd_;
  osd_ = NULL;
}
void OsdAssociatorClientElement::ClientCallbackData::FilterTag(
    const Tag* tag,
    int64 timestamp_ms,
    TagList* out) {
  // Always forward the incoming tag.
  // Some OSD tags may be appended.
  out->push_back(FilteredTag(tag, timestamp_ms));

  if ( tag->type() == streaming::Tag::TYPE_EOS ) {
    ClearOsd();
  }

  for ( uint32 i = 0; i < pending_osd_tags_.size(); i++ ) {
    OsdTag* osd = pending_osd_tags_[i].get();
    osd->set_flavour_mask(flavour_mask());
    osd->set_timestamp_ms(timestamp_ms);
    LOG_DEBUG << "Injected OSD (" << media_name_ << "): " << osd->ToString();
    out->push_back(FilteredTag(osd, timestamp_ms));
  }
  pending_osd_tags_.clear();

  if ( tag->type() == streaming::Tag::TYPE_EOS ) {
    // move EOS tag at the end of "tags" list
    FilteredTag t = *out->begin();
    out->pop_front();
    out->push_back(t);
  }
}

///////////////////////////////////////////////////////////////////////////////

OsdAssociatorClientElement::OsdServer::OsdServer(
    net::Selector* selector,
    const http::ClientParams* http_params,
    const vector<net::HostPort>& servers,
    int num_retries,
    int32 request_timeout_ms,
    int32 reopen_connection_interval_ms,
    rpc::CodecId codec,
    const string& http_request_path,
    const string& auth_user,
    const string& auth_pass,
    const string& service_name)
  : net_factory_(selector),
    rpc_connection_(selector, codec,
        new http::FailSafeClient(selector, http_params, servers,
            NewPermanentCallback(&OsdServer::CreateHttpConnection,
                                 selector, &net_factory_,
                                 net::PROTOCOL_TCP),
            num_retries, request_timeout_ms,
            reopen_connection_interval_ms, ""),
        http_request_path, auth_user, auth_pass),
    rpc_server_(rpc_connection_, service_name) {
}
void OsdAssociatorClientElement::OsdServer::ToSpec(
    OsdAssociatorServerSpec* out) const {
  vector<OsdAssociatorHostPort> rpc_addrs;
  const http::FailSafeClient* http_client =
    rpc_connection_.failsafe_client();
  for ( uint32 i = 0; i < http_client->servers().size(); i++ ) {
    const net::HostPort& host_port = http_client->servers()[i];
    rpc_addrs.push_back(OsdAssociatorHostPort(host_port.ip_object().ToString(),
                                           host_port.port()));
  }

  *out = OsdAssociatorServerSpec(rpc_addrs,
      rpc_connection_.http_request_path(),
      rpc_connection_.auth_user(),
      rpc_connection_.auth_pass(),
      rpc_server_.GetServiceName());
}
string OsdAssociatorClientElement::OsdServer::ToString() const {
  return strutil::StringPrintf("OsdServer{"
      "net_factory_: <no-ToString>, "
      "rpc_connection_: %s, "
      "rpc_server_: %s}",
      rpc_connection_.ToString().c_str(),
      rpc_server_.GetServiceName().c_str());
}
///////////////////////////////////////////////////////////////////////////////

const char OsdAssociatorClientElement::kElementClassName[] =
  "osd_associator_client";
const int32 OsdAssociatorClientElement::kCacheMaxSize = 100;
const int64 OsdAssociatorClientElement::kCacheExpirationMs = 5 * 60000;
const OsdAssociatorCompleteOsd OsdAssociatorClientElement::kEmptyCompleteOsd =
    MakeEmptyCompleteOsd();

OsdAssociatorClientElement::OsdAssociatorClientElement(
    const string& name,
    ElementMapper* mapper,
    net::Selector* selector,
    io::StateKeepUser* state_keeper, // becomes ours
    const string& rpc_path,
    rpc::HttpServer* rpc_server)
  : FilteringElement(kElementClassName, name, mapper, selector),
    ServiceInvokerOsdAssociatorClient(GetClassName()),
    rpc_path_(rpc_path),
    rpc_server_(rpc_server),
    state_keeper_(state_keeper),
    http_params_(),
    cache_(util::CacheBase::LRU, kCacheMaxSize, kCacheExpirationMs,
        &util::DefaultValueDestructor, NULL, false),
    clients_() {
}
OsdAssociatorClientElement::~OsdAssociatorClientElement() {
  for ( OsdServerMap::iterator it = osd_server_map_.begin();
        it != osd_server_map_.end(); ++it ) {
    OsdServer* server = it->second;
    delete server;
  }
  osd_server_map_.clear();
}

//static
OsdAssociatorCompleteOsd OsdAssociatorClientElement::MakeEmptyCompleteOsd() {
  OsdAssociatorCompleteOsd empty;
  empty.overlays.ref();
  empty.crawlers.ref();
  empty.crawl_items.ref();
  empty.eos_clear_crawlers = false;
  empty.eos_clear_overlays = false;
  empty.eos_clear_crawl_items = false;
  empty.eos_clear_pip = false;
  empty.eos_clear_click_url = false;
  return empty;
}

//static
void OsdAssociatorClientElement::AddCompleteOsd(OsdAssociatorCompleteOsd& dst,
    const OsdAssociatorCompleteOsd& src) {
  dst.overlays.ref().insert(dst.overlays.ref().end(),
                            src.overlays.get().begin(),
                            src.overlays.get().end());
  dst.crawlers.ref().insert(dst.crawlers.ref().end(),
                            src.crawlers.get().begin(),
                            src.crawlers.get().end());
  dst.crawl_items.ref().insert(dst.crawl_items.ref().end(),
                               src.crawl_items.get().begin(),
                               src.crawl_items.get().end());
  if ( src.pip.is_set() ) {
    dst.pip = src.pip;
  }
  if ( src.click_url.is_set() ) {
    dst.click_url = src.click_url;
  }
  dst.eos_clear_overlays = src.eos_clear_overlays;
  dst.eos_clear_crawlers = src.eos_clear_crawlers;
  dst.eos_clear_crawl_items = src.eos_clear_crawl_items;
  dst.eos_clear_pip = src.eos_clear_pip;
  dst.eos_clear_click_url = src.eos_clear_click_url;
}
OsdAssociatorClientElement::OsdServer*
OsdAssociatorClientElement::CreateOsdServerFromSpec(
    const OsdAssociatorServerSpec& spec, string* out_error) {
  vector<net::HostPort> addrs;
  for ( uint32 i = 0; i < spec.addresses.get().size(); i++ ) {
    const OsdAssociatorHostPort& rpc_host_port = spec.addresses[i];
    net::HostPort addr(rpc_host_port.ip.c_str(),
                       rpc_host_port.port);
    if ( addr.IsInvalid() ) {
      string error = "Invalid address: " + rpc_host_port.ToString() +
                     ". We accept only addresses in IP(numeric) format.";
      LOG_ERROR << error;
      if ( out_error != NULL ) {
        *out_error = error;
      }
      return NULL;
    }
    addrs.push_back(addr);
  }
  return new OsdServer(
      selector_,
      &http_params_,
      addrs,
      kHttpNumRetries,
      kHttpRequestTimeoutMs,
      kHttpReopenConnectionIntervalMs,
      rpc::kCodecIdJson,
      spec.http_path,
      spec.http_auth_user,
      spec.http_auth_pswd,
      spec.rpc_service_name);
}
bool OsdAssociatorClientElement::AddOsdServer(
    const string& media, const OsdAssociatorServerSpec& spec,
    string* out_error) {
  string error;
  OsdServer* server = CreateOsdServerFromSpec(spec, &error);
  if ( server == NULL ) {
    LOG_ERROR << "CreateOsdServerFromSpec failed: " << error;
    if ( out_error != NULL ) {
      *out_error = error;
    }
    return false;
  }
  pair<OsdServerMap::iterator, bool> result = osd_server_map_.insert(
      make_pair(media, server));
  if ( !result.second ) {
    delete server;
    server = NULL;
    const OsdServer* old = result.first->second;
    string error = "Duplicate server on the same media_path: [" +
                   media + "], old server: " + old->ToString();
    LOG_ERROR << error;
    if ( out_error != NULL ) {
      *out_error = error;
    }
    return false;
  }
  return true;
}
bool OsdAssociatorClientElement::DelOsdServer(const string& media,
                                              string* out_error) {
  OsdServerMap::iterator it = osd_server_map_.find(media);
  if ( it == osd_server_map_.end() ) {
    string error = "No server on media path: [" + media + "]";
    LOG_ERROR << error;
    if ( out_error != NULL ) {
      *out_error = error;
    }
    return false;
  }
  OsdServer* server = it->second;
  LOG_INFO << "Deleted server: " << server->ToString()
           << ", from media path: [" << media << "]";
  delete server;
  osd_server_map_.erase(it);
  return true;
}
void OsdAssociatorClientElement::HandleGetRequestOsdResultForClient(
    ClientCallbackData* client,
    const rpc::CallResult<OsdAssociatorRequestOsd>& result) {
  if ( !result.success_ ) {
    LOG_ERROR << "GetRequestOsd failed: " << result.error_;
    return;
  }
  LOG_DEBUG << "GetRequestOsd succeeded, result: " << result.result_;
  // cache global OSD settings
  if ( result.result_.global.is_set() ) {
    cache_.Add(client->creation_path(),
        new OsdAssociatorCompleteOsd(result.result_.global));
  }
  // maybe the client stream which generated this call is gone already.
  if ( clients_.find(client) == clients_.end() ) {
    LOG_WARNING << "Got response for a deleted client, ignoring response..";
    return;
  }
  // inject osd state into client stream
  if ( result.result_.global.is_set() ) {
    client->SetOsd(result.result_.global);
  }
  if ( result.result_.specific.is_set() ) {
    client->SetOsd(result.result_.specific);
  }
}
void OsdAssociatorClientElement::HandleGetRequestOsdResultForMedia(
    rpc::CallContext< OsdAssociatorCompleteOsd >* call,
    const rpc::CallResult<OsdAssociatorRequestOsd>& result) {
  if ( !result.success_ ) {
    LOG_ERROR << "GetRequestOsd failed: " << result.error_;
    call->Complete(kEmptyCompleteOsd);
    return;
  }
  LOG_DEBUG << "GetRequestOsd succeeded, result: " << result.result_;
  // we don't cache these answers.. although we could..

  OsdAssociatorCompleteOsd osd = kEmptyCompleteOsd;
  // append global settings to "osd"
  if ( result.result_.global.is_set() ) {
    AddCompleteOsd(osd, result.result_.global);
  }
  // append specific settings to "osd"
  if ( result.result_.specific.is_set() ) {
    AddCompleteOsd(osd, result.result_.specific);
  }
  call->Complete(osd);
}


string OsdAssociatorClientElement::StateName() const {
  return strutil::StringPrintf("%s_%s", kElementClassName, name().c_str());
}
void OsdAssociatorClientElement::Save() {
  map<string, OsdAssociatorServerSpec> servers_spec;
  for ( OsdServerMap::const_iterator it = osd_server_map_.begin();
        it != osd_server_map_.end(); ++it ) {
    const string& media = it->first;
    OsdServer* server = it->second;

    OsdAssociatorServerSpec spec;
    server->ToSpec(&spec);

    servers_spec.insert(make_pair(media, spec));
  }
  string state_value;
  rpc::JsonEncoder::EncodeToString(servers_spec, &state_value);

  state_keeper_->SetValue(StateName(), state_value);
  LOG_INFO << "Save completed successfully. element_name: [" << name() << "]";
}
bool OsdAssociatorClientElement::Load() {
  string state_value;
  if ( !state_keeper_->GetValue(StateName(), &state_value) ) {
    LOG_ERROR << "state_keeper_ cannot find key: [" << StateName() << "]";
    return false;
  }

  map<string, OsdAssociatorServerSpec> servers_spec;
  if ( !rpc::JsonDecoder::DecodeObject(state_value, &servers_spec) ) {
    LOG_ERROR << "Failed to decode a "
                 "rpc::Map<rpc::String, OsdAssociatorServerSpec> object from "
                 "data: [" << state_value << "]";
    return false;
  }

  for ( map<string, OsdAssociatorServerSpec>::const_iterator it =
            servers_spec.begin(); it != servers_spec.end(); ++it ) {
    const string& media = it->first;
    const OsdAssociatorServerSpec& spec = it->second;
    string error;
    if ( !AddOsdServer(media, spec, &error) ) {
      LOG_ERROR << "Failed to AddServer spec: " << spec << ", error: " << error;
      return false;
    }
  }
  LOG_INFO << "Load completed successfully. element_name: [" << name() << "]";
  return true;
}

string OsdAssociatorClientElement::ToString() const {
  ostringstream oss;
  oss << "OsdAssociatorClientElement{rpc_path_: " << rpc_path_
      << ", osd_server_map_: " << strutil::ToStringP(osd_server_map_)
      << ", cache_: " << cache_.ToString()
      << ", clients_: #" << clients_.size()
      << "}";
  return oss.str();
}

//////////////////////////////////////////////////////////////////

bool OsdAssociatorClientElement::Initialize() {
  if ( rpc_server_ != NULL &&
       !rpc_server_->RegisterService(rpc_path_, this) ) {
    LOG_ERROR << "rpc_server_->RegisterService failed, on path: ["
              << rpc_path_ << "]";
    return false;
  }
  if ( !Load() ) {
    LOG_ERROR << "Failed to load state, assuming clean start...";
  }
  return true;
}
FilteringCallbackData* OsdAssociatorClientElement::CreateCallbackData(
    const string& media_name, streaming::Request* req) {
  ClientCallbackData* client = new ClientCallbackData(media_name);
  clients_.insert(client);

  // first search cache
  string str_media_name(media_name);
  const OsdAssociatorCompleteOsd* osd = cache_.GetPathBased(str_media_name);
  if ( osd != NULL ) {
    client->SetOsd(*osd);
    return client;
  }
  string path_media_name(media_name);
  // find osd server to ask about this client path
  OsdServer* server = io::FindPathBased(&osd_server_map_, path_media_name);
  if ( server == NULL ) {
    LOG_ERROR << "No OSD rpc server for path: [" << media_name << "]";
    return client;
  }

  // encode "req" into a json string
  MediaRequestInfoSpec req_info_spec;
  req->info().ToSpec(&req_info_spec);

  // ask server about OSDs
  server->rpc_server_.GetRequestOsd(NewCallback(this,
      &OsdAssociatorClientElement::HandleGetRequestOsdResultForClient, client),
      req_info_spec);
  return client;
}
void OsdAssociatorClientElement::DeleteCallbackData(
    FilteringCallbackData* data) {
  ClientCallbackData* client = static_cast<ClientCallbackData*>(data);
  // No reason to call:
  //   client->ClearOsd();
  // because we're deleting this client, it won't have a chance to send
  // the injected tags.
  FilteringElement::DeleteCallbackData(client);
}

//////////////////////////////////////////////////////////////////////

void OsdAssociatorClientElement::AddOsdServer(
    rpc::CallContext< OsdAssociatorResult >* call,
    const string& media_path,
    const OsdAssociatorServerSpec& spec) {
  string error;
  const bool success = AddOsdServer(media_path, spec, &error);
  Save();
  call->Complete(OsdAssociatorResult(success, error));
}
void OsdAssociatorClientElement::DelOsdServerByPath(
    rpc::CallContext< OsdAssociatorResult >* call,
    const string& media_path) {
  string error;
  bool success = DelOsdServer(media_path, &error);
  Save();
  call->Complete(OsdAssociatorResult(success, error));
}
void OsdAssociatorClientElement::GetAllOsdServers(
    rpc::CallContext<map<string, OsdAssociatorServerSpec> >* call) {
  map<string, OsdAssociatorServerSpec> rpc_servers;
  for ( OsdServerMap::const_iterator it = osd_server_map_.begin();
        it != osd_server_map_.end(); ++it ) {
    const string& media = it->first;
    const OsdServer* server = it->second;

    OsdAssociatorServerSpec rpc_server;
    server->ToSpec(&rpc_server);

    rpc_servers.insert(make_pair(media, rpc_server));
  }
  call->Complete(rpc_servers);
}
void OsdAssociatorClientElement::GetMediaOsd(
    rpc::CallContext< OsdAssociatorCompleteOsd >* call,
    const string& media) {
  string path_media_name;
  OsdServer* server = io::FindPathBased(&osd_server_map_, path_media_name);
  if ( server == NULL ) {
    LOG_ERROR << "No OSD rpc server for path: [" << media << "]";
    call->Complete(kEmptyCompleteOsd);
    return;
  }

  // make a streaming::RequestInfo and encode it into a json string
  //
  streaming::RequestInfo req_info;
  req_info.path_ = media;

  MediaRequestInfoSpec req_info_spec;
  req_info.ToSpec(&req_info_spec);

  // ask server about OSDs
  server->rpc_server_.GetRequestOsd(NewCallback(this,
      &OsdAssociatorClientElement::HandleGetRequestOsdResultForMedia, call),
      req_info_spec);
}
void OsdAssociatorClientElement::ShowMediaCache(
    rpc::CallContext< OsdAssociatorCompleteOsd >* call,
    const string& media) {
  const OsdAssociatorCompleteOsd* osd = cache_.Get(media);
  if ( osd == NULL ) {
    call->Complete(kEmptyCompleteOsd);
    return;
  }
  call->Complete(*osd);
}
void OsdAssociatorClientElement::ShowAllCache(
    rpc::CallContext< map<string, OsdAssociatorCompleteOsd> >* call) {
  typedef map<string, const OsdAssociatorCompleteOsd*> CacheMap;
  CacheMap cache_map;
  cache_.GetAll(&cache_map);

  map< string, OsdAssociatorCompleteOsd > rpc_cache_map;
  for ( CacheMap::const_iterator it = cache_map.begin();
        it != cache_map.end(); ++it ) {
    const string& media = it->first;
    const OsdAssociatorCompleteOsd* osd = it->second;
    rpc_cache_map.insert(make_pair(media, *osd));
  }
  call->Complete(rpc_cache_map);
}
void OsdAssociatorClientElement::ClearMediaCache(
    rpc::CallContext< rpc::Void >* call,
    const string& media) {
  cache_.Del(media);
  call->Complete();
}
void OsdAssociatorClientElement::ClearAllCache(
    rpc::CallContext< rpc::Void >* call) {
  cache_.Clear();
  call->Complete(rpc::Void());
}
void OsdAssociatorClientElement::DebugToString(
    rpc::CallContext< string >* call) {
  string in_str = ToString();
  //// replace ", " by "<br>"
  //vector<string> p;
  //strutil::SplitString(in_str, ", ", &p);
  //string out_str = strutil::JoinStrings(p, ", <br>");
  call->Complete(in_str);
}

} // namespace streaming
