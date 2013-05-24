// Copyright (c) 2009, Whispersoft s.r.l.
// All rights reserved.
//
// Authors: Catalin Popescu

#include <string>
#include <map>

#include <whisperlib/common/base/types.h>
#include <whisperlib/common/base/cache.h>
#include WHISPER_HASH_MAP_HEADER

#include <whisperlib/net/base/address.h>
#include <whisperlib/net/http/failsafe_http_client.h>
#include <whisperlib/net/rpc/lib/client/rpc_failsafe_client_connection_http.h>

#include <whisperstreamlib/base/element.h>
#include <whisperstreamlib/base/element_mapper.h>
#include <whisperstreamlib/elements/factory.h>
#include <whisperstreamlib/elements/factory_based_mapper.h>
#include <whisperstreamlib/elements/auto/factory_wrappers.h>

#ifndef __EXTRA_LIBRARY_AUX_ELEMENT_MAPPER_H__
#define __EXTRA_LIBRARY_AUX_ELEMENT_MAPPER_H__

namespace streaming {

// Use
//  ${RESOURCE} in your strings for requested media
//  ${REQ_QUERY} the request query part (e.g. wsp (seek_pos) and so on)
//  ${AUTH_QUERY} the authorization request query part (e.g. wuname and so on)

class AuxElementMapper
    : public ElementMapper {
 public:
  AuxElementMapper(net::Selector* selector,
                   ElementFactory* factory,
                   ElementMapper* master,
                   rpc::HttpServer* rpc_server,
                   const string& config_file);
  virtual ~AuxElementMapper();

  // ElementMapper interface
  virtual bool AddRequest(const string& media_name,
                          streaming::Request* req,
                          streaming::ProcessingCallback* callback);
  virtual void RemoveRequest(streaming::Request* req,
                             ProcessingCallback* callback);

  virtual bool HasMedia(const string& media);
  virtual void ListMedia(const string& media_dir,
                         vector<string>* medias);

  virtual bool GetElementByName(const string& name,
                                streaming::Element** element,
                                vector<streaming::Policy*>** policies);
  virtual void GetAllElements(vector<string>* out_elements) const;
  virtual bool IsKnownElementName(const string& name) {
    return (extra_element_spec_data_.find(name) !=
            extra_element_spec_data_.end());
  }

  // We don't know any of these:
  virtual void GetMediaDetails(const string& protocol,
                               const string& path,
                               streaming::Request* req,
                               Callback1<bool>* completion_callback);

  virtual Authorizer* GetAuthorizer(const string& name) {
    return NULL;
  }
  virtual bool GetMediaAlias(const string& alias_name,
                             string* media_name) const {
    return false;
  }
  virtual string TranslateMedia(const string& media_name) const {
    return media_name;
  }
  virtual bool DescribeMedia(const string& media, MediaInfoCallback* callback) {
    return false;
  }

  virtual int32 AddExportClient(const string& protocol, const string& path) {
    return 0;
  }
  virtual void RemoveExportClient(const string& protocol, const string& path) {
  }
  virtual bool AddImporter(Importer* importer) {
    return false;
  }
  virtual void RemoveImporter(Importer* importer) {
  }
  virtual Importer* GetImporter(Importer::Type importer_type,
                                const string& path) {
    return NULL;
  }

 private:
  bool AddRequest(const string& media_name,
                  streaming::Request* req,
                  streaming::ProcessingCallback* callback,
                  bool do_lookup);

  bool IsAuxiliaryKnownElement(const string& name) {
    // TODO - we may want to restrict this
    return true;
  }
  void SetDataFromServingInfo(const string& norm_path,
                              streaming::Request* req,
                              const ElementExportSpec* spec);

  struct MediaLookupStruct {
    const string protocol_;
    const string path_;
    streaming::Request* const req_;
    Callback1<bool>* const completion_callback_;
    bool external_lookup_done_;
    rpc::CALL_ID rpc_id_;
    MediaLookupStruct(const string& protocol,
                      const string& path,
                      streaming::Request* req,
                      Callback1<bool>* completion_callback)
        : protocol_(protocol),
          path_(path),
          req_(req),
          completion_callback_(completion_callback),
          external_lookup_done_(false) {
    }
  };
  struct LookupReqStruct {
    const string media_name_;
    streaming::Request* req_;
    streaming::ProcessingCallback* callback_;
    bool is_cancelled_;

    http::ClientRequest* http_request_;

    rpc::CALL_ID rpc_id_;
    LookupReqStruct(const string& media,
                    streaming::Request* req,
                    streaming::ProcessingCallback* callback)
        : media_name_(media),
          req_(req),
          callback_(callback),
          is_cancelled_(false),
          http_request_(NULL),
          rpc_id_(0) {
    }
    ~LookupReqStruct() {
      delete http_request_;
    }
  };

  void MediaRpcLookupCompleted(
      MediaLookupStruct* ls,
      const rpc::CallResult<ElementExportSpec>& response);

  bool AddRequestWithPerRequestElements(
      const string& media_name,
      streaming::Request* req,
      streaming::ProcessingCallback* callback);
  bool StartLookupStruct(
      const string& element_name,
      const string& media_name,
      streaming::Request* req,
      streaming::ProcessingCallback* callback);
  void RpcLookupCompleted(
      LookupReqStruct* lr,
      const rpc::CallResult<MediaRequestConfigSpec>& response);
  void LookupCompleted(LookupReqStruct* lr,
                       const MediaRequestConfigSpec& aux_spec);
  void UpdateElementSpec(const MediaElementSpecs& spec);
  void UpdatePolicySpec(const PolicySpecs& spec);
  void ClosePendingRequest(LookupReqStruct* lr);


  LookupReqStruct* PrepareLookupStruct(
      const string&_media,
      streaming::Request* req,
      streaming::ProcessingCallback* callback);

  ElementFactory* factory_;    // can be used by other guys
  FactoryBasedElementMapper* factory_mapper_;  // same as fallback mapper
  rpc::HttpServer* rpc_server_;

  typedef hash_map<Request*, LookupReqStruct*> LookupOpsMap;
  LookupOpsMap lookup_ops_;
  LookupReqStruct* processing_lr_;   // used internally to
                                     // prevent call-in-call deletion trouble

  map<string, string> extra_element_spec_data_;
  map<string, string> extra_policy_spec_data_;

  ElementSpecMap extra_element_spec_map_;
  PolicySpecMap extra_policy_spec_map_;

  net::NetFactory net_factory_;

  typedef map<string, ServiceWrapperElementConfigService*> ServiceMap;
  //
  ServiceMap element_config_services_;
  ServiceMap media_detail_services_;

  http::ClientParams client_params_;
  ResultClosure<http::BaseClientConnection*>* connection_factory_;

  static const int32 kReopenHttpConnectionIntervalMs = 2000;

  ElementMap global_element_map_;
  PolicyMap  global_policy_map_;

  typedef hash_map<streaming::Request*, ElementSpecMap*> ReqElementSpecMap;
  typedef hash_map<streaming::Request*, PolicySpecMap*> ReqPolicySpecMap;

  ReqElementSpecMap request_element_spec_map_;
  ReqPolicySpecMap request_policy_spec_map_;

  // Cache of export paths
  static const int32 kServingInfoCacheSize = 2000;
  static const int64 kCacheExpirationTimeMs = 60000;

  int64 last_cache_expired_;
  util::Cache<string, const ElementExportSpec*> serving_info_cache_;

  DISALLOW_EVIL_CONSTRUCTORS(AuxElementMapper);
};

}

#endif  // __EXTRA_LIBRARY_AUX_ELEMENT_MAPPER_H__
