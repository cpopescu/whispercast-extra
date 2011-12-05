// Copyright (c) 2009, Whispersoft s.r.l.
// All rights reserved.
//
// Author: Catalin Popescu & Cosmin Tudorache
//

#include <whisperlib/common/base/scoped_ptr.h>
#include <whisperlib/net/rpc/lib/codec/json/rpc_json_encoder.h>
#include "extra_library/flavouring/flavouring_element.h"

#define ILOG(level)  LOG(level) << name() << ": "
#define ILOG_DEBUG   ILOG(LDEBUG)
#define ILOG_INFO    ILOG(LINFO)
#define ILOG_WARNING ILOG(LWARNING)
#define ILOG_ERROR   ILOG(LERROR)
#define ILOG_FATAL   ILOG(LFATAL)

namespace streaming {

const char FlavouringElement::kElementClassName[] = "flavouring";

bool FlavouringElement::RequestStruct::Register(FlavourStruct* fs,
    string media) {
  uint32 reg_flavour = (fs->flavour_mask_ & req_->caps().flavour_mask_);
  CHECK(reg_flavour != 0);

  const string up_media = strutil::JoinPaths(fs->media_, media);
  Request* up_req = new Request(*req_);
  up_req->mutable_caps()->flavour_mask_ = kDefaultFlavourMask;
  URL up_url("http://x/" + up_media);
  up_req->SetFromUrl(up_url);

  SourceReg* sr = new SourceReg(fs, up_req, NULL);
  ProcessingCallback* up_callback = NewPermanentCallback(
      this, &FlavouringElement::RequestStruct::ProcessTag, sr);
  sr->up_callback_ = up_callback;

  if ( !mapper_->AddRequest(up_media.c_str(), up_req, up_callback) ) {
    ILOG_ERROR << "Failed to register to media: [" << up_media << "]";
    delete up_req;
    delete sr;
    return false;
  }
  internal_[fs] = sr;
  fs->Add(this);
  return true;
}

void FlavouringElement::RequestStruct::Unregister(FlavourStruct* fs,
                                                  bool send_eos) {
  SourceRegMap::iterator it = internal_.find(fs);
  if ( it == internal_.end() ) {
    ILOG_ERROR << "Cannot unregister from: " << fs->ToString()
              << ", Not found";
    return;
  }
  internal_.erase(it);
  SourceReg* rs = it->second;
  scoped_ptr<SourceReg> auto_del_rs(rs);

  // unregister from upstream
  mapper_->RemoveRequest(rs->up_req_, rs->up_callback_);

  // send EOS downstream for the removed flavour
  if ( send_eos ) {
    callback_->Run(scoped_ref<Tag>(new EosTag(
        0, fs->flavour_mask_, 0, true)).get());
  }

  // no longer registered to this flavour
  fs->Del(this);
}

void FlavouringElement::RequestStruct::UnregisterAll(bool send_eos) {
  while ( !internal_.empty() ) {
    uint32 size = internal_.size();
    Unregister(internal_.begin()->first, send_eos);
    CHECK_EQ(size, internal_.size() + 1);
  }
}

void FlavouringElement::RequestStruct::ProcessTag(SourceReg* sr,
    const Tag* tag) {
  // change flavour_mask to the requested flavour. Avoid cloning if the tag
  // already has the requested flavour.
  scoped_ref<const Tag> to_send = tag;
  if ( tag->flavour_mask() != sr->fs_->flavour_mask_ ) {
    Tag* clone = tag->Clone(-1);
    clone->set_flavour_mask(sr->fs_->flavour_mask_);
    to_send = clone;
  }

  // handle EOS
  if ( tag->type() == Tag::TYPE_EOS ) {
    ILOG_INFO << "Forward EOS";
    mapper_->RemoveRequest(sr->up_req_, sr->up_callback_);
    internal_.erase(sr->fs_);
    sr->fs_->Del(this);
    delete sr;
  }

  // forward tag
  callback_->Run(to_send.get());
}

////////////////////////////////////////////////////////////////////////

FlavouringElement::FlavouringElement(
    const string& my_name,
    const string& id,
    ElementMapper* mapper,
    net::Selector* selector,
    const string& rpc_path,
    rpc::HttpServer* rpc_server,
    io::StateKeepUser* global_state_keeper,
    const vector< pair<string, uint32> >& flav)
    : Element(kElementClassName, my_name.c_str(), id.c_str(), mapper),
      ServiceInvokerFlavouringElementService(
          ServiceInvokerFlavouringElementService::GetClassName()),
      selector_(selector),
      rpc_path_(rpc_path),
      rpc_server_(rpc_server),
      global_state_keeper_(global_state_keeper),
      close_completed_(NULL) {
  for ( int i = 0; i < flav.size(); ++i ) {
    if ( !AddFlav(flav[i].second, flav[i].first, NULL) ) {
      ILOG_FATAL << "Overlapping flavours!";
    }
  }
}

FlavouringElement::~FlavouringElement() {
  CHECK(req_map_.empty()) << " Call Close() before deleting this element";
  for ( FlavourMap::iterator it = flavours_.begin();
        it != flavours_.end(); ++it ) {
    delete it->second;
  }
  flavours_.clear();
}

bool FlavouringElement::Load() {
  if ( global_state_keeper_ == NULL ) {
    ILOG_ERROR << "Failed to Load, NULL state keeper";
  }

  string str_spec;
  if ( !global_state_keeper_->GetValue("FlavouringElementState", &str_spec) ) {
    ILOG_ERROR << "Failed to Load, State not found! assuming clean start";
    return true;
  }
  FlavouringElementSpec spec;
  if ( !rpc::JsonDecoder::DecodeObject(str_spec, &spec) ) {
    ILOG_ERROR << "Failed to Load, cannot decode json spec: " << str_spec;
    return false;
  }
  for ( uint32 i = 0; i < spec.flavours_.size(); i++ ) {
    if ( !AddFlav(spec.flavours_[i].flavour_mask_.get(),
                  spec.flavours_[i].media_prefix_.get(), NULL) ) {
      ILOG_ERROR << "Cannot add flavour from spec: "
                << spec.flavours_[i].ToString();
    }
  }
  return true;
}
bool FlavouringElement::Save() {
  if ( global_state_keeper_ == NULL ) {
    ILOG_ERROR << "Failed to Save, NULL state keeper";
    return false;
  }
  if ( !global_state_keeper_->SetValue("FlavouringElementState",
                                       GetElementConfig()) ) {
    ILOG_ERROR << "Failed to Save, state keeper SetValue failed";
    return false;
  }
  return true;
}

bool FlavouringElement::AddFlav(uint32 flavour_mask, const string& media,
    string* out_err) {
  // check that the new flavour does not overlap with existing flavours
  for ( FlavourMap::const_iterator it = flavours_.begin();
        it != flavours_.end(); ++it ) {
    FlavourStruct* fs = it->second;
    if ( (fs->flavour_mask_ & flavour_mask) != 0 ) {
      string err = strutil::StringPrintf("Cannot AddFlavour: %d, overlaps with"
          " existing: %s", flavour_mask, fs->ToString().c_str());
      ILOG_ERROR << err;
      if ( out_err != NULL ) { *out_err = err; }
      return false;
    }
  }
  // simply add
  flavours_[flavour_mask] = new FlavourStruct(flavour_mask, media);
  Save();
  return true;
}

bool FlavouringElement::DelFlav(uint32 flavour_mask, string* out_err) {
  // find the flavour
  FlavourMap::iterator it = flavours_.find(flavour_mask);
  if ( it == flavours_.end() ) {
    string err = strutil::StringPrintf("Flavour %d not found", flavour_mask);
    ILOG_ERROR << err;
    if ( out_err != NULL ) { *out_err = err; }
    return false;
  }
  FlavourStruct* fs = it->second;
  flavours_.erase(it);

  // send EOS to the clients registered to this flavour
  for ( set<RequestStruct*>::iterator it = fs->rs_.begin();
        it != fs->rs_.end(); ++it ) {
    RequestStruct* rs = *it;
    rs->Unregister(fs, true);
  }
  Save();
  return true;
}

void FlavouringElement::InternalClose(Closure* call_on_close) {
  // no clients? => signal close completed now
  if ( req_map_.empty() ) {
    ILOG_INFO << "No clients, completing close..";
    selector_->RunInSelectLoop(call_on_close);
    return;
  }

  // keep a reference to close_completed
  CHECK_NULL(close_completed_);
  close_completed_ = call_on_close;

  // send EOS to all clients
  vector<RequestStruct*> clients;
  for ( ReqMap::iterator it = req_map_.begin(); it != req_map_.end(); ++it ) {
    clients.push_back(it->second);
  }
  for ( uint32 i = 0; i < clients.size(); ++i ) {
    RequestStruct* client = clients[i];
    // TODO(cosmin): Possible bug: send EOS on a single flavour!
    uint32 mask = client->req_->caps().flavour_mask_;
    client->callback_->Run(scoped_ref<Tag>(new EosTag(
        0, 1 << RightmostFlavourId(mask), 0, true)).get());
  }

  // NEXT: As a result of all the EOSes sent, each client must RemoveRequest()
  //       from us. When all clients left, only then we signal close_completed.
}

bool FlavouringElement::Initialize() {
  bool success = true;
  if ( rpc_server_ != NULL && !rpc_server_->RegisterService(rpc_path_, this) ) {
    ILOG_ERROR << "Failed to register to RPC server";
    success = false;
  }
  if ( !Load() ) {
    success = false;
  }
  return success;
}

bool FlavouringElement::AddRequest(const char* media,
                                   Request* req,
                                   ProcessingCallback* callback) {
  pair<string, string> media_pair = strutil::SplitFirst(media, '/');
  if ( media_pair.first != name() ) {
    return false;
  }
  uint32 flavour_mask = 0;
  RequestStruct* rs = new RequestStruct(mapper_, req, callback, name());
  for ( FlavourMap::iterator it = flavours_.begin();
        it != flavours_.end(); ++it ) {
    FlavourStruct* fs = it->second;
    const uint32 crt_flavour = (fs->flavour_mask_ &
                                req->caps().flavour_mask_);
    // if the request qualifies for this flavour, add a request to upstream
    // source
    if ( crt_flavour == 0 ) {
      continue; // req incompatible with current fs
    }
    if ( !rs->Register(fs, media_pair.second) ) {
      ILOG_ERROR << "Cannot register to media: [" << media_pair.second << "]";
      continue;
    }
    flavour_mask |= crt_flavour;
  }
  // if the request was not registered to any flavours (either because it
  // didn't match any flavour, or because mapper_->AddRequest failed) delete it
  if ( rs->internal_.empty() ) {
    ILOG_ERROR << "Cannot register, "
               << (flavour_mask == 0 ? "no matching flavours" : "wrong media")
               << ", req: " << req->ToString() << ", media: [" << media << "]";
    delete rs;
    return false;
  }
  // ok: request registered to at least 1 upstream media
  CHECK(!rs->internal_.empty());
  req_map_[req] = rs;
  req->mutable_caps()->flavour_mask_ = flavour_mask;
  return true;
}

void FlavouringElement::RemoveRequest(streaming::Request* req) {
  ReqMap::iterator const it = req_map_.find(req);
  if ( it == req_map_.end() ) {
    LOG_ERROR << "RemoveRequest cannot find Request: " << req;
    return;
  }
  RequestStruct* rs = it->second;
  req_map_.erase(it);

  rs->UnregisterAll(false);
  delete rs;

  // if all clients left and we are closing -> signal close completed
  if ( req_map_.empty() && close_completed_ != NULL ) {
    ILOG_INFO << "All clients left, completing close..";
    selector_->RunInSelectLoop(close_completed_);
    close_completed_ = NULL;
  }
}

bool FlavouringElement::HasMedia(const char* media, Capabilities* out) {
  pair<string, string> media_pair = strutil::SplitFirst(media, '/');
  if ( media_pair.first != name() ) {
    return false;
  }
  streaming::Capabilities caps(Tag::kAnyType, kAllFlavours);
  uint32 flavour_mask = 0;
  for ( FlavourMap::const_iterator it = flavours_.begin();
        it != flavours_.end(); ++it ) {
    const FlavourStruct* fs = it->second;
    const string crt = strutil::JoinMedia(fs->media_, media_pair.second);
    streaming::Capabilities crt_caps;
    if ( !mapper_->HasMedia(crt.c_str(), &crt_caps) ) {
      return false;
    }
    caps.IntersectCaps(crt_caps);
    caps.flavour_mask_ = kAllFlavours;

    flavour_mask |= crt_caps.flavour_mask_;
  }
  caps.flavour_mask_ = flavour_mask;
  *out = caps;
  return true;
}

void FlavouringElement::ListMedia(const char* media_dir,
                                  ElementDescriptions* media) {
  pair<string, string> media_pair = strutil::SplitFirst(media_dir, '/');
  if ( media_pair.first != name() ) {
    return;
  }
  hash_map<string, streaming::Capabilities> found_media;
  for ( FlavourMap::const_iterator it = flavours_.begin();
        it != flavours_.end(); ++it ) {
    const FlavourStruct* fs = it->second;
    const string crt = strutil::JoinMedia(fs->media_, media_pair.second);
    ElementDescriptions m;
    mapper_->ListMedia(crt.c_str(), &m);

    for ( int j = 0; j < m.size(); ++j ) {
      const string crt_name = name() + "/" + m[j].first.substr(
          fs->media_.size());
      hash_map<string, streaming::Capabilities>::iterator
          it = found_media.find(crt_name);
      if ( it == found_media.end() ) {
        found_media.insert(make_pair(crt_name, m[j].second));
      } else {
        const uint32 sum_flavours = fs->flavour_mask_ |
                                    it->second.flavour_mask_;
        it->second.IntersectCaps(m[j].second);
        it->second.flavour_mask_ = sum_flavours;
      }
    }
  }
  for ( hash_map<string, streaming::Capabilities>::const_iterator
            it = found_media.begin(); it != found_media.end(); ++it ) {
    media->push_back(*it);
  }
}
bool FlavouringElement::DescribeMedia(const string& media,
                                      MediaInfoCallback* callback) {
  // Unfortunately, in order to find the media, a flavour is needed.
  return false;
}

void FlavouringElement::Close(Closure* call_on_close) {
  selector_->RunInSelectLoop(NewCallback(this,
      &FlavouringElement::InternalClose, call_on_close));
}

string FlavouringElement::GetElementConfig() {
  FlavouringElementSpec spec;
  spec.flavours_.ref();
  for ( FlavourMap::const_iterator it = flavours_.begin();
        it != flavours_.end(); ++it ) {
    FlavourStruct* fs = it->second;
    spec.flavours_.push_back(FlavouringSpec(fs->flavour_mask_, fs->media_));
  }
  string element_spec;
  rpc::JsonEncoder::EncodeToString(spec, &element_spec);
  return element_spec;
}

void FlavouringElement::AddFlavour(
    rpc::CallContext< MediaOperationErrorData >* call,
    const FlavouringSpec& spec) {
  string err;
  if ( !AddFlav(spec.flavour_mask_.get(), spec.media_prefix_.get(), &err) ) {
    call->Complete(MediaOperationErrorData(1, err));
    return;
  }
  call->Complete(MediaOperationErrorData(0, ""));
}
void FlavouringElement::DelFlavour(
    rpc::CallContext< MediaOperationErrorData >* call,
    int32 flavour_mask) {
  string err;
  if ( !DelFlav(flavour_mask, &err) ) {
    call->Complete(MediaOperationErrorData(1,err));
    return;
  }
  call->Complete(MediaOperationErrorData(0, ""));
}
void FlavouringElement::GetFlavours(
    rpc::CallContext< vector< FlavouringSpec > >* call) {
  vector< FlavouringSpec > result;
  for ( FlavourMap::const_iterator it = flavours_.begin();
        it != flavours_.end(); ++it ) {
    FlavourStruct* fs = it->second;
    result.push_back(FlavouringSpec(fs->flavour_mask_, fs->media_));
  }
  call->Complete(result);
}
void FlavouringElement::SetFlavours(
    rpc::CallContext< MediaOperationErrorData >* call,
    const vector<FlavouringSpec>& new_flav) {
  // check sanity of the new_flav
  uint32 all_flav = 0;
  for ( uint32 i = 0; i < new_flav.size(); i++ ) {
    if ( (all_flav & new_flav[i].flavour_mask_.get()) != 0 ) {
      string err = "Overlapping flavour in: " + strutil::ToString(new_flav);
      ILOG_ERROR << err;
      call->Complete(MediaOperationErrorData(1,err));
      return;
    }
    all_flav |= new_flav[i].flavour_mask_.get();
  }

  // erase all existing flavours
  while ( !flavours_.empty() ) {
    uint32 begin_size = flavours_.size();
    bool success = DelFlav(flavours_.begin()->first, NULL);
    CHECK(success) << " Cannot delete flavour!";
    uint32 end_size = flavours_.size();
    CHECK_EQ(begin_size, end_size + 1);
  }

  // add new flavours
  for ( uint32 i = 0; i < new_flav.size(); i++ ) {
    bool success = AddFlav(new_flav[i].flavour_mask_.get(),
                           new_flav[i].media_prefix_.get(), NULL);
    CHECK(success) << " Curious thing, sanity check did pass";
  }

  call->Complete(MediaOperationErrorData(0, ""));
}

}
