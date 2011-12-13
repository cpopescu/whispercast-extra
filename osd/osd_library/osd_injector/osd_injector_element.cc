// Copyright 2009 WhisperSoft s.r.l.
// Author: Cosmin Tudorache

#include <whisperlib/net/rpc/lib/codec/json/rpc_json_encoder.h>
#include <whisperstreamlib/rtmp/objects/rtmp_objects.h>
#include <whisperstreamlib/flv/flv_tag.h>
#include "osd/osd_library/osd_injector/osd_injector_element.h"

namespace {

/////////////////////////////////////////////////////////////////////////
//
// A pair of callbacks (upstream, downstream) that injects osd
// tags.
//
class OsdCallbackData : public streaming::FilteringCallbackData {
 public:
  OsdCallbackData()
      : streaming::FilteringCallbackData() {
  }
  virtual ~OsdCallbackData() {
  }

  // Schedules for injection a copy of the given OSD tag.
  void InjectOsdTag(const streaming::OsdTag& tag) {
    if (here_process_tag_callback_ == NULL) {
      CHECK(registered_type_ == streaming::Tag::TYPE_OSD);

      scoped_ref<streaming::Tag> t(tag.Clone());
      t->set_flavour_mask(flavour_mask());
      DLOG_DEBUG << "Directly injected OSD: " << t->ToString();

      CHECK_NOT_NULL(client_process_tag_callback_);
      client_process_tag_callback_->Run(t.get(), 0);
      return;
    }
    pending_osd_tags_.push_back(scoped_ref<streaming::OsdTag>(
        new streaming::OsdTag(tag)));
  }

  // Register our processing callback to upstream element
  virtual bool Register(streaming::Request* req) {
    if (media_name_.empty()) {
      req->mutable_caps()->tag_type_ = streaming::Tag::TYPE_OSD;
      registered_type_ = streaming::Tag::TYPE_OSD;
      return true;
    }
    return streaming::FilteringCallbackData::Register(req);
  }
  virtual bool Unregister(streaming::Request* req) {
    if (media_name_.empty()) {
      registered_type_ = streaming::Tag::kInvalidType;
      return true;
    }
    return streaming::FilteringCallbackData::Unregister(req);
  }

 protected:
  ///////////////////////////////////////////////////////////////////////
  //
  // FilteringCallbackData methods
  //
  virtual void FilterTag(const streaming::Tag* tag,
                         int64 timestamp_ms,
                         TagList* out);

 private:
  // Osd tags to be sent on next media tag.
  vector< scoped_ref<streaming::OsdTag> > pending_osd_tags_;
};


///////////////////////////////////////////////////////////////////////////

void OsdCallbackData::FilterTag(const streaming::Tag* tag,
                                int64 timestamp_ms,
                                TagList* out) {
  // Always forward the incoming original tag.
  // Some OSD tags may be appended.
  out->push_back(FilteredTag(tag, timestamp_ms));

  if ( tag->type() == streaming::Tag::TYPE_EOS ) {
    return;
  }

  for ( uint32 i = 0; i < pending_osd_tags_.size(); i++ ) {
    scoped_ref<streaming::OsdTag>& osd = pending_osd_tags_[i];
    osd->set_flavour_mask(flavour_mask());
    osd->set_timestamp_ms(timestamp_ms);
    LOG_DEBUG << "Injected OSD (" << media_name_ << "): " << osd->ToString();
    out->push_back(FilteredTag(osd.get(), timestamp_ms));
  }
  pending_osd_tags_.clear();

  return;
}
}

////////////////////////////////////////////////////////////////////////////

namespace streaming {

const char OsdInjectorElement::kElementClassName[] = "osd_injector";

OsdInjectorElement::OsdInjectorElement(const char* name,
                                       const char* id,
                                       ElementMapper* mapper,
                                       net::Selector* selector,
                                       const char* rpc_path,
                                       const char* local_rpc_path,
                                       rpc::HttpServer* rpc_server)
    : FilteringElement(kElementClassName, name, id, mapper, selector),
      ServiceInvokerOsdInjector(ServiceInvokerOsdInjector::GetClassName()),
      rpc_path_(rpc_path),
      local_rpc_path_(local_rpc_path),
      rpc_server_(rpc_server),
      is_registered_(false) {
}
OsdInjectorElement::~OsdInjectorElement() {
  if ( is_registered_ ) {
    rpc_server_->UnregisterService(local_rpc_path_, this);
  }
}

bool OsdInjectorElement::Initialize() {
  if ( rpc_server_ != NULL ) {
    is_registered_ = rpc_server_->RegisterService(local_rpc_path_, this);
    if ( is_registered_ && local_rpc_path_ != rpc_path_ ) {
      rpc_server_->RegisterServiceMirror(rpc_path_, local_rpc_path_);
    }
    return is_registered_;
  }
  return true;
}

void OsdInjectorElement::InjectOsdTag(OsdTag* tag) {
  CHECK(selector_->IsInSelectThread());
  for ( FilteringCallbackMap::iterator it = callbacks_.begin();
        it != callbacks_.end(); ++it ) {
    OsdCallbackData* data = static_cast<OsdCallbackData*>(it->second);
    data->InjectOsdTag(*tag);
  }
  LOG_DEBUG << "OSD tag injected in " << callbacks_.size() << " clients.";
  delete tag;
}

FilteringCallbackData* OsdInjectorElement::CreateCallbackData(
    const char* media_name,
    streaming::Request* req) {
  return new OsdCallbackData();
}

void OsdInjectorElement::CreateOverlay(rpc::CallContext<rpc::Void >* call,
                                       const CreateOverlayParams& params) {
  LOG_DEBUG << "RPC OSD: " << params;
  selector_->RunInSelectLoop(
      NewCallback(this, &OsdInjectorElement::InjectOsdTag,
                  new OsdTag(0, streaming::kDefaultFlavourMask, 0, params)));
  call->Complete(rpc::Void());
}
void OsdInjectorElement::DestroyOverlay(rpc::CallContext<rpc::Void>* call,
                                        const DestroyOverlayParams& params) {
  LOG_DEBUG << "RPC OSD: " << params;
  selector_->RunInSelectLoop(
      NewCallback(this, &OsdInjectorElement::InjectOsdTag,
                  new OsdTag(0, streaming::kDefaultFlavourMask, 0, params)));
  call->Complete(rpc::Void());
}
void OsdInjectorElement::ShowOverlays(rpc::CallContext<rpc::Void>* call,
                                      const ShowOverlaysParams& params) {
  LOG_DEBUG << "RPC OSD: " << params;
  selector_->RunInSelectLoop(
      NewCallback(this, &OsdInjectorElement::InjectOsdTag,
                  new OsdTag(0, streaming::kDefaultFlavourMask, 0, params)));
  call->Complete(rpc::Void());
}
void OsdInjectorElement::CreateCrawler(rpc::CallContext<rpc::Void>* call,
                                       const CreateCrawlerParams& params) {
  LOG_DEBUG << "RPC OSD: " << params;
  selector_->RunInSelectLoop(
      NewCallback(this, &OsdInjectorElement::InjectOsdTag,
                  new OsdTag(0, streaming::kDefaultFlavourMask, 0, params)));
  call->Complete(rpc::Void());
}
void OsdInjectorElement::DestroyCrawler(rpc::CallContext<rpc::Void>* call,
                                        const DestroyCrawlerParams& params) {
  LOG_DEBUG << "RPC OSD: " << params;
  selector_->RunInSelectLoop(
      NewCallback(this, &OsdInjectorElement::InjectOsdTag,
                  new OsdTag(0, streaming::kDefaultFlavourMask, 0, params)));
  call->Complete(rpc::Void());
}
void OsdInjectorElement::ShowCrawlers(rpc::CallContext<rpc::Void>* call,
                                      const ShowCrawlersParams& params) {
  LOG_DEBUG << "RPC OSD: " << params;
  selector_->RunInSelectLoop(
      NewCallback(this, &OsdInjectorElement::InjectOsdTag,
                  new OsdTag(0, streaming::kDefaultFlavourMask, 0, params)));
  call->Complete(rpc::Void());
}
void OsdInjectorElement::AddCrawlerItem(rpc::CallContext<rpc::Void>* call,
                                        const AddCrawlerItemParams& params) {
  LOG_DEBUG << "RPC OSD: " << params;
  selector_->RunInSelectLoop(
      NewCallback(this, &OsdInjectorElement::InjectOsdTag,
                  new OsdTag(0, streaming::kDefaultFlavourMask, 0, params)));
  call->Complete(rpc::Void());
}
void OsdInjectorElement::RemoveCrawlerItem(
    rpc::CallContext<rpc::Void>* call,
    const RemoveCrawlerItemParams& params) {
  LOG_DEBUG << "RPC OSD: " << params;
  selector_->RunInSelectLoop(
      NewCallback(this, &OsdInjectorElement::InjectOsdTag,
                  new OsdTag(0, streaming::kDefaultFlavourMask, 0, params)));
  call->Complete(rpc::Void());
}
void OsdInjectorElement::SetPictureInPicture(
    rpc::CallContext<rpc::Void>* call,
    const SetPictureInPictureParams& params) {
  LOG_DEBUG << "RPC OSD: " << params;
  selector_->RunInSelectLoop(
      NewCallback(this, &OsdInjectorElement::InjectOsdTag,
                  new OsdTag(0, streaming::kDefaultFlavourMask, 0, params)));
  call->Complete(rpc::Void());
}
void OsdInjectorElement::SetClickUrl(
    rpc::CallContext<rpc::Void>* call,
    const SetClickUrlParams& params) {
  LOG_DEBUG << "RPC OSD: " << params;
  selector_->RunInSelectLoop(
      NewCallback(this, &OsdInjectorElement::InjectOsdTag,
                  new OsdTag(0, streaming::kDefaultFlavourMask, 0, params)));
  call->Complete(rpc::Void());
}
void OsdInjectorElement::CreateMovie(rpc::CallContext<rpc::Void>* call,
                                     const CreateMovieParams& params) {
  LOG_DEBUG << "RPC OSD: " << params;
  selector_->RunInSelectLoop(
      NewCallback(this, &OsdInjectorElement::InjectOsdTag,
                  new OsdTag(0, streaming::kDefaultFlavourMask, 0, params)));
  call->Complete(rpc::Void());
}
void OsdInjectorElement::DestroyMovie(rpc::CallContext<rpc::Void>* call,
                                      const DestroyMovieParams& params) {
  LOG_DEBUG << "RPC OSD: " << params;
  selector_->RunInSelectLoop(
      NewCallback(this, &OsdInjectorElement::InjectOsdTag,
                  new OsdTag(0, streaming::kDefaultFlavourMask, 0, params)));
  call->Complete(rpc::Void());
}
}
