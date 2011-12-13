// Copyright (c) 2009, Whispersoft s.r.l.
// All rights reserved.
//
// Author: Cosmin Tudorache
//
#ifndef __STREAMING_ELEMENTS_SCHEDULE_POLICY_H__
#define __STREAMING_ELEMENTS_SCHEDULE_POLICY_H__

#include <string>
#include <vector>
#include <deque>
#include <utility>
#include <algorithm>

#include <whisperlib/common/base/date.h>
#include <whisperlib/common/io/checkpoint/state_keeper.h>
#include <whisperlib/net/base/selector.h>
#include <whisperstreamlib/base/element.h>
#include <whisperlib/net/rpc/lib/server/rpc_http_server.h>
#include <whisperstreamlib/elements/util/media_date.h>

#include "extra_library/auto/extra_library_invokers.h"

#include <whisperstreamlib/elements/standard_library/policies/policy.h>

namespace streaming {

//////////////////////////////////////////////////////////////////////

// A playlist policy where every item has an absolute start time.
// Currently: the start time is the local time of the day (relative to 00:00).
// In the future: the start time will be calendar time + repeating spec.
//
class SchedulePlaylistPolicy
    : public Policy,
      public ServiceInvokerSchedulePlaylistPolicyService {
 private:
  // -1 = id used for default_media_
  static const int64 kInvalidItemId;
  // 100 ms = we allow some clock / alarm errors .. small
  static const int32 kMinScheduleError;
  // 500 ms = crappy media .. we don't eat eos-s that fast - is definitely some
  //          problem somewhere
  static const int32 kMinEosInterval;
  // 2000 ms = and int that case we don't react that fast..
  static const int32 kMinEosRetry;
  // We use this big number a maximum duration for an event - one year is enough
  static const int64 kMaxEventDurationSec;
  // For 0 duration media we use this as the duration of the media. Useful for
  // live streams.
  static const int64 kUndefinedEventDurationSec;
  struct ScheduleItem {
    int64 id_;
    int64 begin_ts_;
    int64 switch_ts_;
    int64 end_ts_;
    ScheduleItem(int64 id, int64 begin_ts, int64 switch_ts, int64 end_ts)
      : id_(id), begin_ts_(begin_ts), switch_ts_(switch_ts), end_ts_(end_ts) {
    }
    string ToString() const {
      return strutil::StringPrintf("ScheduleItem{id_: %"PRId64""
          ", begin_ts_: %"PRId64"(%s), switch_ts_: %"PRId64"(%s)"
          ", end_ts_: %"PRId64"(%s)}", id_,
          begin_ts_, timer::Date(begin_ts_).ToString().c_str(),
          switch_ts_, timer::Date(switch_ts_).ToString().c_str(),
          end_ts_, timer::Date(end_ts_).ToString().c_str());
    }
  };

 public:
  SchedulePlaylistPolicy(
      const char* name,
      PolicyDrivenElement* element,
      net::Selector* selector,
      bool is_temp_policy,
      io::StateKeepUser* global_state_keeper,
      io::StateKeepUser* local_state_keeper,
      rpc::HttpServer* rpc_http_server,
      const string& rpc_path,
      const string& local_rpc_path,
      const string& default_media,
      const vector<const SchedulePlaylistItemSpec*>& playlist,
      int32 min_switch_delta_ms_);

  virtual ~SchedulePlaylistPolicy();

  static const char kPolicyClassName[];

  ////////////////////////////////////////////////////////////////////////
  //
  // Policy methods
  //
  virtual bool Initialize();
  virtual void Refresh() { }
  virtual void Reset() { }
  virtual bool NotifyEos();
  virtual bool NotifyTag(const Tag* tag, int64 timestamp_ms) { return true; }
  virtual string GetPolicyConfig() { return ""; }

  ////////////////////////////////////////////////////////////////////////
  //
  // ServiceInvokerSchedulePlaylistPolicyService methods
  //
 private:

  // Utilities
  void SetPlayInfo(int64 id, int64 begin_ts, SchedulePlayInfo* ret) const {
    ret->begin_ts_.ref() = begin_ts;
    if ( id == kInvalidItemId ) {
      ret->info_.ref().id_.ref() = id;
      ret->info_.ref().media_.ref() = default_media_;
      ret->info_.ref().start_time_.ref() = EmptyTimeSpec();
    } else {
      const SpecMap::const_iterator it = specs_.find(id);
      if ( it != specs_.end() ) {
        ret->info_.ref() = *it->second;
      } else {
        ret->info_.ref();
        ret->info_.ref().start_time_.ref() = EmptyTimeSpec();
      }
    }
  }
  void ClearPlaylist() {
    for ( SpecMap::const_iterator it = specs_.begin();
          it != specs_.end(); ++it ) {
      delete it->second;
    }
    specs_.clear();
  }
  // Returns the current playing item and the last switch to this item.
  // - [IN] now: the moment you wish to find 'current' item
  // - [OUT] out_item_id: the items who should be playing at 'now' (name it A)
  // - [OUT] out_begin_ts: the absolute start time of A
  // - [OUT] out_switch_ts: the last switch to A (if another item plays over A)
  //                        If no overlapping on A, this equals out_begin_ts.
  // - [OUT] out_end_ts: the absolute end time of A. For items with no duration,
  //                     the implicit duration is 1 YEAR.
  // The order is:
  //   out_begin_ts <= out_switch_ts <= now
  //   out_end_ts > now
  // If the specs_ list is empty, returns:
  //  out_item_id = kInvalidId
  //  out_begin_ts = now //now - kMaxEventDurationSec * 1000. (1 YEAR in the past)
  //  out_switch_ts = out_begin_ts
  //  out_end_ts = now + kMaxEventDurationSec * 1000. (1 YEAR in the future)
  // e.g.
  //         03:07 - start A (duration 37 seconds)
  //         03:10 - start B (duration 3)
  //         03:13 - end B
  //         03:20 - start C (duration 7)
  //         03:27 - end C
  //         03:44 - end A
  //  And now is -> 03:15 .
  //  Returns id: A, begin_ts: 03:07, switch_ts: 03:13, end_ts: 03:44.
  void GetCurrentItemAtTime(const timer::Date& now,
                            int64* out_item_id,
                            int64* out_begin_ts,
                            int64* out_switch_ts,
                            int64* out_end_ts) const;
  // Returns the closest item with a start time after 'now'.
  // If there is no item to satisfy the conditions, returns:
  //  out_item_id: kInvalidId
  //  out_begin_ts: now + kMaxEventDurationSec * 1000. (1 YEAR in the future)
  void GetFutureItemAtTime(const timer::Date& now,
                           int64* out_item_id, int64* out_begin_ts) const;
  // Returns the item to play on the given 'now' moment.
  // [IN]  now: a given moment
  // [OUT] out_item_id: the item to play right 'now'
  // [OUT] out_begin_ts: the begin timestamp of 'out_item_id'
  // [OUT] out_end_ts: the end timestamp of 'out_item_id'. This is either
  //                   the absolute item end, or the beginning of the next
  //                   interrupting event.
  void GetItemAtTime(const timer::Date& now,
                     int64* out_item_id,
                     int64* out_begin_ts,
                     int64* out_switch_ts,
                     int64* out_end_ts) const;
  // Uses GetItemAtTime to build a vector of 'num_to_schedule' items.
  // The first item should be playing at 'start_time'.
  // The second item follows .. and so on.
  void GetScheduleAtTime(int num_to_schedule,
                         vector< ScheduleItem >* schedule,
                         const timer::Date& start_time) const;
  void GoToNext(bool is_eos);
  void GetPlaylist(SchedulePlaylistPolicySpec* playlist) const;
  bool SetPlaylist(const SetSchedulePlaylistPolicySpec& playlist);


  // State keeping

  void LoadState();
  bool SaveState();
  void ClearState();

  // RPC interface

  virtual void GetPlaylist(rpc::CallContext<SchedulePlaylistPolicySpec>* call);
  virtual void SetPlaylist(rpc::CallContext<MediaOperationErrorData>* call,
                           const SetSchedulePlaylistPolicySpec& playlist);

  virtual void GetDefaultMedia(rpc::CallContext<string>* call);
  virtual void SetDefaultMedia(rpc::CallContext<rpc::Void>* call,
                               const string& default_media,
                               bool switch_now);
  virtual void GetNextPlay(rpc::CallContext<SchedulePlayInfo>* call);
  virtual void GetCurrentPlay(rpc::CallContext<SchedulePlayInfo>* call);
  virtual void GetProgram(rpc::CallContext<ScheduleProgram>* call,
                          const string& date,
                          int32 num_schedules);

 private:
  net::Selector* const selector_;
  const bool is_temp_policy_;
  io::StateKeepUser* const global_state_keeper_;
  io::StateKeepUser* const local_state_keeper_;
  rpc::HttpServer* const rpc_http_server_;
  const string rpc_path_;
  const string local_rpc_path_;

  string default_media_;
  const int32 min_switch_delta_ms_;

  typedef hash_map<int64, SchedulePlaylistItemSpec*> SpecMap;
  SpecMap specs_;

  // The current item playing.
  // -1 for default_media_.
  int current_id_;
  // the moment 'current_' play started.
  int64 current_begin_ts_;

  Closure* const next_callback_;
  bool is_registered_;

  DISALLOW_EVIL_CONSTRUCTORS(SchedulePlaylistPolicy);
};
}

#endif //  __STREAMING_ELEMENTS_SCHEDULE_POLICY_H__
