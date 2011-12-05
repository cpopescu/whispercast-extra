// Copyright 2009 WhisperSoft s.r.l.
// Author: Cosmin Tudorache

// A TCP server that listens on port 843 and serves Flash player policy file.
//
// How it works:
// ------------------------
//  The Flash player connects on port 843 and sends the following XML string:
//   <policy-file-request/>
//
//  Out server then must send the following XML in reply:
//   <cross-domain-policy>
//      <allow-access-from domain="*" to-ports="*" />
//   </cross-domain-policy>
//
//  * is the wildcard and means "all ports/domains". If you want to restrict
//  access to a particular port, enter the port number, or a list or range
//  of numbers.

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include "common/app/app.h"

#include "common/base/types.h"
#include "common/base/log.h"
#include "common/base/system.h"
#include "common/base/gflags.h"
#include "common/base/errno.h"
#include "common/base/strutil.h"
#include "common/io/buffer/io_memory_stream.h"
#include "common/io/file/file.h"

#include "net/base/selector.h"
#include "net/base/connection.h"

//////////////////////////////////////////////////////////////////////

DEFINE_int32(tcp_port, 843, "The port on which we accept TCP connections.");

DEFINE_string(flash_policy_file, "",
              "Flash XML policy file to be sent to clients.");

//////////////////////////////////////////////////////////////////////

namespace flash_policy_server {

class FlashPolicyServerConnection {
 public:
  FlashPolicyServerConnection(net::Selector* selector,
                              net::NetConnection * net_connection,
                              bool auto_delete_on_close,
                              const std::string& flash_policy_xml)
    : selector_(selector),
      net_connection_(net_connection),
      auto_delete_on_close_(auto_delete_on_close),
      flash_policy_xml_(flash_policy_xml),
      timeouter_(selector_, NewPermanentCallback(
          this, &FlashPolicyServerConnection::TimeoutHandler)) {
    LOG_INFO << "++FlashPolicyServerConnection";
    net_connection_->SetReadHandler(NewPermanentCallback(
        this, &FlashPolicyServerConnection::ConnectionReadHandler), true);
    net_connection_->SetWriteHandler(NewPermanentCallback(
        this, &FlashPolicyServerConnection::ConnectionWriteHandler), true);
    net_connection_->SetCloseHandler(NewPermanentCallback(
        this, &FlashPolicyServerConnection::ConnectionCloseHandler), true);
    timeouter_.SetTimeout(kRequestEvent, kRequestTimeout);
  }
  virtual ~FlashPolicyServerConnection() {
    LOG_INFO << "--FlashPolicyServerConnection";
    delete net_connection_;
  }

  void TimeoutHandler(int64 timeout_id) {
    switch (timeout_id) {
      case kRequestEvent:
        LOG_WARNING << "Timeout flash policy request(" << kRequestTimeout
                    << " ms), closing connection.";
        net_connection_->ForceClose();
        return;
      default:
        LOG_FATAL << "no such timeout_id: " << timeout_id;
        return;
    };
  }

  //////////////////////////////////////////////////////////////////////
  //
  //    net::BufferedConnection methods
  //

  virtual void XHandleRead() {
  }

  //  Handle incoming data.
  //  Use net::BufferedConnection::HandleRead() to buffer incoming data
  //  then process data in net::BufferedConnection::inbuf()
  // returns:
  //   true - data successfully read. Even if an incomplete packet was received.
  //   false - error reading data from network. The caller will close
  //           the connection immediately on return.
  bool ConnectionReadHandler() {
    // get the BufferedConnection's internal memory stream
    io::MemoryStream* const ms = net_connection_->inbuf();

    char text[256] = {0,};
    static const std::string expected_request("<policy-file-request/>");

    ms->MarkerSet();
    int32 len = ms->Read(text, sizeof(text) - 1);
    CHECK_LT(len, sizeof(text));
    if ( 0 != ::strncmp(expected_request.c_str(),
        text,
        std::min(len, static_cast<int32>(expected_request.size())))) {
      // bad request
      ms->MarkerRestore();
      LOG_ERROR << "Bad request from remote: "
                << net_connection_->remote_address()
                << " data: " << ms;
      return false;
    }
    if ( len < expected_request.size() ) {
      // not enough data
      ms->MarkerClear();
      return true;
    }
    ms->MarkerClear();

    LOG_INFO << "Replying to request from: "
             << net_connection_->remote_address();

    // send reply over network
    //
    net_connection_->Write(flash_policy_xml_);

    // IMPORTANT! close the connection, otherwise the client does not
    // check received data.
    LOG_INFO << "Auto closing connection from: "
             << net_connection_->remote_address();
    net_connection_->FlushAndClose();

    return true;
  }

  virtual void XHandleWrite() {
  }

  bool ConnectionWriteHandler() {
    return true;
  }

  virtual void XHandleConnect() {
  }

  virtual void XHandleError() {
  }
  virtual void XHandleTimeout() {
  }
  virtual void XHandleAccept() {
  }


  void ConnectionCloseHandler(int err, net::NetConnection::CloseWhat what) {
    if ( err != 0 ) {
      LOG_ERROR << "ConnectionCloseHandler"
                    " err: " << GetSystemErrorDescription(err)
                <<  " what: " << net::NetConnection::CloseWhatName(what);
    }
    if ( what != net::NetConnection::CLOSE_READ_WRITE ) {
      net_connection_->FlushAndClose();
      return;
    }
    if ( auto_delete_on_close_ ) {
      LOG_WARNING << "Auto deleting ConnectionCloseHandler";
      selector_->DeleteInSelectLoop(this);
    }
  }

 private:
  net::Selector* selector_;
  net::NetConnection* net_connection_;
  const bool auto_delete_on_close_;
  static const int64 kRequestEvent = 1;        // and ID for this timeout event
  static const int64 kRequestTimeout = 20000;  // milliseconds
  const std::string flash_policy_xml_;
  net::Timeouter timeouter_;
};

class FlashPolicyServer {
 public:
  // flash_allow : map of [domain, ports]
  FlashPolicyServer(net::Selector* selector,
                    Closure* error_callback,
                    const std::string& flash_policy_xml)
    : selector_(selector),
      net_acceptor_(new net::TcpAcceptor(selector)),
      error_callback_(error_callback),
      flash_policy_xml_(flash_policy_xml) {
    net_acceptor_->SetFilterHandler(NewPermanentCallback(
        this, &FlashPolicyServer::AcceptorFilterHandler), true);
    net_acceptor_->SetAcceptHandler(NewPermanentCallback(
        this, &FlashPolicyServer::AcceptorAcceptHandler), true);
  }
  virtual ~FlashPolicyServer() {
    delete net_acceptor_;
    net_acceptor_ = NULL;
  }

  // Open on the given local address & port.
  void Open(net::HostPort addr) {
    // do not try double Open
    CHECK_EQ(net_acceptor_->state(), net::NetAcceptor::DISCONNECTED);
    CHECK(selector_->IsInSelectThread());

    net_acceptor_->Listen(addr);
    if ( !net_acceptor_->Listen(addr) ) {
      LOG_ERROR << "Open completed by error";
      OnError();
      return;
    }
    CHECK_EQ(net_acceptor_->state(), net::NetAcceptor::LISTENING);
    LOG_INFO << "FlashPolicyServer listening on port "
             << net_acceptor_->local_address().port();
    return;
  }

  bool IsOpen() const {
    return net_acceptor_->state() == net::NetAcceptor::LISTENING;
  }

  void Close() {
    if ( IsOpen() ) {
      LOG_INFO << "FlashPolicyServer shutdown (from  port "
               << net_acceptor_->local_address().port() << ")";
      net_acceptor_->Close();
    }
  }

 protected:
  void OnError() {
    if ( error_callback_ ) {
      error_callback_->Run();
      error_callback_ = NULL;
    }
  }

  //////////////////////////////////////////////////////////////////////
  //
  //    net::Connection methods
  //
  virtual void XHandleRead() {
  }

  virtual void XHandleWrite() {
  }

  virtual void XHandleConnect() {
  }

  bool ConnectionWriteHandler() {
    return true;
  }

  virtual void XHandleError() {
  }
  virtual void XHandleTimeout() {
  }
  virtual void XHandleAccept() {
  }

  bool AcceptorFilterHandler(const net::HostPort& remote) {
    return true;
  }
  void AcceptorAcceptHandler(net::NetConnection* net_connection) {
    LOG_INFO << "New TCP Connection from " << net_connection->remote_address();

    // auto deletes on close
    new FlashPolicyServerConnection(
        selector_, net_connection, true, flash_policy_xml_);
  };

 private:
  net::Selector* selector_;
  net::NetAcceptor* net_acceptor_;
  Closure* error_callback_;  // to be called if an error occures
  const std::string flash_policy_xml_;  // policy xml
};

};  // namespace flash_policy_server

class FlashPolicyApp : public app::App {
 private:
  net::Selector* selector_;
  thread::Thread* selector_thread_;

  flash_policy_server::FlashPolicyServer* flash_policy_server_;

 public:
  FlashPolicyApp(int &argc, char **&argv) :
    app::App(argc, argv),
    selector_(NULL),
    selector_thread_(NULL),
    flash_policy_server_(NULL) {
  }
  virtual ~FlashPolicyApp() {
    CHECK_NULL(selector_);
    CHECK_NULL(selector_thread_);
    CHECK_NULL(flash_policy_server_);
  }

  void FlashPolicyServerError() {
    app::App::stop_event_.Signal();
  }

 protected:
  int Initialize() {
    common::Init(argc_, argv_);

    //////////////////////////////////////////////////////////////////////
    //
    // Read Flash XML policy file
    //
    if ( FLAGS_flash_policy_file == "" ) {
      LOG_ERROR << "No flash_policy_file specified."
                   " Please specify an XML file.";
      return -1;
    }
    LOG_INFO << "Using flash_policy file: " << FLAGS_flash_policy_file;
    io::File file;
    if ( !file.Open(FLAGS_flash_policy_file.c_str(),
                    io::File::GENERIC_READ,
                    io::File::OPEN_EXISTING) ) {
      LOG_ERROR << "Failed to open file: " << FLAGS_flash_policy_file
                << " error: " << GetLastSystemErrorDescription();
      return -1;
    }
    std::ostringstream oss;
    while ( true ) {
      char data[256] = {0,};
      int read = file.Read(data, sizeof(data) - 1);
      if ( read == 0 ) {
        break;
      }
      oss << data;
    }
    file.Close();
    const std::string flash_policy_xml = oss.str();

    //////////////////////////////////////////////////////////////////////
    //
    // Create the selector
    //
    selector_ = new net::Selector();

    //////////////////////////////////////////////////////////////////////
    //
    // Create the FlashPolicyServer
    //
    flash_policy_server_ =
      new flash_policy_server::FlashPolicyServer(
        selector_, NewCallback(this, &FlashPolicyApp::FlashPolicyServerError),
        flash_policy_xml);
    selector_->RunInSelectLoop(NewCallback(
                                 flash_policy_server_,
                                 &flash_policy_server::FlashPolicyServer::Open,
                                 net::HostPort(0, FLAGS_tcp_port)));

    return 0;
  }

  void Run() {
    selector_->Loop();
  }
  void Stop() {
    selector_->RunInSelectLoop(
      NewCallback(flash_policy_server_,
                  &flash_policy_server::FlashPolicyServer::Close));
    selector_->RunInSelectLoop(
      NewCallback(selector_, &net::Selector::MakeLoopExit));
  }

  void Shutdown() {
    //////////////////////////////////////////////////////////////////////
    //
    // Delete the FlashPolicyServer
    //
    delete flash_policy_server_;
    flash_policy_server_ = NULL;

    //////////////////////////////////////////////////////////////////////
    //
    // Delete selector
    //
    delete selector_;
    selector_ = NULL;
  }
};

int main(int argc, char *argv[]) {
  return common::Exit(FlashPolicyApp(argc, argv).Main());
}

///////////////////////////////////////////////////////////////////////////////
