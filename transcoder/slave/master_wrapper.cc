// Copyright (c) 2009, Whispersoft s.r.l.
// All rights reserved.
//
// Author: Cosmin Tudorache
//
//////////////////////////////////////////////////////////////////////

#include <whisperlib/net/rpc/lib/client/rpc_client_connection_tcp.h>
#include <whisperlib/net/rpc/lib/client/rpc_client_connection_http.h>
#include "master_wrapper.h"

namespace manager {
namespace slave {

MasterWrapper::MasterWrapper(net::Selector& selector,
                             net::NetFactory& net_factory,
                             const std::string& uri,
                             const net::HostPort& host_port,
                             const std::string& rpc_http_path,
                             const std::string& rpc_http_user,
                             const std::string& rpc_http_pswd,
                             rpc::CONNECTION_TYPE rpc_connection_type,
                             rpc::CODEC_ID rpc_codec_id)
  : selector_(selector),
    net_factory_(net_factory),
    uri_(uri),
    host_port_(host_port),
    rpc_http_path_(rpc_http_path),
    rpc_http_user_(rpc_http_user),
    rpc_http_pswd_(rpc_http_pswd),
    rpc_connection_type_(rpc_connection_type),
    rpc_codec_id_(rpc_codec_id),
    rpc_connection_(NULL),
    rpc_client_wrapper_(NULL) {
}

MasterWrapper::~MasterWrapper() {
  Stop();
  CHECK_NULL(rpc_connection_);
  CHECK_NULL(rpc_client_wrapper_);
}

bool MasterWrapper::Start() {
  if(rpc_connection_type_ == rpc::CONNECTION_TCP) {
    rpc_connection_ = new rpc::ClientConnectionTCP(
        selector_, net_factory_, net::PROTOCOL_TCP, host_port_, rpc_codec_id_);
  } else if(rpc_connection_type_ == rpc::CONNECTION_HTTP) {
    rpc::ClientConnectionHTTP* rpc_http_connection =
        new rpc::ClientConnectionHTTP(
            selector_, net_factory_, net::PROTOCOL_TCP,
            http::ClientParams(), host_port_, rpc_codec_id_, rpc_http_path_);
    rpc_http_connection->SetHttpAuthentication(rpc_http_user_, rpc_http_pswd_);
    rpc_connection_ = rpc_http_connection;
  } else {
    LOG_FATAL << "No such rpc_connection_type_: " << rpc_connection_type_;
    return false;
  }
  rpc_client_wrapper_ = new ServiceWrapperMasterManager(
      *rpc_connection_, "MasterManager");
  return true;
}

void MasterWrapper::Stop() {
  if(rpc_client_wrapper_) {
    delete rpc_client_wrapper_;
    rpc_client_wrapper_ = NULL;
  }
  if(rpc_connection_) {
    delete rpc_connection_;
    rpc_connection_ = NULL;
  }
}

ServiceWrapperMasterManager& MasterWrapper::RPCWrapper() {
  CHECK_NOT_NULL(rpc_client_wrapper_) << "Not started!";
  return *rpc_client_wrapper_;
}

void MasterWrapper::AddFile(MediaFile* file) {
  std::pair<std::set<MediaFile*>::iterator, bool> result = files_.insert(file);
  CHECK(result.second) << "Duplicate AddFile file: " << *file;
}
void MasterWrapper::RemFile(MediaFile* file) {
  int result = files_.erase(file);
  CHECK(result == 1) << "Missing RemFile file: " << *file;
}
const std::set<MediaFile*> MasterWrapper::Files() const {
  return files_;
}

void MasterWrapper::RPCPin() {
  rpc_pin_count_++;
}
void MasterWrapper::RPCUnpin() {
  CHECK_GE(rpc_pin_count_, 1);
  rpc_pin_count_--;
}
uint32 MasterWrapper::RPCPinCount() const {
  return rpc_pin_count_;
}

const std::string& MasterWrapper::uri() const {
  return uri_;
}

std::string MasterWrapper::ToString() const {
  std::ostringstream oss;
  oss << "MasterWrapper{"
      << "uri_: " << uri()
      << ", host_port_: " << host_port_
      << ", rpc_http_path_: " << rpc_http_path_
      << ", rpc_http_user_: " << rpc_http_user_
      << ", rpc_http_pswd_: " << rpc_http_pswd_
      << ", rpc_connection_type_: " << rpc_connection_type_
      << ", rpc_codec_id_: " << rpc_codec_id_
      << ", files_=#" << files_.size()
      << ", rpc_pin_count_=" << RPCPinCount()
      << "}";
  return oss.str();
}

std::ostream& operator<<(std::ostream& os, const MasterWrapper& master) {
  return os << master.ToString();
}

}    // namespace slave
}    // namespace manager
