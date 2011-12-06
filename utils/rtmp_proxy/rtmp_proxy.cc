// Copyright 2009 WhisperSoft s.r.l.
// Author: Catalin Popescu

#include <whisperlib/common/base/types.h>
#include <whisperlib/common/base/log.h>
#include <whisperlib/common/base/system.h>
#include <whisperlib/common/base/gflags.h>
#include <whisperlib/common/base/errno.h>
#include <whisperlib/common/base/strutil.h>

#include "net/base/address.h"
#include "net/base/selector.h"
#include "net/base/connection.h"

#include <whisperstreamlib/rtmp/objects/amf/amf0_util.h>
#include <whisperstreamlib/rtmp/objects/amf/amf_util.h>

#include <whisperstreamlib/rtmp/rtmp_protocol.h>
#include <whisperstreamlib/rtmp/rtmp_coder.h>
#include <whisperstreamlib/rtmp/events/rtmp_event.h>
#include <whisperstreamlib/rtmp/events/rtmp_event_notify.h>

//////////////////////////////////////////////////////////////////////

DEFINE_int32(local_port,
             0,
             "We listen on this port");
DEFINE_string(remote_host,
              "",
              "We act as a proxy for a server serving on this host:port");

DEFINE_int32(server_timeout,
             5000,
             "Timout to impose between server and proxy");
DEFINE_int32(client_timeout,
             5000,
             "Timout to impose between client and proxy");
DEFINE_bool(dump_all,
            false,
            "Write all stuff");


//////////////////////////////////////////////////////////////////////

struct RtmpDecoder {
  RtmpDecoder(const char* prefix)
      : prefix_(prefix),
        handshaked_(false),
        decode_buffer_(),
        coder_(&protocol_, 4 << 20),
        error_encountered_(false) {
  }
  void DecodeMore(io::MemoryStream* buffer);

 private:
  const char* prefix_;
  bool handshaked_;
  io::MemoryStream decode_buffer_;
  rtmp::ProtocolData protocol_;
  rtmp::Coder coder_;
  bool error_encountered_;
};

void RtmpDecoder::DecodeMore(io::MemoryStream* buffer) {
  if ( error_encountered_ )  {
    printf("%s -- ERROR NO Decode\n", prefix_);
    fflush(stdout);
    return;
  }
  decode_buffer_.AppendStreamNonDestructive(buffer);
  // printf("%s -- To decode %d\n", prefix_, decode_buffer_.Size());
  if ( !handshaked_ ) {
    if ( decode_buffer_.Size() < 2 * rtmp::kHandshakeSize + 1 ) {
      return;
    }
    int8 first_byte = io::NumStreamer::ReadByte(&decode_buffer_);
    if ( first_byte != rtmp::kHandshakeLeadByte ) {
      printf("%s Invalid first byte in rtmp protocol\n", prefix_);
      fflush(stdout);
    }
    decode_buffer_.Skip(2 * rtmp::kHandshakeSize);
    printf("%s [Handshake Completed]\n", prefix_);
    fflush(stdout);
    handshaked_ = true;
  }

  // Now is time to decode :)
  rtmp::Event* event = NULL;
  //printf("=========================================\n"
  //       "%s DECODING FROM: %d bytes\n %s",
  //       prefix_, decode_buffer_.Size(),
  //       decode_buffer_.DumpContent().c_str());
  while ( true ) {
    rtmp::AmfUtil::ReadStatus err = coder_.Decode(
        &decode_buffer_, rtmp::AmfUtil::AMF0_VERSION, &event);
    if ( err == rtmp::AmfUtil::READ_NO_DATA ) {
      // 0    printf("%s -- NO DATA - %d\n", prefix_, decode_buffer_.Size());
      fflush(stdout);
      return;
    }
    if ( err != rtmp::AmfUtil::READ_OK ) {
      printf("%s Error encountered: %s\n",
             prefix_, rtmp::AmfUtil::ReadStatusName(err));
      fflush(stdout);
      error_encountered_ = true;
      return;
    }
    if ( event != NULL ) {
      printf("==========================================================\n");
      printf("%sEVENT: %s\n", prefix_, event->header()->ToString().c_str());
      printf("%s %s\n",prefix_, event->ToString().c_str());
      fflush(stdout);
      if ( event->event_type() == rtmp::EVENT_CHUNK_SIZE ) {
        const int chunk_size =
            reinterpret_cast<rtmp::EventChunkSize*>(event)->chunk_size();
        protocol_.set_read_chunk_size(chunk_size);
      }
      bool is_metadata = false;
      if ( event->event_type() == rtmp::EVENT_NOTIFY ) {
        rtmp::EventNotify* ev =
            reinterpret_cast<rtmp::EventNotify*>(event);
        is_metadata = (ev->name().value() == "onMetaData");
      }
      if ( event->event_type() == rtmp::EVENT_MEDIA_DATA ) {
        rtmp::ProtocolData protocol2;
        rtmp::BulkDataEvent* bd  =
            reinterpret_cast< rtmp::BulkDataEvent* >(event);
        rtmp::AmfUtil::ReadStatus err2;
        while ( !bd->data().IsEmpty() ) {
          rtmp::Header* const header = new rtmp::Header(&protocol2);
          err2 = header->ReadMediaFromMemoryStream(bd->mutable_data(),
                                                   rtmp::AmfUtil::AMF0_VERSION);
          if ( err2 != rtmp::AmfUtil::READ_OK ) {
            printf("%s Error decoding header in media buffer w/ %d left\n",
                   prefix_,
                   bd->data().Size());
            delete header;
          } else {
            rtmp::Event* const event2 = rtmp::Coder::CreateEvent(header);
            if ( event2 == NULL ) {
              printf("%s Error creating event from header: %s\n",
                     prefix_,
                     header->ToString().c_str());
              delete header;
            } else {
              err2 = event2->ReadFromMemoryStream(bd->mutable_data(),
                                                  rtmp::AmfUtil::AMF0_VERSION);
              if ( err2 == rtmp::AmfUtil::READ_OK ) {
              const int32 len = io::NumStreamer::ReadInt32(bd->mutable_data(),
                                                           common::BIGENDIAN);
              printf("     INSIDE-EVENT: len: %d -- %s\n%s\n",
                     len, event2->header()->ToString().c_str(),
                     event2->ToString().c_str());
              } else {
                printf("%s Error decoding event after header: %s\n",
                       prefix_,
                       header->ToString().c_str());
              }
              delete event2;
            }
          }
        }
      }
      delete event;
    }
    fflush(stdout);
  }
}

//////////////////////////////////////////////////////////////////////

static const char kServerPrefix[] = "<<<<< ";
static const char kClientPrefix[] = ">>>>> ";

class ProxyServerConnection;

class ProxyClientConnection {
  static const int64 kConnectEvent = 1;
  static const int64 kWriteEvent = 2;

 public:
  ProxyClientConnection(net::Selector* selector,
                        net::NetConnection* connection,
                        ProxyServerConnection* const proxy_server);
  ~ProxyClientConnection();

  void ForceCloseConnection();
  bool StartClient();
  void NotifyWrite();

  net::NetConnection* net_connection() const { return net_connection_; }

 private:
  void TimeoutHandler(int64 timeout_id);
  void ConnectionConnectHandler();
  bool ConnectionReadHandler();
  bool ConnectionWriteHandler();
  void ConnectionCloseHandler(int err, net::NetConnection::CloseWhat what);
 private:
  net::Selector* const selector_;
  ProxyServerConnection* proxy_server_;
  net::NetConnection* net_connection_;
  bool is_outbuf_timeout_set_;
  net::Timeouter timeouter_;
  RtmpDecoder* decoder_;
};

class ProxyServerConnection {
  static const int32 kWriteEvent = 2;

 public:
  ProxyServerConnection(net::Selector* selector,
                        net::NetConnection* connection);
  ~ProxyServerConnection();

  void set_proxy_client(ProxyClientConnection* c);

  void ForceCloseConnection();
  void NotifyWrite();
  net::NetConnection* net_connection() const { return net_connection_; }

 private:
  void TimeoutHandler(int64 timeout_id);
  bool ConnectionReadHandler();
  bool ConnectionWriteHandler();
  void ConnectionCloseHandler(int err, net::NetConnection::CloseWhat what);

 private:
  net::Selector* selector_;
  net::NetConnection* net_connection_;
  ProxyClientConnection* proxy_client_;
  bool is_outbuf_timeout_set_;
  net::Timeouter timeouter_;
  RtmpDecoder* decoder_;
};


//////////////////////////////////////////////////////////////////////

ProxyServerConnection::ProxyServerConnection(
    net::Selector* selector,
    net::NetConnection* connection)
    : selector_(selector),
      net_connection_(connection),
      proxy_client_(NULL),
      is_outbuf_timeout_set_(false),
      timeouter_(selector, NewPermanentCallback(
                     this, &ProxyServerConnection::TimeoutHandler)),
      decoder_(new RtmpDecoder(kClientPrefix)) {
  net_connection_->SetReadHandler(NewPermanentCallback(this,
      &ProxyServerConnection::ConnectionReadHandler), true);
  net_connection_->SetWriteHandler(NewPermanentCallback(this,
      &ProxyServerConnection::ConnectionWriteHandler), true);
  net_connection_->SetCloseHandler(NewPermanentCallback(this,
      &ProxyServerConnection::ConnectionCloseHandler), true);
  if ( FLAGS_v ) {
    printf("%s Client Connected\n", kClientPrefix);
    fflush(stdout);
  }
}
ProxyServerConnection::~ProxyServerConnection() {
  delete decoder_;
  delete net_connection_;
  net_connection_ = NULL;
}
void ProxyServerConnection::set_proxy_client(ProxyClientConnection* c) {
  CHECK(proxy_client_ == NULL);
  proxy_client_ = c;
}
void ProxyServerConnection::ForceCloseConnection() {
  LOG_INFO << net_connection_->PrefixInfo()
           << " - Proxy Server connection forced closing.";
  proxy_client_ = NULL;
  net_connection_->FlushAndClose();
}
void ProxyServerConnection::NotifyWrite() {
  if ( net_connection_->state() != net::NetConnection::CONNECTED ||
       net_connection_->outbuf()->IsEmpty() ) {
    return;
  }
  if ( !is_outbuf_timeout_set_ ) {
    timeouter_.SetTimeout(kWriteEvent, FLAGS_client_timeout);
    is_outbuf_timeout_set_ = true;
  }
  net_connection_->RequestWriteEvents(true);
}

void ProxyServerConnection::TimeoutHandler(int64 timeout_id) {
  if ( FLAGS_v ) {
    printf("%s Client Timed out\n", kClientPrefix);
    fflush(stdout);
  }
  LOG_ERROR << net_connection_->PrefixInfo()
            << " - Proxy Server Timeout.";
  net_connection_->FlushAndClose();
}
bool ProxyServerConnection::ConnectionReadHandler() {
  if ( decoder_ != NULL )  {
    decoder_->DecodeMore(net_connection_->inbuf());
  }
  if ( FLAGS_dump_all ) {
    LOG_INFO << kClientPrefix << "========== "
             << net_connection_->inbuf()->Size() << "\n"
             << net_connection_->inbuf()->DebugString();
  }
  if ( proxy_client_ ) {
    proxy_client_->net_connection()->outbuf()->AppendStream(
        net_connection_->inbuf());
    proxy_client_->NotifyWrite();
  }
  return true;
}
bool ProxyServerConnection::ConnectionWriteHandler() {
  if ( net_connection_->outbuf()->IsEmpty() ) {
    timeouter_.UnsetTimeout(kWriteEvent);
    is_outbuf_timeout_set_ = false;
  } else {
    timeouter_.SetTimeout(kWriteEvent, FLAGS_client_timeout);
    is_outbuf_timeout_set_ = true;
  }
  return true;
}
void ProxyServerConnection::ConnectionCloseHandler(
    int err, net::NetConnection::CloseWhat what) {
  if ( FLAGS_v ) {
    printf("%s Client Closed connection\n", kClientPrefix);
    fflush(stdout);
  }
  LOG_WARNING << net_connection_->PrefixInfo()
              << "Proxy Server ConnectionCloseHandler err=" << err
              << " what=" <<  net::NetConnection::CloseWhatName(what);
  if ( what != net::NetConnection::CLOSE_READ_WRITE ) {
    net_connection_->FlushAndClose();
    return;
  }
  if ( proxy_client_ ) {
    proxy_client_->ForceCloseConnection();
  }
  selector_->DeleteInSelectLoop(this);
}

//////////////////////////////////////////////////////////////////////

ProxyClientConnection::ProxyClientConnection(
    net::Selector* selector,
    net::NetConnection* connection,
    ProxyServerConnection* const proxy_server)
    : selector_(selector),
      proxy_server_(proxy_server),
      net_connection_(connection),
      is_outbuf_timeout_set_(false),
      timeouter_(selector, NewPermanentCallback(
                     this, &ProxyClientConnection::TimeoutHandler)),
      decoder_(new RtmpDecoder(kServerPrefix)) {
  net_connection_->SetReadHandler(
      NewPermanentCallback(
          this, &ProxyClientConnection::ConnectionReadHandler), true);
  net_connection_->SetWriteHandler(
      NewPermanentCallback(
          this, &ProxyClientConnection::ConnectionWriteHandler), true);
  net_connection_->SetConnectHandler(
        NewPermanentCallback(
            this, &ProxyClientConnection::ConnectionConnectHandler), true);
  net_connection_->SetCloseHandler(
      NewPermanentCallback(
          this, &ProxyClientConnection::ConnectionCloseHandler), true);
  proxy_server_->set_proxy_client(this);
}
ProxyClientConnection::~ProxyClientConnection() {
  delete decoder_;
  delete net_connection_;
  net_connection_ = NULL;
}
void ProxyClientConnection::ForceCloseConnection() {
  LOG_INFO << net_connection_->PrefixInfo()
           << " Client connection forced closing.";
  proxy_server_ = NULL;
  net_connection_->ForceClose();
}
bool ProxyClientConnection::StartClient() {
  if ( !net_connection_->Connect(net::HostPort(FLAGS_remote_host.c_str())) ) {
    return false;
  }
  timeouter_.SetTimeout(kConnectEvent, FLAGS_server_timeout);
  return true;
}
void ProxyClientConnection::NotifyWrite() {
  if ( net_connection_->state() != net::NetConnection::CONNECTED ||
       net_connection_->outbuf()->IsEmpty() ) {
    return;
  }
  if ( !is_outbuf_timeout_set_ ) {
    timeouter_.SetTimeout(kWriteEvent, FLAGS_server_timeout);
    is_outbuf_timeout_set_ = true;
  }
  net_connection_->RequestWriteEvents(true);
}

void ProxyClientConnection::TimeoutHandler(int64 timeout_id) {
  if ( FLAGS_v ) {
    printf("%s Server Timed out\n", kServerPrefix);
    fflush(stdout);
  }
  LOG_ERROR << net_connection_->PrefixInfo() << " Client server timeout !";
  net_connection_->ForceClose();
}

void ProxyClientConnection::ConnectionConnectHandler() {
  if ( FLAGS_v ) {
    printf("%s Connected to Server\n", kServerPrefix);
    fflush(stdout);
  }
  timeouter_.UnsetTimeout(kConnectEvent);
  NotifyWrite();
}

bool ProxyClientConnection::ConnectionReadHandler() {
  if ( decoder_ != NULL )  {
    decoder_->DecodeMore(net_connection_->inbuf());
  }
  if ( FLAGS_dump_all ) {
    LOG_INFO << kServerPrefix << "=========="
             << net_connection_->inbuf()->Size() << "\n"
             << net_connection_->inbuf()->DebugString();
  }
  if ( proxy_server_ ) {
    proxy_server_->net_connection()->outbuf()->AppendStream(
        net_connection_->inbuf());
    proxy_server_->NotifyWrite();
  }
  return true;
}
bool ProxyClientConnection::ConnectionWriteHandler() {
  if ( net_connection_->outbuf()->IsEmpty() ) {
    timeouter_.UnsetTimeout(kWriteEvent);
    is_outbuf_timeout_set_ = false;
  } else {
    timeouter_.SetTimeout(kWriteEvent, FLAGS_server_timeout);
    is_outbuf_timeout_set_ = true;
  }
  return true;
}
void ProxyClientConnection::ConnectionCloseHandler(
    int err, net::NetConnection::CloseWhat what) {
  if ( FLAGS_v ) {
    printf("%s Server Closed connection\n", kServerPrefix);
    fflush(stdout);
  }
  LOG_WARNING << net_connection_->PrefixInfo()
              << " Proxy client : ConnectionCloseHandler err=" << err
              << " what=" <<  net::NetConnection::CloseWhatName(what);
  if ( what != net::NetConnection::CLOSE_READ_WRITE ) {
    net_connection_->FlushAndClose();
    return;
  }
  if ( proxy_server_ ) {
    proxy_server_->ForceCloseConnection();
  }
  selector_->DeleteInSelectLoop(this);
}

//////////////////////////////////////////////////////////////////////

class ProxyServer {
public:
  ProxyServer(net::Selector* selector,
              net::NetFactory* net_factory,
              net::PROTOCOL net_protocol)
    : selector_(selector),
      net_factory_(net_factory),
      net_protocol_(net_protocol),
      net_acceptor_(net_factory->CreateAcceptor(net_protocol)) {
    net_acceptor_->SetFilterHandler(NewPermanentCallback(
        this, &ProxyServer::AcceptorFilterHandler), true);
    net_acceptor_->SetAcceptHandler(NewPermanentCallback(
        this, &ProxyServer::AcceptorAcceptHandler), true);
    selector_->RunInSelectLoop(NewCallback(this, &ProxyServer::StartServer));
  }
  ~ProxyServer() {
    delete net_acceptor_;
  }

  void StartServer() {
    CHECK(net_acceptor_->Listen(
              net::HostPort("0.0.0.0", FLAGS_local_port)));
  }

  bool AcceptorFilterHandler(const net::HostPort& client_addr) {
    return true; // accept all
  }
  void AcceptorAcceptHandler(net::NetConnection* client_connection) {
    LOG_INFO << net_acceptor_->PrefixInfo() << "Client accepted from "
             << client_connection->remote_address();
    // auto-deletes on close
    ProxyServerConnection* const proxy_server =
        new ProxyServerConnection(selector_, client_connection);
    ProxyClientConnection* const proxy_client =
        new ProxyClientConnection(selector_,
                                  net_factory_->CreateConnection(net_protocol_),
                                  proxy_server);
    if ( !proxy_client->StartClient() ) {
      delete proxy_server;
      delete proxy_client;
    } // else ok -> autodelete
  }

 private:
  net::Selector* const selector_;
  net::NetFactory* const net_factory_;
  const net::PROTOCOL net_protocol_;
  net::NetAcceptor* const net_acceptor_;
};

//////////////////////////////////////////////////////////////////////

int main(int argc, char* argv[]) {
  common::Init(argc, argv);
  net::Selector selector;

  net::NetFactory net_factory(&selector);
  new ProxyServer(&selector, &net_factory, net::PROTOCOL_TCP); // auto deletes

  selector.Loop();

  common::Exit(0);
}
