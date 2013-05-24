#ifndef RPC_MASTER_MANAGER_SERVICE_H_
#define RPC_MASTER_MANAGER_SERVICE_H_

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>

#include <whisperlib/common/base/types.h>
#include <whisperlib/common/base/log.h>
#include <whisperlib/common/io/checkpoint/state_keeper.h>
#include <whisperlib/net/base/selector.h>
#include <whisperlib/net/rpc/lib/rpc_util.h>

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

class RPCMasterManagerService : public ServiceInvokerMasterManager {
private:
  static const string kStateName;
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
  RPCMasterManagerService(net::Selector& selector,
                          net::NetFactory& net_factory,
                          const string& scp_username,
                          const net::IpAddress& external_ip,
                          int16 rpc_port,
                          const string& rpc_http_path,
                          const string& rpc_http_auth_user,
                          const string& rpc_http_auth_pswd,
                          rpc::CONNECTION_TYPE rpc_connection_type,
                          rpc::CodecId rpc_codec_id,
                          const vector<string>& output_dirs,
                          const string& state_dir);
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
                rpc::CodecId rpc_codec_id,
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
  MediaFile * GrabMediaFile(SlaveController* slave);
  // Called by SlaveController when he completed a file.
  //input:
  // slave: the SlaveController calling this function.
  // file: the completed MediaFile. It's state should be FSTATE_DONE.
  void CompleteMediaFile(SlaveController* slave, MediaFile* file);

  /////////////////////////////////////////////////////////////////////
  //                       Control methods                           //
  /////////////////////////////////////////////////////////////////////
  //  Add a new media file. This is the entry point for new files in
  //  MasterManager.
  void AddMediaFile(const string& file_id, const string& file_path,
                    const map<string, string>& process_params);

  //  Get MediaFile (main + duplicates) based on file_id.
  //  If file not found, returns a CompleteMediaFile containing main_ == NULL.
  const MediaFile* GetMediaFile(const string& file_id) const;

  //  Retrieve all files. Append to "out".
  void GetMediaFiles(vector<const MediaFile*>* out) const;

  //  Completely removes file& transcoding result for the given file_id.
  //  If both erase_original and erase_output are false, the Delete is
  //  identical to Complete.
  void Delete(const string& file_id,
              bool erase_master_original,
              bool erase_slave_output);

private:
  // save/load current configuration using the state_keeper_
  void Save();
  bool Load();

  // [NOT Thread Safe]
  //  The reverse of AddMediaFile.
  //  Erases the file from local structures.
  // erase_original: if true, it also erases original file on disk.
  //                 else, the original file remains on disk.
  void DeleteMediaFile(MediaFile* file, bool erase_original);

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

  // directories where we copy completed files
  const vector<string> output_dirs_;

  // file_id -> MediaFile*
  // We make this a map instead of queue, because we need to search these
  // files on GetMediaFile.
  typedef map<string, MediaFile*> MediaFileMap;
  MediaFileMap pending_files_;

  // "host:port" -> SlaveController*
  typedef map<string, SlaveController*> SlaveControllerMap;
  SlaveControllerMap slaves_;

  io::StateKeeper state_keeper_;
};

}; }; // namespace master // namespace manager

#endif /*RPC_MASTER_MANAGER_SERVICE_H_*/
