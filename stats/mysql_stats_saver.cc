// Copyright (c) 2010, Whispersoft s.r.l.
// All rights reserved.

// Author: Cosmin Tudorache

#include "mysql_stats_saver.h"

MySqlMediaStatsSaver::MySqlMediaStatsSaver(const string& host, uint16 port,
    const string& user, const string& passwd, const string& database,
    const string& table)
  : host_(host), port_(port), user_(user),
    passwd_(passwd), database_(database), table_(table),
    sql_conn_(NULL), sql_query_(NULL),
    sql_transaction_(NULL),
    stat_count_saved_(0) {
}
MySqlMediaStatsSaver::~MySqlMediaStatsSaver() {
  ClearConnection();
}

bool MySqlMediaStatsSaver::InitializeConnection() {
  LOG_INFO << "Initializing the MySQL connection, using settings: \n"
           << " - host_: [" << host_ << "]\n"
           << " - port_: " << port_ << "\n"
           << " - database_: [" << database_ << "]\n"
           << " - table_: [" << table_ << "]\n"
           << " - user_: [" << user_ << "]\n"
           << " - passwd_: [" << passwd_ << "]";

  CHECK_NULL(sql_conn_);
  try {
    sql_conn_ = new mysqlpp::Connection(
      database_.c_str(),
      host_.empty() ? NULL : host_.c_str(),
      user_.empty() ? NULL : user_.c_str(),
      passwd_.empty() ? NULL : passwd_.c_str(),
      port_);
    sql_query_ = new mysqlpp::Query(sql_conn_->query());

    LOG_INFO << "The MySQL connection initialized successfully";
  } catch ( const mysqlpp::Exception& er ) {
    // Catch-all for any other MySQL++ exceptions
    LOG_ERROR << "MySQL error:\n"
              << er.what() << "\nMySQL failed to initialize";
    ClearConnection();
    return false;
  }
  return true;
}

void MySqlMediaStatsSaver::ClearConnection() {
  delete sql_transaction_;
  sql_transaction_ = NULL;
  delete sql_query_;
  sql_query_ = NULL;
  delete sql_conn_;
  sql_conn_ = NULL;
}

bool MySqlMediaStatsSaver::IsConnected() const {
  return sql_conn_ != NULL;
}

bool MySqlMediaStatsSaver::BeginTransaction() {
  CHECK_NULL(sql_transaction_) << "SQL transaction already in progress..";
  try {
    sql_transaction_ = new mysqlpp::Transaction(*sql_conn_);
  } catch (const mysqlpp::Exception& er) {
    LOG_ERROR << "SQL Error: " << er.what() << endl;
    ClearConnection();
    return false;
  }
  return true;
}
bool MySqlMediaStatsSaver::EndTransaction() {
  CHECK_NOT_NULL(sql_transaction_) << "SQL transaction not found";
  try {
    sql_transaction_->commit();
  } catch (const mysqlpp::Exception& er) {
    LOG_ERROR << "SQL Error: " << er.what() << endl;
    ClearConnection();
    return false;
  }
  delete sql_transaction_;
  sql_transaction_ = NULL;
  return true;
}

int64 MySqlMediaStatsSaver::stat_count_saved() const {
  return stat_count_saved_;
}
