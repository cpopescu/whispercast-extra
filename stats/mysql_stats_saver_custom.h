// Copyright (c) 2010, Whispersoft s.r.l.
// All rights reserved.

// Author: Cosmin Tudorache

#ifndef __MYSQL_STATS_SAVER_CUSTOM_H__
#define __MYSQL_STATS_SAVER_CUSTOM_H__

#include "mysql_stats_saver.h"

class MySqlMediaStatsSaverCustom : public MySqlMediaStatsSaver {
public:
  MySqlMediaStatsSaverCustom(const string& host, uint16 port,
                               const string& user, const string& passwd,
                               const string& database,
                               const string& table,
                               const string& fields);
  virtual ~MySqlMediaStatsSaverCustom();

  //////////////////////////////////////////////////////////////////////////
  // MySqlMediaStatsSaver methods

  virtual bool InitializeConnection();
  virtual void ClearConnection();

  virtual int64 Save(const string& server_id,
                     int64 server_instance,
                     const MediaBegin& media_begin,
                     const MediaEnd& media_end);
private:
  class Field {
  public:
    Field() {}
    virtual ~Field() {}
    virtual mysqlpp::SQLTypeAdapter extract(const string& server_id,
                                            const MediaBegin& media_begin,
                                            const MediaEnd& media_end) = 0;
  };

  class Field_server_id : public Field {
    virtual mysqlpp::SQLTypeAdapter extract(const string& server_id,
                                            const MediaBegin& media_begin,
                                            const MediaEnd& media_end) {
      return mysqlpp::SQLTypeAdapter(mysqlpp::Null<string>(server_id));
    }
  };
  class Field_stream_id : public Field {
    virtual mysqlpp::SQLTypeAdapter extract(const string& server_id,
                                            const MediaBegin& media_begin,
                                            const MediaEnd& media_end) {
      if (media_begin.content_id_.get().empty())
        return mysqlpp::SQLTypeAdapter(mysqlpp::Null<string>(mysqlpp::null));
      return mysqlpp::SQLTypeAdapter(media_begin.content_id_.get());
    }
  };
  class Field_affiliate_id : public Field {
    virtual mysqlpp::SQLTypeAdapter extract(const string& server_id,
                                            const MediaBegin& media_begin,
                                            const MediaEnd& media_end) {
      const StreamBegin& stream_begin = media_begin.stream_.get();
      if (stream_begin.affiliate_id_.get().empty())
        return mysqlpp::SQLTypeAdapter(mysqlpp::Null<string>(mysqlpp::null));
      return mysqlpp::SQLTypeAdapter(stream_begin.affiliate_id_.get());
    }
  };
  class Field_session_id : public Field {
    virtual mysqlpp::SQLTypeAdapter extract(const string& server_id,
                                            const MediaBegin& media_begin,
                                            const MediaEnd& media_end) {
      const StreamBegin& stream_begin = media_begin.stream_.get();
      if (stream_begin.session_id_.get().empty())
        return mysqlpp::SQLTypeAdapter(mysqlpp::Null<string>(mysqlpp::null));
      return mysqlpp::SQLTypeAdapter(stream_begin.session_id_.get());
    }
  };
  class Field_client_id : public Field {
    virtual mysqlpp::SQLTypeAdapter extract(const string& server_id,
                                            const MediaBegin& media_begin,
                                            const MediaEnd& media_end) {
      const StreamBegin& stream_begin = media_begin.stream_.get();
      return mysqlpp::SQLTypeAdapter(stream_begin.client_id_.get());
    }
  };
  class Field_client_host : public Field {
    virtual mysqlpp::SQLTypeAdapter extract(const string& server_id,
                                            const MediaBegin& media_begin,
                                            const MediaEnd& media_end) {
      const StreamBegin& stream_begin = media_begin.stream_.get();
      const ConnectionBegin& connection_begin = stream_begin.connection_.get();
      return mysqlpp::SQLTypeAdapter(connection_begin.remote_host_.get());
    }
  };
  class Field_client_port : public Field {
    virtual mysqlpp::SQLTypeAdapter extract(const string& server_id,
                                            const MediaBegin& media_begin,
                                            const MediaEnd& media_end) {
      const StreamBegin& stream_begin = media_begin.stream_.get();
      const ConnectionBegin& connection_begin = stream_begin.connection_.get();
      return mysqlpp::SQLTypeAdapter(connection_begin.remote_port_.get());
    }
  };
  class Field_local_port : public Field {
    virtual mysqlpp::SQLTypeAdapter extract(const string& server_id,
                                            const MediaBegin& media_begin,
                                            const MediaEnd& media_end) {
      const StreamBegin& stream_begin = media_begin.stream_.get();
      const ConnectionBegin& connection_begin = stream_begin.connection_.get();
      return mysqlpp::SQLTypeAdapter(connection_begin.local_port_.get());
    }
  };
  class Field_begin : public Field {
    virtual mysqlpp::SQLTypeAdapter extract(const string& server_id,
                                            const MediaBegin& media_begin,
                                            const MediaEnd& media_end) {
      timer::Date begin_time(media_begin.timestamp_utc_ms_.get(), true);
      return mysqlpp::SQLTypeAdapter(
        mysqlpp::Null<mysqlpp::DateTime>(
          mysqlpp::DateTime(
            begin_time.GetYear(),
            begin_time.GetMonth()+1,
            begin_time.GetDay(),
            begin_time.GetHour(),
            begin_time.GetMinute(),
            begin_time.GetSecond()
          )
        )
      );
    }
  };
  class Field_end : public Field {
    virtual mysqlpp::SQLTypeAdapter extract(const string& server_id,
                                            const MediaBegin& media_begin,
                                            const MediaEnd& media_end) {
      timer::Date end_time(media_end.timestamp_utc_ms_.get(), true);
      return mysqlpp::SQLTypeAdapter(
        mysqlpp::Null<mysqlpp::DateTime>(
          mysqlpp::DateTime(
            end_time.GetYear(),
            end_time.GetMonth()+1,
            end_time.GetDay(),
            end_time.GetHour(),
            end_time.GetMinute(),
            end_time.GetSecond()
          )
        )
      );
    }
  };
  class Field_video_frames : public Field {
    virtual mysqlpp::SQLTypeAdapter extract(const string& server_id,
                                            const MediaBegin& media_begin,
                                            const MediaEnd& media_end) {
      return mysqlpp::SQLTypeAdapter(media_end.video_frames_.get());
    }
  };
  class Field_video_dropped_frames : public Field {
    virtual mysqlpp::SQLTypeAdapter extract(const string& server_id,
                                            const MediaBegin& media_begin,
                                            const MediaEnd& media_end) {
      return mysqlpp::SQLTypeAdapter(media_end.video_frames_dropped_.get());
    }
  };
  class Field_audio_frames : public Field {
    virtual mysqlpp::SQLTypeAdapter extract(const string& server_id,
                                            const MediaBegin& media_begin,
                                            const MediaEnd& media_end) {
      return mysqlpp::SQLTypeAdapter(media_end.audio_frames_.get());
    }
  };
  class Field_audio_dropped_frames : public Field {
    virtual mysqlpp::SQLTypeAdapter extract(const string& server_id,
                                            const MediaBegin& media_begin,
                                            const MediaEnd& media_end) {
      return mysqlpp::SQLTypeAdapter(media_end.audio_frames_dropped_.get());
    }
  };
  class Field_total_bytes_up : public Field {
    virtual mysqlpp::SQLTypeAdapter extract(const string& server_id,
                                            const MediaBegin& media_begin,
                                            const MediaEnd& media_end) {
      return mysqlpp::SQLTypeAdapter(media_end.bytes_up_.get());
    }
  };
  class Field_total_bytes_down : public Field {
    virtual mysqlpp::SQLTypeAdapter extract(const string& server_id,
                                            const MediaBegin& media_begin,
                                            const MediaEnd& media_end) {
      return mysqlpp::SQLTypeAdapter(media_end.bytes_down_.get());
    }
  };
  class Field_result : public Field {
    virtual mysqlpp::SQLTypeAdapter extract(const string& server_id,
                                            const MediaBegin& media_begin,
                                            const MediaEnd& media_end) {
      const string& result = media_end.result_.get().result_.get();
      if (result.empty())
        return mysqlpp::SQLTypeAdapter(mysqlpp::Null<string>(mysqlpp::null));
      return mysqlpp::SQLTypeAdapter(result);
    }
  };

  const string& fields_;
  map<string, Field*> field_map_;
};

#endif // __MYSQL_STATS_SAVER_CUSTOM_H__
