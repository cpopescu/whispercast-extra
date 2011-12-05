// Copyright (c) 2010, Whispersoft s.r.l.
// All rights reserved.

// Author: Cosmin Tudorache

#include "mysql_stats_saver_complete.h"

namespace {
sql_create_24(
  connections,
  1, 24,
  mysqlpp::Null<mysqlpp::sql_bigint>, id,
  mysqlpp::sql_char, server_id,
  mysqlpp::Null<mysqlpp::sql_char>, stream_id,
  mysqlpp::Null<mysqlpp::sql_char>, affiliate_id,
  mysqlpp::Null<mysqlpp::sql_char>, session_id,
  mysqlpp::Null<mysqlpp::sql_char>, client_id,
  mysqlpp::sql_char, client_host,
  mysqlpp::sql_int, client_port,
  mysqlpp::sql_int, local_port,
  mysqlpp::sql_char, begin,
  mysqlpp::sql_char, end,
  mysqlpp::sql_bigint, video_frames,
  mysqlpp::sql_bigint, video_bytes,
  mysqlpp::sql_bigint, video_dropped_frames,
  mysqlpp::sql_bigint, video_dropped_bytes,
  mysqlpp::sql_bigint, audio_frames,
  mysqlpp::sql_bigint, audio_bytes,
  mysqlpp::sql_bigint, audio_dropped_frames,
  mysqlpp::sql_bigint, audio_dropped_bytes,
  mysqlpp::sql_bigint, total_bytes_up,
  mysqlpp::sql_bigint, total_bytes_down,
  mysqlpp::Null<mysqlpp::sql_char>, result,
  mysqlpp::Null<mysqlpp::sql_int>, previous_id,
  mysqlpp::Null<mysqlpp::sql_int>, next_id);
}

/* The corresponding MySQL table:

CREATE TABLE `connections` (
  `id` int(11) NOT NULL auto_increment,
  `server_id` char(32) collate utf8_unicode_ci default NULL,
  `stream_id` char(32) collate utf8_unicode_ci default NULL,
  `affiliate_id` char(32) collate utf8_unicode_ci default NULL,
  `session_id` char(32) collate utf8_unicode_ci default NULL,
  `client_id` char(32) collate utf8_unicode_ci default NULL,
  `client_host` char(15) collate utf8_unicode_ci default NULL,
  `client_port` int(10) unsigned default NULL,
  `local_port` int(10) unsigned default NULL,
  `begin` datetime NOT NULL,
  `end` datetime NOT NULL,
  `video_frames` bigint(20) unsigned default NULL,
  `video_bytes` bigint(20) unsigned default NULL,
  `video_dropped_frames` bigint(20) unsigned default NULL,
  `video_dropped_bytes` bigint(20) unsigned default NULL,
  `audio_frames` bigint(20) unsigned default NULL,
  `audio_bytes` bigint(20) unsigned default NULL,
  `audio_dropped_frames` bigint(20) unsigned default NULL,
  `audio_dropped_bytes` bigint(20) unsigned default NULL,
  `total_bytes_up` bigint(20) unsigned default NULL,
  `total_bytes_down` bigint(20) unsigned default NULL,
  `result` char(32) collate utf8_unicode_ci default NULL,
  `previous_id` int(11) default NULL,
  `next_id` int(11) default NULL,
  PRIMARY KEY  (`id`),
  KEY `stream_id` (`stream_id`),
  KEY `session_id` (`session_id`),
  KEY `client_id` (`client_id`),
  KEY `result` (`result`)
  ) ENGINE=InnoDB DEFAULT CHARSET=utf8 COLLATE=utf8_unicode_ci ;
*/

//////////////////////////////////////////////////////////////////////

MySqlMediaStatsSaverComplete::MySqlMediaStatsSaverComplete(
    const string& host, uint16 port,
    const string& user, const string& passwd,
    const string& database,
    const string& table)
  : MySqlMediaStatsSaver(host, port, user, passwd, database, table) {
  connections::table(table_.c_str());
}

MySqlMediaStatsSaverComplete::~MySqlMediaStatsSaverComplete () {
}

int64 MySqlMediaStatsSaverComplete::Save(const string& server_id,
                                         int64 server_instance,
                                         const MediaBegin& media_begin,
                                         const MediaEnd& media_end) {
  const StreamBegin& stream_begin = media_begin.stream_.get();
  const ConnectionBegin& connection_begin = stream_begin.connection_.get();

  // Convert all the timestamps to localtime, as mysqlpp needs it
  timer::Date begin_time(media_begin.timestamp_utc_ms_.get(), true);
  timer::Date end_time(media_end.timestamp_utc_ms_.get(), true);
  const string str_begin_time(
      strutil::StringPrintf("%04d-%02d-%02d %02d:%02d:%02d",
                            static_cast<int32>(begin_time.GetYear()),
                            static_cast<int32>(begin_time.GetMonth() + 1),
                            static_cast<int32>(begin_time.GetDay()),
                            static_cast<int32>(begin_time.GetHour()),
                            static_cast<int32>(begin_time.GetMinute()),
                            static_cast<int32>(begin_time.GetSecond())));
  const string str_end_time(
      strutil::StringPrintf("%04d-%02d-%02d %02d:%02d:%02d",
                            static_cast<int32>(end_time.GetYear()),
                            static_cast<int32>(end_time.GetMonth() + 1),
                            static_cast<int32>(end_time.GetDay()),
                            static_cast<int32>(end_time.GetHour()),
                            static_cast<int32>(end_time.GetMinute()),
                            static_cast<int32>(end_time.GetSecond())));

  const string& result = media_end.result_.get().result_.get();

  mysqlpp::Null<mysqlpp::sql_bigint> sql_id =
    mysqlpp::Null<mysqlpp::sql_bigint>(0);
  sql_id = mysqlpp::null; // fix warning in g++ 4.3.4
  mysqlpp::sql_char sql_server_id = server_id;
  mysqlpp::Null<mysqlpp::sql_char> sql_stream_id =
      (media_begin.content_id_.get().empty()
       ? mysqlpp::Null<mysqlpp::sql_char>(mysqlpp::null)
       : mysqlpp::Null<mysqlpp::sql_char>(
           media_begin.content_id_.get()));
  mysqlpp::Null<mysqlpp::sql_char> sql_affiliate_id =
      (stream_begin.affiliate_id_.get().empty()
       ? mysqlpp::Null<mysqlpp::sql_char>(mysqlpp::null)
       : mysqlpp::Null<mysqlpp::sql_char>(
           stream_begin.affiliate_id_.get()));
  mysqlpp::Null<mysqlpp::sql_char> sql_session_id =
      (stream_begin.session_id_.get().empty()
       ? mysqlpp::Null<mysqlpp::sql_char>(mysqlpp::null)
       : mysqlpp::Null<mysqlpp::sql_char>(
           stream_begin.session_id_.get()));
  mysqlpp::Null<mysqlpp::sql_char> sql_client_id =
      (stream_begin.client_id_.get().empty()
       ? mysqlpp::Null<mysqlpp::sql_char>(mysqlpp::null)
       : mysqlpp::Null<mysqlpp::sql_char>(
           stream_begin.client_id_.get()));
  mysqlpp::sql_char sql_client_host = connection_begin.remote_host_.get();
  mysqlpp::sql_int sql_client_port = connection_begin.remote_port_.get();
  mysqlpp::sql_int sql_local_port = connection_begin.local_port_.get();
  mysqlpp::sql_char sql_begin = str_begin_time;
  mysqlpp::sql_char sql_end = str_end_time;
  mysqlpp::sql_bigint sql_video_frames = media_end.video_frames_.get();
  mysqlpp::sql_bigint sql_video_bytes = 0;  // stat->video_bytes_;
  mysqlpp::sql_bigint sql_video_dropped_frames =
      media_end.video_frames_dropped_.get();
  mysqlpp::sql_bigint sql_video_dropped_bytes = 0; // stat->video_dropped_bytes_;
  mysqlpp::sql_bigint sql_audio_frames = media_end.audio_frames_.get();
  mysqlpp::sql_bigint sql_audio_bytes = 0;  // stat->audio_bytes_;
  mysqlpp::sql_bigint sql_audio_dropped_frames =
      media_end.audio_frames_dropped_.get();
  mysqlpp::sql_bigint sql_audio_dropped_bytes = 0;  // stat->audio_dropped_bytes_;
  mysqlpp::sql_bigint sql_total_bytes_up = media_end.bytes_up_.get();
  mysqlpp::sql_bigint sql_total_bytes_down = media_end.bytes_down_.get();
  mysqlpp::Null<mysqlpp::sql_char> sql_result =
    //mysqlpp::Null<mysqlpp::sql_char>(mysqlpp::null);
    (result.empty() ? mysqlpp::Null<mysqlpp::sql_char>(mysqlpp::null)
                    : mysqlpp::Null<mysqlpp::sql_char>(result));
  mysqlpp::Null<mysqlpp::sql_int> sql_previous_id =
    mysqlpp::Null<mysqlpp::sql_int>(0);
  sql_previous_id = mysqlpp::null; // fix warning in g++ 4.3.4
  mysqlpp::Null<mysqlpp::sql_int> sql_next_id =
    mysqlpp::Null<mysqlpp::sql_int>(0);
  sql_next_id = mysqlpp::null; // fix warning in g++ 4.3.4

  LOG_DEBUG << " Saving to MySQL:"
            << "  id: " << sql_id
            << ", server_id: " << sql_server_id
            << ", stream_id: " << sql_stream_id
            << ", affiliate_id: " << sql_affiliate_id
            << ", session_id: " << sql_session_id
            << ", client_id: " << sql_client_id
            << ", client_host: " << sql_client_host
            << ", client_port: " << sql_client_port
            << ", local_port: " << sql_local_port
            << ", begin: " << sql_begin
            << ", end: " << sql_end
            << ", video_frames: " << sql_video_frames
            << ", video_bytes: " << sql_video_bytes
            << ", video_dropped_frames: " << sql_video_dropped_frames
            << ", video_dropped_bytes: " << sql_video_dropped_bytes
            << ", audio_frames: " << sql_audio_frames
            << ", audio_bytes: " << sql_audio_bytes
            << ", audio_dropped_frames: " << sql_audio_dropped_frames
            << ", audio_dropped_bytes: " << sql_audio_dropped_bytes
            << ", total_bytes_up: " << sql_total_bytes_up
            << ", total_bytes_down: " << sql_total_bytes_down
            << ", result: " << sql_result
            << ", previous_id: " << sql_previous_id
            << ", next_id: " << sql_next_id;

  // insert / update one line in SQL table
  try {
    connections row(sql_id,
                    sql_server_id,
                    sql_stream_id,
                    sql_affiliate_id,
                    sql_session_id,
                    sql_client_id,
                    sql_client_host,
                    sql_client_port,
                    sql_local_port,
                    sql_begin,
                    sql_end,
                    sql_video_frames,
                    sql_video_bytes,
                    sql_video_dropped_frames,
                    sql_video_dropped_bytes,
                    sql_audio_frames,
                    sql_audio_bytes,
                    sql_audio_dropped_frames,
                    sql_audio_dropped_bytes,
                    sql_total_bytes_up,
                    sql_total_bytes_down,
                    sql_result,
                    sql_previous_id,
                    sql_next_id);
    sql_query_->insert(row);
    sql_query_->execute();
    stat_count_saved_++;
    // return the ID generated for the AUTO_INCREMENT column
    return sql_query_->insert_id();
  } catch (const mysqlpp::ConnectionFailed& er) {
    LOG_ERROR << "SQL Connection error: " << er.what();
    ClearConnection();
    return -1;
  } catch (const mysqlpp::BadQuery& er) {
    LOG_ERROR << "SQL Query error: " << er.what();
    ClearConnection();
    return -1;
  } catch (const mysqlpp::BadConversion& er) {
    LOG_ERROR << "SQL Conversion error: " << er.what()
              << "\n\tretrieved data size: " << er.retrieved
              << ", actual size: " << er.actual_size;
    ClearConnection();
    return -1;
  } catch (const mysqlpp::Exception& er) {
    LOG_ERROR << "SQL Error: " << er.what() << endl;
    ClearConnection();
    return -1;
  }
}
