#ifndef MEDIA_FILE_H_
#define MEDIA_FILE_H_

#include <string>
#include <iostream>
#include "common/base/types.h"
#include "../constants.h"
#include "master_wrapper.h"

namespace manager { namespace slave {

class MasterWrapper;

class MediaFile {
public:
  MediaFile(int64 file_id,
            const string& slave_id,
            MasterWrapper* master,
            const string& remote_url,
            const string& path,
            const string& transcoding_status_file_,
            ProcessCmd process_cmd,
            const map<string, string>& process_params,
            FState state,
            const FileTranscodingStatus& transcoding_status);

  int64 id() const;
  const string& slave_id() const;
  MasterWrapper* master() const;
  const string& remote_url() const;
  const string& path() const;
  const string& transcoding_status_file() const;
  ProcessCmd process_cmd() const;
  const string& process_cmd_name() const;
  const map<string, string>& process_params() const;
  FState state() const;
  const string& state_name2() const;
  const string& error() const;
  int64 ts_state_change() const;
  const FileSlaveOutput& output() const;
  const FileTranscodingStatus& transcoding_status() const;
  int64 ts_transcoding_status_change() const;

  void set_path(const string& path);
  void set_transcoding_status_file(const string& transcoding_status_file);
  void set_process_cmd(ProcessCmd process_cmd);
  void set_state(FState state);
  void set_error(const string error);
  void set_ts_state_change(int64 ts_state_change);
  void set_output(const FileSlaveOutput& output);
  void set_transcoding_status(const FileTranscodingStatus& transcoding_status);
  void set_ts_transcoding_status_change(int64 ts_transcoding_status_change);

  string ToString() const;

private:
  // file identifier
  const int64 id_;
  // the slave identifier, according to master. Closely related to file id.
  const string slave_id_;
  // the master who owns the file
  MasterWrapper* const master_;
  // the original url for the file
  // For multiple files this is a comma separated list of urls.
  const string remote_url_;
  // current complete path to file
  string path_;
  // path to the file containing the transcoding status
  string transcoding_status_file_;
  // what to do with the file (e.g. transcode, post-process, copy, .. )
  ProcessCmd process_cmd_;
  // processing params (e.g. encoding params)
  map<string, string> process_params_;
  // file state
  FState state_;
  // error description, corresponding to current file state
  string error_;
  // timestamp of the last state change [milliseconds since epoch]
  int64 ts_state_change_;
  // output [transcoded] files path
  FileSlaveOutput output_;
  // transcoding progress status
  FileTranscodingStatus transcoding_status_;
  // timestamp of the last transcoding_status change [milliseconds since epoch]
  int64 ts_transcoding_status_change_;
};

ostream& operator<<(ostream& os, const MediaFile& file);

}; }; // namespace slave // namespace manager

#endif /*MEDIA_FILE_H_*/
