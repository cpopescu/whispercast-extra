#ifndef __RPC_CMD_SERVICE_H__
#define __RPC_CMD_SERVICE_H__

#include <whisperlib/common/base/types.h>
#include <whisperlib/common/sync/process.h>
#include <whisperlib/net/base/selector.h>
#include "auto/server_invokers.h"

namespace cmd {

class RpcCmdService : public ServiceInvokerCmd {
private:
  struct CallProcess {
    rpc::CallContext<int32>* call_;
    process::Process* process_;
    CallProcess(rpc::CallContext<int32>* call,
                process::Process* process)
      : call_(call),
        process_(process) {
    }
    ~CallProcess() {
      delete process_;
      process_ = NULL;
    }
  };
public:
  // selector: selector
  RpcCmdService(net::Selector& selector, const string& script_dir);
  virtual ~RpcCmdService();

  // Kill all running processes.
  void Cleanup();

  // [in selector_ thread] Called with every terminated process.
  //  p: the CallProcess object containing the completed process
  //  exit_code: process exit code
  void ProcessCompleted(CallProcess* p, int exit_code);

  /**********************************************************************/
  /*                     ServiceInvokerCmd methods                      */
  /**********************************************************************/
  virtual void RunScript(rpc::CallContext<int32>* call,
                         const string& script,
                         const vector<string>& args);

private:
  net::Selector& selector_;

  // path to directory containing executable scripts
  const string script_dir_;

  // running processes
  typedef set<CallProcess*> CallProcessSet;
  CallProcessSet processes_;
};

} // namespace cmd

#endif /*__RPC_CMD_SERVICE_H__*/
