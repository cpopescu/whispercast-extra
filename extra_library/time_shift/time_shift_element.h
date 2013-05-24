// (c) 2012, Whispersoft s.r.l.
// All rights reserved.
//
// Author: Cosmin Tudorache
//

#include <list>
#include <whisperlib/common/base/types.h>
#include <whisperlib/common/base/date.h>
#include <whisperlib/net/base/selector.h>
#include <whisperstreamlib/base/element.h>

#ifndef __MEDIA_ELEMENTS_TIME_SHIFT_ELEMENT_H__
#define __MEDIA_ELEMENTS_TIME_SHIFT_ELEMENT_H__

namespace streaming {
class BufferedSaver;
class TimeShiftElement : public Element {
 public:
  static const char kElementClassName[];
  TimeShiftElement(const string& name,
                   ElementMapper* mapper,
                   net::Selector* selector,
                   const string& media_path,
                   const string& save_dir,
                   const string& time_range_element,
                   int64 buffer_size_ms);
  virtual ~TimeShiftElement();

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

  string info() const { return name(); }
 private:
  // the number of ms that are currently buffered
  int64 BufferedMs() const;

  // these tags are buffered for time shifting
  void ProcessTag(const Tag* tag, int64 timestamp);

  // the indicated req has finished playing on buf
  void PlayFinished(BufferedSaver* buf, Request* req, ProcessingCallback* c);

  // register/unregister from the 'media_path_' upstream element
  bool Register();
  void Unregister();

 private:
  net::Selector* const selector_;

  // media coming from this source is buffered, for time shifting
  const string media_path_;

  // the directory where we save files on disk
  const string save_dir_;

  // internal path to a TimeRangeElement containing the saved files
  const string time_range_element_;

  // buffer size in milliseconds
  const int64 buffer_size_ms_;

  // permanent callback to ProcessTag
  streaming::Request* internal_req_;
  streaming::ProcessingCallback* process_tag_callback_;

  // to compute the stream time of the incoming tags
  streaming::StreamTimeCalculator calculator_;

  // The media buffers.
  // buffers_.back(): is the active buffer, this one accumulates media,
  //                  until if becomes full; then a new buffer is appended
  //                  to the end.
  // The rest of the buffers: no longer receive tags, but they still server
  //                          past requests. Their media has been saved on disk.
  list<BufferedSaver*> buffers_;

  // the map of requests
  typedef map<Request*, BufferedSaver*> BuffReqMap;
  BuffReqMap buff_requests_;

  // these threads do the saving of Buffers to disk
  //thread::ThreadPool saver_threads_;

  // permanent callback to PlayFinished()
  Callback3<BufferedSaver*, Request*, ProcessingCallback*>* play_finished_callback_;

  // called when asynchronous Close completes
  Closure* close_completed_;

  DISALLOW_EVIL_CONSTRUCTORS(TimeShiftElement);
};

}

#endif  // __MEDIA_ELEMENTS_TIME_SHIFT_ELEMENT_H__
