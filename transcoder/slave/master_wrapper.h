// Copyright (c) 2009, Whispersoft s.r.l.
// All rights reserved.
//
// Author: Cosmin Tudorache
//
//////////////////////////////////////////////////////////////////////

#ifndef MASTER_WRAPPER_H_
#define MASTER_WRAPPER_H_

#include <whisperlib/net/base/connection.h>
#include <whisperlib/net/rpc/lib/rpc_util.h>
#include "../auto/client_wrappers.h"
#include "../constants.h"
#include "media_file.h"

namespace manager { namespace slave {

class MediaFile;

class MasterWrapper
{
public:
  MasterWrapper(net::Selector& selector,
                net::NetFactory& net_factory,
                const std::string& uri,
                const net::HostPort& host_port,
                const std::string& rpc_http_path,
                const std::string& rpc_http_user,
                const std::string& rpc_http_pswd,
                rpc::CONNECTION_TYPE rpc_connection_type,
                rpc::CODEC_ID rpc_codec_id);
  virtual ~MasterWrapper();

  bool Start();
  void Stop();

  ServiceWrapperMasterManager& RPCWrapper();

  void AddFile(MediaFile* file);
  void RemFile(MediaFile* file);
  const std::set<MediaFile*> Files() const;

  // Counts active RPC calls through RPCWrapper.
  // Helps on deleting the wrapper when there are no more RPCs active.
  void RPCPin();
  void RPCUnpin();
  uint32 RPCPinCount() const;

  const std::string& uri() const;

  std::string ToString() const;

private:
  net::Selector& selector_;
  net::NetFactory& net_factory_;

  // master identifier, contains all elements necessary to connect to master.
  const std::string uri_;

  // master host port
  const net::HostPort host_port_;

  // rpc stuff
  const std::string rpc_http_path_; // useful for HTTP only: request path
  const std::string rpc_http_user_; // useful for HTTP only: authorization user
  const std::string rpc_http_pswd_; // useful for HTTP only: authorization pswd
  const rpc::CONNECTION_TYPE rpc_connection_type_;
  const rpc::CODEC_ID rpc_codec_id_;
  rpc::IClientConnection* rpc_connection_;
  ServiceWrapperMasterManager* rpc_client_wrapper_;

  // files referenced by this master
  std::set<MediaFile*> files_;

  // counts active rpc calls
  uint32 rpc_pin_count_;
};

std::ostream& operator<<(std::ostream& os, const MasterWrapper& master);

}  // namespace slave
}  // namespace manager

#endif /*MASTER_WRAPPER_H_*/
