// (c) 2009, Whispersoft s.r.l.
// All rights reserved.
//
// Author: Catalin Popescu
//

#include <string>

#include <whisperlib/common/base/types.h>
#include WHISPER_HASH_SET_HEADER

#include <whisperlib/net/base/selector.h>
#include <whisperstreamlib/base/element.h>
#include <whisperlib/common/base/date.h>
#include <whisperlib/net/rpc/lib/server/rpc_http_server.h>
#include <whisperlib/common/io/checkpoint/state_keeper.h>

#include <whisperstreamlib/flv/flv_tag.h>
#include "extra_library/auto/extra_library_invokers.h"

#ifndef __MEDIA_ELEMENTS_TIME_RANGE_ELEMENT_H__
#define __MEDIA_ELEMENTS_TIME_RANGE_ELEMENT_H__

namespace streaming {

class TimeRangeRequest;

class TimeRangeElement : public Element,
                         public ServiceInvokerTimeRangeElementService {
 public:
  TimeRangeElement(const char* name,
                   const char* id,
                   ElementMapper* mapper,
                   net::Selector* selector,
                   io::StateKeepUser* global_state_keeper,
                   io::StateKeepUser* local_state_keeper,
                   const char* rpc_path,
                   rpc::HttpServer* rpc_server,
                   const char* home_dir,
                   const char* root_media_path,
                   const char* file_prefix,
                   bool utc_requests);
  virtual ~TimeRangeElement();

  static const char kElementClassName[];

  // Element Interface:
  virtual bool Initialize();
  virtual bool AddRequest(const char* media, streaming::Request* req,
                          streaming::ProcessingCallback* callback);
  virtual void RemoveRequest(streaming::Request* req);
  virtual bool HasMedia(const char* media, Capabilities* out);
  virtual void ListMedia(const char* media_dir, ElementDescriptions* medias);
  virtual bool DescribeMedia(const string& media, MediaInfoCallback* callback);
  virtual void Close(Closure* call_on_close);

  // ServiceInvokerTimeRangeElementService Interface
  virtual void GetRange(
      rpc::CallContext<string>* call,
      const string& range_name);
  virtual void GetAllRangeNames(
      rpc::CallContext< vector<string> >* call);
  virtual void SetRange(
      rpc::CallContext<bool>* call,
      const string& range_name,
      const string& range_dates);


 private:
  bool SetDateFromUrlComponent(const string& comp,
                               timer::Date* t);
  bool GetDates(const string& str_dates,
                timer::Date* start_time,
                timer::Date* end_time);

  string info() const {
    return name();
  }

  net::Selector* const selector_;
  io::StateKeepUser* const global_state_keeper_;
  io::StateKeepUser* const local_state_keeper_;
  const string rpc_path_;
  rpc::HttpServer* const rpc_server_;
  const string home_dir_;
  const string root_media_path_;
  const string file_prefix_;
  const bool utc_requests_;
  bool is_registered_;

  static const int64 kCacheExpirationMs = 60000;
  int64 cache_compute_time_;
  streaming::ElementDescriptions medias_cache_;

  typedef hash_map<streaming::Request*, TimeRangeRequest*> ReqMap;
  ReqMap requests_;

  Capabilities default_caps_;

  // called when asynchronous Close completes
  Closure* close_completed_;

  DISALLOW_EVIL_CONSTRUCTORS(TimeRangeElement);
};
}

#endif  // __MEDIA_ELEMENTS_TIME_RANGE_ELEMENT_H__
