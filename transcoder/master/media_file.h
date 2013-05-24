#ifndef MEDIA_FILE_H_
#define MEDIA_FILE_H_

#include <sstream>
#include "common/base/types.h"
#include "../constants.h"
#include "../auto/types.h"

namespace manager { namespace master {

class MediaFile {
public:
  MediaFile()
    : id_(), path_(),
      state_(FSTATE_PENDING), error_(), ts_state_change_(0),
      transcoding_status_(kEmptyTranscodingStatus) {
  }
  MediaFile(const string& id,
            const string& path,
            const map<string, string>& process_params,
            FState state,
            const string& error,
            int64 ts_state_change,
            const string& slave,
            const TranscodingStatus& transcoding_status)
    : id_(id),
      path_(path),
      process_params_(process_params),
      state_(state),
      error_(error),
      ts_state_change_(ts_state_change),
      slave_(slave),
      transcoding_status_(transcoding_status) {
  }
  MediaFile(const MediaFile& other)
    : id_(other.id()),
      path_(other.path()),
      process_params_(other.process_params()),
      state_(other.state()),
      error_(other.error()),
      ts_state_change_(other.ts_state_change()),
      slave_(other.slave()),
      transcoding_status_(other.transcoding_status()) {
  }

  virtual ~MediaFile() {
  }

  const string& id() const { return id_; }
  const string& path() const { return path_; }
  const map<string, string>& process_params() const { return process_params_; }
  FState state() const { return state_; }
  const char* state_name() const { return FStateName(state_); }
  const string& error() const { return error_; }
  int64 ts_state_change() const { return ts_state_change_; }
  const string& slave() const { return slave_; }
  const TranscodingStatus& transcoding_status() const { return transcoding_status_; }

  void set_id(const string& id) { id_ = id; }
  void set_path(const string& path) { path_ = path; }
  void set_process_params(const map<string, string>& process_params) { process_params_ = process_params; }
  void set_state(FState state) { state_ = state; }
  void set_error(const string& error) { error_ = error; }
  void set_state(FState state, const string& error) { set_state(state); set_error(error); }
  void set_ts_state_change(int64 ts) { ts_state_change_ = ts; }
  void set_slave(const string& slave) { slave_ = slave; }
  void set_transcoding_status(const TranscodingStatus& transcoding_status) { transcoding_status_ = transcoding_status; }

  const MediaFile& operator=(const MediaFile& other) {
    id_ = other.id();
    path_ = other.path();
    process_params_ = other.process_params();
    state_ = other.state();
    error_ = other.error();
    ts_state_change_ = other.ts_state_change();
    slave_ = other.slave();
    transcoding_status_ = other.transcoding_status();
    return *this;
  }

  string ToString() const {
    ostringstream oss;
    oss << "MediaFile{"
        << "id=" << id()
        << ", path_=" << path()
        << ", process_params_=" << strutil::ToString(process_params())
        << ", state_=" << state() << "(" << state_name() << ")"
        << ", error_=" << error()
        << ", ts_state_change_=" << timer::Date(ts_state_change())
        << ", slave_=" << slave()
        << ", transcoding_status_=" << transcoding_status()
        << "}";
    return oss.str();
  }

private:
  // file identifier
  string id_;
  // local file path, absolute
  string path_;
  // parameters for processing
  map<string, string> process_params_;
  // file state
  FState state_;
  // last error description, provided by slave
  string error_;
  // timestamp of the last state change [milliseconds since epoch]
  int64 ts_state_change_;
  // rpc-uri of the slave processing this file
  string slave_;
  // transcoding status
  TranscodingStatus transcoding_status_;
};

} // namespace master
} // namespace manager

inline std::ostream& operator<<(std::ostream& os,
    const manager::master::MediaFile& file) {
  return os << file.ToString();
}

#endif /*MEDIA_FILE_H_*/
