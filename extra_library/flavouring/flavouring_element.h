// Copyright (c) 2009, Whispersoft s.r.l.
// All rights reserved.
//
// Author: Catalin Popescu & Cosmin Tudorache


#ifndef __MEDIA_BASE_MEDIA_FLAVOURING_ELEMENT_H__
#define __MEDIA_BASE_MEDIA_FLAVOURING_ELEMENT_H__

#include <whisperlib/common/base/types.h>
#include <whisperlib/common/io/checkpoint/state_keeper.h>
#include <whisperlib/net/base/selector.h>
#include <whisperlib/net/rpc/lib/server/rpc_http_server.h>
#include <whisperstreamlib/base/element.h>
#include "../auto/extra_library_types.h"
#include "../auto/extra_library_invokers.h"

namespace streaming {

class FlavouringElement
    : public Element,
      public ServiceInvokerFlavouringElementService {
 private:
  struct FlavourStruct;

  // associated with the downstream client
  struct RequestStruct {
    ElementMapper* mapper_;

    // downstream client
    Request* req_;
    ProcessingCallback* callback_;

    // just for logging
    const string name_;

    // associated with 1 upstream source
    struct SourceReg {
      FlavourStruct* fs_;
      Request* up_req_;
      ProcessingCallback* up_callback_;
      SourceReg(FlavourStruct* fs, Request* up_req,
                ProcessingCallback* up_callback)
        : fs_(fs), up_req_(up_req), up_callback_(up_callback) {}
      ~SourceReg() {
        delete up_callback_;
      }
    };

    // upstream requests (multiple streams, all are sent to one
    // downstream client). Map by flavour.
    typedef map<FlavourStruct*, SourceReg*> SourceRegMap;
    SourceRegMap internal_;

    RequestStruct(ElementMapper* mapper, Request* req,
                  ProcessingCallback* callback, const string& name)
       : mapper_(mapper), req_(req), callback_(callback), name_(name) {}
    ~RequestStruct() {
      CHECK(internal_.empty());
    }
    const string& name() const { return name_; }

    // register/unregister upstream
    bool Register(FlavourStruct* fs, string media);
    void Unregister(FlavourStruct* fs, bool send_eos);
    void UnregisterAll(bool send_eos);

    // all upstream sources (we're registered to) call this function.
    void ProcessTag(SourceReg* sr, const Tag* tag, int64 timestamp_ms);
  };

  // associated with the upstream source
  struct FlavourStruct {
    uint32 flavour_mask_;
    string media_;
    // those who read from media_
    set<RequestStruct*> rs_;
    FlavourStruct(uint32 flavour_mask, string media )
      : flavour_mask_(flavour_mask), media_(media) {}
    ~FlavourStruct() {
      CHECK(rs_.empty());
    }
    // manage clients which are registered to this source
    void Add(RequestStruct* rs) {
      rs_.insert(rs);
    }
    void Del(RequestStruct* rs) {
      rs_.erase(rs);
    }
    string ToString() const {
      return strutil::StringPrintf("FlavourStruct{flavour_mask_: %d"
          ", media_: %s}", flavour_mask_, media_.c_str());
    }
  };

 public:
  FlavouringElement(const string& name,
                    const string& id,
                    ElementMapper* mapper,
                    net::Selector* selector,
                    const string& rpc_path,
                    rpc::HttpServer* rpc_server,
                    io::StateKeepUser* global_state_keeper,
                    const vector< pair<string, uint32> >& flav);
  virtual ~FlavouringElement();

  static const char kElementClassName[];

 private:
  bool Load();
  bool Save();

  bool AddFlav(uint32 flavour_mask, const string& media, string* out_err);
  bool DelFlav(uint32 flavour_mask, string* out_err);

  void InternalClose(Closure* call_on_close);

 public:
  //////////////////////////////////////////////////////////////////
  // Element interface methods
  virtual bool Initialize();
  virtual bool AddRequest(const char* media, streaming::Request* req,
                          streaming::ProcessingCallback* callback);
  virtual void RemoveRequest(streaming::Request* req);
  virtual bool HasMedia(const char* media, Capabilities* out);
  virtual void ListMedia(const char* media_dir, ElementDescriptions* media);
  virtual bool DescribeMedia(const string& media, MediaInfoCallback* callback);
  virtual void Close(Closure* call_on_close);
  virtual string GetElementConfig();

  //////////////////////////////////////////////////////////////////
  // ServiceInvokerFlavouringElementService methods
  virtual void AddFlavour(rpc::CallContext< MediaOperationErrorData >* call,
                          const FlavouringSpec& spec);
  virtual void DelFlavour(rpc::CallContext< MediaOperationErrorData >* call,
                          int32 flavour_mask);
  virtual void GetFlavours(rpc::CallContext< vector< FlavouringSpec > >* call);
  virtual void SetFlavours(rpc::CallContext< MediaOperationErrorData >* call,
                           const vector< FlavouringSpec >& flavours);

 private:
  net::Selector* const selector_;
  const string rpc_path_;
  rpc::HttpServer* rpc_server_;
  io::StateKeepUser* global_state_keeper_;

  // This is the whole state.
  // An association of flavour_mask -> upstream media.
  typedef map<uint32, FlavourStruct*> FlavourMap;
  FlavourMap flavours_;

  // runtime clients, map by client request
  typedef map<Request*, RequestStruct*> ReqMap;
  ReqMap req_map_;

  Closure* close_completed_;

  DISALLOW_EVIL_CONSTRUCTORS(FlavouringElement);
};
}

#endif  // __MEDIA_BASE_MEDIA_FLAVOURING_ELEMENT_H__
