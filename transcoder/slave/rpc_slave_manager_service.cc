#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <whisperlib/common/base/strutil.h>
#include <whisperlib/common/io/ioutil.h>
#include <whisperlib/common/io/file/file.h>
#include <whisperlib/common/io/file/file_input_stream.h>
#include <whisperlib/common/base/errno.h>
#include <whisperlib/common/base/scoped_ptr.h>
#include <whisperlib/net/rpc/lib/types/rpc_all_types.h>
#include <whisperlib/net/rpc/lib/codec/json/rpc_json_encoder.h>
#include <whisperlib/net/rpc/lib/codec/json/rpc_json_decoder.h>
#include "rpc_slave_manager_service.h"
#include "../common.h"
#include "auto/types.h"

// defined in slave.cc
DECLARE_string(processor);
DECLARE_int32(parallel_processing);

namespace manager { namespace slave {

const string RPCSlaveManagerService::kStateName("slave");
const string RPCSlaveManagerService::kStateKeyName("state");

string RPCSlaveManagerService::FileOutputDir(const MediaFile& file) const {
  return strutil::JoinPaths(output_dir_, file.id());
}

RPCSlaveManagerService::RPCSlaveManagerService(
      net::Selector& selector,
      net::NetFactory& net_factory,
      const std::string& scp_username,
      const net::IpAddress& external_ip,
      const string& output_dir,
      const string& state_dir)
  : ServiceInvokerSlaveManager(ServiceInvokerSlaveManager::GetClassName()),
    selector_(selector),
    net_factory_(net_factory),
    scp_username_(scp_username),
    external_ip_(external_ip),
    output_dir_(output_dir),
    files_(),
    masters_(),
    active_processes_(),
    state_keeper_(state_dir, kStateName),
    state_keep_user_(&state_keeper_, "slave_manager_service", 0) {
}
RPCSlaveManagerService::~RPCSlaveManagerService() {
  Stop();
}

bool RPCSlaveManagerService::Start() {
  #define VERIFY_DIR(dirname) \
    if(dirname.empty() || !io::IsDir(dirname.c_str())) {\
      LOG_ERROR << "Invalid " #dirname ": [" << dirname << "]";\
      return false;\
    }
  VERIFY_DIR(output_dir_);
  #undef VERIFY_DIR

  if ( !io::IsReadableFile(FLAGS_processor) ) {
    LOG_ERROR << "Processor executable: [" << FLAGS_processor << "]"
                 " does not exist";
    return false;
  }

  if( !state_keeper_.Initialize() ) {
    LOG_ERROR << "Failed to start state_keeper_ .";
    return false;
  }
  if ( !Load() ) {
    LOG_WARNING << "Load failed, assuming clean start";
  }

  for ( MasterMap::iterator it = masters_.begin(); it != masters_.end(); ++it ) {
    MasterWrapper* mw = it->second;
    if ( !mw->Start() ) {
      LOG_ERROR << "Failed to start MasterWrapper";
      return false;
    }
  }
  return true;
}
void RPCSlaveManagerService::Stop() {
  LOG_INFO << "Killing " << active_processes_.size() << " active processes";
  for ( ProcessSet::iterator it = active_processes_.begin();
        it != active_processes_.end(); ++it ) {
    process::Process* p = *it;
    p->Kill();
    delete p;
  }
  active_processes_.clear();
  for ( MasterMap::iterator it = masters_.begin(); it != masters_.end(); ++it ) {
    MasterWrapper* mw = it->second;
    mw->Stop();
  }
  Save();
}

void RPCSlaveManagerService::ChangeFileState(MediaFile* file, FState state,
                                             const string& error) {
  CHECK_NOT_NULL(file);
  file->set_state(state);
  file->set_error(error);
  FileState new_state(file->id(),
                      EncodeFState(file->state()),
                      file->error(),
                      "",
                      file->transcoding_status());
  // send notification with new_state
  file->master()->Pin();
  file->master()->RPCWrapper().NotifyStateChange(
      NewCallback(this,
                  &RPCSlaveManagerService::HandleNotifyStateChangeResult,
                  file->master()),
      file->slave_id(),
      new_state);
  Save();
}

bool RPCSlaveManagerService::AddFile(
    const string& file_id,
    const string& slave_id,
    const string& master_uri,
    const net::HostPort& master_alternate_ip,
    const string& remote_scp_path,
    const map<string, string>& process_params,
    FState state,
    const string& error,
    string* out_err) {
  MediaFileMap::iterator it = files_.find(file_id);
  if ( it != files_.end() ) {
    *out_err = "File ID: [" + file_id + "] already exists";
    return false;
  }

  // parse master_uri, possibly replace 0.0.0.0 with master_alternate_ip
  net::HostPort master_host_port;
  string master_http_path;
  string master_http_user;
  string master_http_pswd;
  rpc::CONNECTION_TYPE master_connection_type;
  rpc::CodecId master_codec;
  bool success = rpc::ParseUri(master_uri, &master_host_port, &master_http_path,
                               &master_http_user, &master_http_pswd,
                               &master_connection_type, &master_codec);
  if ( !success ) {
    LOG_ERROR << "Failed to parse master address from: [" << master_uri << "]";
    *out_err = "Cannot parse master_uri";
    return false;
  }
  if ( master_host_port.ip_object().IsInvalid() ) {
    // use alternate IP
    master_host_port.set_ip(master_alternate_ip.ip_object());
  }

  // recompose final master uri (contains the real IP, instead of 0.0.0.0)
  const string final_master_uri = rpc::CreateUri(
      master_host_port, master_http_path, master_http_user,
      master_http_pswd, master_connection_type, master_codec);

  // possibly add a new MasterWrapper
  //
  MasterWrapper* master = NULL;
  MasterMap::iterator mit = masters_.find(final_master_uri);
  if ( mit == masters_.end() ) {
    master = new MasterWrapper(selector_, net_factory_, master_uri,
                               master_host_port, master_http_path,
                               master_http_user, master_http_pswd,
                               master_connection_type, master_codec);
    bool success = master->Start();
    if ( !success ) {
      LOG_ERROR << "Cannot start MasterWrapper: " << master->ToString()
                << ", for media file ID: " << file_id;
      *out_err = "Cannot start MasterWrapper.";
      delete master;
      return false;
    }
    masters_[final_master_uri] = master;
  } else {
    master = mit->second;
  }

  // finally create & add the MediaFile
  files_[file_id] = new MediaFile(file_id, slave_id, master, remote_scp_path,
                                  process_params, state, error);

  return true;
}

void RPCSlaveManagerService::DelFile(const string& file_id) {
  MediaFileMap::iterator it = files_.find(file_id);
  if ( it == files_.end() ) {
    LOG_ERROR << "DelFile(" << file_id << "), no such file_id";
    return;
  }
  MediaFile* file = it->second;
  io::Rm(FileOutputDir(*file));
  files_.erase(it);
  delete file;
}

void RPCSlaveManagerService::Save() {
  // create an array of all media files
  vector< StateMediaFile > rpc_files(files_.size());
  uint32 i = 0;
  for ( MediaFileMap::const_iterator it = files_.begin();
        it != files_.end(); ++it, ++i ) {
    const MediaFile& file = *it->second;
    rpc_files[i] = StateMediaFile(file.id(),
                                  file.slave_id(),
                                  file.master()->uri(),
                                  file.remote_scp_path(),
                                  file.process_params(),
                                  EncodeFState(file.state()),
                                  file.error());
  }

  state_keep_user_.SetValue(kStateKeyName,
      rpc::JsonEncoder::EncodeObject(rpc_files));
  LOG_INFO << "Save: finished. #" << files_.size() << " files saved.";
}
bool RPCSlaveManagerService::Load() {
  string str_encoded_files;
  bool success = state_keep_user_.GetValue(kStateKeyName, &str_encoded_files);
  if(!success) {
    LOG_ERROR << "Load: key not found: " << kStateKeyName;
    return false;
  }

  vector< StateMediaFile > rpc_files;
  if ( !rpc::JsonDecoder::DecodeObject(str_encoded_files, &rpc_files) ) {
    LOG_ERROR << "Load: failed to decode str_encoded_files:"
                 " [" << str_encoded_files << "]";
    return false;
  }
  LOG_INFO << "Load: load state succeeded, #" << rpc_files.size()
           << " files found, checking disk";

  // load every file
  //
  for(uint32 i = 0, size = rpc_files.size(); i < size; ++i) {
    const StateMediaFile& rpc_file = rpc_files[i];
    string err;
    AddFile(rpc_file.id_.get(),
            rpc_file.slave_id_.get(),
            rpc_file.master_uri_.get(),
            net::HostPort(),
            rpc_file.src_.get(),
            rpc_file.process_params_.get(),
            DecodeFState(rpc_file.state_.get()),
            rpc_file.error_.get(),
            &err);
  }

  // resume work on files in FSTATE_PROCESSING state
  for ( MediaFileMap::iterator it = files_.begin(); it != files_.end(); ++it ) {
    MediaFile* file = it->second;
    if ( file->state() == FSTATE_PROCESSING ) {
      LOG_DEBUG << "Load: resuming PROCESSING file: " << *file;
      StartProcessing(file);
    }
  }
  LOG_INFO << "Load: finished. #" << files_.size() << " files added.";

  return true;
}

void RPCSlaveManagerService::StartProcessingMoreFiles() {
  // count the files that are already started
  uint32 count_processing = 0;
  for ( MediaFileMap::iterator it = files_.begin(); it != files_.end(); ++it ) {
    MediaFile* file = it->second;
    if ( file->state() == FSTATE_PROCESSING ) {
      count_processing++;
    }
  }
  // start processing new files
  for ( MediaFileMap::iterator it = files_.begin();
        it != files_.end() && count_processing < FLAGS_parallel_processing;
        ++it ) {
    MediaFile* file = it->second;
    if ( file->state() == FSTATE_PENDING ) {
      StartProcessing(file);
      count_processing++;
    }
  }
}

namespace {
void HandleStdout(const string& line) {
  LOG_WARNING << "#### HandleStdout: [" << line << "]";
}
void HandleStderr(const string& line) {
  LOG_WARNING << "#### HandleStderr: [" << line << "]";
}
}
void RPCSlaveManagerService::StartProcessing(MediaFile* file) {
  vector<string> transcoder_args(2);
  transcoder_args[0] = file->remote_scp_path();
  transcoder_args[1] = FileOutputDir(*file);

  // start the Processing process
  process::Process* process = new process::Process(
      FLAGS_processor, transcoder_args);
  if ( !process->Start(
      //NewPermanentCallback(&HandleStdout), true,
      //NewPermanentCallback(&HandleStderr), true,
      // receive progress feedback from the transcoder through child's stderr
      // on HandleProcessMessage callback
      NULL, false,
      NewPermanentCallback(this,
          &RPCSlaveManagerService::HandleProcessMessage, file->id()), true,
      // transcoding completion callback
      NewCallback(this,
          &RPCSlaveManagerService::ProcessingCompleted, file->id(), process),
      &selector_) ) {
    delete process;
    process = NULL;
    ChangeFileState(file, FSTATE_ERROR,
                    string("cannot start 'transcode' process: ") +
                      GetLastSystemErrorDescription());
    // Don't erase files in error state.
    //DelFile(file, true);
    return;
  }
  active_processes_.insert(process);

  ChangeFileState(file, FSTATE_PROCESSING, "");
}
void RPCSlaveManagerService::HandleProcessMessage(string file_id,
                                                  const string& msg) {
  LOG_INFO << "################################### HandleProcessMessage: " << msg;
  MediaFileMap::iterator it = files_.find(file_id);
  if ( it == files_.end() ) {
    LOG_ERROR << "No such file_id: " << file_id;
    return;
  }
  MediaFile* file = it->second;

  TranscodingProgress p;
  if ( !rpc::JsonDecoder::DecodeObject(msg, &p) ) {
    LOG_ERROR << "Failed to decode transcoding progress message from: ["
              << msg << "]";
    return;
  }
  if ( p.progress_.is_set() ) {
    file->SetTranscodingProgress(p.progress_.get());
  }
  if ( p.message_.is_set() ) {
    file->AddTranscodingMsg(p.message_.get());
  }
}
void RPCSlaveManagerService::ProcessingCompleted(string file_id,
    process::Process* process, int transcode_exit_code) {
  int p_count = active_processes_.erase(process);
  CHECK_EQ(p_count, 1);
  selector_.DeleteInSelectLoop(process);
  process = NULL;

  MediaFileMap::iterator it = files_.find(file_id);
  if ( it == files_.end() ) {
    LOG_ERROR << "No such file_id: " << file_id;
    return;
  }
  MediaFile* file = it->second;
  if ( transcode_exit_code != 0 ) {
    ChangeFileState(file, FSTATE_ERROR, strutil::StringPrintf(
        "transcode failed with exit_code: %d", transcode_exit_code));
    // Don't erase files in error state.
    //EraseFile(file, true);
    return;
  }
  ChangeFileState(file, FSTATE_SLAVE_READY, "");
}

void RPCSlaveManagerService::HandleNotifyStateChangeResult(
    MasterWrapper* master, const rpc::CallResult<rpc::Void>& result) {
  if ( !result.success_ ) {
    LOG_ERROR << "NotifyStateChange failed with error: " << result.error_;
  }
  master->Unpin();
  TryEraseMaster(master);
}

void RPCSlaveManagerService::TryEraseMaster(MasterWrapper* master) {
  if ( master->Files().empty() &&
       master->PinCount() == 0 ) {
    LOG_INFO << "Deleting master: " << master->ToString();
    int result = masters_.erase(master->uri());
    CHECK_EQ(result, 1);
    delete master;
  }
}

string RPCSlaveManagerService::ToString(const char* sep) const {
  ostringstream oss;
  oss << "RPCSlaveManagerService{";
  oss << "scp_username_: " << scp_username_ << sep;
  oss << "external_ip_: " << external_ip_.ToString() << sep;
  oss << "output_dir_: " << output_dir_ << sep;
  oss << "files_: " << strutil::ToString(files_) << sep;
  oss << "masters_: #" << masters_.size() << "{";
  for ( MasterMap::const_iterator it = masters_.begin();
        it != masters_.end(); ++it ) {
    const MasterWrapper* master = it->second;
    if ( it != masters_.begin() ) {
      oss << ", ";
    }
    oss << master->ToString();
  }
  oss << "}" << sep;
  oss << "active_processes_: #" << active_processes_.size() << sep;
  oss << "}";
  return oss.str();
}

/**********************************************************************/
/*              ServiceInvokerSlaveManager methods                 */
/**********************************************************************/
void RPCSlaveManagerService::AddFile(rpc::CallContext<string>* call,
                                     const string& file_id,
                                     const string& file_path,
                                     const map<string, string>& process_params,
                                     const string& master_uri,
                                     const string& slave_id) {
  string err;
  if ( !AddFile(file_id,
                slave_id,
                master_uri,
                call->Transport().peer_address(),
                file_path,
                process_params,
                FSTATE_PENDING,
                "",
                &err) ) {
    LOG_ERROR << "Failed to AddFile: " << err;
    call->Complete("Error: " + err);
    return;
  }
  StartProcessingMoreFiles();
  call->Complete("");
}

void RPCSlaveManagerService::DelFile(rpc::CallContext< rpc::Void >* call,
                                     const string& file_id) {
  DelFile(file_id);
  call->Complete();
}

void RPCSlaveManagerService::GetFileState(rpc::CallContext< FileState >* call,
                                          const string& file_id) {
  MediaFileMap::iterator it = files_.find(file_id);
  if ( it == files_.end() ) {
    LOG_WARNING << "GetFileState(" << file_id << ") => no such file id";
    call->Complete(
        FileState(file_id,
                  EncodeFState(FSTATE_ERROR),
                  "No such file_id: " +file_id,
                  "",
                  kEmptyTranscodingStatus));
    return;
  }
  MediaFile* file = it->second;
  call->Complete(FileState(file->id(),
                           EncodeFState(file->state()),
                           file->error(),
                           FileOutputDir(*file),
                           file->transcoding_status()));
}
void RPCSlaveManagerService::ListFiles(
    rpc::CallContext< vector<FileState> >* call) {
  vector<FileState> result;
  result.reserve(files_.size());
  for ( MediaFileMap::iterator it = files_.begin(); it != files_.end(); ++it ) {
    const MediaFile* file = it->second;
    result.push_back(FileState(file->id(),
                               EncodeFState(file->state()),
                               file->error(),
                               FileOutputDir(*file),
                               file->transcoding_status()));
  }
  call->Complete(result);
}

void RPCSlaveManagerService::DebugToString(
    rpc::CallContext< string >* call) {
  call->Complete(ToString("<br>"));
}

}; // namespace slave
}; // namespace manager
