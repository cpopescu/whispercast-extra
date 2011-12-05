// Copyright (c) 2010, Whispersoft s.r.l.
// All rights reserved.

// Author: Cosmin Tudorache

#ifndef __MYSQL_STATS_SAVER_COMPLETE_H__
#define __MYSQL_STATS_SAVER_COMPLETE_H__

#include "mysql_stats_saver.h"

class MySqlMediaStatsSaverComplete : public MySqlMediaStatsSaver {
public:
  MySqlMediaStatsSaverComplete(const string& host, uint16 port,
                               const string& user, const string& passwd,
                               const string& database,
                               const string& table);
  virtual ~MySqlMediaStatsSaverComplete();

  //////////////////////////////////////////////////////////////////////////
  // MySqlMediaStatsSaver methods
  virtual int64 Save(const string& server_id,
                     int64 server_instance,
                     const MediaBegin& media_begin,
                     const MediaEnd& media_end);
};

#endif // __MYSQL_STATS_SAVER_COMPLETE_H__
