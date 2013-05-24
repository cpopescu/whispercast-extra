#ifndef MEDIA_FILE_H_
#define MEDIA_FILE_H_

#include <string>
#include <sstream>
#include <whisperlib/common/base/types.h>
#include "../constants.h"
#include "master_wrapper.h"

namespace manager { namespace slave {

class MasterWrapper;

class MediaFile {
public:
  MediaFile(const string& file_id,
            const string& slave_id,
            MasterWrapper* master,
            const string& remote_scp_path,
            const map<string, string>& process_params,
            FState state,
            const string& error)
    : id_(file_id),
      slave_id_(slave_id),
      master_(master),
      remote_scp_path_(remote_scp_path),
      process_params_(process_params),
      state_(state),
      error_(error),
      transcoding_status_(kEmptyTranscodingStatus),
      ts_transcoding_status_change_(0) {
  }

  const string& id() const { return id_; }
  const string& slave_id() const { return slave_id_; }
  MasterWrapper* master() const { return master_; }
  const string& remote_scp_path() const { return remote_scp_path_; }
  const map<string, string>& process_params() const { return process_params_; }
  FState state() const { return state_; }
  const char* state_name() const { return FStateName(state_); }
  const string& error() const { return error_; }
  const TranscodingStatus& transcoding_status() const { return transcoding_status_; }
  int64 ts_transcoding_status_change() const { return ts_transcoding_status_change_; }

  void set_state(FState state) {
    state_ = state;
  }
  void set_error(const string& error) {
    error_ = error;
  }
  /*
  void set_transcoding_status(const TranscodingStatus& transcoding_status) {
    transcoding_status_ = transcoding_status;
    ts_transcoding_status_change_ = timer::Date::Now();
  }
  void set_ts_transcoding_status_change(int64 ts_transcoding_status_change) {
    ts_transcoding_status_change_ = ts_transcoding_status_change;
  }
  */
  void AddTranscodingMsg(const string& msg) {
    transcoding_status_.messages_.ref().push_back(msg);
    ts_transcoding_status_change_ = timer::Date::Now();
  }
  void SetTranscodingProgress(double progress) {
    transcoding_status_.progress_ = progress;
    ts_transcoding_status_change_ = timer::Date::Now();
  }

  string Extension() const { return strutil::Extension(remote_scp_path_); }
  string OriginalOutputFilename() const { return "original." + Extension(); }

  string ToString() const {
    ostringstream oss;
    oss << "MediaFile{id_=" << id()
        << ", slave_id_=" << slave_id()
        << ", master_uri_=" << master_->uri()
        << ", remote_scp_path_=" << remote_scp_path()
        << ", process_params_=" << strutil::ToString(process_params())
        << ", state_=" << state() << "(" << state_name() << ")"
        << ", error_=" << error()
        << ", transcoding_status_=" << transcoding_status()
        << ", ts_transcoding_status_change_=" << timer::Date(ts_transcoding_status_change())
        << "}";
    return oss.str();
  }

private:
  // file identifier
  const string id_;
  // the slave identifier, according to master. Closely related to file id.
  const string slave_id_;
  // the master who owns the file
  MasterWrapper* const master_;
  // the original master url for the file (where it was downloaded from)
  // e.g. "user@192.168.2.7:/var/media/upload/file1.avi"
  const string remote_scp_path_;
  // processing params (e.g. encoding params)
  map<string, string> process_params_;
  // file state
  FState state_;
  // error description, corresponding to current file state
  string error_;
  // transcoding progress status
  TranscodingStatus transcoding_status_;
  // timestamp of the last transcoding_status change [milliseconds since epoch]
  int64 ts_transcoding_status_change_;
};

} // namespace slave
} // namespace manager

inline std::ostream& operator<<(std::ostream& os, const manager::slave::MediaFile& file) {
  return os << file.ToString();
}

#endif /*MEDIA_FILE_H_*/
