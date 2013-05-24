// Copyright 2009 WhisperSoft s.r.l.
// Author: Cosmin Tudorache

#ifndef __MEDIA_OSD_INJECTOR_SOURCE_H__
#define __MEDIA_OSD_INJECTOR_SOURCE_H__

#include <whisperlib/net/rpc/lib/server/rpc_services_manager.h>
#include <whisperlib/net/rpc/lib/server/rpc_http_server.h>
#include <whisperstreamlib/base/filtering_element.h>
#include "osd/base/auto/osd_types.h"
#include "osd/base/auto/osd_invokers.h"
#include "osd/base/osd_tag.h"

namespace streaming {

class OsdInjectorElement;

/////////////////////////////////////////////////////////////////////////
//
// A element that injects control tags.
// It works only for FLV media, and the injected tags are metadata.
//
class OsdInjectorElement : public FilteringElement,
                           public ServiceInvokerOsdInjector {
 public:
  OsdInjectorElement(const string& name,
                     ElementMapper* mapper,
                     net::Selector* selector,
                     const string& rpc_path,
                     const string& local_rpc_path,
                     rpc::HttpServer* rpc_server);
  virtual ~OsdInjectorElement();

  static const char kElementClassName[];

  virtual bool Initialize();

  bool is_registered() const {
    return is_registered_;
  }

 private:
  // Called in selector's context to broadcast the "tag" to all OsdCallbackData
  void InjectOsdTag(OsdTag* tag);

 protected:
  //////////////////////////////////////////////////////////////////

  // FilteringElement methods

  virtual FilteringCallbackData* CreateCallbackData(const string& media_name,
                                                    streaming::Request* req);

  //////////////////////////////////////////////////////////////////

  // ServiceInvokerControlInjector methods

  virtual void CreateOverlay(
      rpc::CallContext<rpc::Void>* call,
      const CreateOverlayParams& params);
  virtual void DestroyOverlay(
      rpc::CallContext<rpc::Void>* call,
      const DestroyOverlayParams& params);
  virtual void ShowOverlays(
      rpc::CallContext<rpc::Void>* call,
      const ShowOverlaysParams& params);
  virtual void CreateCrawler(
      rpc::CallContext<rpc::Void>* call,
      const CreateCrawlerParams& params);
  virtual void DestroyCrawler(
      rpc::CallContext<rpc::Void>* call,
      const DestroyCrawlerParams& params);
  virtual void ShowCrawlers(
      rpc::CallContext<rpc::Void>* call,
      const ShowCrawlersParams& params);
  virtual void AddCrawlerItem(
      rpc::CallContext<rpc::Void>* call,
      const AddCrawlerItemParams& params);
  virtual void RemoveCrawlerItem(
      rpc::CallContext<rpc::Void>* call,
      const RemoveCrawlerItemParams& params);
  virtual void SetPictureInPicture(
      rpc::CallContext<rpc::Void>* call,
      const SetPictureInPictureParams& params);
  virtual void SetClickUrl(
      rpc::CallContext<rpc::Void>* call,
      const SetClickUrlParams& params);
  virtual void CreateMovie(
      rpc::CallContext<rpc::Void>* call,
      const CreateMovieParams& params);
  virtual void DestroyMovie(
      rpc::CallContext<rpc::Void>* call,
      const DestroyMovieParams& params);

 private:
  const string rpc_path_;
  const string local_rpc_path_;
  rpc::HttpServer* const rpc_server_;
  bool is_registered_;
  DISALLOW_EVIL_CONSTRUCTORS(OsdInjectorElement);
};
}

#endif  // __MEDIA_OSD_INJECTOR_SOURCE_H__
