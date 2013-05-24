// Copyright 2009 WhisperSoft s.r.l.
// Author: Cosmin Tudorache

#include <whisperlib/common/base/scoped_ptr.h>
#include <whisperlib/common/io/ioutil.h>
#include <whisperlib/net/rpc/lib/types/rpc_all_types.h>
#include <whisperlib/net/rpc/lib/codec/json/rpc_json_encoder.h>
#include <whisperlib/net/rpc/lib/codec/json/rpc_json_decoder.h>
#include <whisperlib/net/rpc/lib/rpc_util.h>
#include "rpc_master_manager_service.h"
#include "../common.h"

namespace manager {
namespace master {

const string RPCMasterManagerService::kStateName("master");
const string RPCMasterManagerService::kStatePendingFiles("pending_files");
const string RPCMasterManagerService::kProcessingExtension(".processing");
const string RPCMasterManagerService::kDoneExtension(".done");
const string RPCMasterManagerService::kErrorExtension(".error");

RPCMasterManagerService::RPCMasterManagerService(
  net::Selector& selector,
  net::NetFactory& net_factory,
  const std::string& scp_username,
  const net::IpAddress& external_ip,
  int16 rpc_port,
  const string& rpc_http_path,
  const string& rpc_http_auth_user,
  const string& rpc_http_auth_pswd,
  rpc::CONNECTION_TYPE rpc_connection_type,
  rpc::CodecId rpc_codec_id,
  const vector<string>& output_dirs,
  const string& state_dir)
  : ServiceInvokerMasterManager(
      ServiceInvokerMasterManager::GetClassName()),
    selector_(selector),
    net_factory_(net_factory),
    scp_username_(scp_username),
    external_ip_(external_ip),
    master_uri_(rpc::CreateUri(net::HostPort(external_ip, rpc_port),
                               rpc_http_path,
                               rpc_http_auth_user,
                               rpc_http_auth_pswd,
                               rpc_connection_type,
                               rpc_codec_id)),
    output_dirs_(output_dirs),
    pending_files_(),
    slaves_(),
    state_keeper_(state_dir, kStateName) {
}

RPCMasterManagerService::~RPCMasterManagerService() {
  Stop();
  for ( SlaveControllerMap::iterator it = slaves_.begin();
        it != slaves_.end(); ++it ) {
    SlaveController* slave = it->second;
    delete slave;
  }
  slaves_.clear();
  for ( MediaFileMap::iterator it = pending_files_.begin();
        it != pending_files_.end(); ++it ) {
    MediaFile* file = it->second;
    delete file;
  }
  pending_files_.clear();
}

const string& RPCMasterManagerService::scp_username() const {
  return scp_username_;
}
const net::IpAddress& RPCMasterManagerService::external_ip() const {
  return external_ip_;
}
const string& RPCMasterManagerService::master_uri() const {
  return master_uri_;
}

void RPCMasterManagerService::AddSlave(
    const net::HostPort& rpc_host_port,
    const string& rpc_http_path,
    const string& rpc_http_auth_user,
    const string& rpc_http_auth_pswd,
    rpc::CONNECTION_TYPE rpc_connection_type,
    rpc::CodecId rpc_codec_id,
    uint32 max_paralel_processing_file_count) {
  SlaveController* slave = new SlaveController(selector_,
                                               net_factory_,
                                               *this,
                                               rpc_host_port,
                                               rpc_http_path,
                                               rpc_http_auth_user,
                                               rpc_http_auth_pswd,
                                               rpc_connection_type,
                                               rpc_codec_id,
                                               max_paralel_processing_file_count,
                                               state_keeper_);
  pair<std::map<std::string, SlaveController*>::iterator, bool> result =
    slaves_.insert(make_pair(slave->name(), slave));
  if ( !result.second ) {
    SlaveController* old = result.first->second;
    LOG_ERROR << "Duplicate slave"
                 ", keeping the old: " << old->ToString()
              << ", ignoring the new : " << slave->ToString();
    delete slave;
    return;
  }
  LOG_INFO << "Added slave: " << slave->ToString();
}

bool RPCMasterManagerService::Start() {
  for ( uint32 i = 0; i < output_dirs_.size(); i++ ) {
    if ( output_dirs_[i] == "" || !io::IsDir(output_dirs_[i]) ) {
      LOG_ERROR << "Invalid output_dir: [" << output_dirs_[i] << "]";
      return false;
    }
  }
  if ( scp_username_.empty() ) {
    LOG_ERROR << "Invalid scp_username: [" << scp_username_ << "]";
    return false;
  }
  if ( !state_keeper_.Initialize() ) {
    LOG_ERROR << "Failed to initialize state_keeper_";
    return false;
  }
  if ( !Load() ) {
    LOG_ERROR << "Load failed, assuming clean start";
  }
  // start slave controllers
  for ( SlaveControllerMap::iterator it = slaves_.begin();
        it != slaves_.end(); ++it ) {
    SlaveController* slave = it->second;
    slave->Start();
  }
  LOG_INFO << "Started, #" << slaves_.size() << " slaves";
  return true;
}

void RPCMasterManagerService::Stop() {
  // stop slave controllers
  for ( SlaveControllerMap::iterator it = slaves_.begin();
        it != slaves_.end(); ++it ) {
    SlaveController& slave = *it->second;
    slave.Stop();
  }
  Save();
  LOG_INFO << "Stopped.";
}

MediaFile* RPCMasterManagerService::GrabMediaFile(SlaveController* slave) {
  // grab from pending files
  if ( pending_files_.empty() ) {
    return NULL;
  }
  MediaFileMap::iterator pit = pending_files_.begin();
  MediaFile* file = pit->second;
  pending_files_.erase(pit);

  return file;
}
void RPCMasterManagerService::CompleteMediaFile(SlaveController * slave,
                                                MediaFile* file) {
  LOG_FATAL << "TODO(cosmin): Distribute the file: " << file->ToString();
}

void RPCMasterManagerService::AddMediaFile(const string& file_id,
    const string& file_path, const map<string, string>& process_params) {
  MediaFile* file = new MediaFile(file_id,
                                  file_path,
                                  process_params,
                                  FSTATE_PENDING, "", 0, "",
                                  kEmptyTranscodingStatus);
  pending_files_.insert(make_pair(file_id, file));
  LOG_INFO << "Added MediaFile: " << file->ToString();
}

const MediaFile* RPCMasterManagerService::GetMediaFile(const string& id) const {
  // search idle files
  //
  MediaFileMap::const_iterator pit = pending_files_.find(id);
  if ( pit != pending_files_.end() ) {
    return pit->second;
  }
  // search working files
  //
  for ( SlaveControllerMap::const_iterator it = slaves_.begin();
        it != slaves_.end(); ++it ) {
    const SlaveController* slave = it->second;
    const MediaFile* file = slave->GetMediaFile(id);
    if ( file != NULL ) { return file; }
  }
  // no such file_id
  //
  return NULL;
}

void RPCMasterManagerService::GetMediaFiles(vector<const MediaFile*>* out) const {
  for ( MediaFileMap::const_iterator it = pending_files_.begin();
        it != pending_files_.end(); ++it ) {
    const MediaFile* file = it->second;
    out->push_back(file);
  }
  for ( SlaveControllerMap::const_iterator it = slaves_.begin();
        it != slaves_.end(); ++it ) {
    const SlaveController* slave = it->second;
    slave->GetMediaFiles(out);
  }
}

void RPCMasterManagerService::Delete(const string& file_id,
                                     bool erase_master_original,
                                     bool erase_slave_output) {
  // search idle files
  //
  MediaFileMap::iterator pit = pending_files_.find(file_id);
  if ( pit != pending_files_.end() ) {
    MediaFile* file = pit->second;
    pending_files_.erase(pit);
    DeleteMediaFile(file, erase_master_original);
    return;
  }

  LOG_ERROR << "Delete: No such file_id: " << file_id;
}

void RPCMasterManagerService::Save() {
  vector<StateMediaFile> rpc_pending_files(pending_files_.size());
  uint32 i = 0;
  for ( MediaFileMap::const_iterator it = pending_files_.begin();
        it != pending_files_.end(); ++it, ++i ) {
    const MediaFile& file = *it->second;
    rpc_pending_files[i] = StateMediaFile(
        file.id(),
        file.path(),
        file.process_params(),
        EncodeFState(file.state()),
        file.error());
  }
  const string state_pending_files =
    rpc::JsonEncoder::EncodeObject(rpc_pending_files);

  state_keeper_.SetValue(kStatePendingFiles, state_pending_files);

  LOG_INFO << "Save: success. #" << pending_files_.size() << " pending files.";
}
bool RPCMasterManagerService::Load() {
  CHECK(pending_files_.empty()) << "Load: duplicate Load";

  string state_pending_files;
  if ( !state_keeper_.GetValue(kStatePendingFiles, &state_pending_files) ) {
    LOG_ERROR << "Load: cannot find key: [" << kStatePendingFiles << "]";
    return false;
  }
  vector<StateMediaFile> rpc_pending_files;
  if ( !rpc::JsonDecoder::DecodeObject(state_pending_files,
                                       &rpc_pending_files) ) {
    LOG_ERROR << "Load: failed to decode rpc pending files, from string: ["
              << state_pending_files << "]";
    return false;
  }

  for ( uint32 i = 0; i < rpc_pending_files.size(); i++ ) {
    const StateMediaFile& rpc_file = rpc_pending_files[i];
    MediaFile* file = new MediaFile(
        rpc_file.id_,
        rpc_file.path_,
        rpc_file.process_params_,
        DecodeFState(rpc_file.state_),
        rpc_file.error_,
        0,
        "",
        kEmptyTranscodingStatus);
    pending_files_.insert(make_pair(file->id(), file));
    LOG_INFO << "Added MediaFile: " << *file;
  }
  LOG_INFO << "Load: #" << pending_files_.size() << " pending files loaded";

  // Load slave controllers
  //
  LOG_INFO << "Load: loading #" << slaves_.size() << " slave controllers..";
  for ( SlaveControllerMap::iterator it = slaves_.begin();
        it != slaves_.end(); ++it ) {
    SlaveController* slave = it->second;
    // Ignore slave failed load, it may be a newly added slave.
    if ( !slave->Load() ) {
      LOG_WARNING << "Load: failed to load slave: " << slave->ToString()
                  << " assuming new slave.";
    }
    vector<const MediaFile*> files_on_slave;
    slave->GetMediaFiles(&files_on_slave);
    LOG_INFO << "Load: found #" << files_on_slave.size()
             << " processing files on slave: " << slave->name();
  }

  LOG_INFO << "Load: success.";
  return true;
}

void RPCMasterManagerService::DeleteMediaFile(MediaFile* file,
                                              bool erase_master_original) {
  if ( erase_master_original && !IsScpPath(file->path())) {
    io::Rm(file->path());
  }
  delete file;
}

void RPCMasterManagerService::NotifyStateChange(
    rpc::CallContext< rpc::Void >* call,
    const string& slave_id,
    const  FileState& rpc_state) {
  SlaveControllerMap::iterator it = slaves_.find(slave_id);
  if ( it == slaves_.end() ) {
    LOG_ERROR << "No such slave_id: " << slave_id
              << ", rpc_state: " << rpc_state;
    call->Complete();
    return;
  }
  SlaveController& slave = *it->second;
  slave.NotifyMediaFileStateChange(rpc_state);
  call->Complete();
}

}
}
