#ifndef MEDIA_FILE_H_
#define MEDIA_FILE_H_

#include <sstream>
#include "common/base/types.h"
#include "../constants.h"
#include "../auto/types.h"

namespace manager { namespace master {

class MediaFile {
public:
  MediaFile();
  MediaFile(int64 id,
            const string& path,
            ProcessCmd process_cmd,
            const map<string, string>& process_params,
            FState state,
            const string& error,
            int64 ts_state_change,
            const string& slave,
            const FileSlaveOutput& output,
            const FileTranscodingStatus& transcoding_status);
  MediaFile(const MediaFile& src);
  ~MediaFile();

  int64 id() const;
  const string& path() const;
  ProcessCmd process_cmd() const;
  const string& process_cmd_name() const;
  const map<string, string>& process_params() const;
  FState state() const;
  const string& state_name2() const;
  const string& error() const;
  int64 ts_state_change() const;
  const string& slave() const;
  const FileSlaveOutput& output() const;
  const FileTranscodingStatus& transcoding_status() const;

  void set_id(int64 id);
  void set_path(const string& path);
  void set_process_cmd(ProcessCmd process_cmd);
  void set_process_params(const map<string, string>& process_params);
  void set_state(FState state);
  void set_error(const string& error);
  void set_state(FState state, const string& error);
  void set_ts_state_change(int64 ts);
  void set_slave(const string& slave);
  void set_output(const FileSlaveOutput& output);
  void set_transcoding_status(const FileTranscodingStatus& transcoding_status);

  const MediaFile& operator=(const MediaFile& src);

  string ToString() const;

private:
  // file identifier
  int64 id_;
  // local file path, absolute
  string path_;
  // what to do with this file (e.g. transcode, post-process or just copy ..)
  ProcessCmd process_cmd_;
  // parameters for process_cmd
  map<string, string> process_params_;
  // file state
  FState state_;
  // last error description, provided by slave
  string error_;
  // timestamp of the last state change [milliseconds since epoch]
  int64 ts_state_change_;
  // rpc-uri of the slave processing this file
  string slave_;
  // path to output (result) files. These are slave relative paths.
  FileSlaveOutput output_;
  // transcoding status
  FileTranscodingStatus transcoding_status_;
};

ostream& operator<<(ostream& os, const MediaFile& file);

}; }; // namespace master // namespace manager

#endif /*MEDIA_FILE_H_*/
