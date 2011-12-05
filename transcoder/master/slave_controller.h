#ifndef SLAVE_CONTROLLER_H_
#define SLAVE_CONTROLLER_H_

#include "common/sync/mutex.h"
#include "common/io/checkpoint/state_keeper.h"
#include "net/rpc/lib/client/rpc_client_connection_tcp.h"
#include "net/rpc/lib/client/rpc_client_connection_http.h"
#include "net/rpc/lib/rpc_util.h"
#include "../constants.h"
#include "common/base/alarm.h"
#include "media_file.h"
#include "rpc_master_manager_service.h"

namespace manager { namespace master {

class RPCMasterManagerService;

class SlaveController
{
private:
  static const string kStateFiles;

  struct CMediaFile {
    MediaFile * file_;
    util::Alarm alarm_;
    bool complete_reported_;
    CMediaFile(net::Selector & selector, MediaFile * file)
      : file_(file), alarm_(selector), complete_reported_(false) {}
  };

public:
  SlaveController(net::Selector & selector,
                  net::NetFactory & net_factory,
                  RPCMasterManagerService & master,
                  const net::HostPort & host_port,
                  const string & rpc_http_path,
                  const string& rpc_http_auth_user,
                  const string& rpc_http_auth_pswd,
                  rpc::CONNECTION_TYPE rpc_connection_type,
                  rpc::CODEC_ID rpc_codec_id,
                  uint32 max_paralel_processing_file_count,
                  io::StateKeeper & state_keeper);
  virtual ~SlaveController();

  const string& name() const;
  const net::HostPort& host_port() const;

  bool Start();
  void Stop();

  // Returns the media file identified by "file_id",
  // or NULL if file not found.
  const MediaFile* GetMediaFile(int64 file_id) const;
  // Appends all files_ to "out";
  void GetMediaFiles(vector<const MediaFile*> & out) const;

  // Add the given file to pending_files_ queue.
  // At some point we'll process this file, but not now.
  void QueueMediaFile(MediaFile* file);

  // Called by RPC result handlers to change the file state accordingly.
  // Called by RPCMasterManagerService (when a slave notifies us) to change the file state accordingly.
  void NotifyMediaFileStateChange(const FileState & file_state);
  void NotifyMediaFileError(int64 file_id, const string & error_description);
  void NotifyMediaFileError(int64 file_id, FState error_state, const string & error_description);

  //  Called by RPCMasterManagerService.
  //  Erases the identified MediaFile from local structures.
  //  Triggers an RPC Delete on slave machine.
  // Returns the MediaFile, or NULL if no such file id.
  MediaFile * DeleteMediaFile(int64 file_id,
                              bool erase_slave_output);

  // Called by RPCMasterManagerService.
  // Switch file state from any error state to IDLE.
  // If file is in TRANSCODED state, does nothing.
  // Returns new file state.
  FState RetryMediaFile(int64 file_id);

private:
  static int64 DelayProcessFile(FState state);

  // Add file to files_ .
  // Returns the CMediaFile containing the given file;
  // If duplicate ID exists, returns NULL and the existing file is not changed.
  CMediaFile* AddFile(MediaFile * file);
  void RemFile(CMediaFile * file);

  void ChangeFileState(CMediaFile & file_alarm, FState state, const string & error);
  void ScheduleProcessFile(CMediaFile & file_alarm);

  // used as alarm for every processing media file
  void ProcessFile(int64 file_id);

  // used as alarm to check if we can start processing a new media file
  void MaybeGrabMediaFile();

  ///////////////////////////////////////////////////////////////////////////
  // RPC Asynchronous Response Handlers.
  // Thread Safe - all handlers.
  void HandleCopyResult(int64 file_id, const rpc::CallResult<FileState> & result);
  void HandleProcessResult(int64 file_id, const rpc::CallResult<FileState> & result);
  void HandleGetFileStateResult(int64 file_id, const rpc::CallResult<FileState> & result);
  void HandleDeleteResult(int64 file_id, const rpc::CallResult<rpc::Void> & result);

public:
  // Save/Load persistent state.
  void Save();
  bool Load();

  string ToString() const;

private:
  net::Selector & selector_;
  net::NetFactory & net_factory_;
  RPCMasterManagerService & master_;

  // slave controller name. This is the string of "host:port".
  const string name_;

  // these are files we're not working on yet.
  // (Basically these are distribution files.)
  // When there's room for processing another file, we check&pop this first.
  typedef map<int64, MediaFile*> PendingFileMap;
  PendingFileMap pending_files_;

  // all files on this slave. Some are processing, some are processed.
  // Mapped by file_id.
  typedef map<int64, CMediaFile*> CMediaFileMap;
  CMediaFileMap files_;

  // how many files can we start processing in paralel on the slave
  const uint32 max_paralel_processing_file_count_;

  // slave host port
  const net::HostPort host_port_;

  // rpc stuff
  const string rpc_http_path_;
  const string rpc_http_auth_user_;
  const string rpc_http_auth_pswd_;
  const rpc::CONNECTION_TYPE rpc_connection_type_;
  const rpc::CODEC_ID rpc_codec_id_;
  rpc::IClientConnection * rpc_connection_;
  ServiceWrapperSlaveManager * rpc_client_wrapper_;

  // permanent callback to MaybeGrabMediaFile
  util::Alarm maybe_grab_media_file_alarm_;

  io::StateKeepUser state_keep_user_;
};

ostream& operator<<(ostream& os, const SlaveController& slave);

}; };

#endif /*SLAVE_CONTROLLER_H_*/
