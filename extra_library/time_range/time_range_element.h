// (c) 2009, Whispersoft s.r.l.
// All rights reserved.
//
// Author: Catalin Popescu
//

#include <string>

#include <whisperlib/common/base/types.h>
#include <whisperlib/common/base/date.h>
#include <whisperlib/common/io/checkpoint/state_keeper.h>
#include <whisperlib/net/base/selector.h>
#include <whisperlib/net/rpc/lib/server/rpc_http_server.h>
#include <whisperstreamlib/base/element.h>
#include <whisperstreamlib/base/time_range.h>

#include <whisperstreamlib/flv/flv_tag.h>
#include "private/extra_library/auto/extra_library_invokers.h"

#ifndef __MEDIA_ELEMENTS_TIME_RANGE_ELEMENT_H__
#define __MEDIA_ELEMENTS_TIME_RANGE_ELEMENT_H__

namespace streaming {

class TimeRangeRequest;

class TimeRangeElement : public Element,
                         public ServiceInvokerTimeRangeElementService {
 public:
  static const char kElementClassName[];
  TimeRangeElement(const string& name,
                   ElementMapper* mapper,
                   net::Selector* selector,
                   io::StateKeepUser* global_state_keeper,
                   io::StateKeepUser* local_state_keeper,
                   const string& rpc_path,
                   rpc::HttpServer* rpc_server,
                   const string& aio_file_element,
                   uint32 update_media_interval_sec);
  virtual ~TimeRangeElement();

  bool SetRange(const string& range_name, const string& range_dates,
                string* out_error);
  string GetRange(const string& range_name) const;

  //////////////////////////////////////////////////////////////////////////
  // Element Interface:
  virtual bool Initialize();
  virtual bool AddRequest(const string& media, Request* req,
                          ProcessingCallback* callback);
  virtual void RemoveRequest(Request* req);
  virtual bool HasMedia(const string& media);
  virtual void ListMedia(const string& media_dir, vector<string>* medias);
  virtual bool DescribeMedia(const string& media, MediaInfoCallback* callback);
  virtual void Close(Closure* call_on_close);

  //////////////////////////////////////////////////////////////////////////
  void GetAllTimeSpans(vector<TimeSpan>* out_spans);
  void GetAllRangeNames(vector<string>* out_range_names);

  //////////////////////////////////////////////////////////////////////////
  // ServiceInvokerTimeRangeElementService Interface
  virtual void GetAllTimeSpans(rpc::CallContext< vector<TimeSpan> >* call);
  virtual void GetRange(rpc::CallContext<string>* call, const string& name);
  virtual void GetAllRangeNames(rpc::CallContext< vector<string> >* call);
  virtual void SetRange(rpc::CallContext<MediaOpResult>* call,
                        const string& name, const string& dates);

 private:
  // e.g. "20111207-144331" => 07.12.2011 14:43:31
  //      "1329415828" => 02.16.2012 18:10:27 (unix epoch ts in seconds)
  static bool ParseDateTime(const string& comp, int64* out_ts);
  // e.g. "20111207-144331/20111207-145323"
  //      "1329415828/1329428173"
  //      "20111207-144331"  # the second component is unspecified
  static bool ParseDateTimeRange(const string& s,
                                 int64* out_start_ts,
                                 int64* out_end_ts);
  static bool IsValidDateTimeRange(const string& s);

  void UpdateMedia();

  // set the files to play in 'trr'.
  // Returns false if no
  void SetRangeFiles(TimeRangeRequest* trr);

  string info() const { return name(); }

 private:
  net::Selector* const selector_;
  io::StateKeepUser* const global_state_keeper_;
  io::StateKeepUser* const local_state_keeper_;
  const string rpc_path_;
  rpc::HttpServer* const rpc_server_;

  // internal path through an AioFileElement containing the timerange files.
  const string aio_file_element_;

  // All the media files available for this range. Obtained from:
  //  aio_file_element->ListMedia(). Updated from time to time.
  // Relative to 'aio_file_element_'
  vector<string> media_;

  // Updates 'media_' vector from time to time
  util::Alarm update_media_alarm_;
  // seconds between consecutive media updates
  const uint32 update_media_interval_sec_;

  ///////////////////////////////////////////////////////////////////////////

  typedef hash_map<streaming::Request*, TimeRangeRequest*> ReqMap;
  ReqMap requests_;

  // permanent callback to SetRangeFiles(..)
  Callback1<TimeRangeRequest*>* set_range_files_callback_;

  // called when asynchronous Close completes
  Closure* close_completed_;

  DISALLOW_EVIL_CONSTRUCTORS(TimeRangeElement);
};
}

#endif  // __MEDIA_ELEMENTS_TIME_RANGE_ELEMENT_H__
