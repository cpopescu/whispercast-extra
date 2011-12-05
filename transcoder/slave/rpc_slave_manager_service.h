#ifndef RPC_SLAVE_MANAGER_SERVICE_H_
#define RPC_SLAVE_MANAGER_SERVICE_H_

#include "common/sync/mutex.h"
#include "common/sync/process.h"
#include "common/io/checkpoint/state_keeper.h"
#include "net/base/selector.h"

#include "../auto/types.h"
#include "../auto/server_invokers.h"
#include "auto/types.h"
#include "../directory_active_monitor.h"
#include "../constants.h"
#include "common/base/alarm.h"
#include "media_file.h"
#include "master_wrapper.h"

namespace manager { namespace slave {

class RPCSlaveManagerService : public ServiceInvokerSlaveManager
{
protected:
  // state key name. We save/load under this key name.
  static const string kStateKeyName;
  // milliseconds between consecutive scans of transcoder's output directory
  static const uint32 kTranscoderOutputMonitorInterval;
  // milliseconds between consecutive reads of transcoder's status file
  static const uint32 kTranscoderStatusMonitorInterval;
  // milliseconds between consecutive scans of postprocessor's output directory
  static const uint32 kPostprocessorOutputMonitorInterval;

public:
  // state_name: file name inside the 'state_dir'
  RPCSlaveManagerService(net::Selector& selector,
                         net::NetFactory& net_factory,
                         const std::string& scp_username,
                         const net::IpAddress& external_ip,
                         const string& download_dir,
                         const string& transcoder_input_dir,
                         const string& transcoder_output_dir,
                         const string& postprocessor_input_dir,
                         const string& postprocessor_output_dir,
                         const vector<string>& output_dirs,
                         const string& state_dir,
                         const string& state_name);
  virtual ~RPCSlaveManagerService();

  bool Start();
  void Stop();

private:
  // Directory and path where we download the original file.
  // e.g. for XxxDir:  "/tmp/download/5764"
  //      for XxxPath: "/tmp/download/5764/abc.avi"
  string FileDownloadDir(int64 file_id);
  string FileDownloadDir(const MediaFile* file);
  // We can download multiple files, this is why there is no FileDownloadPath

  // Directory and path where we should move the original file for transcoding.
  // e.g. for XxxDir:  "/tmp/transcoder/input/5764"
  // e.g. for XxxPath: "/tmp/transcoder/input/5764/abc.avi"
  string FileTranscodeInputDir(const MediaFile* file);
  string FileTranscodeInputPath(const MediaFile* file);
  // Directory where the transcoder puts result files.
  string FileTranscodeOutputDir(const MediaFile* file);

  // Directory and path where we should move the original file for
  // postprocessing.
  // e.g. for XxxDir:  "/tmp/postprocessor/input/5764"
  // e.g. for XxxPath: "/tmp/postprocessor/input/5764/abc.avi"
  string FilePostProcessInputDir(const MediaFile* file);
  string FilePostProcessInputPath(const MediaFile* file);
  // Directory where the postprocessor puts result files.
  string FilePostProcessOutputDir(const MediaFile* file);

  // returns either:: /tmp/transcoder/input/5764/abc.avi.status
  //             or : /tmp/postprocessor/input/5764/abc.avi.status
  string FileProcessingStatusPath(const MediaFile* file);

  // Directory where the result files should end.
  //string FileOutputDir(const MediaFile* file, uint32 output_dir_index);

  // Returns the file_id from a processed file path.
  // e.g. if file_path = "/tmp/transcoder/5764/abc.avi.done"
  //      or file_path = "/tmp/transcoder/5764/123.avi.done"
  //      returns 5764
  // On error (path not inside processor_dir, or subdir not a number) returns -1
  int64 ReadIdFromProcessedFile(const string& file_path);

  // Create a basename directory to group files.
  // e.g. "20091030-212311-000-00000000000000000123"
  //      Where the first 17 digits represent the current datetime,
  //      and the last 20 digits represent the file_id.
  //string MakeRelativeUniqueDir(int64 file_id);

  // Physically erase file on disk. Clears all file garbage (transcoding,
  // postprocessing, download,.. and optionally original & output).
  void CleanDisk(MediaFile& file, bool erase_output);
  void CleanDisk(const StateMediaFile& rpc_file, bool erase_output);

  // !!! [NOT Thread Safe] !!!
  // Adds a new file and the corresponding master.
  //input:

  // [IN] id: file id
  // [IN] slave_id: a value to be sent back to master with every
  //                NotifyStateChange
  // [IN] master_uri: all-in-one rpc address of the file master
  // [IN] master_alternate_ip: if master_uri contains IP "0.0.0.0", then use
  //              this ip (format b1.b2.b3.b4 <==> 0xb1b2b3b4)
  // [IN] remote_url: file source, where to download from
  // [IN] path: local path to store the file
  // [IN] process_cmd: what to do with the file (e.g. transcode,
  //                   post-process, copy .. )
  // [IN] state: file state
  // [IN] error: a description for the current file state (or "")
  // [IN] ts_state_change: timestamp of the last state change (useful on Load())
  // [OUT] out_error_state: filled with a state code in case of failure
  // [OUT] out_error_description: filled with an error description in case
  //                 of failure
  // returns:
  //   The newly created file on success.
  //   NULL on failure: duplicate id, invalid master_uri.
  MediaFile* AddFile(int64 id,
                      const string& slave_id,
                      const string& master_uri,
                      const net::HostPort& master_alternate_ip,
                      const string& remote_url,
                      const string& path,
                      ProcessCmd process_cmd,
                      const map<string, string>& process_params,
                      FState state,
                      const string& error,
                      int64 ts_state_change,
                      const FileSlaveOutput& output,
                      const FileTranscodingStatus& transcoding_status,
                      FState& out_error_state,
                      string& out_error_description);

  // Removes "file" from files_ and deletes it.
  // It may also remove the corresponding master wrapper.
  // Physically (on disk) deletes:
  //   - the file from download_dir_
  //   - the symlink in transcode_dir
  //   - the processing_dir.
  // Optionally deletes:
  //   - IF erase_output is true => deletes the file from output_dir_ .
  void EraseFile(MediaFile* file, bool erase_output);

  // If the given master has no more files referenced
  // and no RPCs active, then it is removed from masters_ and deleted;
  // Else nothing happens.
  void TryEraseMaster(MasterWrapper* master);

  // called from various threads (thread pool& selector)
  // If out_state != NULL, output new state in 'out_state';
  // else, send notification to master containing the new state.
  void ChangeFileState(MediaFile* file, FState state,
                       const string& error, FileState* out_state);

  // Reads transcoding status of the given file from disk,
  // and sets file.transcoding_status_ .
  void ReadTranscoderStatus(MediaFile& file);

  //////////////////////////////////////////////////////////////////////////
  // For all StartXxxx functions
  //  [IN] file: the file to transfer
  //  [OUT] state: if NULL => an automatic notification is sent to master
  //                          containing new FileState.
  //               if not NULL => filled with new file state, no automatic
  //                              notification is send to master.
  //  returns: success. If this function fails, then there was an error and
  //           the 'file' has been deleted.
  //

  // Starts transferring the given file.
  void StartTransfer(MediaFile* file, FileState* out_state);
  // Completion callback of the transfer process.
  void TransferCompleted(int64 file_id, process::Process* p,
                         int scp_exit_status);

  // Starts transcoding the given file.
  void StartTranscode(MediaFile* file, FileState* out_state);
  // Called when the transcoder finished processing some file.
  // On success: error == "".
  // On failure: error indicates an error description.
  void TranscodeCompleted(int64 file_id, const string& error);

  // Starts post processing the given file.
  void StartPostProcess(MediaFile* file, FileState* out_state);
  // Called when the transcoder finished processing some file.
  // On success: error == "".
  // On failure: error indicates an error description.
  void PostProcessCompleted(int64 file_id, const string& error);

  // Start copying the result files into multiple output dirs.
  void StartOutput(MediaFile* file, FileState* out_state);
  // Completion callback for the distribution process.
  void OutputCompleted(int64 file_id, process::Process* p, int cp_exit_status);

  //////////////////////////////////////////////////////////////////////////

  // Used as a callback by transcoder_monitor_ to announce new files.
  void NotifyNewTranscodedFile(const string& file_path);

  // Used as a callback by post_processor_monitor_ to announce new files.
  void NotifyNewPostProcessedFile(const string& file_path);

  // Used as a callback by transcoder_status_monitor_.
  // Reads transcoder status(progress).
  void NotifyUpdateTranscoderStatus();

  // Used as a completion callback by master::NotifyStateChange
  void HandleNotifyStateChangeResult(MasterWrapper* master,
                                     const rpc::CallResult<rpc::Void>& result);

  // Persistent state save/load.
  void Save();
  bool Load();

  string ToString(const string& sep = ", ") const;

  /**********************************************************************/
  /*              ServiceInvokerSlaveManager methods                    */
  /**********************************************************************/
  virtual void Copy(rpc::CallContext<FileState>* call,
                    const string& rpc_file_url,
                    int64 rpc_file_id,
                    const string& rpc_file_process_cmd,
                    const map<string, string>& rpc_file_pparams,
                    const string& rpc_master_uri,
                    const string& rpc_slave_id);
  virtual void Process(rpc::CallContext<FileState>* call,
                       int64 rpc_file_id);
  virtual void GetFileState(rpc::CallContext<FileState>* call,
                            int64 rpc_file_id);
  virtual void Delete(rpc::CallContext<rpc::Void>* call,
                      int64 rpc_file_id, bool rpc_erase_output);
  virtual void DebugToString(rpc::CallContext<string>* call);

protected:
  net::Selector& selector_;
  net::NetFactory& net_factory_;

  // slave username. The master uses this to copy remote files.
  const string scp_username_;
  // slave external IP address
  const net::IpAddress external_ip_;

  // directory where to download files
  const string download_dir_;
  // directory where transcoder is listening for input files
  const string transcoder_input_dir_;
  // directory where transcoder puts result files
  const string transcoder_output_dir_;
  // directory where postprocessor is listening for input files
  // These are already processed files.
  const string postprocessor_input_dir_;
  // directory where postprocessor puts result files
  const string postprocessor_output_dir_;
  // directories where to move transcoded files
  const vector<string> output_dirs_;

  typedef map<int64, MediaFile*> MediaFileMap;
  MediaFileMap files_;

  // master uri -> master
  typedef map<string, MasterWrapper*> MasterMap;
  MasterMap masters_;

  // the processes we're running on any given time.
  // When we start the process, we push it here.
  // When the process finishes execution, we pop it from here.
  // On destructor we have to cleanup all these.
  typedef set<process::Process*> ProcessSet;
  ProcessSet active_processes_;

  // synchronize everything
  synch::Mutex sync_;

  // matches "*.done" files for both transcoder and postprocessor
  re::RE processor_regexp_;

  // permanent callback to NotifyNewTranscodedFile
  Callback1<const string &>* notify_new_transcoded_file_callback_;
  // looks for transcoder's completion files
  DirectoryActiveMonitor transcoder_input_monitor_;

  // permanent callback to NotifyNewPostProcessedFile
  Callback1<const string &>* notify_new_post_processed_file_callback_;
  // looks for postprocessor's completion files
  DirectoryActiveMonitor postprocessor_input_monitor_;

  // calls from time to time NotifyUpdateTranscoderStatus
  util::Alarm transcoder_status_monitor_;

  io::StateKeeper state_keeper_;
  io::StateKeepUser state_keep_user_;
};

}; // namespace slave
}; // namespace manager


#endif /*RPC_SLAVE_MANAGER_SERVICE_H_*/
