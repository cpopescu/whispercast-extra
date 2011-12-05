// Copyright 2009 WhisperSoft s.r.l.
// Author: Catalin Popescu

#ifndef __MEDIA_OSD_ASSOCIATOR_ELEMENT_H__
#define __MEDIA_OSD_INJECTOR_SOURCE_H__

#include <whisperlib/net/rpc/lib/codec/json/rpc_json_encoder.h>
#include <whisperstreamlib/rtmp/objects/rtmp_objects.h>
#include <whisperstreamlib/flv/flv_tag.h>
#include <whisperlib/common/io/checkpoint/state_keeper.h>

#include <whisperlib/net/rpc/lib/server/rpc_services_manager.h>
#include <whisperlib/net/rpc/lib/server/rpc_http_server.h>
#include <whisperstreamlib/base/filtering_element.h>
#include "osd/base/auto/osd_types.h"
#include "osd/base/auto/osd_invokers.h"
#include "osd/base/osd_tag.h"

namespace streaming {
class OsdAssocCallbackData;
struct ClipOsds;
/////////////////////////////////////////////////////////////////////////
//
// A element that injects control tags based on some saved parameters tha
// associates some osds to each media.
// It works only for FLV media, and the injected tags are metadata.
//
class OsdAssociatorElement : public FilteringElement,
                             public ServiceInvokerOsdAssociator {
 public:
  OsdAssociatorElement(const char* name,
                       const char* id,
                       ElementMapper* mapper,
                       net::Selector* selector,
                       io::StateKeepUser* state_keeper,   // becomes ours
                       const char* rpc_path,
                       rpc::HttpServer* rpc_server);
  virtual ~OsdAssociatorElement();

  static const char kElementClassName[];

  virtual bool Initialize();

  //////////////////////////////////////////////////////////////////////

  // RPC interface

  virtual void CreateOverlayType(
      rpc::CallContext< rpc::Void >* call,
      const CreateOverlayParams& params);
  virtual void DestroyOverlayType(
      rpc::CallContext< rpc::Void >* call,
      const DestroyOverlayParams& params);
  virtual void GetOverlayType(
      rpc::CallContext< CreateOverlayParams >* call,
      const string& id);
  virtual void GetAllOverlayTypeIds(
      rpc::CallContext< vector<string> >* call);
  virtual void CreateCrawlerType(
      rpc::CallContext< rpc::Void >* call,
      const CreateCrawlerParams& params);
  virtual void DestroyCrawlerType(
      rpc::CallContext< rpc::Void >* call,
      const DestroyCrawlerParams& params);
  virtual void GetCrawlerType(
      rpc::CallContext< CreateCrawlerParams >* call,
      const string& id);
  virtual void GetAllCrawlerTypeIds(
      rpc::CallContext< vector<string> >* call);
  virtual void GetAssociatedOsds(
      rpc::CallContext< AssociatedOsds >* call,
      const string& media);
  virtual void SetAssociatedOsds(
      rpc::CallContext< rpc::Void >* call,
      const AssociatedOsds& osds);
  virtual void DeleteAssociatedOsds(
      rpc::CallContext< rpc::Void >* call,
      const string& media);
  virtual void GetFullState(
      rpc::CallContext< FullOsdState >* call);
  virtual void SetFullState(
      rpc::CallContext< rpc::Void >* call,
      const FullOsdState& state);
  virtual void GetRequestOsd(
      rpc::CallContext< OsdAssociatorRequestOsd >* call,
      const MediaRequestInfoSpec& request);

 public:
  // the functions who do the actual things from above..
  void SetAssociatedOsds(const AssociatedOsds& osds);
  void DeleteAssociatedOsds(const string& media);
  void CreateOverlayType(const CreateOverlayParams& params);
  void DestroyOverlayType(const string& id);
  void CreateCrawlerType(const CreateCrawlerParams& params);
  void DestroyCrawlerType(const string& id);
  void GetFullState(FullOsdState* state);
  void SetFullState(const FullOsdState& state, bool just_add);
  void GetRequestOsd(const MediaRequestInfoSpec& request,
                     OsdAssociatorRequestOsd* out_osd) const;


  ClipOsds* GetMediaOsds(const string& media);
  const ClipOsds* GetMediaOsds(const string& media) const;
  void AddUserForMedia(const string& media, OsdAssocCallbackData* data);
  void DelUserFromMedia(const string& media, OsdAssocCallbackData* data);

  //////////////////////////////////////////////////////////////////

  // FilteringElement methods

  virtual FilteringCallbackData* CreateCallbackData(const char* media_name,
                                                    streaming::Request* req);
  virtual void DeleteCallbackData(FilteringCallbackData* data);


 protected:
  // Loads the state from the persistent storage
  bool LoadState();

  io::StateKeepUser* const state_keeper_;
  const string rpc_path_;
  rpc::HttpServer* const rpc_server_;
  bool is_registered_;

  typedef map<string, CreateOverlayParams*> OverlayMap;
  OverlayMap overlay_types_;
  typedef map<string, CreateCrawlerParams*> CrawlerMap;
  CrawlerMap crawler_types_;

  typedef map<string, ClipOsds*> OsdMap;
  OsdMap associated_osds_;
  typedef set<OsdAssocCallbackData*> AssocSet;
  typedef map<string, AssocSet*> AssocMap;
  AssocMap active_requests_;
  AssocSet all_requests_;
};
}

#endif  // __MEDIA_OSD_ASSOCIATOR_ELEMENT_H__
