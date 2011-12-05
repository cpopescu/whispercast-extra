// Copyright (c) 2009, Whispersoft s.r.l.
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
// * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
// * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
// * Neither the name of Whispersoft s.r.l. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//

#ifndef __MEDIA_OSD_ASSOCIATOR_CLIENT_ELEMENT_H__
#define __MEDIA_OSD_ASSOCIATOR_CLIENT_ELEMENT_H__

#include <whisperlib/common/base/cache.h>
#include <whisperlib/common/io/checkpoint/state_keeper.h>
#include <whisperlib/net/http/failsafe_http_client.h>
#include <whisperlib/net/rpc/lib/codec/json/rpc_json_encoder.h>
#include <whisperlib/net/rpc/lib/server/rpc_services_manager.h>
#include <whisperlib/net/rpc/lib/server/rpc_http_server.h>
#include <whisperlib/net/rpc/lib/client/rpc_failsafe_client_connection_http.h>

#include <whisperstreamlib/base/filtering_element.h>
#include <whisperstreamlib/rtmp/objects/rtmp_objects.h>
#include <whisperstreamlib/flv/flv_tag.h>

#include "osd/base/auto/osd_types.h"
#include "osd/base/auto/osd_invokers.h"
#include "osd/base/auto/osd_client_wrappers.h"
#include "osd/base/osd_tag.h"

namespace streaming {

/////////////////////////////////////////////////////////////////////////
//
// An element that injects OSD tags in stream by request.
// It asks an external RPC service about what OSDs to inject given the
// client request. Received OSDs are cached so that we don't ask too often.
// It works only for FLV media, and the injected tags are metadata.
//

class OsdAssociatorClientElement : public FilteringElement,
                                   public ServiceInvokerOsdAssociatorClient {
 public:
  // The "CallbackData" is associated with every client stream
  class ClientCallbackData : public FilteringCallbackData {
  public:
   ClientCallbackData(const string& creation_path);
   virtual ~ClientCallbackData();

   const string& creation_path() const;

   // We inject the tag and we own it.
   void Inject(OsdTag* tag);

   // Inject osd creation tags according to the "complete" state.
   void InjectCreateAll(const OsdAssociatorCompleteOsd& osd);
   // Inject osd destroyer tags according to the "complete" state.
   void InjectDestroyAll(const OsdAssociatorCompleteOsd& osd);

   // Inject osd tags according to the "complete" state.
   void SetOsd(const OsdAssociatorCompleteOsd& osd);
   // Inject
   void ClearOsd();

   ///////////////////////////////////////////////////////////////////////
   // FilteringCallbackData methods
   virtual void FilterTag(const streaming::Tag* tag, TagList* out);
  private:
   // the stream path
   const string creation_path_;
   // the tags that are waiting to be injected in stream.
   // We will send them with the first FilterTag call.
   vector< scoped_ref<OsdTag> > pending_osd_tags_;
   // the complete OSD state. Contains all overlays, crawlers and items needed.
   // We own it.
   OsdAssociatorCompleteOsd* osd_;
  };
  struct OsdServer {
    net::NetFactory net_factory_;
    rpc::FailsafeClientConnectionHTTP rpc_connection_;
    ServiceWrapperOsdAssociator rpc_server_;

    // Dumb callback to create the HTTP connection.
    static http::BaseClientConnection* CreateHttpConnection(
        net::Selector* selector,
        net::NetFactory* net_factory,
        net::PROTOCOL net_protocol) {
      http::SimpleClientConnection* client =
          new http::SimpleClientConnection(selector,
                                           *net_factory, net_protocol);
      return client;
    }

    void ToSpec(OsdAssociatorServerSpec* out) const;

    OsdServer(net::Selector* selector,
              const http::ClientParams* http_params,
              const vector<net::HostPort>& servers,
              int num_retries,
              int32 request_timeout_ms,
              int32 reopen_connection_interval_ms,
              rpc::CODEC_ID rpc_codec_id,
              const string& http_request_path,
              const string& auth_user,
              const string& auth_pass,
              const string& service_name);

    string ToString() const;
  };
 public:
  static const char kElementClassName[];
  static const int32 kCacheMaxSize;
  static const int64 kCacheExpirationMs;
  static const OsdAssociatorCompleteOsd kEmptyCompleteOsd;

  OsdAssociatorClientElement(const char* name,
                             const char* id,
                             ElementMapper* mapper,
                             net::Selector* selector,
                             io::StateKeepUser* state_keeper, // becomes ours
                             const char* rpc_path,
                             rpc::HttpServer* rpc_server);
  virtual ~OsdAssociatorClientElement();

 private:
  static OsdAssociatorCompleteOsd MakeEmptyCompleteOsd();
  // does: dst += src;
  static void AddCompleteOsd(OsdAssociatorCompleteOsd& dst,
                             const OsdAssociatorCompleteOsd& src);

  OsdServer* CreateOsdServerFromSpec(const OsdAssociatorServerSpec& in,
                                     string* out_error);

  bool AddOsdServer(const string& media,
                    const OsdAssociatorServerSpec& spec,
                    string* out_error);
  bool DelOsdServer(const string& media, string* out_error);

  void HandleGetRequestOsdResultForClient(
      ClientCallbackData* client,
      const rpc::CallResult<OsdAssociatorRequestOsd>& result);

  void HandleGetRequestOsdResultForMedia(
      rpc::CallContext< OsdAssociatorCompleteOsd >* call,
      const rpc::CallResult<OsdAssociatorRequestOsd>& result);

  // we save into state_keeper under this name
  string StateName() const;
  // save to state_keeper
  void Save();
  // load from state_keeper
  bool Load();

  string ToString() const;

  //////////////////////////////////////////////////////////////////
  // FilteringElement methods
  virtual bool Initialize();
  virtual FilteringCallbackData* CreateCallbackData(const char* media_name,
                                                    streaming::Request* req);
  virtual void DeleteCallbackData(FilteringCallbackData* data);

  //////////////////////////////////////////////////////////////////////
  // RPC interface
  virtual void AddOsdServer(rpc::CallContext< OsdAssociatorResult >* call,
                            const string& media_path,
                            const OsdAssociatorServerSpec& server);
  virtual void DelOsdServerByPath(rpc::CallContext< OsdAssociatorResult >* call,
                                  const string& media_path);
  virtual void GetAllOsdServers(
      rpc::CallContext< map< string, OsdAssociatorServerSpec > >* call);

  virtual void GetMediaOsd(rpc::CallContext< OsdAssociatorCompleteOsd >* call,
                           const string& media);
  virtual void ShowMediaCache(rpc::CallContext< OsdAssociatorCompleteOsd >* call,
                              const string& media);
  virtual void ShowAllCache(
      rpc::CallContext< map< string, OsdAssociatorCompleteOsd > >* call);
  virtual void ClearMediaCache(rpc::CallContext< rpc::Void >* call,
                               const string& media);
  virtual void ClearAllCache(rpc::CallContext< rpc::Void >* call);
  virtual void DebugToString(rpc::CallContext< string >* call);

 protected:
  static const int32 kHttpReopenConnectionIntervalMs = 1000;
  static const int32 kHttpRequestTimeoutMs = 5000;
  static const int32 kHttpNumRetries = 2;

  const string rpc_path_;
  rpc::HttpServer* const rpc_server_;
  io::StateKeepUser* state_keeper_;

  // used for all Http connections
  http::ClientParams http_params_;

  // The servers we query for OSD state
  typedef map<string, OsdServer*> OsdServerMap;
  OsdServerMap osd_server_map_;

  // map [request path -> complete OSD]
  util::Cache<string, const OsdAssociatorCompleteOsd*> cache_;

  // stream clients
  set<ClientCallbackData*> clients_;
};
}

inline ostream& operator<<(ostream& os,
    const streaming::OsdAssociatorClientElement::OsdServer& osd_server) {
  return os << osd_server.ToString();
}

#endif  // __MEDIA_OSD_ASSOCIATOR_CLIENT_ELEMENT_H__
