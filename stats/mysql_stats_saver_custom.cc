// Copyright (c) 2010, Whispersoft s.r.l.
// All rights reserved.

// Author: Cosmin Tudorache

#include "mysql_stats_saver_custom.h"

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

MySqlMediaStatsSaverCustom::MySqlMediaStatsSaverCustom(
    const string& host, uint16 port,
    const string& user, const string& passwd,
    const string& database,
    const string& table,
    const string& fields)
  : MySqlMediaStatsSaver(host, port, user, passwd, database, table),
    fields_(fields) {
}

MySqlMediaStatsSaverCustom::~MySqlMediaStatsSaverCustom () {
  CHECK(field_map_.empty());
}

bool MySqlMediaStatsSaverCustom::InitializeConnection() {
  map<string, string> field_map;
  strutil::SplitPairs(fields_, string(":"), string(","), &field_map);

  for (std::map<string, string>::const_iterator f = field_map.begin();
      f != field_map.end(); ++f) {
    if (field_map_.find(f->first) != field_map_.end()) {
      LOG_ERROR << "Duplicate table field \"" << f->first << "\"";
      ClearConnection();
      return false;
    }

    if (f->second == "server_id") {
      field_map_[f->first] = new Field_server_id();
    } else
    if (f->second == "stream_id") {
      field_map_[f->first] = new Field_stream_id();
    } else
    if (f->second == "affiliate_id") {
      field_map_[f->first] = new Field_affiliate_id();
    } else
    if (f->second == "session_id") {
      field_map_[f->first] = new Field_session_id();
    } else
    if (f->second == "client_id") {
      field_map_[f->first] = new Field_client_id();
    } else
    if (f->second == "client_host") {
      field_map_[f->first] = new Field_client_host();
    } else
    if (f->second == "client_port") {
      field_map_[f->first] = new Field_client_port();
    } else
    if (f->second == "local_port") {
      field_map_[f->first] = new Field_local_port();
    } else
    if (f->second == "begin") {
      field_map_[f->first] = new Field_begin();
    } else
    if (f->second == "end") {
      field_map_[f->first] = new Field_end();
    } else
    if (f->second == "video_frames") {
      field_map_[f->first] = new Field_video_frames();
    } else
    if (f->second == "video_dropped_frames") {
      field_map_[f->first] = new Field_video_dropped_frames();
    } else
    if (f->second == "audio_frames") {
      field_map_[f->first] = new Field_audio_frames();
    } else
    if (f->second == "audio_dropped_frames") {
      field_map_[f->first] = new Field_audio_dropped_frames();
    } else
    if (f->second == "total_bytes_up") {
      field_map_[f->first] = new Field_total_bytes_up();
    } else
    if (f->second == "total_bytes_down") {
      field_map_[f->first] = new Field_total_bytes_down();
    } else
    if (f->second == "result") {
      field_map_[f->first] = new Field_result();
    } else {
      LOG_ERROR << "Unknown log field \"" << f->second << "\"";
      ClearConnection();
      return false;
    }
  }

  if (!MySqlMediaStatsSaver::InitializeConnection()) {
    return false;
  }
  CHECK_NOT_NULL(sql_query_);
  try {
    *sql_query_ << "insert into %" << field_map_.size() << ":table(";
    for (std::map<string, Field*>::const_iterator f = field_map_.begin();
        f != field_map_.end(); ++f) {
      *sql_query_ << f->first;
    }
    *sql_query_ << ") values (";
    int index = 0;
    for (std::map<string, Field*>::const_iterator f = field_map_.begin();
        f != field_map_.end(); ++f) {
      if (f != field_map_.begin()) {
        *sql_query_ << ',';
      }
      *sql_query_ << '%' << index << 'q';
      ++index;
    }
    *sql_query_ << ");";

    sql_query_->parse();
  } catch ( const mysqlpp::Exception& er ) {
    // Catch-all for any other MySQL++ exceptions
    LOG_ERROR << "MySQL error:\n"
              << er.what() << "\nMySQL failed to initialize";
    ClearConnection();
    return false;
  }
  LOG_INFO << "The MySQL custom query setup successfully";
  return true;
}

void MySqlMediaStatsSaverCustom::ClearConnection() {
  while (!field_map_.empty()) {
    delete field_map_.begin()->second;
    field_map_.erase(field_map_.begin());
  }
  MySqlMediaStatsSaver::ClearConnection();
}

int64 MySqlMediaStatsSaverCustom::Save(const string& server_id,
                                         int64 server_instance,
                                         const MediaBegin& media_begin,
                                         const MediaEnd& media_end) {
  try{
    mysqlpp::SQLQueryParms sql_params(sql_query_);
    for (std::map<string, Field*>::iterator f = field_map_.begin();
        f != field_map_.end(); ++f) {
      sql_params += f->second->extract(server_id, media_begin, media_end);
    }
    sql_query_->execute(sql_params);
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
