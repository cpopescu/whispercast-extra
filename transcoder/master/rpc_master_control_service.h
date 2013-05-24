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
  virtual void AddFile(rpc::CallContext<rpc::Void>* call,
                       const string& file_id,
                       const string& file_path,
                       const map<string, string>& process_params);
  virtual void DelFile(rpc::CallContext<rpc::Void>* call,
                       const string& file_id,
                       bool erase_original,
                       bool erase_output);
  virtual void GetFileState(rpc::CallContext<FileStateControl>* call,
                            const string& file_id);
  virtual void ListFiles(rpc::CallContext<vector<FileStateControl> >* call);

private:
  net::Selector & selector_;

  // master
  RPCMasterManagerService & master_;
};

}; }; // namespace master // namespace manager

#endif /*RPC_MASTER_CONTROL_SERVICE_H_*/
