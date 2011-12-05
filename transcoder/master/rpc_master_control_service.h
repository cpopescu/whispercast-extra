#ifndef RPC_MASTER_CONTROL_SERVICE_H_
#define RPC_MASTER_CONTROL_SERVICE_H_

#include "common/base/types.h"
#include "common/sync/mutex.h"

#include "net/base/selector.h"

#include "../auto/types.h"
#include "../constants.h"
#include "auto/types.h"

#include "rpc_master_manager_service.h"

namespace manager { namespace master {

class RPCMasterControlService : public ServiceInvokerMasterControl
{
public:
  // selector: selector
  // manager: the master manager
  RPCMasterControlService(net::Selector& selector,
                          RPCMasterManagerService& master);
  virtual ~RPCMasterControlService();

  /**********************************************************************/
  /*              ServiceInvokerSlaveManager methods                 */
  /**********************************************************************/
  virtual void AddFile(rpc::CallContext<int64>* call,
                       const string& rpc_file_path,
                       const string& process_cmd,
                       const map<string, string>& process_params);
  virtual void GetFileState(rpc::CallContext<FileStateControl>* call,
                            int64 file_id);
  virtual void ListFiles(rpc::CallContext<vector<FileStateControl> >* call);
  virtual void DebugDumpFiles(rpc::CallContext<vector<FileStateControl> >* call);
  virtual void Complete(rpc::CallContext<rpc::Void>* call, int64 file_id);
  virtual void Delete(rpc::CallContext<rpc::Void>* call,
                      int64 file_id,
                      bool erase_master_original, bool erase_slave_output);
  virtual void Retry(rpc::CallContext<string>* call, int64 file_id);

private:
  net::Selector & selector_;

  // master
  RPCMasterManagerService & master_;
};

}; }; // namespace master // namespace manager

#endif /*RPC_MASTER_CONTROL_SERVICE_H_*/
