// Copyright 2009 WhisperSoft s.r.l.
// Author: Cosmin Tudorache

#include "common/base/scoped_ptr.h"
#include "common/io/ioutil.h"
#include "net/rpc/lib/types/rpc_all_types.h"
#include "net/rpc/lib/codec/json/rpc_json_encoder.h"
#include "net/rpc/lib/codec/json/rpc_json_decoder.h"
#include "net/rpc/lib/rpc_util.h"
#include "rpc_master_manager_service.h"
#include "../common.h"

namespace manager {
namespace master {

const string RPCMasterManagerService::kStateNextFileId("next_file_id");
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
  rpc::CODEC_ID rpc_codec_id,
  int64 gen_file_id_start,
  int64 gen_file_id_increment,
  const string& input_dir,
  const string& process_dir,
  const string& output_dir,
  const string& state_dir,
  const string& state_name)
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
    input_dir_(input_dir),
    process_dir_(process_dir),
    output_dir_(output_dir),
    next_file_id_(gen_file_id_start),
    increment_file_id_(gen_file_id_increment),
    pending_files_(),
    slaves_(),
    file_slaves_(),
    found_new_input_file_callback_(
      NewPermanentCallback(this, &RPCMasterManagerService::FoundNewInputFile)),
    files_regexp_("\\.flv$"),
    files_monitor_(selector_,
                   input_dir_,
                   &files_regexp_,
                   10000,
                   found_new_input_file_callback_),
    state_keeper_(state_dir.c_str(), state_name.c_str()) {
}

RPCMasterManagerService::~RPCMasterManagerService() {
  Stop();
  for ( SlaveControllerMap::iterator it = slaves_.begin();
        it != slaves_.end(); ++it ) {
    SlaveController* slave = it->second;
    delete slave;
  }
  slaves_.clear();
  file_slaves_.clear();
  for ( MediaFileMap::iterator it = pending_files_.begin();
        it != pending_files_.end(); ++it ) {
    MediaFile* file = it->second;
    delete file;
  }
  pending_files_.clear();
  delete found_new_input_file_callback_;
  found_new_input_file_callback_ = NULL;
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
    rpc::CODEC_ID rpc_codec_id,
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
    LOG_ERROR << "Duplicate slave, keeping the old: "
              << *old << ", ignoring the new : " << *slave;
    delete slave;
    return;
  }
  LOG_INFO << "Added slave: " << *slave;
}

bool RPCMasterManagerService::Start() {
  if ( input_dir_.empty() || !io::IsDir(input_dir_.c_str()) ) {
    LOG_ERROR << "Invalid input_dir: [" << input_dir_ << "]";
    return false;
  }
  if ( process_dir_.empty() || !io::IsDir(process_dir_.c_str()) ) {
    LOG_ERROR << "Invalid process_dir: [" << process_dir_ << "]";
    return false;
  }
  if ( output_dir_.empty() || !io::IsDir(output_dir_.c_str()) ) {
    LOG_ERROR << "Invalid output_dir: [" << output_dir_ << "]";
    return false;
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
  // start listening for input files
  if ( !files_monitor_.Start() ) {
    LOG_ERROR << "Failed to start files_monitor.";
    return false;
  }
  LOG_INFO << "Started, #" << slaves_.size() << " slaves";
  return true;
}

void RPCMasterManagerService::Stop() {
  // stop listening for input files
  files_monitor_.Stop();
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

  // add [file id -> slave] to file_slaves_
  AddSlaveByFileId(file->id(), slave, true);

  return file;
}
void RPCMasterManagerService::CompleteMediaFile(SlaveController * slave,
                                                MediaFile* file) {
  if ( file->output().dirs_.empty() ) {
    LOG_ERROR << "MediaFile completed but no results found: " << *file;
    // do nothing for files with no results
    return;
  }
  // TODO(cosmin): find a solution for files with multiple output!!
  //               Now we choose only the first result for distribution!
  // TODO(cosmin): specify remote location!! This file is on a remote slave!!
  const FileSlaveOutput& slave_output = file->output();
  const string& remote_scp_username = slave_output.scp_username_;
  const net::IpAddress remote_ip(slave_output.ip_.c_str());
  const FileOutputDir& output_dir = slave_output.dirs_[0];
  const vector<string>& output_files = output_dir.files_;
  vector<string> src_files;
  for ( uint32 i = 0; i < output_files.size(); i++ ) {
    src_files.push_back(MakeScpPath(remote_scp_username,
                                    remote_ip,
                                    output_files[i]));
  }
  const string src_file = strutil::JoinStrings(src_files, ",");

  FileSlaveMap::iterator it = file_slaves_.find(file->id());
  CHECK(it != file_slaves_.end()) << "Weird file: " << *file;
  // these are the slaves that already have the file.
  FileSlave& file_slave = it->second;

  // Find the slaves that don't have the file, and add the file on them.
  // Use the PROCESS_CMD_COPY to copy the remote file directly, no transcoding.
  //
  for ( SlaveControllerMap::iterator it = slaves_.begin();
        it != slaves_.end(); ++it ) {
    SlaveController* slave = it->second;
    if ( file_slave.Contains(slave) ) {
      // This slave already has the file.
      continue;
    }
    slave->QueueMediaFile(new MediaFile(file->id(),
                                        src_file,
                                        PROCESS_CMD_COPY,
                                        file->process_params(),
                                        FSTATE_IDLE,
                                        "",
                                        timer::Date::Now(),
                                        "",
                                        kEmptyFileSlaveOutput,
                                        kEmptyFileTranscodingStatus));
    // add [file id -> slave] to file_slaves_
    AddSlaveByFileId(file->id(), slave, false);
  }
}

int64 RPCMasterManagerService::GenerateFileId() {
  while ( pending_files_.find(next_file_id_) != pending_files_.end() ) {
    next_file_id_ += increment_file_id_;
  }
  int64 id = next_file_id_;
  next_file_id_ += increment_file_id_;
  return id;
}

int64 RPCMasterManagerService::AddMediaFile(const string& input_file_path,
    ProcessCmd process_cmd, const map<string, string>& process_params) {
  int64 file_id = 0;

  while ( true ) {
    file_id = GenerateFileId();

    const FullMediaFile existing = GetMediaFile(file_id);
    if ( existing.main_ == NULL ) {
      break;
    }
    LOG_ERROR << "Duplicate file id returned by GenerateFileId: " << file_id
              << " existing file: " << *existing.main_;
  }

  MediaFile* file = new MediaFile(file_id,
                                  input_file_path,
                                  process_cmd,
                                  process_params,
                                  FSTATE_IDLE, "", 0, "", kEmptyFileSlaveOutput,
                                  kEmptyFileTranscodingStatus);
  pending_files_.insert(make_pair(file_id, file));
  LOG_INFO << "Added MediaFile: " << *file;
  return file_id;
}

/*
const MediaFile* RPCMasterManagerService::GetMediaFile(int64 file_id,
    vector<const MediaFile*>* duplicates) const {
  // search idle files
  //
  MediaFileMap::const_iterator pit = pending_files_.find(file_id);
  if ( pit != pending_files_.end() ) {
    const MediaFile* file = pit->second;
    return file;
  }
  // search processing files
  //
  FileSlaveMap::const_iterator it = file_slaves_.find(file_id);
  if ( it != file_slaves_.end() ) {
    const FileSlave& file_slave = it->second;
    const MediaFile* main_file = file_slave.main_->GetMediaFile(file_id);
    CHECK_NOT_NULL(main_file) << "File #" << file_id << " should be on"
                                 " SlaveController: " << *file_slave.main_;
    if ( duplicates != NULL ) {
      for ( set<SlaveController*>::const_iterator it =
            file_slave.duplicates_.begin();
            it != file_slave.duplicates_.end(); ++it ) {
        const SlaveController* duplicate_slave = *it;
        const MediaFile* file = duplicate_slave->GetMediaFile(file_id);
        CHECK_NOT_NULL(file) << "File #" << file_id << " should be on"
                                " SlaveController: " << *duplicate_slave;
        duplicates->push_back(file);
      }
    }
    return main_file;
  }
  // no such file_id
  //
  return NULL;
}
*/
FullMediaFile RPCMasterManagerService::GetMediaFile(int64 file_id) const {
  // search idle files
  //
  MediaFileMap::const_iterator pit = pending_files_.find(file_id);
  if ( pit != pending_files_.end() ) {
    const MediaFile* file = pit->second;
    return FullMediaFile(file);
  }
  // search processing files
  //
  FileSlaveMap::const_iterator it = file_slaves_.find(file_id);
  if ( it != file_slaves_.end() ) {
    const FileSlave& file_slave = it->second;
    const MediaFile* main_file = file_slave.main_->GetMediaFile(file_id);
    CHECK_NOT_NULL(main_file) << "File #" << file_id << " should be on"
                                 " SlaveController: " << *file_slave.main_;
    FullMediaFile complete_file(main_file);
    for ( set<SlaveController*>::const_iterator it =
          file_slave.duplicates_.begin();
          it != file_slave.duplicates_.end(); ++it ) {
      const SlaveController* duplicate_slave = *it;
      const MediaFile* file = duplicate_slave->GetMediaFile(file_id);
      CHECK_NOT_NULL(file) << "File #" << file_id << " should be on"
                              " SlaveController: " << *duplicate_slave;
      complete_file.duplicates_.push_back(file);
    }
    return complete_file;
  }
  // no such file_id
  //
  return NULL;
}

void RPCMasterManagerService::GetMediaFiles(vector<FullMediaFile>& out) const {
  out.reserve(out.size() + pending_files_.size() + file_slaves_.size());
  // append idle files
  //
  LOG_WARNING << "GetMediaFiles: #" << file_slaves_.size() << " pending files";
  for ( MediaFileMap::const_iterator it = pending_files_.begin();
        it != pending_files_.end(); ++it ) {
    const MediaFile* file = it->second;
    out.push_back(FullMediaFile(file));
  }
  // append processing files
  //
  LOG_WARNING << "GetMediaFiles: #" << file_slaves_.size() << " processing files";
  for ( FileSlaveMap::const_iterator it = file_slaves_.begin();
        it != file_slaves_.end(); ++it ) {
    int64 file_id = it->first;
    const FileSlave& file_slave = it->second;
    const MediaFile* main_file = file_slave.main_->GetMediaFile(file_id);
    FullMediaFile full_file(main_file);
    CHECK_NOT_NULL(full_file.main_) << " for file_id: " << file_id
                                    << ", main_file: " << main_file
                                    << ", slave: " << *file_slave.main_;
    for ( set<SlaveController*>::const_iterator it = file_slave.duplicates_.begin();
          it != file_slave.duplicates_.end(); ++it ) {
      const SlaveController* duplicate_slave = *it;
      full_file.duplicates_.push_back(duplicate_slave->GetMediaFile(file_id));
    }
    out.push_back(full_file);
  }
}

void RPCMasterManagerService::DebugDumpFiles(vector<const MediaFile*>& out) const {
  for ( MediaFileMap::const_iterator it = pending_files_.begin();
        it != pending_files_.end(); ++it ) {
    const MediaFile* file = it->second;
    out.push_back(file);
  }
  for ( SlaveControllerMap::const_iterator it = slaves_.begin();
        it != slaves_.end(); ++it ) {
    const SlaveController* slave = it->second;
    slave->GetMediaFiles(out);
  }
}

void RPCMasterManagerService::Delete(int64 file_id,
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

  // search processing files
  //
  FileSlaveMap::iterator sit = file_slaves_.find(file_id);
  if ( sit != file_slaves_.end() ) {
    FileSlave& file_slave = sit->second;
    // erase duplicates
    for ( set<SlaveController*>::iterator it = file_slave.duplicates_.begin();
          it != file_slave.duplicates_.end(); ++it ) {
      SlaveController& slave = **it;
      MediaFile* file = slave.DeleteMediaFile(file_id,
                                              erase_slave_output);
      CHECK_NOT_NULL(file);
      DeleteMediaFile(file, erase_master_original);
    }
    // erase original
    file_slave.duplicates_.clear();
    MediaFile* file = file_slave.main_->DeleteMediaFile(file_id,
                                                         erase_slave_output);
    CHECK_NOT_NULL(file);
    DeleteMediaFile(file, erase_master_original);
    file_slaves_.erase(sit);
    return;
  }

  LOG_ERROR << "Delete: No such file_id: " << file_id;
}

FState RPCMasterManagerService::Retry(int64 file_id) {
  // search idle files
  //
  {
    MediaFileMap::iterator it = pending_files_.find(file_id);
    if ( it != pending_files_.end() ) {
      LOG_INFO << "Retry: file already FSTATE_IDLE : " << *it->second;
      return FSTATE_IDLE;
    }
  }

  // search processing files
  //
  {
    FileSlaveMap::iterator sit = file_slaves_.find(file_id);
    if ( sit != file_slaves_.end() ) {
      // all the slaves containing the file
      FileSlave& file_slave = sit->second;

      // remove file from all slaves
      for ( set<SlaveController*>::iterator it = file_slave.duplicates_.begin();
            it != file_slave.duplicates_.end(); ++it ) {
        SlaveController* slave = *it;
        MediaFile* file = slave->DeleteMediaFile(file_id, false);
        CHECK_NOT_NULL(file);
        DeleteMediaFile(file, false);
      }
      // remove original file too, keep a copy for re-adding the file.
      MediaFile original_file;
      MediaFile* file = file_slave.main_->DeleteMediaFile(file_id, false);
      original_file = *file;
      DeleteMediaFile(file, false);
      // WARNING! erasing 'sit' invalidates 'file_slave' reference!
      file_slaves_.erase(sit);

      // re-add the file in IDLE state
      MediaFile* retry_file = new MediaFile(original_file.id(),
                                            original_file.path(),
                                            original_file.process_cmd(),
                                            original_file.process_params(),
                                            FSTATE_IDLE,
                                            "",
                                            0,
                                            "",
                                            kEmptyFileSlaveOutput,
                                            kEmptyFileTranscodingStatus);
      pending_files_.insert(make_pair(retry_file->id(), retry_file));
      return FSTATE_IDLE;
    }
  }

  LOG_ERROR << "Retry: No such file_id: " << file_id;
  return FSTATE_NOT_FOUND;
}

void RPCMasterManagerService::Save() {
  const string state_next_file_id = strutil::StringOf(next_file_id_);

  vector<StateMediaFile> rpc_pending_files(pending_files_.size());
  uint32 i = 0;
  for ( MediaFileMap::const_iterator it = pending_files_.begin();
        it != pending_files_.end(); ++it, ++i ) {
    const MediaFile& file = *it->second;
    rpc_pending_files[i] = StateMediaFile(
        file.id(),
        file.path(),
        EncodeProcessCmd(file.process_cmd()),
        file.process_params(),
        EncodeFState(file.state()),
        file.error(),
        file.ts_state_change(),
        file.output(),
        file.transcoding_status(),
        false);
  }
  const string state_pending_files =
    rpc::JsonEncoder::EncodeObject(rpc_pending_files);

  state_keeper_.SetValue(kStateNextFileId, state_next_file_id);
  state_keeper_.SetValue(kStatePendingFiles, state_pending_files);

  LOG_INFO << "Save: success.";
}
bool RPCMasterManagerService::Load() {
  CHECK(pending_files_.empty()) << "Load: duplicate Load";

  string state_next_file_id;
  if ( !state_keeper_.GetValue(kStateNextFileId, &state_next_file_id) ) {
    LOG_ERROR << "Load: cannot find key: [" << kStateNextFileId << "]";
    return false;
  }
  int64 next_file_id = ::atoll(state_next_file_id.c_str());
  if ( next_file_id == 0 && state_next_file_id.c_str()[0] != '0' ) {
    LOG_ERROR << "Load: unable to read int64 from string: ["
              << state_next_file_id << "]";
    return false;
  }

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

  next_file_id_ = next_file_id;
  LOG_INFO << "Load: next_file_id is " << next_file_id_;
  for ( uint32 i = 0; i < rpc_pending_files.size(); i++ ) {
    const StateMediaFile& rpc_file = rpc_pending_files[i];
    MediaFile* file = new MediaFile(
        rpc_file.id_,
        rpc_file.path_,
        DecodeProcessCmd(rpc_file.process_cmd_),
        rpc_file.process_params_,
        DecodeFState(rpc_file.state_),
        rpc_file.error_,
        rpc_file.ts_state_change_,
        "",
        rpc_file.output_,
        rpc_file.transcoding_status_);
    pending_files_.insert(make_pair(file->id(), file));
    LOG_INFO << "Added MediaFile: " << *file;
  }
  LOG_INFO << "Load: #" << pending_files_.size() << " pending files loaded";

  // Load slave controllers and rebuild file_slaves_
  //
  LOG_INFO << "Load: loading #" << slaves_.size() << " slave controllers..";
  file_slaves_.clear();
  for ( SlaveControllerMap::iterator it = slaves_.begin();
        it != slaves_.end(); ++it ) {
    SlaveController* slave = it->second;
    // Ignore slave failed load, it may be a newly added slave.
    if ( !slave->Load() ) {
      LOG_WARNING << "Load: failed to load slave: " << *slave
                  << " assuming new slave.";
    }
    vector<const MediaFile*> files_on_slave;
    slave->GetMediaFiles(files_on_slave);
    LOG_INFO << "Load: found #" << files_on_slave.size()
             << " processing files on slave: " << slave->name();
    for ( uint32 i = 0; i < files_on_slave.size(); i++ ) {
      const MediaFile* f = files_on_slave[i];
      // add [file id -> slave] to file_slaves_
      AddSlaveByFileId(f->id(), slave, !IsScpPath(f->path()));
    }
  }
  LOG_INFO << "Load: #" << file_slaves_.size() << " total processing files";

  LOG_INFO << "Load: success.";
  return true;
}

void RPCMasterManagerService::FoundNewInputFile(const string& file_path) {
  CHECK(strutil::StrStartsWith(file_path, input_dir_));
  map<string, string> empty_params;
  AddMediaFile(file_path, PROCESS_CMD_TRANSCODE, empty_params);
}

void RPCMasterManagerService::AddSlaveByFileId(int64 file_id,
                                               SlaveController* slave,
                                               bool is_main) {
  // add [file id -> slave] to file_slaves_
  FileSlaveMap::iterator it = file_slaves_.find(file_id);
  if ( it == file_slaves_.end() ) {
    it = file_slaves_.insert(make_pair(file_id,
                                       FileSlave())).first;
  }
  FileSlave& file_slave = it->second;
  if ( is_main ) {
    CHECK_NULL(file_slave.main_) << "Duplicate main slave: " << slave
                                 << " previous set: " << *file_slave.main_;
    file_slave.main_ = slave;
  } else {
    pair<set<SlaveController*>::iterator, bool> result =
      file_slave.duplicates_.insert(slave);
    CHECK(result.second) << "Duplicate secondary slave: " << *slave
                         << " previous set: " << **result.first;
  }
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
