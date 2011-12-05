// Copyright (c) 2010, Whispersoft s.r.l.
// All rights reserved.

// Author: Cosmin Tudorache

#ifndef __MYSQL_STATS_SAVER_H__
#define __MYSQL_STATS_SAVER_H__

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


class MySqlMediaStatsSaver {
 public:
  MySqlMediaStatsSaver(const string& host, uint16 port,
                       const string& user, const string& passwd,
                       const string& database,
                       const string& table);
  virtual ~MySqlMediaStatsSaver();

  virtual bool InitializeConnection();
  virtual void ClearConnection();

  bool IsConnected() const;

  // returns the ID for the newly inserted row (>0) on success.
  // returns -1 on failure.
  virtual int64 Save(const string& server_id,
                     int64 server_instance,
                     const MediaBegin& media_begin,
                     const MediaEnd& media_end) = 0;
  bool BeginTransaction();
  bool EndTransaction();

  int64 stat_count_saved() const;

 protected:
  // How to connect to the database
  const string host_;
  const uint16 port_;
  const string user_;
  const string passwd_;
  const string database_;
  const string table_;

  mysqlpp::Connection* sql_conn_;
  mysqlpp::Query* sql_query_;
  mysqlpp::Transaction* sql_transaction_;

  // count rows saved to mysql
  int64 stat_count_saved_;

  DISALLOW_EVIL_CONSTRUCTORS(MySqlMediaStatsSaver);
};

#endif // __MYSQL_STATS_SAVER_H__
