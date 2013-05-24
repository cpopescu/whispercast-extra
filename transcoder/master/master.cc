// Copyright 2009 WhisperSoft s.r.l.
// Author: Cosmin Tudorache

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <netdb.h> // for gethostbyname
#include <whisperlib/common/app/app.h>

#include <whisperlib/common/io/ioutil.h>

#include <whisperlib/net/base/selector.h>
#include <whisperlib/net/http/http_server_protocol.h>
#include <whisperlib/net/rpc/lib/server/rpc_http_server.h>
#include <whisperlib/net/rpc/lib/rpc_constants.h>

#include "rpc_master_manager_service.h"
#include "rpc_master_control_service.h"
#include "../constants.h"
#include "../common.h"

//////////////////////////////////////////////////////////////////////

DEFINE_int32(http_port,
             8080,
             "The port on which we accept HTTP connections.");
DEFINE_string(output_dirs,
              "",
              "Comma separated list of output directories.");
DEFINE_string(scp_username,
              "",
              "The username for scp transfers.");
DEFINE_string(external_ip,
              "",
              "The IP for remote transfers.");
DEFINE_string(slaves,
              "",
              "Comma separated list of slave descriptions.\n"
              " Description format: "
              "user=<username> rpc_uri=<protocol>://<ip>:<port>?codec=<rpc_codec>\n"
              "e.g. \"user=whispercast rpc_uri=tcp://192.168.1.7:3456?codec=binary, "
              "user=radiolynx rpc_uri=http://192.168.1.2:8080/rpc_slave_manager?codec=json\"");

DEFINE_string(state_dir, "", "Directory where to save state.");

DEFINE_string(http_auth_user,
              "",
              "Http authentication for accessing MasterControl & MasterManager"
              " services.");
DEFINE_string(http_auth_pswd,
              "",
              "Http authentication for accessing MasterControl & MasterManager"
              " services.");
DEFINE_string(http_auth_realm,
              "",
              "If you set a user/pswd, you also need this.");

//////////////////////////////////////////////////////////////////////

class MasterManagerApp : public app::App {
 private:
  net::Selector* selector_;
  net::NetFactory* net_factory_;

  rpc::HttpServer* rpc_processor_;
  net::SimpleUserAuthenticator* rpc_authenticator_;

  manager::master::RPCMasterManagerService* master_manager_;
  manager::master::RPCMasterControlService* master_control_;

  http::Server* http_server_;

  net::HostPort master_server_;

 public:
  MasterManagerApp(int argc, char **&argv) :
    app::App(argc, argv),
    selector_(NULL),
    net_factory_(NULL),
    rpc_processor_(NULL),
    rpc_authenticator_(NULL),
    master_manager_(NULL),
    master_control_(NULL),
    http_server_(NULL) {
  }
  virtual ~MasterManagerApp() {
    CHECK_NULL(selector_);
    CHECK_NULL(net_factory_);
    CHECK_NULL(rpc_processor_);
    CHECK_NULL(rpc_authenticator_);
    CHECK_NULL(master_manager_);
    CHECK_NULL(master_control_);
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
    return 0;
  }
  int InitializeMaster() {
    common::Init(argc_, argv_);

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

    const string rpc_http_path = "/rpc_master_manager";
    vector<string> output_dirs;
    strutil::SplitString(FLAGS_output_dirs, ",", &output_dirs);

    master_manager_ = new manager::master::RPCMasterManagerService(
        *selector_,
        *net_factory_,
        FLAGS_scp_username,
        external_ip,
        FLAGS_http_port,
        rpc_http_path,
        FLAGS_http_auth_user,
        FLAGS_http_auth_pswd,
        rpc::CONNECTION_HTTP,
        rpc::kCodecIdJson,
        output_dirs,
        FLAGS_state_dir);
    vector<string> slaves;
    strutil::SplitString(FLAGS_slaves, ",", &slaves);
    for (vector<string>::const_iterator it = slaves.begin();
         it != slaves.end(); ++it) {
      const string& slave = *it;
      if (slave.empty()) {
        continue;
      }
      net::HostPort host_port;
      string http_path;
      string http_auth_user;
      string http_auth_pswd;
      rpc::CONNECTION_TYPE connection_type;
      rpc::CodecId codec_id;
      bool success = rpc::ParseUri(slave,
                                   &host_port, &http_path,
                                   &http_auth_user, &http_auth_pswd,
                                   &connection_type, &codec_id);
      if (!success) {
        LOG_ERROR << "Bad slave RPC URI: [" << slave << "] , ignoring slave..";
        continue;
      }
      master_manager_->AddSlave(host_port, http_path,
                                http_auth_user, http_auth_pswd,
                                connection_type, codec_id, 2);
    }
    bool success = master_manager_->Start();
    if (!success) {
      LOG_ERROR << "Failed to start master_manager_";
      return -1;
    }
    master_control_ = new manager::master::RPCMasterControlService(
        *selector_, *master_manager_);

    rpc_processor_ = new rpc::HttpServer(selector_,
        http_server_,
        rpc_authenticator_,
        rpc_http_path,
        true,
        true, 10, "");
    CHECK(rpc_processor_->RegisterService(master_control_));
    CHECK(rpc_processor_->RegisterService(master_manager_));

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
    if ( 0 != InitializeMaster() ) {
      return;
    }
    selector_->Loop();
    ShutdownMaster();
  }
  void StopInSelectorThread() {
    CHECK(selector_->IsInSelectThread());
    /////////////////////////////////////////////////////////////////////
    // Cleanup
    master_manager_->Stop();
    /////////////////////////////////////////////////////////////////////
    // Stop selector
    selector_->RunInSelectLoop(
        NewCallback(selector_, &net::Selector::MakeLoopExit));
  }
  void Stop() {
    // !!! NOT IN SELECTOR THREAD !!!
    selector_->RunInSelectLoop(
        NewCallback(this, &MasterManagerApp::StopInSelectorThread));
  }

  void Shutdown() {
  }
  void ShutdownMaster() {
    //////////////////////////////////////////////////////////////////////
    //
    // Delete RPC layer
    //
    delete rpc_processor_;
    rpc_processor_ = NULL;
    delete master_control_;
    master_control_ = NULL;
    delete master_manager_;
    master_manager_ = NULL;

    //////////////////////////////////////////////////////////////////////
    //
    // Delete authenticator
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
  return common::Exit(MasterManagerApp(argc, argv).Main());
}
