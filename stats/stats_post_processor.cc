// Copyright (c) 2009, Whispersoft s.r.l.
// All rights reserved.

// Author: Catalin Popescu & Cosmin Tudorache

#include <signal.h>
#include <time.h>
#include <unistd.h>
#include <ext/functional> // for select2nd

#include <whisperlib/common/base/types.h>
#include <whisperlib/common/base/log.h>
#include <whisperlib/common/base/system.h>
#include <whisperlib/common/base/gflags.h>
#include <whisperlib/common/base/errno.h>
#include <whisperlib/common/base/strutil.h>
#include <whisperlib/common/base/signal_handlers.h>
#include <whisperlib/common/base/timer.h>
#include <whisperlib/common/base/re.h>
#include <whisperlib/common/app/app.h>
#include <whisperlib/common/io/buffer/memory_stream.h>
#include <whisperlib/common/io/logio/logio.h>
#include <whisperlib/common/io/checkpoint/checkpointing.h>
#include <whisperlib/common/io/ioutil.h>
#include <whisperlib/net/rpc/lib/codec/json/rpc_json_encoder.h>
#include <whisperlib/net/rpc/lib/codec/json/rpc_json_decoder.h>
#include <whisperstreamlib/stats2/auto/media_stats_types.h>

#define MYSQLPP_MYSQL_HEADERS_BURIED
#include <mysql++/mysql++.h>
#include <mysql++/ssqls.h>

#include "mysql_stats_saver.h"
#include "mysql_stats_saver_complete.h"
#include "mysql_stats_saver_light.h"
#include "mysql_stats_saver_custom.h"

//////////////////////////////////////////////////////////////////////

DEFINE_string(state_dir,
              "",
              "Where is the directory for the state");
DEFINE_string(state_filebase,
              "",
              "The prefix of our state (i.e. state name)");
DEFINE_string(log_dir,
              "",
              "Where the stats log is saved");
DEFINE_string(log_filebase,
              "",
              "The name of the stats log");
DEFINE_int32(transaction_records,
             1000,
             "We create a new SQL transaction & save internal state"
             " every so many records");
DEFINE_int32(num_checkpoints_to_keep,
             10,
             "We keep these many past checkpoints");
//DEFINE_int32(num_input_files_to_keep,
//             2,
//             "We keep this many old processed input files."
//             " Not counting the current input file.");

//////////////////////////////////////////////////////////////////////

DEFINE_string(mysql_database,
              "",
              "If specified we connect to this database");
DEFINE_string(mysql_host,
              "",
              "If specified, we dump stats to this server");
DEFINE_int32(mysql_port,
             3306,
              "We connect to mysql stats server on this port");
DEFINE_string(mysql_user,
              "",
              "We connect to db using this user (if specified)");
DEFINE_string(mysql_passwd,
              "",
              "We connect to db using this password (if specified)");
DEFINE_string(mysql_table,
              "connections",
              "We use this table (defaults to \"connections\")");
DEFINE_string(mysql_custom,
              "",
              "Use a custom table, format is: table_name:field_name,...");
DEFINE_int32(mysql_retry_timeout,
             30,
             "Seconds between mysql connect tries");
DEFINE_bool(mysql_light_connection,
            false,
            "true for the light connections table, "
            "false for the original complete connection table.");

DEFINE_bool(print_all,
            false,
            "We print all messages that we process..");

DEFINE_int32(print_statistics,
             10000,
             "Print internal processing statistics every so many records.");

DEFINE_bool(enable_force_insert,
            false,
            "Ignore state, parse all log, insert into mysql stats having"
            " timestamp between [force_insert_begin_ts, force_insert_end_ts],"
            " don't save state");
DEFINE_int64(force_insert_begin_ts,
             0,
             "Ms from epoch: the start timestamp for forcing stats into mysql");
DEFINE_int64(force_insert_end_ts,
             0,
             "Ms from epoch: the end timestamp for forcing stats into mysql");

DEFINE_string(match_content,
              "",
              "This is a regular expression. We process only stats with "
              "content_id matching this expression. If empty we process "
              "all stats.");

DEFINE_string(stats_post_processor_name,
              "",
              "Name to use for marking .finished files.");

//DEFINE_bool(first_run,
//            false,
//            "Before running this guy ever - you need to run w/ this flag "
//            "on to create an empty checkpoint (and state)");

//////////////////////////////////////////////////////////////////////

typedef map<string, ConnectionBegin*> ConnectionStatsMap;
typedef map<string, StreamBegin*> StreamStatsMap;
typedef map<string, MediaBegin*> MediaStatsMap;

enum PROCESS_STAT_RESULT {
  PROCESS_STAT_SUCCESS,
  PROCESS_STAT_SQL_ERROR,
  PROCESS_STAT_INTERNAL_ERROR
};

class ServerStats {
 public:
  ServerStats(MySqlMediaStatsSaver* sql_stats_saver,
              const string& server_id,
              int64 server_instance,
              re::RE* match_content)
      : sql_stats_saver_(sql_stats_saver),
        server_id_(server_id),
        server_instance_(server_instance),
        conn_stats_(),
        stream_stats_(),
        media_stats_(),
        match_content_(match_content) {
  }
  virtual ~ServerStats() {
    #define CLEAR_STAT_MAP(m) \
      while ( !m.empty() ) {\
        delete m.begin()->second;\
        m.erase(m.begin());\
      }
    CLEAR_STAT_MAP(conn_stats_);
    CLEAR_STAT_MAP(stream_stats_);
    CLEAR_STAT_MAP(media_stats_);
    #undef CLEAR_STAT_MAP
  }

  const string& server_id() const { return server_id_; }
  int64 server_instance() const { return server_instance_; }
  const ConnectionStatsMap& conn_stats() const { return conn_stats_; }
  const StreamStatsMap& stream_stats() const { return stream_stats_; }
  const MediaStatsMap& media_stats() const { return media_stats_; }

  // Process given stat event. Returns process status.
  PROCESS_STAT_RESULT AddStat(const MediaStatEvent& ev) {
    int64 ts =
        ev.connection_begin_.is_set()
        ? ev.connection_begin_.get().timestamp_utc_ms_.get()
        : (ev.connection_end_.is_set()
           ? ev.connection_end_.get().timestamp_utc_ms_.get()
           : (ev.stream_begin_.is_set()
              ? ev.stream_begin_.get().timestamp_utc_ms_.get()
              : (ev.stream_end_.is_set()
                 ? ev.stream_end_.get().timestamp_utc_ms_.get()
                 : (ev.media_begin_.is_set()
                    ? ev.media_begin_.get().timestamp_utc_ms_.get()
                    : (ev.media_end_.is_set()
                       ? ev.media_end_.get().timestamp_utc_ms_.get()
                       : timer::Date::Now())))));
    LOG_INFO << "Stat: "
      << (ev.connection_begin_.is_set() ? "CONNECTION_BEGIN" :
          ev.connection_end_.is_set() ? "CONNECTION_END" :
          ev.stream_begin_.is_set() ? "STREAM_BEGIN" :
          ev.stream_end_.is_set() ? "STREAM_END" :
          ev.media_begin_.is_set() ? "MEDIA_BEGIN" :
          ev.media_end_.is_set() ? "MEDIA_END" : "UNKNOWN")
             << ", server_id: " << ev.server_id_
             << ", server_instance: " << ev.server_instance_
             << ", ts: " << timer::Date(ts)
             << ", "
      << (FLAGS_print_all ?
           (ev.connection_begin_.is_set() ? ev.connection_begin_.ToString() :
            ev.connection_end_.is_set() ? ev.connection_end_.ToString() :
            ev.stream_begin_.is_set() ? ev.stream_begin_.ToString() :
            ev.stream_end_.is_set() ? ev.stream_end_.ToString() :
            ev.media_begin_.is_set() ? ev.media_begin_.ToString() :
            ev.media_end_.is_set() ? ev.media_end_.ToString() :
            string("UNKNOWN") ) : string(""));
    CHECK(ev.server_id_.get() == server_id())
        << " server_id(): "  << server_id()
        << "server_id mismatch, while attempting to add MediaStatEvent: " << ev;
    if ( ev.server_instance_ != server_instance_ ) {
      LOG_WARNING << "on server_id: " << server_id()
                  << " server_instance has changed from: " << server_instance()
                  << " , to: " << ev.server_instance_
                  << " , assuming the old instance has crashed.."
                     " closing all stats..";
      CompleteAllStats(ts, "SERVER CRASH");
      server_instance_ = ev.server_instance_;
      LOG_INFO << "Updated server_instance: " << server_instance();
    }
    if ( ev.connection_begin_.is_set() ) {
      const ConnectionBegin& connection_begin = ev.connection_begin_;
      const string& connection_id =
        connection_begin.connection_id_;
      ConnectionBegin* new_begin = new ConnectionBegin(connection_begin);
      pair<ConnectionStatsMap::iterator, bool> result =
        conn_stats_.insert(make_pair(connection_id, new_begin));
      if ( !result.second ) {
        LOG_ERROR << "Duplicate connection_begin"
                     ", on connection_id: " << connection_id
                  << ", replacing old: " << *(result.first->second)
                  << ", with new: " << connection_begin;
        delete result.first->second;
        result.first->second = new_begin;
        return PROCESS_STAT_SUCCESS;
      }
      return PROCESS_STAT_SUCCESS;
    }
    if ( ev.stream_begin_.is_set() ) {
      const StreamBegin& stream_begin = ev.stream_begin_;
      const string& stream_id = stream_begin.stream_id_;
      StreamBegin* new_begin = new StreamBegin(stream_begin);
      pair<StreamStatsMap::iterator, bool> result =
        stream_stats_.insert(make_pair(stream_id, new_begin));
      if ( !result.second ) {
        LOG_ERROR << "Duplicate stream_begin"
                     ", on stream_id: " << stream_id
                  << ", replacing old: " << *(result.first->second)
                  << ", with new: " << stream_begin;
        delete result.first->second;
        result.first->second = new_begin;
        return PROCESS_STAT_SUCCESS;
      }
      return PROCESS_STAT_SUCCESS;
    }
    if ( ev.media_begin_.is_set() ) {
      const MediaBegin& media_begin = ev.media_begin_;
      const string& media_id = media_begin.media_id_;
      MediaBegin* new_begin = new MediaBegin(media_begin);
      pair<MediaStatsMap::iterator, bool> result =
        media_stats_.insert(make_pair(media_id, new_begin));
      if ( !result.second ) {
        LOG_ERROR << "Duplicate media_begin"
                     ", on media_id: " << media_id
                  << ", replacing old: " << *(result.first->second)
                  << ", with new: " << ev;
        delete result.first->second;
        result.first->second = new_begin;
        return PROCESS_STAT_SUCCESS;
      }
      return PROCESS_STAT_SUCCESS;
    }
    if ( ev.connection_end_.is_set() ) {
      const ConnectionEnd& connection_end = ev.connection_end_;
      const string& connection_id = connection_end.connection_id_;
      ConnectionStatsMap::iterator it = conn_stats_.find(connection_id);
      if ( it == conn_stats_.end() ) {
        LOG_ERROR << " End of CONNECTION w/o begin"
                     ", on connection_id: " << connection_id
                  << " , stat: " << connection_end;
        return PROCESS_STAT_INTERNAL_ERROR;
      }
      const ConnectionBegin* connection_begin = it->second;
      PROCESS_STAT_RESULT result = CompleteConnectionStat(*connection_begin,
                                                          connection_end);
      delete connection_begin;
      conn_stats_.erase(it);
      return result;
    }
    if ( ev.stream_end_.is_set() ) {
      const StreamEnd& stream_end = ev.stream_end_;
      const string& stream_id = stream_end.stream_id_;
      StreamStatsMap::iterator it = stream_stats_.find(stream_id);
      if ( it == stream_stats_.end() ) {
        LOG_ERROR << " End of STREAM w/o begin"
                     ", on stream_id: " << stream_id
                  << " , stat: " << ev;
        return PROCESS_STAT_INTERNAL_ERROR;
      }
      const StreamBegin* stream_begin = it->second;
      PROCESS_STAT_RESULT result = CompleteStreamStat(*stream_begin,
                                                      stream_end);
      delete stream_begin;
      stream_stats_.erase(it);
      return result;
    }
    if ( ev.media_end_.is_set() ) {
      const MediaEnd& media_end = ev.media_end_;
      const string& media_id = media_end.media_id_;
      MediaStatsMap::iterator it = media_stats_.find(media_id);
      if ( it == media_stats_.end() ) {
        LOG_ERROR << " End of MEDIA w/o begin"
                     ", on media_id: " << media_id
                  << ", stat: " << ev;
        return PROCESS_STAT_INTERNAL_ERROR;
      }
      const MediaBegin* media_begin = it->second;
      PROCESS_STAT_RESULT result = CompleteMediaStat(*media_begin,
                                                     media_end);
      delete media_begin;
      media_stats_.erase(it);
      return result;
    }
    LOG_ERROR << "Don't know how to process strange MediaStatEvent: " << ev;
    return PROCESS_STAT_INTERNAL_ERROR;
  }
  // test if there are no pending stats
  bool Empty() const {
    return conn_stats_.empty() &&
           stream_stats_.empty() &&
           media_stats_.empty();
  }

 private:
  PROCESS_STAT_RESULT CompleteConnectionStat(
       const ConnectionBegin& begin, const ConnectionEnd& end) {
    if ( !sql_stats_saver_ ) {
      LOG_DEBUG << "no sql_stats_saver_, skipping SQL save..";
      return PROCESS_STAT_SUCCESS; // just processing and printing
    }
    // TODO: we don't record anything per-connection - so far..
    return PROCESS_STAT_SUCCESS;
  }
  PROCESS_STAT_RESULT CompleteStreamStat(
       const StreamBegin& begin, const StreamEnd& end) {
    if ( !sql_stats_saver_ ) {
      LOG_DEBUG << "no sql_stats_saver_, skipping SQL save..";
      return PROCESS_STAT_SUCCESS; // just processing and printing
    }
    // TODO: we don't record anything per-stream - so far..
    return PROCESS_STAT_SUCCESS;
  }
  PROCESS_STAT_RESULT CompleteMediaStat(
      const MediaBegin& begin, const MediaEnd& end) {
    if ( FLAGS_enable_force_insert ) {
      if ( end.timestamp_utc_ms_ < FLAGS_force_insert_begin_ts ||
           end.timestamp_utc_ms_ > FLAGS_force_insert_end_ts ) {
        LOG_DEBUG << "force insert, out of range, skipping SQL save..";
        return PROCESS_STAT_SUCCESS; // just processing and printing
      } else {
        LOG_WARNING << "force insert, in range, doing SQL save..";
      }
    }
    if ( match_content_ != NULL &&
         !match_content_->Matches(begin.content_id_) ) {
      // Skip stats not matching FLAGS_match_content
      LOG_DEBUG << "Skipping unmatched stat: " << begin;
      return PROCESS_STAT_SUCCESS;
    }
    if ( !sql_stats_saver_ ) {
      LOG_DEBUG << "no sql_stats_saver_, skipping SQL save..";
      return PROCESS_STAT_SUCCESS; // just processing and printing
    }
    int64 id = sql_stats_saver_->Save(server_id(), server_instance(),
                                      begin, end);
    return id == -1 ? PROCESS_STAT_SQL_ERROR :
                      PROCESS_STAT_SUCCESS;
  }
  void CompleteAllStats(int64 end_time, const string& result) {
    LOG_INFO << "======================================================\n"
             << "Completing all pending stats with result: " << result
             << "\n - active connections: " << conn_stats_.size()
             << "\n - active streams: " << stream_stats_.size()
             << "\n - active media stats: " << media_stats_.size();
    for ( MediaStatsMap::iterator it = media_stats_.begin();
          it != media_stats_.end(); ++it ) {
      const MediaBegin* begin = it->second;
      const MediaEnd end(begin->media_id_, end_time,
                         0, 0, 0, 0, 0, 0, 0, MediaResult(result));
      CompleteMediaStat(*begin, end);
      delete begin;
    }
    media_stats_.clear();
    for ( StreamStatsMap::iterator it = stream_stats_.begin();
          it != stream_stats_.end(); ++it ) {
      const StreamBegin* begin = it->second;
      const StreamEnd end(begin->stream_id_, end_time,
                          StreamResult(result));
      CompleteStreamStat(*begin, end);
      delete begin;
    }
    stream_stats_.clear();
    for ( ConnectionStatsMap::iterator it = conn_stats_.begin();
          it != conn_stats_.end(); ++it ) {
      const ConnectionBegin* begin = it->second;
      const ConnectionEnd end(begin->connection_id_, end_time,
                              0, 0, ConnectionResult(result));
      CompleteConnectionStat(*begin, end);
      delete begin;
    }
    conn_stats_.clear();
  }

 private:
  MySqlMediaStatsSaver* const sql_stats_saver_;
  const string server_id_;
  int64 server_instance_;

  ConnectionStatsMap conn_stats_;
  StreamStatsMap stream_stats_;
  MediaStatsMap media_stats_;

  re::RE* match_content_;

  DISALLOW_EVIL_CONSTRUCTORS(ServerStats);
};

class StatsPostProcessor : public app::App {
private:
  // reads statistics from disk
  io::LogReader* stats_reader_;

  // writes statistics to mysql
  MySqlMediaStatsSaver* sql_stats_saver_;

  // map "server_id" -> ServerStats
  typedef map<string, ServerStats*> ServerStatsMap;
  ServerStatsMap server_stats_;

  // Used to signal the Run() loop it should end.
  synch::Event exit_event_;

  // statistics for this processor
  //
  // counts total stats processed
  int64 stat_process_total_count_;
  // the stat_process_total_count_ value on last statistics print
  int64 stat_process_last_count_;
  // the count of SQL rows saved on last statistics print
  int64 stat_sql_save_last_count_;
  // the timestamp on last read
  int64 stat_last_ts_;

  re::RE* match_content_;

  static const string keyLogPos;
  static const string keyPendingStats;

public:
  StatsPostProcessor(int &argc, char **&argv) :
    app::App(argc, argv),
    stats_reader_(NULL),
    sql_stats_saver_(NULL),
    server_stats_(),
    exit_event_(false, true),
    stat_process_total_count_(0),
    stat_process_last_count_(0),
    stat_sql_save_last_count_(0),
    stat_last_ts_(timer::TicksMsec()),
    match_content_(NULL) {
  }
  virtual ~StatsPostProcessor() {
    CHECK_NULL(stats_reader_);
    CHECK_NULL(sql_stats_saver_);
    CHECK(server_stats_.empty());
    CHECK_NULL(match_content_);
  }

protected:
  PROCESS_STAT_RESULT AddStat(const MediaStatEvent& stat) {
    const string server_id = stat.server_id_;
    ServerStatsMap::iterator it = server_stats_.find(server_id);
    // LOG_INFO << "Processing: " << stat.ToString();
    ServerStats* server = NULL;
    if ( it == server_stats_.end() ) {
      LOG_INFO << " Adding a new server: " << server_id;
      server = new ServerStats(sql_stats_saver_,
                               stat.server_id_,
                               stat.server_instance_,
                               match_content_);
      pair<ServerStatsMap::iterator, bool> result =
        server_stats_.insert(make_pair(server_id, server));
      CHECK(result.second);
    } else {
      server = it->second;
      CHECK_NOT_NULL(server);
    }
    PROCESS_STAT_RESULT result = server->AddStat(stat);
    if ( result != PROCESS_STAT_SUCCESS ) {
      LOG_ERROR << "Failed to process statistic: " << stat << " ..ignoring";
      return result;
    }
    stat_process_total_count_++;
    if ( server->Empty() ) {
      LOG_INFO << " Removing empty server: " << server_id;
      delete server;
      server = NULL;
      server_stats_.erase(it);
    }
    return PROCESS_STAT_SUCCESS;
  }
  void ClearServerStats() {
    for ( ServerStatsMap::iterator it = server_stats_.begin();
          it != server_stats_.end(); ++it ) {
      ServerStats* server = it->second;
      delete server;
    }
    server_stats_.clear();
  }
  void PrintStatistics() {
    int64 now = timer::TicksMsec();
    int64 elapsed_ms = now - stat_last_ts_;
    int64 process_count = stat_process_total_count_ - stat_process_last_count_;
    float process_speed = process_count * 1000.0f / elapsed_ms;

    int64 save_count = 0;
    int64 save_total = 0;
    if ( sql_stats_saver_ != NULL ) {
      save_total = sql_stats_saver_->stat_count_saved();
      save_count = save_total - stat_sql_save_last_count_;
      stat_sql_save_last_count_ = sql_stats_saver_->stat_count_saved();
    }
    float save_speed = save_count * 1000.0f / elapsed_ms;

    // Read the timestamp of some pending stat.
    // To get an idea of what stats are we processing right now.
    int64 some_ts = 0;
    if ( !server_stats_.empty() ) {
      const ServerStats& some_server = *server_stats_.begin()->second;
      if ( !some_server.media_stats().empty() ) {
        const MediaBegin& some_media_begin =
            *(--some_server.media_stats().end())->second;
        some_ts = some_media_begin.timestamp_utc_ms_;
      }
    }

    // count pending stats
    uint32 count_pending_stats = 0;
    for ( ServerStatsMap::iterator it = server_stats_.begin();
          it != server_stats_.end(); ++it ) {
      const ServerStats& server = *it->second;
      count_pending_stats += server.conn_stats().size();
      count_pending_stats += server.stream_stats().size();
      count_pending_stats += server.media_stats().size();
    }

    LOG_WARNING << "Statistics: "
                << "\n - total stats processed: " << stat_process_total_count_
                << "\n - processing speed: " << process_speed << " stats/sec"
                << "\n - total stats SQL saved: " << save_total
                << "\n - SQL saving speed: " << save_speed << " rows/sec"
                << "\n - total pending stats: " << count_pending_stats
                << "\n - some pending stat ts: " << timer::Date(some_ts)
                << "\n - servers: " << server_stats_.size()
                << "\n - log position: " << stats_reader_->Tell().ToString();

    stat_process_last_count_ = stat_process_total_count_;
    stat_last_ts_ = now;
  }
  void Save() {
    CHECK(!FLAGS_enable_force_insert);
    CHECK_NOT_NULL(sql_stats_saver_) << "no SQL connection, no save";
    LOG_INFO << "Saving..";
    const io::LogPos pos = stats_reader_->Tell();
    LOG_INFO << "Saving: log position is: " << pos.ToString();

    uint32 count_pending_stats = 0;
    for ( ServerStatsMap::iterator it = server_stats_.begin();
          it != server_stats_.end(); ++it ) {
      const ServerStats& server = *it->second;
      count_pending_stats += server.conn_stats().size();
      count_pending_stats += server.stream_stats().size();
      count_pending_stats += server.media_stats().size();
    }

    // copy all pending conn_stats, stream_stats, and media_stats
    vector<MediaStatEvent> rpc_pending_stats(count_pending_stats);
    uint32 i = 0;
    for ( ServerStatsMap::iterator it = server_stats_.begin();
          it != server_stats_.end(); ++it ) {
      const ServerStats& server = *it->second;

      for ( ConnectionStatsMap::const_iterator it = server.conn_stats().begin();
            it != server.conn_stats().end(); ++it, ++i ) {
        const ConnectionBegin& connection_begin = *it->second;
        MediaStatEvent ev;
        ev.server_id_ = server.server_id();
        ev.server_instance_ = server.server_instance();
        ev.connection_begin_ = connection_begin;
        rpc_pending_stats[i] = ev;
      }
      for ( StreamStatsMap::const_iterator it = server.stream_stats().begin();
            it != server.stream_stats().end(); ++it, ++i ) {
        const StreamBegin& stream_begin = *it->second;
        MediaStatEvent ev;
        ev.server_id_ = server.server_id();
        ev.server_instance_ = server.server_instance();
        ev.stream_begin_ = stream_begin;
        rpc_pending_stats[i] = ev;
      }
      for ( MediaStatsMap::const_iterator it = server.media_stats().begin();
            it != server.media_stats().end(); ++it, ++i ) {
        const MediaBegin& media_begin = *it->second;
        MediaStatEvent ev;
        ev.server_id_ = server.server_id();
        ev.server_instance_ = server.server_instance();
        ev.media_begin_ = media_begin;
        rpc_pending_stats[i] = ev;
      }
    }
    CHECK_EQ(i, count_pending_stats);

    LOG_INFO << "Saving: gathered #" << count_pending_stats
             << " pending stats";

    string str_pending_stats;
    rpc::JsonEncoder::EncodeToString(rpc_pending_stats, &str_pending_stats);

    io::CheckpointWriter cw(FLAGS_state_dir.c_str(),
                            FLAGS_state_filebase.c_str());
    cw.BeginCheckpoint();
    cw.AddRecord(keyLogPos, pos.StrEncode());
    cw.AddRecord(keyPendingStats, str_pending_stats);
    bool success = cw.EndCheckpoint();
    CHECK(success) << "Saving: failed to write checkpoint";
    LOG_INFO << "Saving: deleting old checkpoints..";
    cw.CleanOldCheckpoints(FLAGS_num_checkpoints_to_keep);
    LOG_INFO << "Saving: success.";
  }
  bool Load() {
    CHECK(!FLAGS_enable_force_insert);
    LOG_INFO << "Loading..";
    map<string, string> checkpoint;
    if ( !io::ReadCheckpoint(FLAGS_state_dir.c_str(),
        FLAGS_state_filebase.c_str(), &checkpoint) ) {
      LOG_ERROR << "Loading: failed to read checkpoint"
                   " from dir: [" << FLAGS_state_dir << "]"
                   " , filebase: [" << FLAGS_state_filebase << "]";
      return false;
    }

    map<string, string>::const_iterator log_pos_it = checkpoint.find(keyLogPos);
    if ( log_pos_it == checkpoint.end() ) {
      LOG_ERROR << "Loading: cannot find '" << keyLogPos << "' key"
                   " in checkpoint data: " << strutil::ToString(checkpoint);
      return false;
    }
    io::LogPos log_pos;
    memcpy(&log_pos, log_pos_it->second.c_str(), sizeof(log_pos));
    LOG_INFO << "Loading: found log position: " << log_pos.ToString()
             << " seeking..";

    bool success = stats_reader_->Seek(log_pos);
    if ( !success ) {
      LOG_ERROR << "Loading: seek failed for position: " << log_pos.ToString();
      return false;
    }
    LOG_INFO << "Loading: positioned stats reader on: " << log_pos.ToString();

    map<string, string>::const_iterator pending_stats_it =
      checkpoint.find(keyPendingStats);
    if ( pending_stats_it == checkpoint.end() ) {
      LOG_ERROR << "Loading: cannot find '" << keyPendingStats << "' key"
                   " in checkpoint data: " << strutil::ToString(checkpoint);
      return false;
    }
    const string& str_pending_stats = pending_stats_it->second;
    vector<MediaStatEvent> rpc_pending_stats;
    if ( !rpc::JsonDecoder::DecodeObject(str_pending_stats,
                                         &rpc_pending_stats) ) {
      LOG_ERROR << "Loading: failed to decode pending stats"
                   ", from data: " << str_pending_stats;
      return false;
    }
    LOG_INFO << "Loading: found #" << rpc_pending_stats.size()
             << " pending stats";
    for ( uint32 i = 0; i < rpc_pending_stats.size(); i++ ) {
      const MediaStatEvent& ev = rpc_pending_stats[i];
      PROCESS_STAT_RESULT result = AddStat(ev);
      if ( result != PROCESS_STAT_SUCCESS ) {
        LOG_ERROR << "Loading: failed to process stat: " << ev;
        return false;
      }
    }
    LOG_INFO << "Loading: success.";
    return true;
  }
  int Initialize() {
    common::Init(argc_, argv_);

    if ( FLAGS_state_dir.empty() || FLAGS_state_filebase.empty() ) {
      LOG_ERROR << "You need to specify --state_dir and --state_filebase";
      return -1;
    }

    if ( FLAGS_match_content != "" ) {
      match_content_ = new re::RE(FLAGS_match_content);
      if  ( match_content_->HasError() ) {
        LOG_ERROR << "Error in FLAGS_match_content: [" << FLAGS_match_content
                  << "], error: " << match_content_->ErrorName();
        return -1;
      }
    }
    if ( FLAGS_stats_post_processor_name == "" ) {
      LOG_ERROR << "Missing FLAGS_stats_post_processor_name";
      return -1;
    }

    //////////////////////////////////////////////////////////////////////
    //
    // Create SQL stats saver
    //
    if ( !FLAGS_mysql_host.empty() ) {
      if ( FLAGS_mysql_custom.empty() ) {
      if ( FLAGS_mysql_light_connection ) {
        sql_stats_saver_ = new MySqlMediaStatsSaverLight(FLAGS_mysql_host,
                                                         FLAGS_mysql_port,
                                                         FLAGS_mysql_user,
                                                         FLAGS_mysql_passwd,
                                                         FLAGS_mysql_database,
                                                         FLAGS_mysql_table);
      } else {
        sql_stats_saver_ = new MySqlMediaStatsSaverComplete(FLAGS_mysql_host,
                                                            FLAGS_mysql_port,
                                                            FLAGS_mysql_user,
                                                            FLAGS_mysql_passwd,
                                                            FLAGS_mysql_database,
                                                            FLAGS_mysql_table);
      }
      } else {
        sql_stats_saver_ = new MySqlMediaStatsSaverCustom(FLAGS_mysql_host,
                                                            FLAGS_mysql_port,
                                                            FLAGS_mysql_user,
                                                            FLAGS_mysql_passwd,
                                                            FLAGS_mysql_database,
                                                            FLAGS_mysql_table,
                                                            FLAGS_mysql_custom);
      }
      if ( !sql_stats_saver_->InitializeConnection() ) {
        LOG_ERROR << "Failed to initialize SQL connection to: "
                  << FLAGS_mysql_host << ":" << FLAGS_mysql_port;
        return -1;
      }
    } else {
      LOG_WARNING << " WARNING : ---- No MySQL stats saver - just printing";
      LOG_WARNING << " WARNING : ---- Statistics are not consumed.";
    }

    //////////////////////////////////////////////////////////////////////
    //
    // Create Statistics reader
    //
    stats_reader_ = new io::LogReader(FLAGS_log_dir.c_str(),
                                      FLAGS_log_filebase.c_str());

    return 0;
  }

  // Delete older files, starting with file number: del_file_num.
  // Also deletes: del_file_num - 1, del_file_num - 2, ... stopping on
  // the first file not found.
  void DeleteOldInputFiles(int32 del_file_num) {
    for ( int32 file_num = del_file_num; file_num >= 0; file_num-- ) {
      const string filename = stats_reader_->ComposeFileName(file_num);
      if ( !io::Exists(filename) ) {
        break;
      }
      LOG_WARNING << "Deleting old input file: [" << filename << "]";
      if ( !io::Rm(filename) ) {
        LOG_ERROR << "Cannot delete file: [" << filename << "]"
                       ", error: " << GetLastSystemErrorDescription();
        break;
      }
    }
  }
  // Marks done input files with number: mark_file_num, mark_file_num - 1, ...
  void MarkDoneOldInputFiles(int32 mark_file_num) {
    for ( int32 file_num = mark_file_num; file_num >= 0; file_num-- ) {
      const string stat_file = stats_reader_->ComposeFileName(file_num);
      const string done_file = stat_file + ".finished." +
                               FLAGS_stats_post_processor_name;
      if ( !io::Exists(stat_file) ) {
        break;
      }
      if ( io::Exists(done_file) ) {
        continue;
      }
      LOG_WARNING << "Mark done old input file: [" << stat_file << "]";
      if ( !io::Touch(done_file) ) {
        LOG_ERROR << "Cannot touch file: [" << done_file << "]"
                     ", error: " << GetLastSystemErrorDescription();
        break;
      }
    }
  }

  void Run() {
    ////////////////////////////////////////////////////////////////////
    // loop:
    //   Establish SQL connection
    //   Load last saved state
    //   BeginTransaction
    //     Save stat
    //     Save stat
    //     ......... (on SQL error GOTO loop begin)
    //   EndTransaction
    //   Save current state
    //   Delete old input files
    //

    // performance: clear & load last saved state only if necessary
    bool rollback_state = true;
    while ( !exit_event_.Wait(0) ) {
      ////////////////////////////////////////////////////////////////////
      // Establish SQL connection
      //
      while ( sql_stats_saver_ != NULL &&
              !sql_stats_saver_->IsConnected() &&
              !exit_event_.Wait(0) ) {
        if ( !sql_stats_saver_->InitializeConnection() ) {
          LOG_ERROR << "Failed to connect to SQL server on: "
                    << FLAGS_mysql_host << ":" << FLAGS_mysql_port
                    << " waiting " << FLAGS_mysql_retry_timeout
                    << " seconds before retry";
          exit_event_.Wait(FLAGS_mysql_retry_timeout*1000);
        }
      }
      if ( exit_event_.Wait(0) ) {
        break;
      }

      if ( FLAGS_enable_force_insert ) {
        LOG_WARNING << "force_insert mode, skipping Load..";
      } else if ( rollback_state ) {
        //////////////////////////////////////////////////////////////////
        // Clear internal state & Load last saved state
        //
        ClearServerStats();
        if ( !Load() ) {
          LOG_ERROR << "Failed to Load state from"
                       " FLAGS_state_dir: [" << FLAGS_state_dir << "]"
                       ", FLAGS_state_filebase: [" << FLAGS_state_filebase << "]";
          LOG_ERROR << "Assuming clean start..";
        }
      }
      rollback_state = true;

      ////////////////////////////////////////////////////////////////////
      // Begin SQL transaction
      //
      if ( sql_stats_saver_ != NULL ) {
        if ( !sql_stats_saver_->BeginTransaction() ) {
          LOG_ERROR << "SQL BeginTransaction failed";
          sql_stats_saver_->ClearConnection();
          continue;
        }
      }

      ////////////////////////////////////////////////////////////////////
      // Read, process and save stats to SQL
      //
      int64 num_records = 0;
      while ( !exit_event_.Wait(0) ) {
        // Read 1 record at a time, every record contains 1 statistic.
        io::MemoryStream s;
        if ( !stats_reader_->GetNextRecord(&s) ) {
          LOG_DEBUG << "... waiting for more records to process ...";
          exit_event_.Wait(10000);
          continue;
        }

        // Decode the MediaStatEvent inside the record
        rpc::JsonDecoder decoder(s);
        MediaStatEvent stat;
        const rpc::DECODE_RESULT decode_result = decoder.Decode(stat);
        if ( decode_result != rpc::DECODE_RESULT_SUCCESS ) {
          LOG_ERROR << "Failed to decode MediaStatEvent"
                       ", result: " << decode_result << " ignoring..";
          continue;
        }

        // Process the MediaStatEvent, possibly saving to SQL.
        PROCESS_STAT_RESULT process_result = AddStat(stat);
        if ( process_result == PROCESS_STAT_SQL_ERROR ) {
          if ( !sql_stats_saver_->IsConnected() ) {
            LOG_ERROR << "SQL connection broken";
            break;
          }
          LOG_ERROR << "ignoring stat: " << stat;
        }
        if ( process_result == PROCESS_STAT_INTERNAL_ERROR ) {
          LOG_ERROR << "ignoring stat: " << stat;
        }
        CHECK(process_result == PROCESS_STAT_INTERNAL_ERROR ||
              process_result == PROCESS_STAT_SUCCESS ||
              (process_result == PROCESS_STAT_SQL_ERROR &&
               sql_stats_saver_->IsConnected()));

        // Print internal statistics (like processing speed).
        if ( stat_process_total_count_ % FLAGS_print_statistics == 0 &&
             stat_process_total_count_ > stat_process_last_count_ ) {
          // Some stats fail and they don't count in stat_process_total_count_.
          // The PrintStatistics() sets stat_process_last_count_ =
          //   stat_process_total_count_ .
          PrintStatistics();
        }

        ++num_records;
        if ( num_records >= FLAGS_transaction_records ) {
          break;
        }
      }

      ///////////////////////////////////////////////////////////////////
      // End SQL transaction
      //
      if ( sql_stats_saver_ == NULL ) {
        LOG_INFO << "no SQL server, skipping state save..";
        // no SQL error, no need to clear & load state
        rollback_state = false;
        continue;
      }
      if ( !sql_stats_saver_->EndTransaction() ) {
        LOG_ERROR << "SQL EndTransaction failed, roll back to last save";
        sql_stats_saver_->ClearConnection();
        continue;
      }
      if ( FLAGS_enable_force_insert ) {
        LOG_INFO << "force_insert mode, skipping state save..";
        continue;
      }

      ///////////////////////////////////////////////////////////////////
      // Save internal state
      //
      Save();

      // Delete old input stat files.
      // NOTE: if no SQL server provided (sql_stats_saver_ == NULL)
      //       DON'T delete any files !!!
      CHECK_NOT_NULL(sql_stats_saver_); // sql_stats_saver_ != NULL test above
      MarkDoneOldInputFiles(stats_reader_->Tell().file_num_ - 1);
      //DeleteOldInputFiles(stats_reader_->Tell().file_num_ - 1 -
      //                    (FLAGS_num_input_files_to_keep > 0 ?
      //                     FLAGS_num_input_files_to_keep : 0));

      // no SQL error, no need to clear & load state
      rollback_state = false;
    }

    LOG_INFO << "Stat reader main loop terminated";
  }
  void Stop() {
    exit_event_.Signal();
  }

  void Shutdown() {
    //////////////////////////////////////////////////////////////////////
    //
    // Clear ServerStats map
    //
    ClearServerStats();

    //////////////////////////////////////////////////////////////////////
    //
    // Delete Statistics reader
    //
    delete stats_reader_;
    stats_reader_ = NULL;

    //////////////////////////////////////////////////////////////////////
    //
    // Delete SQL stats saver
    //
    delete sql_stats_saver_;
    sql_stats_saver_ = NULL;

    delete match_content_;
    match_content_ = NULL;
  }
};

const string StatsPostProcessor::keyLogPos("log_pos");
const string StatsPostProcessor::keyPendingStats("pending_stats");


int main (int argc, char *argv[]) {
  return common::Exit(StatsPostProcessor(argc, argv).Main());
}

///////////////////////////////////////////////////////////////////////////////
