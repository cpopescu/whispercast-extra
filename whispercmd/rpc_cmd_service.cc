
#include <whisperlib/common/base/log.h>
#include <whisperlib/net/rpc/lib/rpc_util.h>
#include "rpc_cmd_service.h"

namespace cmd {

RpcCmdService::RpcCmdService(net::Selector& selector,
                             const string& script_dir)
  : ServiceInvokerCmd(ServiceInvokerCmd::ServiceClassName()),
    selector_(selector),
    script_dir_(script_dir),
    processes_() {
}
RpcCmdService::~RpcCmdService() {
  Cleanup();
  CHECK(processes_.empty());
}

void RpcCmdService::Cleanup() {
  for ( CallProcessSet::iterator it = processes_.begin();
        it != processes_.end(); ++it ) {
    CallProcess* p = *it;
    p->call_->Complete(-1);
    delete p;
  }
  processes_.clear();
}

void RpcCmdService::ProcessCompleted(CallProcess* p, int exit_code) {
  CHECK(selector_.IsInSelectThread());

  LOG_INFO << "Script: " << p->process_->path() << " terminated"
              ", exit_code: " << exit_code;

  // complete rpc call
  p->call_->Complete(exit_code);

  // delete CallProcess
  int p_erase_count = processes_.erase(p);
  CHECK_EQ(p_erase_count, 1);
  delete p;
}

void RpcCmdService::RunScript(rpc::CallContext<int32> * call,
                              const string& rpc_script,
                              const vector<string>& args) {
  const string script = strutil::JoinPaths(script_dir_, rpc_script);

  LOG_INFO << "Starting script: " << script;

  process::Process* process = new process::Process(script, args);
  CallProcess* call_process = new CallProcess(call, process);
  processes_.insert(call_process);

  if ( !process->Start(NULL, false, NULL, false,
      NewCallback(this, &RpcCmdService::ProcessCompleted, call_process),
      &selector_) ) {
    processes_.erase(call_process);
    delete call_process;
    LOG_ERROR << "Failed to start process, exe: [" << script << "]"
                 ", error: " << GetLastSystemErrorDescription();
    call->Complete(-1);
    return;
  }

  // the "script" process is running now. When the execution terminates,
  // the ProcessCompleted is called indicating the exit code.
  // Also ProcessCompleted will complete the rpc "call".
}

} // namespace cmd
