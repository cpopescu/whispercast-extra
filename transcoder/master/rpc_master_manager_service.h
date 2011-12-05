#ifndef RPC_MASTER_MANAGER_SERVICE_H_
#define RPC_MASTER_MANAGER_SERVICE_H_

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>

#include "common/base/types.h"
#include "common/base/log.h"
#include "common/base/system.h"
#include "common/base/gflags.h"
#include "common/base/errno.h"
#include "common/base/strutil.h"
#include "common/sync/process.h"
#include "common/sync/mutex.h"
#include "common/io/checkpoint/state_keeper.h"

#include "net/base/selector.h"
#include "net/rpc/lib/client/rpc_client_connection_tcp.h"
#include "net/rpc/lib/client/rpc_service_wrapper.h"
#include "net/rpc/lib/rpc_util.h"

#include "../auto/types.h"
#include "../auto/server_invokers.h"
#include "../auto/client_wrappers.h"
#include "../directory_active_monitor.h"
#include "../constants.h"
#include "auto/types.h"

#include "media_file.h"
#include "slave_controller.h"

namespace manager { namespace master {

class SlaveController;

struct FullMediaFile {
  const MediaFile* main_;
  vector<const MediaFile*> duplicates_;
  FullMediaFile(const MediaFile* main)
    : main_(main), duplicates_() {}
};

class RPCMasterManagerService : public ServiceInvokerMasterManager
{
private:
  static const string kStateNextFileId;
  static const string kStatePendingFiles;
  static const string kProcessingExtension;
  static const string kDoneExtension;
  static const string kErrorExtension;

public:
  // selector: selector
  // rpc_host_port: master's RPC server host+port
  // rpc_http_path: master's RPC server http path (only for HTTP server)
  // rpc_http_auth_user: master's RPC server authentication username
  // rpc_http_auth_pswd: master's RPC server authentication password
  // rpc_connection_type: master's RPC server protocol
  // rpc_codec_id: master's RPC server codec
  // input_dir: directory path to listen for input files
  RPCMasterManagerService(net::Selector& selector,
                          net::NetFactory& net_factory,
                          const string& scp_username,
                          const net::IpAddress& external_ip,
                          int16 rpc_port,
                          const string& rpc_http_path,
                          const string& rpc_http_auth_user,
                          const string& rpc_http_auth_pswd,
                          rpc::CONNECTION_TYPE rpc_connection_type,
                          rpc::CODEC_ID rpc_codec_id,
                          int64 gen_file_id_start,
                          int64 gen_file_id_increment,
                          const string& input_dir,
                          const string& process_dir,
                          const string& output_dir,
                          const string& state_dir,
                          const string& state_name);
  virtual ~RPCMasterManagerService();

  // returns: master's local username, used for SCP transfers
  const string& scp_username() const;
  // returns: master's external IP, used for SCP transfers
  const net::IpAddress& external_ip() const;
  // returns: all-in-one master's RPC server address
  //          e.g. "tcp://10.16.200.2:7023?codec=json"
  const string& master_uri() const;

  // called by the main application to build up the slaves map
  void AddSlave(const net::HostPort& rpc_host_port,
                const string& rpc_http_path,
                const string& rpc_http_auth_user,
                const string& rpc_http_auth_pswd,
                rpc::CONNECTION_TYPE rpc_connection_type,
                rpc::CODEC_ID rpc_codec_id,
                uint32 max_paralel_processing_file_count);

  // Starts working: listening for input file, connecting to slaves.
  bool Start();
  void Stop();

  /////////////////////////////////////////////////////////////////////
  //                    Slave interface methods                      //
  /////////////////////////////////////////////////////////////////////
  // Called by SlaveController to get a new file.
  //input:
  // slave: the SlaveController calling this function.
  MediaFile * GrabMediaFile(SlaveController * slave);
  // Called by SlaveController when he completed a file.
  //input:
  // slave: the SlaveController calling this function.
  // file: the completed MediaFile. It's state should be READY.
  void CompleteMediaFile(SlaveController * slave, MediaFile* file);

  /////////////////////////////////////////////////////////////////////
  //                       Control methods                           //
  /////////////////////////////////////////////////////////////////////
  // Returns a unique file identifier.
  int64 GenerateFileId();

  //  Add a new media file. This is the entry point for new files in
  //  MasterManager. A file id is automatically generated.
  //  Returns file id, or INVALID_FILE_ID if "input_file_path" does not
  //  point a file.
  int64 AddMediaFile(const string& input_file_path, ProcessCmd process_cmd,
                     const map<string, string>& process_params);

  //  Get MediaFile based on file_id. Returns the main file with the given
  //  'file_id', appends to 'duplicates' (if not NULL) the duplicates.
  //  Returns NULL if file not found.
  //const MediaFile* GetMediaFile(int64 file_id,
  //    vector<const MediaFile*>* duplicates = NULL) const;

  //  Get MediaFile (main + duplicates) based on file_id.
  //  If file not found, returns a CompleteMediaFile containing main_ == NULL.
  FullMediaFile GetMediaFile(int64 file_id) const;

  //  Retrieve all files information. Append to "out".
  void GetMediaFiles(vector<FullMediaFile>& out) const;

  //  Retrieve all files.
  void DebugDumpFiles(vector<const MediaFile*>& out) const;

  //  Completely removes file& transcoding result for the given file_id.
  //  If both erase_original and erase_output are false, the Delete is
  //  identical to Complete.
  void Delete(int64 file_id, bool erase_master_original,
                             bool erase_slave_output);

  //  If the file is in error state, restart whole processing from IDLE.
  //  If the file is TRANSCODED, do nothing.
  //  Returns new file state.
  FState Retry(int64 file_id);

private:
  // save/load current configuration using the state_keeper_
  void Save();
  bool Load();

  // [NOT Thread Safe]
  //  The reverse of AddMediaFile.
  //  Erases the file from local structures (like files_ and slaves_by_file_id_).
  // erase_original: if true, it also erases original file on disk.
  //                 else, the original file remains on disk.
  void DeleteMediaFile(MediaFile * file, bool erase_original);

  // Used by the files_monitor no announce new files found.
  void FoundNewInputFile(const string& file_path);

  // inserts [file id -> slave] in slaves_by_file_id_
  void AddSlaveByFileId(int64 file_id, SlaveController* slave, bool is_main);

  /**********************************************************************/
  /*              RPCServiceInvokerSlaveManager methods                 */
  /**********************************************************************/
  virtual void NotifyStateChange(rpc::CallContext< rpc::Void > * call,
                                 const string& rpc_slave_id,
                                 const FileState& rpc_state);

private:
  net::Selector& selector_;
  net::NetFactory& net_factory_;
  // master's username for SCP transfers. Usually a local login username.
  const string scp_username_;
  // master's external IP address
  const net::IpAddress external_ip_;
  // master's rpc server
  const string master_uri_;

  // directory where we listen for input media files
  const string input_dir_;
  // directory where we keep media files while processing
  const string process_dir_;
  // directory where we move completed files
  const string output_dir_;

  // used to generate consecutive unique file id s
  int64 next_file_id_;
  int64 increment_file_id_;

  // file_id -> MediaFile*
  // We make this a map instead of queue, because we need to search these
  // files on GetMediaFile.
  typedef map<int64, MediaFile*> MediaFileMap;
  MediaFileMap pending_files_;

  // "host:port" -> SlaveController*
  typedef map<string, SlaveController*> SlaveControllerMap;
  SlaveControllerMap slaves_;

  // file_id -> SlaveController*
  //typedef map<int64, SlaveController*> SlaveControllerByMediaFileMap;
  //SlaveControllerByMediaFileMap slaves_by_file_id_;

  struct FileSlave {
    // this slave has the original file processed
    SlaveController* main_;
    // these slaves have result duplicates
    set<SlaveController*> duplicates_;
    FileSlave() : main_(NULL), duplicates_() {}
    bool Contains(const SlaveController* slave) const {
      return slave == main_ || duplicates_.find(
          const_cast<SlaveController*>(slave)) != duplicates_.end();
    }
  };

  // file_id -> FileSlave
  typedef map<int64, FileSlave> FileSlaveMap;
  FileSlaveMap file_slaves_;

  // file_id -> vector of SlaveController*
  //typedef set<SlaveController*> SlaveControllerSet;
  //typedef map< int64, SlaveControllerSet> SlaveControllerByFileIdMap;
  //SlaveControllerByFileIdMap slaves_by_file_id_;

  // permanent callback to FoundNewInputFile, used as a report handler by the files_monitor_
  DirectoryActiveMonitor::HandleFile * found_new_input_file_callback_;
  // a regular expression to match input files
  re::RE files_regexp_;
  // a tool to scan input directory for input files
  DirectoryActiveMonitor files_monitor_;

  io::StateKeeper state_keeper_;
};

}; }; // namespace master // namespace manager

#endif /*RPC_MASTER_MANAGER_SERVICE_H_*/
