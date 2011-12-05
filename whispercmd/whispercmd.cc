// Copyright 2009 WhisperSoft s.r.l.
// Author: Cosmin Tudorache

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <netdb.h> // for gethostbyname

#include <whisperlib/common/base/log.h>
#include <whisperlib/common/base/gflags.h>
#include <whisperlib/common/app/app.h>
#include <whisperlib/common/io/ioutil.h>
#include <whisperlib/net/base/selector.h>
#include <whisperlib/net/http/http_server_protocol.h>
#include <whisperlib/net/rpc/lib/server/rpc_http_server.h>
#include <whisperlib/net/rpc/lib/client/rpc_service_wrapper.h>

#include "rpc_cmd_service.h"

//////////////////////////////////////////////////////////////////////

DEFINE_int32(http_port,
             8080,
             "The port on which we accept RPC HTTP connections.");

DEFINE_string(script_dir,
              "",
              "Directory where the executable scripts are.");

DEFINE_string(http_auth_user,
              "",
              "Http authentication for accessing the RPC"
              " services.");
DEFINE_string(http_auth_pswd,
              "",
              "Http authentication for accessing the RPC"
              " services.");
DEFINE_string(http_auth_realm,
              "",
              "If you set a user/pswd, you also need this.");

//////////////////////////////////////////////////////////////////////

class WhisperCmdApp : public app::App {
 private:
  net::Selector* selector_;
  net::NetFactory* net_factory_;

  net::SimpleUserAuthenticator* rpc_authenticator_;

  //rpc::ServicesManager* rpc_manager_;
  //rpc::ExecutionPool* rpc_executor_;
  rpc::HttpServer* rpc_processor_;

  cmd::RpcCmdService* cmd_service_;

  http::Server* http_server_;

 public:
  WhisperCmdApp(int argc, char **&argv) :
    app::App(argc, argv),
    selector_(NULL),
    net_factory_(NULL),
    rpc_authenticator_(NULL),
    rpc_processor_(NULL),
    cmd_service_(NULL),
    http_server_(NULL) {
  }
  virtual ~WhisperCmdApp() {
    CHECK_NULL(selector_);
    CHECK_NULL(net_factory_);
    CHECK_NULL(rpc_processor_);
    CHECK_NULL(cmd_service_);
    CHECK_NULL(http_server_);
  }

 protected:
  void RPCServerOpenCompleted(bool success) {
    if (!success) {
      LOG_ERROR << "Terminating application on RPCServerOpenCompleted fail.";
      stop_event_.Signal();
    }
  }
  int Initialize() {
    common::Init(argc_, argv_);

    if ( !io::IsDir(FLAGS_script_dir) ) {
      LOG_ERROR << "Invalid script_dir: [" << FLAGS_script_dir << "]";
      return 1;
    }

    //////////////////////////////////////////////////////////////////////
    //
    // Create the selector
    //
    selector_ = new net::Selector();

    //////////////////////////////////////////////////////////////////////
    //
    // Create the net factory
    //
    net_factory_ = new net::NetFactory(selector_);

    //////////////////////////////////////////////////////////////////////
    //
    // Create authenticator
    //
    if ( !FLAGS_http_auth_realm.empty() ) {
      rpc_authenticator_ = new net::SimpleUserAuthenticator(
          FLAGS_http_auth_realm);
      rpc_authenticator_->set_user_password(FLAGS_http_auth_user,
                                            FLAGS_http_auth_pswd);
    }

    //////////////////////////////////////////////////////////////////////
    //
    // Create the HTTP server
    //
    http::ServerParams http_server_params;
    http_server_ = new http::Server(
        "Whisper Commander Server 0.1",
        selector_,
        *net_factory_,
        http_server_params);

    //////////////////////////////////////////////////////////////////////
    //
    // Create the RPC layer
    //
    cmd_service_ = new cmd::RpcCmdService(*selector_, FLAGS_script_dir);
    rpc_processor_ = new rpc::HttpServer(selector_,
                                         http_server_,
                                         rpc_authenticator_,
                                         "/rpc_cmd",
                                         true,
                                         true, 10, "");
    CHECK(rpc_processor_->RegisterService(cmd_service_));

    //////////////////////////////////////////////////////////////////////
    //
    // Start the HTTP server
    //
    http_server_->AddAcceptor(net::PROTOCOL_TCP,
                              net::HostPort(0, FLAGS_http_port));
    selector_->RunInSelectLoop(NewCallback(
                                   http_server_,
                                   &http::Server::StartServing));

    return 0;
  }

  void Run() {
    selector_->Loop();
  }
  void Stop() {
    if ( !selector_->IsInSelectThread() ) {
      // !!! NOT IN SELECTOR THREAD !!!
      selector_->RunInSelectLoop(NewCallback(this, &WhisperCmdApp::Stop));
      return;
    }
    CHECK(selector_->IsInSelectThread());
    /////////////////////////////////////////////////////////////////////
    // Cleanup

    /////////////////////////////////////////////////////////////////////
    // Stop selector
    selector_->RunInSelectLoop(
        NewCallback(selector_, &net::Selector::MakeLoopExit));
  }

  void Shutdown() {
    //////////////////////////////////////////////////////////////////////
    //
    // Delete RPC layer
    //
    delete rpc_processor_;
    rpc_processor_ = NULL;

    delete cmd_service_;
    cmd_service_ = NULL;

    //////////////////////////////////////////////////////////////////////
    //
    // Delete the authenticator
    //
    delete rpc_authenticator_;
    rpc_authenticator_ = NULL;

    //////////////////////////////////////////////////////////////////////
    //
    // Delete the HTTP server
    //
    delete http_server_;
    http_server_ = NULL;

    //////////////////////////////////////////////////////////////////////
    //
    // Delete net factory
    //
    delete net_factory_;
    net_factory_ = NULL;

    //////////////////////////////////////////////////////////////////////
    //
    // Delete selector
    //
    delete selector_;
    selector_ = NULL;
  }
};

int main(int argc, char *argv[]) {
  return common::Exit(WhisperCmdApp(argc, argv).Main());
}
