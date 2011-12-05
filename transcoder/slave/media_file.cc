#include <sstream>
#include "common/base/log.h"
#include "common/base/date.h"
#include "media_file.h"
#include "master_wrapper.h"

namespace manager { namespace slave {

MediaFile::MediaFile(int64 file_id,
                     const string& slave_id,
                     MasterWrapper* master,
                     const string& remote_url,
                     const string& path,
                     const string& transcoding_status_file,
                     ProcessCmd process_cmd,
                     const map<string, string>& process_params,
                     FState state,
                     const FileTranscodingStatus& transcoding_status)
  : id_(file_id),
    slave_id_(slave_id),
    master_(master),
    remote_url_(remote_url),
    path_(path),
    transcoding_status_file_(transcoding_status_file),
    process_cmd_(process_cmd),
    process_params_(process_params),
    state_(state),
    error_(),
    ts_state_change_(0),
    output_(),
    transcoding_status_(transcoding_status),
    ts_transcoding_status_change_(0) {
}

int64 MediaFile::id() const {
  return id_;
}
const string& MediaFile::slave_id() const {
  return slave_id_;
}
MasterWrapper* MediaFile::master() const {
  return master_;
}
const string& MediaFile::remote_url() const {
  return remote_url_;
}
const string& MediaFile::path() const {
  return path_;
}
const string& MediaFile::transcoding_status_file() const {
  return transcoding_status_file_;
}
ProcessCmd MediaFile::process_cmd() const {
  return process_cmd_;
}
const string& MediaFile::process_cmd_name() const {
  return ProcessCmdName(process_cmd());
}
const map<string, string>& MediaFile::process_params() const {
  return process_params_;
}
FState MediaFile::state() const {
  return state_;
}
const string& MediaFile::state_name2() const {
  return FStateName2(state());
}
const string& MediaFile::error() const {
  return error_;
}
int64 MediaFile::ts_state_change() const {
  return ts_state_change_;
}
const FileSlaveOutput& MediaFile::output() const {
  return output_;
}
const FileTranscodingStatus& MediaFile::transcoding_status() const {
  return transcoding_status_;
}
int64 MediaFile::ts_transcoding_status_change() const {
  return ts_transcoding_status_change_;
}

void MediaFile::set_path(const string& path) {
  path_ = path;
}
void MediaFile::set_transcoding_status_file(const string& transcoding_status_file) {
  transcoding_status_file_ = transcoding_status_file;
}
void MediaFile::set_process_cmd(ProcessCmd process_cmd) {
  process_cmd_ = process_cmd;
}
void MediaFile::set_state(FState state) {
  state_ = state;
  ts_state_change_ = timer::Date::Now();
}
void MediaFile::set_error(const string error) {
  error_ = error;
}
void MediaFile::set_ts_state_change(int64 ts_state_change) {
  ts_state_change_ = ts_state_change;
}
void MediaFile::set_output(const FileSlaveOutput& output) {
  output_ = output;
}
void MediaFile::set_transcoding_status(const FileTranscodingStatus& transcoding_status) {
  transcoding_status_ = transcoding_status;
  ts_transcoding_status_change_ = timer::Date::Now();
}
void MediaFile::set_ts_transcoding_status_change(int64 ts_transcoding_status_change) {
  ts_transcoding_status_change_ = ts_transcoding_status_change;
}

string MediaFile::ToString() const {
  ostringstream oss;
  oss << "MediaFile{id_=" << id()
      << ", slave_id_=" << slave_id()
      << ", master_uri_=" << master_->uri()
      << ", remote_url_=" << remote_url()
      << ", path_=" << path()
      << ", transcoding_status_file_=" << transcoding_status_file()
      << ", process_cmd_=" << process_cmd() << "(" << process_cmd_name() << ")"
      << ", process_params_=" << strutil::ToString(process_params())
      << ", state_=" << state() << "(" << state_name2() << ")"
      << ", error_=" << error()
      << ", ts_state_change_=" << timer::Date(ts_state_change())
      << ", output_=" << output()
      << ", transcoding_status_=" << transcoding_status()
      << ", ts_transcoding_status_change_=" << ts_transcoding_status_change()
      << "}";
  return oss.str();
}

ostream& operator<<(ostream& os, const MediaFile& file) {
  return os << file.ToString();
}

}; }; // namespace slave // namespace manager
