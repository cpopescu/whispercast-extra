// Copyright 2009 WhisperSoft s.r.l.
// Author: Cosmin Tudorache

#include <whisperlib/common/app/app.h>

#include <whisperlib/common/base/types.h>
#include <whisperlib/common/base/log.h>
#include <whisperlib/common/base/system.h>
#include <whisperlib/common/base/gflags.h>
#include <whisperlib/common/base/errno.h>
#include <whisperlib/common/base/strutil.h>
#include <whisperlib/common/sync/process.h>
#include <whisperlib/common/sync/mutex.h>
#include <whisperlib/common/io/ioutil.h>

#include <whisperlib/net/base/selector.h>
#include <whisperlib/net/http/http_server_protocol.h>
#include <whisperlib/net/rpc/lib/server/rpc_http_server.h>

#include "../constants.h"
#include "../common.h"
#include "rpc_slave_manager_service.h"

//////////////////////////////////////////////////////////////////////

DEFINE_int32(http_port,
             8080,
             "The port on which we accept HTTP connections.");

//////////////////////////////////////////////////////////////////////

DEFINE_string(media_output_dir,
              "",
              "Directory where to put result files.");
DEFINE_string(scp_username,
              "",
              "The username for scp transfers.");
DEFINE_string(external_ip,
              "",
              "The IP for remote transfers.");

DEFINE_string(state_dir,
              "",
              "Directory where to save state.");

DEFINE_string(http_auth_user,
              "",
              "Http authentication for accessing SlaveManager.");
DEFINE_string(http_auth_pswd,
              "",
              "Http authentication for accessing SlaveManager.");
DEFINE_string(http_auth_realm,
              "",
              "If you set a user/pswd, you also need this.");

DEFINE_string(processor,
              "",
              "The processor executable."
              " It will be run: processor <input-file> <output-dir>");

DEFINE_int32(parallel_processing,
             1,
             "The number of media files that can be processed in parallel.");

//////////////////////////////////////////////////////////////////////

class SlaveManagerApp : public app::App {
private:
  net::Selector* selector_;
  net::NetFactory* net_factory_;

  rpc::HttpServer* rpc_http_processor_;
  net::SimpleUserAuthenticator* rpc_authenticator_;

  manager::slave::RPCSlaveManagerService* slave_manager_;

  http::Server* http_server_;

  net::HostPort master_server_;

public:
  SlaveManagerApp(int &argc, char **&argv) :
    app::App(argc, argv),
    selector_(NULL),
    net_factory_(NULL),
    rpc_http_processor_(NULL),
    rpc_authenticator_(NULL),
    slave_manager_(NULL),
    http_server_(NULL) {
  }
  virtual ~SlaveManagerApp() {
    CHECK_NULL(selector_);
    CHECK_NULL(net_factory_);
    CHECK_NULL(rpc_http_processor_);
    CHECK_NULL(rpc_authenticator_);
    CHECK_NULL(slave_manager_);
    CHECK_NULL(http_server_);
  }

protected:
  void RPCServerOpenCompleted(bool success) {
    if(!success) {
      LOG_ERROR << "Terminating application on RPCServerOpenCompleted fail.";
      stop_event_.Signal();
    }
  }
  int Initialize() {
    common::Init(argc_, argv_);
    if ( !io::IsDir(FLAGS_media_output_dir) ) {
      LOG_ERROR << "Output directory not found: [" << FLAGS_media_output_dir << "]";
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
        "Whisper Slave Manager Server 0.1",
        selector_,
        *net_factory_,
        http_server_params);

    //////////////////////////////////////////////////////////////////////
    //
    // Create the RPC layer
    //
    const net::IpAddress external_ip(FLAGS_external_ip.c_str());

    slave_manager_ = new manager::slave::RPCSlaveManagerService(
        *selector_,
        *net_factory_,
        FLAGS_scp_username,
        external_ip,
        FLAGS_media_output_dir,
        FLAGS_state_dir);

    rpc_http_processor_ = new rpc::HttpServer(selector_,
        http_server_,
        rpc_authenticator_,
        "/rpc_slave_manager",
        true,
        true, 10, "");
    CHECK(rpc_http_processor_->RegisterService(slave_manager_));

    //////////////////////////////////////////////////////////////////////
    //
    // Start the HTTP server
    //
    http_server_->AddAcceptor(net::PROTOCOL_TCP,
                              net::HostPort(0, FLAGS_http_port));
    selector_->RunInSelectLoop(NewCallback(http_server_,
                                           &http::Server::StartServing));
    return 0;
  }

  void Run() {
    if ( !slave_manager_->Start() ) {
      LOG_ERROR << "Failed to start slave_manager_";
      return;
    }
    selector_->Loop();
  }
  void StopInSelectorThread() {
    CHECK(selector_->IsInSelectThread());
    //////////////////////////////////////////////////////////////////////
    //
    // Delete RPC layer
    //
    delete rpc_http_processor_;
    rpc_http_processor_ = NULL;

    delete slave_manager_;
    slave_manager_ = NULL;

    //////////////////////////////////////////////////////////////////////
    //
    // Stop selector
    //
    selector_->RunInSelectLoop(
        NewCallback(selector_, &net::Selector::MakeLoopExit));
  }
  void Stop() {
    // !!! NOT IN SELECTOR THREAD !!!
    selector_->RunInSelectLoop(
        NewCallback(this, &SlaveManagerApp::StopInSelectorThread));
  }

  void Shutdown() {
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

int main (int argc, char *argv[]) {
  return common::Exit(SlaveManagerApp(argc, argv).Main());
}

///////////////////////////////////////////////////////////////////////////////
