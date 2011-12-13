// Copyright 2009 WhisperSoft s.r.l.
// Author: Cosmin Tudorache

#ifndef __MEDIA_OSD_STATE_KEEPER_SOURCE_H__
#define __MEDIA_OSD_STATE_KEEPER_SOURCE_H__

#include <string>
#include <map>
#include <whisperlib/net/rpc/lib/server/rpc_services_manager.h>
#include <whisperlib/net/rpc/lib/server/rpc_http_server.h>
#include <whisperlib/common/io/checkpoint/state_keeper.h>
#include <whisperstreamlib/base/filtering_element.h>
#include <whisperstreamlib/base/request.h>

#include "osd/base/auto/osd_types.h"
#include "osd/base/auto/osd_invokers.h"
#include "osd/base/osd_tag.h"


namespace streaming {

// A filtering element that remembers stream osd state and
// syncs new clients by injecting osd tags at stream begin.
// It also provides osd state information through an RPC service.
//
// We don't modify the tags in any way.
// All clients should request the same media name.
// According to FilteringElement behaviour, we forward the client callback to
// the upstream element. So we get a different stream for every client.
// In order to receive a single set of osd tags, we register a dead-end
// callback here, that reads osd tags and updates the current osd state.
// When a new client arrives we send him the osd tags that make the current
// osd state.
//
class OsdStateKeeperElement
  : public FilteringElement, public ServiceInvokerOsdInspector {
 public:
  OsdStateKeeperElement(const char* name,
                        const char* id,
                        ElementMapper* mapper,
                        net::Selector* selector,
                        io::StateKeepUser* state_keeper,   // becomes ours
                        const char* media_name,
                        const char* rpc_path,
                        rpc::HttpServer* rpc_server);
  virtual ~OsdStateKeeperElement();
  static const char kElementClassName[];

  /////////////////////////////////////////////////////////////////
  // FilteringElement methods
  virtual bool Initialize();
  virtual void Close(Closure* call_on_close);

 protected:
  bool is_initialized() {
    return ((rpc_server_ == NULL || is_registered_) &&
            process_tag_callback_ != NULL);
  }
  // We register the ProcessTag with the element_ on path: media_name_ .
  // Returns success status.
  // This function should be called right after constructor.
  void OpenMedia();
  void CloseMedia();

  // Saves the state of the state keeper to persistent storage
  bool SaveState();

  // A dead-end callback, looks like a client callback for the element_.
  // We receive & process osd tags here, in order to update our osd state.
  void ProcessTag(const Tag* tag, int64 timestamp_ms);

  void ProcessOsd(const CreateOverlayParams& fparams);
  void ProcessOsd(const DestroyOverlayParams& fparams);
  void ProcessOsd(const ShowOverlaysParams& fparams);
  void ProcessOsd(const CreateCrawlerParams& fparams);
  void ProcessOsd(const DestroyCrawlerParams& fparams);
  void ProcessOsd(const ShowCrawlersParams& fparams);
  void ProcessOsd(const AddCrawlerItemParams& fparams);
  void ProcessOsd(const RemoveCrawlerItemParams& fparams);
  void ProcessOsd(const SetPictureInPictureParams& fparams);
  void ProcessOsd(const SetClickUrlParams& fparams);
  void ProcessOsd(const CreateMovieParams& fparams);
  void ProcessOsd(const DestroyMovieParams& fparams);

 public:
  // Injects current osd state tags to the given client.
  void AppendBootstrap(FilteringCallbackData::TagList* tags,
                       uint32 flavour_mask,
                       int64 timestamp_ms);

 protected:
  //////////////////////////////////////////////////////////////////
  //
  // FilteringElement methods
  //
  virtual FilteringCallbackData* CreateCallbackData(
      const char* media_name, streaming::Request* req);

  //////////////////////////////////////////////////////////////////
  //
  // ServiceInvokerOsdInspector methods
  //
  void GetOverlays(rpc::CallContext< OverlaysState >* call);
  void GetCrawlers(rpc::CallContext< CrawlersState >* call);
  void GetMovies(rpc::CallContext< MoviesState >* call);
  void GetOverlay(rpc::CallContext< CreateOverlayParams >* call,
                  const string& id);
  void GetCrawler(rpc::CallContext< CreateCrawlerParams >* call,
                  const string& id);
  void GetMovie(rpc::CallContext< CreateMovieParams >* call,
                const string& id);
  void GetPictureInPicture(rpc::CallContext<PictureInPictureState>* call);
  void GetClickUrl(rpc::CallContext<ClickUrlState>* call);

 private:
  const string media_name_;
  streaming::Request* internal_req_;
  ProcessingCallback* process_tag_callback_;

  // map of [overlay id, CreateOverlayParams]
  map<string, CreateOverlayParams*> overlays_;
  ShowOverlaysParams show_overlays_;

  // map of [crawler id, CreateCrawlerParams]
  map<string, CreateCrawlerParams*> crawlers_;
  ShowCrawlersParams show_crawlers_;

  SetPictureInPictureParams pip_;
  SetClickUrlParams click_url_;

  // map of [movie id, CreateMovieParams]
  map<string, CreateMovieParams*> movies_;

  io::StateKeepUser* const state_keeper_;

  const string rpc_path_;
  rpc::HttpServer* const rpc_server_;
  bool is_registered_;

  util::Alarm open_media_alarm_;
 private:
  template <typename FPARAMS>
  void AppendOsd(const FPARAMS& p,
                 FilteringCallbackData::TagList* tags,
                 uint32 flavour_mask,
                 int64 timestamp_ms) {
    tags->push_back(FilteringCallbackData::FilteredTag(
        new OsdTag(0, flavour_mask, timestamp_ms, p), timestamp_ms
    ));
  }

  DISALLOW_EVIL_CONSTRUCTORS(OsdStateKeeperElement);
};
}
#endif  // __MEDIA_OSD_STATE_KEEPER_SOURCE_H__
