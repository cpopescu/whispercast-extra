// Copyright (c) 2010, Whispersoft s.r.l.
// All rights reserved.

// Author: Cosmin Tudorache

#include "mysql_stats_saver_light.h"

namespace {
sql_create_9(
  connections,
  1, 9,
  mysqlpp::Null<mysqlpp::sql_bigint>, id,
  mysqlpp::sql_char, server_id,
  mysqlpp::Null<mysqlpp::sql_char>, stream_id,
  mysqlpp::Null<mysqlpp::sql_char>, affiliate_id,
  mysqlpp::Null<mysqlpp::sql_char>, client_id,
  mysqlpp::sql_char, client_host,
  mysqlpp::sql_char, begin,
  mysqlpp::sql_char, end,
  mysqlpp::Null<mysqlpp::sql_char>, result);
}

/* The corresponding MySQL table:

CREATE TABLE `connections` (
  `id` int(11) NOT NULL auto_increment,
  `server_id` char(32) collate utf8_unicode_ci default NULL,
  `stream_id` char(32) collate utf8_unicode_ci default NULL,
  `affiliate_id` char(32) collate utf8_unicode_ci default NULL,
  `client_id` char(32) collate utf8_unicode_ci default NULL,
  `client_host` char(15) collate utf8_unicode_ci default NULL,
  `begin` datetime NOT NULL,
  `end` datetime NOT NULL,
  `result` char(32) collate utf8_unicode_ci default NULL,
  PRIMARY KEY  (`id`),
  KEY `stream_id` (`stream_id`),
  KEY `client_id` (`client_id`)
  ) ENGINE=InnoDB DEFAULT CHARSET=utf8 COLLATE=utf8_unicode_ci ;
*/

//////////////////////////////////////////////////////////////////////

MySqlMediaStatsSaverLight::MySqlMediaStatsSaverLight(
    const string& host, uint16 port,
    const string& user, const string& passwd,
    const string& database,
    const string& table)
  : MySqlMediaStatsSaver(host, port, user, passwd, database, table) {
  connections::table(table_.c_str());
}
MySqlMediaStatsSaverLight::~MySqlMediaStatsSaverLight() {
}


int64 MySqlMediaStatsSaverLight::Save(const string& server_id,
                                      int64 server_instance,
                                      const MediaBegin& media_begin,
                                      const MediaEnd& media_end) {
  const StreamBegin& stream_begin = media_begin.stream_;
  const ConnectionBegin& connection_begin = stream_begin.connection_;

  // Convert all the timestamps to localtime, as mysqlpp needs it
  timer::Date begin_time(media_begin.timestamp_utc_ms_, true);
  timer::Date end_time(media_end.timestamp_utc_ms_, true);
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
      (media_begin.content_id_.empty()
       ? mysqlpp::Null<mysqlpp::sql_char>(mysqlpp::null)
       : mysqlpp::Null<mysqlpp::sql_char>(
           media_begin.content_id_));
  mysqlpp::Null<mysqlpp::sql_char> sql_affiliate_id =
      (stream_begin.affiliate_id_.empty()
       ? mysqlpp::Null<mysqlpp::sql_char>(mysqlpp::null)
       : mysqlpp::Null<mysqlpp::sql_char>(
           stream_begin.affiliate_id_));
  mysqlpp::Null<mysqlpp::sql_char> sql_client_id =
      (stream_begin.client_id_.empty()
       ? mysqlpp::Null<mysqlpp::sql_char>(mysqlpp::null)
       : mysqlpp::Null<mysqlpp::sql_char>(
           stream_begin.client_id_));
  mysqlpp::sql_char sql_client_host = connection_begin.remote_host_;
  mysqlpp::sql_char sql_begin = str_begin_time;
  mysqlpp::sql_char sql_end = str_end_time;
  mysqlpp::Null<mysqlpp::sql_char> sql_result =
    //mysqlpp::Null<mysqlpp::sql_char>(mysqlpp::null);
    (result.empty() ? mysqlpp::Null<mysqlpp::sql_char>(mysqlpp::null)
                    : mysqlpp::Null<mysqlpp::sql_char>(result));


  LOG_DEBUG << " Saving to MySQL:"
            << "  id: " << sql_id
            << ", server_id: " << sql_server_id
            << ", stream_id: " << sql_stream_id
            << ", affiliate_id: " << sql_affiliate_id
            << ", client_id: " << sql_client_id
            << ", client_host: " << sql_client_host
            << ", begin: " << sql_begin
            << ", end: " << sql_end
            << ", result: " << sql_result;

  // insert / update one line in SQL table
  try {
    connections row(sql_id,
                    sql_server_id,
                    sql_stream_id,
                    sql_affiliate_id,
                    sql_client_id,
                    sql_client_host,
                    sql_begin,
                    sql_end,
                    sql_result);
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
