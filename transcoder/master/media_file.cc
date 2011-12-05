
#include <sstream>
#include "common/base/log.h"
#include "common/base/date.h"
#include "media_file.h"

namespace manager { namespace master {

MediaFile::MediaFile()
  : id_(INVALID_FILE_ID), path_(),
    process_cmd_(PROCESS_CMD_TRANSCODE),
    state_(FSTATE_IDLE), error_(), ts_state_change_(0), output_(),
    transcoding_status_(kEmptyFileTranscodingStatus) {
}
MediaFile::MediaFile(int64 id,
                     const string& path,
                     ProcessCmd process_cmd,
                     const map<string, string>& process_params,
                     FState state,
                     const string& error,
                     int64 ts_state_change,
                     const string& slave,
                     const FileSlaveOutput& output,
                     const FileTranscodingStatus& transcoding_status)
  : id_(id),
    path_(path),
    process_cmd_(process_cmd),
    process_params_(process_params),
    state_(state),
    error_(error),
    ts_state_change_(ts_state_change),
    slave_(slave),
    output_(output),
    transcoding_status_(transcoding_status) {
}
MediaFile::MediaFile(const MediaFile& src)
  : id_(src.id()),
    path_(src.path()),
    process_cmd_(src.process_cmd()),
    process_params_(src.process_params()),
    state_(src.state()),
    error_(src.error()),
    ts_state_change_(src.ts_state_change()),
    slave_(src.slave()),
    output_(src.output()),
    transcoding_status_(src.transcoding_status()){
}
MediaFile::~MediaFile() {
}

int64 MediaFile::id() const {
  return id_;
}
const string& MediaFile::path() const {
  return path_;
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
const string& MediaFile::slave() const {
  return slave_;
}
const FileSlaveOutput& MediaFile::output() const {
  return output_;
}
const FileTranscodingStatus& MediaFile::transcoding_status() const {
  return transcoding_status_;
}

void MediaFile::set_id(int64 id) {
  id_ = id;
}
void MediaFile::set_path(const string& path) {
  path_ = path;
}
void MediaFile::set_process_cmd(ProcessCmd process_cmd) {
  process_cmd_ = process_cmd;
}
void MediaFile::set_process_params(const map<string, string>& process_params) {
  process_params_ = process_params;
}
void MediaFile::set_state(FState state) {
  state_ = state;
}
void MediaFile::set_error(const string& error) {
  error_ = error;
}
void MediaFile::set_state(FState state, const string& error) {
  set_state(state);
  set_error(error);
}
void MediaFile::set_ts_state_change(int64 ts) {
  ts_state_change_ = ts;
}
void MediaFile::set_slave(const string& slave) {
  slave_ = slave;
}
void MediaFile::set_output(const FileSlaveOutput& output) {
  output_ = output;
}
void MediaFile::set_transcoding_status(const FileTranscodingStatus& transcoding_status) {
  transcoding_status_ = transcoding_status;
}

const MediaFile& MediaFile::operator=(const MediaFile& src) {
  set_id(src.id());
  set_path(src.path());
  set_process_cmd(src.process_cmd());
  set_process_params(src.process_params());
  set_state(src.state());
  set_error(src.error());
  set_ts_state_change(src.ts_state_change());
  set_slave(src.slave());
  set_output(src.output());
  set_transcoding_status(src.transcoding_status());
  return *this;
}

string MediaFile::ToString() const {
  ostringstream oss;
  oss << "MediaFile{"
      << "id=" << id()
      << ", path_=" << path()
      << ", process_cmd_=" << process_cmd_name()
      << ", process_params_=" << strutil::ToString(process_params())
      << ", state_=" << state() << "(" << state_name2() << ")"
      << ", error_=" << error()
      << ", ts_state_change_=" << timer::Date(ts_state_change())
      << ", slave_=" << slave()
      << ", output_=" << output()
      << ", transcoding_status_=" << transcoding_status()
      << "}";
  return oss.str();
}

ostream& operator<<(ostream& os, const MediaFile& file) {
  return os << file.ToString();
}

}; }; // namespace master // namespace manager
