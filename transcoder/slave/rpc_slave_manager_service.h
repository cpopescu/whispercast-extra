#ifndef RPC_SLAVE_MANAGER_SERVICE_H_
#define RPC_SLAVE_MANAGER_SERVICE_H_

#include <map>
#include <set>

#include <whisperlib/common/sync/mutex.h>
#include <whisperlib/common/sync/process.h>
#include <whisperlib/common/base/alarm.h>
#include <whisperlib/common/io/checkpoint/state_keeper.h>
#include <whisperlib/net/base/selector.h>

#include "../auto/types.h"
#include "../auto/server_invokers.h"
#include "auto/types.h"
#include "../directory_active_monitor.h"
#include "../constants.h"
#include "media_file.h"
#include "master_wrapper.h"

namespace manager { namespace slave {

class RPCSlaveManagerService : public ServiceInvokerSlaveManager
{
protected:
  // the state keeper uses this base filename
  static const string kStateName;
  // state key name. We save/load under this key name.
  static const string kStateKeyName;

  string FileDownloadPath(const MediaFile& file) const;
  string FileOutputDir(const MediaFile& file) const;

public:
  RPCSlaveManagerService(net::Selector& selector,
                         net::NetFactory& net_factory,
                         const std::string& scp_username,
                         const net::IpAddress& external_ip,
                         const string& output_dir,
                         const string& state_dir);
  virtual ~RPCSlaveManagerService();

  bool Start();
  void Stop();

private:
  void ChangeFileState(MediaFile* file, FState state, const string& error);

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
  // [IN] state: file state
  // [IN] error: a description for the current file state (or "")
  // [IN] ts_state_change: timestamp of the last state change (useful on Load())
  // [OUT] out_err: filled with an error description in case of failure
  // returns:
  //   Success status. On failure 'out_err' is filled.
  bool AddFile(const string& file_id,
               const string& slave_id,
               const string& master_uri,
               const net::HostPort& master_alternate_ip,
               const string& remote_url,
               const map<string, string>& process_params,
               FState state,
               const string& error,
               string* out_err);

  void DelFile(const string& file_id);

  // checks how many files are being process, and maybe starts new processes
  void StartProcessingMoreFiles();

  // starts the external transcoding process
  void StartProcessing(MediaFile* file);
  // The transcoding process sends back status messages with this callback
  void HandleProcessMessage(string file_id, const string& msg);
  // Completion callback for transcoding process
  void ProcessingCompleted(string file_id, process::Process* process,
                           int transcode_exit_code);

  void HandleNotifyStateChangeResult(
      MasterWrapper* master, const rpc::CallResult<rpc::Void>& result);
  void TryEraseMaster(MasterWrapper* master);

  // Persistent state save/load.
  void Save();
  bool Load();

  string ToString(const char* sep = ", ") const;

  /**********************************************************************/
  /*              ServiceInvokerSlaveManager methods                    */
  /**********************************************************************/
  virtual void AddFile(rpc::CallContext<string>* call,
                       const string& file_id,
                       const string& file_path,
                       const map<string, string>& process_params,
                       const string& master_uri,
                       const string& slave_id);
  virtual void DelFile(rpc::CallContext<rpc::Void>* call,
                       const string& file_id);
  virtual void GetFileState(rpc::CallContext<FileState>* call,
                            const string& file_id);
  virtual void ListFiles(rpc::CallContext< vector<FileState> >* call);
  virtual void DebugToString(rpc::CallContext<string>* call);

protected:
  net::Selector& selector_;
  net::NetFactory& net_factory_;

  // slave username. The master uses this to copy remote files.
  const string scp_username_;
  // slave external IP address
  const net::IpAddress external_ip_;

  // directory where to output processed files
  const string output_dir_;

  // file_id -> file struct
  typedef std::map<string, MediaFile*> MediaFileMap;
  MediaFileMap files_;

  // master uri -> master
  typedef std::map<string, MasterWrapper*> MasterMap;
  MasterMap masters_;

  // the processes we're running on any given time.
  // When we start the process, we push it here.
  // When the process finishes execution, we pop it from here.
  typedef std::set<process::Process*> ProcessSet;
  ProcessSet active_processes_;

  // persistent state
  io::StateKeeper state_keeper_;
  io::StateKeepUser state_keep_user_;
};

}; // namespace slave
}; // namespace manager


#endif /*RPC_SLAVE_MANAGER_SERVICE_H_*/
