#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include "common/io/ioutil.h"
#include "common/io/file/file.h"
#include "common/io/file/file_input_stream.h"
#include "common/base/errno.h"
#include "common/base/scoped_ptr.h"
#include "net/rpc/lib/types/rpc_all_types.h"
#include "net/rpc/lib/codec/json/rpc_json_encoder.h"
#include "net/rpc/lib/codec/json/rpc_json_decoder.h"
#include "rpc_slave_manager_service.h"
#include "../common.h"
#include "auto/types.h"

namespace manager { namespace slave {

const string RPCSlaveManagerService::kStateKeyName("state");
const uint32 RPCSlaveManagerService::kTranscoderOutputMonitorInterval = 15000;
const uint32 RPCSlaveManagerService::kTranscoderStatusMonitorInterval = 5000;
const uint32 RPCSlaveManagerService::kPostprocessorOutputMonitorInterval = 15000;

RPCSlaveManagerService::RPCSlaveManagerService(
      net::Selector& selector,
      net::NetFactory& net_factory,
      const std::string& scp_username,
      const net::IpAddress& external_ip,
      const string& download_dir,
      const string& transcoder_input_dir,
      const string& transcoder_output_dir,
      const string& postprocessor_input_dir,
      const string& postprocessor_output_dir,
      const vector<string>& output_dirs,
      const string& state_dir,
      const string& state_name)
  : ServiceInvokerSlaveManager(ServiceInvokerSlaveManager::GetClassName()),
    selector_(selector),
    net_factory_(net_factory),
    scp_username_(scp_username),
    external_ip_(external_ip),
    download_dir_(download_dir),
    transcoder_input_dir_(transcoder_input_dir),
    transcoder_output_dir_(transcoder_output_dir),
    postprocessor_input_dir_(postprocessor_input_dir),
    postprocessor_output_dir_(postprocessor_output_dir),
    output_dirs_(output_dirs),
    files_(),
    masters_(),
    active_processes_(),
    sync_(),
    processor_regexp_("\\.(done|error)$", REG_EXTENDED),
    notify_new_transcoded_file_callback_(NewPermanentCallback(this,
        &RPCSlaveManagerService::NotifyNewTranscodedFile)),
    transcoder_input_monitor_(selector_,
                              transcoder_input_dir_,
                              &processor_regexp_,
                              kTranscoderOutputMonitorInterval,
                              notify_new_transcoded_file_callback_),
    notify_new_post_processed_file_callback_(NewPermanentCallback(this,
        &RPCSlaveManagerService::NotifyNewPostProcessedFile)),
    postprocessor_input_monitor_(selector_,
                                 postprocessor_input_dir_,
                                 &processor_regexp_,
                                 kPostprocessorOutputMonitorInterval,
                                 notify_new_post_processed_file_callback_),
    transcoder_status_monitor_(selector_),
    state_keeper_(state_dir.c_str(), state_name.c_str()),
    state_keep_user_(&state_keeper_, "slave_manager_service", 0) {
  transcoder_status_monitor_.Set(NewPermanentCallback(this,
      &RPCSlaveManagerService::NotifyUpdateTranscoderStatus), true,
      kTranscoderStatusMonitorInterval, true, true);
}
RPCSlaveManagerService::~RPCSlaveManagerService() {
  Stop();
  CHECK(active_processes_.empty()) << "Stop should have cleared all processes";
  for(MasterMap::iterator it = masters_.begin(); it != masters_.end(); ++it) {
    MasterWrapper * mw = it->second;
    delete mw;
  }
  masters_.clear();
  delete notify_new_transcoded_file_callback_;
  notify_new_transcoded_file_callback_ = NULL;
  delete notify_new_post_processed_file_callback_;
  notify_new_post_processed_file_callback_ = NULL;
}

bool RPCSlaveManagerService::Start() {
  #define VERIFY_DIR(dirname) \
    if(dirname.empty() || !io::IsDir(dirname.c_str())) {\
      LOG_ERROR << "Invalid " #dirname ": [" << dirname << "]";\
      return false;\
    }
  VERIFY_DIR(download_dir_);
  VERIFY_DIR(transcoder_input_dir_);
  VERIFY_DIR(transcoder_output_dir_);
  VERIFY_DIR(postprocessor_input_dir_);
  VERIFY_DIR(postprocessor_output_dir_);
  for ( uint32 i = 0; i < output_dirs_.size(); i++ ) {
    const string& output_dir = output_dirs_[i];
    VERIFY_DIR(output_dir);
  }
  #undef VERIFY_DIR

  bool success = state_keeper_.Initialize();
  if(!success) {
    LOG_ERROR << "Failed to start state_keeper_ .";
    return false;
  }
  success = Load();
  if(!success) {
    LOG_WARNING << "Load failed, assuming clean start";
  }

  CHECK(!transcoder_input_monitor_.IsRunning()) << "Duplicate start!";
  if(!transcoder_input_monitor_.Start()) {
    LOG_ERROR << "Failed to start transcoder_input_monitor_ .";
    return false;
  }
  CHECK(!postprocessor_input_monitor_.IsRunning()) << "Duplicate start!";
  if(!postprocessor_input_monitor_.Start()) {
    LOG_ERROR << "Failed to start postprocessor_input_monitor_ .";
    return false;
  }
  for(MasterMap::iterator it = masters_.begin(); it != masters_.end(); ++it) {
    MasterWrapper* mw = it->second;
    bool success = mw->Start();
    if(!success) {
      LOG_ERROR << "Failed to start MasterWrapper";
      Stop();
      return false;
    }
  }
  return true;
}
void RPCSlaveManagerService::Stop() {
  transcoder_input_monitor_.Stop();
  postprocessor_input_monitor_.Stop();
  LOG_INFO << "Killing " << active_processes_.size() << " active processes";
  for ( ProcessSet::iterator it = active_processes_.begin();
        it != active_processes_.end(); ++it ) {
    process::Process* p = *it;
    p->Kill();
    delete p;
  }
  active_processes_.clear();
  for ( MasterMap::iterator it = masters_.begin();
        it != masters_.end(); ++it ) {
    MasterWrapper* mw = it->second;
    mw->Stop();
  }
  Save();
}

string RPCSlaveManagerService::FileDownloadDir(int64 file_id) {
  return strutil::JoinPaths(download_dir_, strutil::StringOf(file_id));
}
string RPCSlaveManagerService::FileDownloadDir(const MediaFile* file) {
  return FileDownloadDir(file->id());
}
string RPCSlaveManagerService::FileTranscodeInputDir(const MediaFile* file) {
  return strutil::JoinPaths(transcoder_input_dir_,
                            strutil::StringOf(file->id()));
}
string RPCSlaveManagerService::FileTranscodeInputPath(const MediaFile* file) {
  return strutil::JoinPaths(FileTranscodeInputDir(file),
                            strutil::Basename(file->path()));
}
string RPCSlaveManagerService::FileTranscodeOutputDir(const MediaFile* file) {
  return strutil::JoinPaths(transcoder_output_dir_,
                            strutil::StringOf(file->id()));
}
string RPCSlaveManagerService::FilePostProcessInputDir(const MediaFile* file) {
  return strutil::JoinPaths(postprocessor_input_dir_,
                            strutil::StringOf(file->id()));
}
string RPCSlaveManagerService::FilePostProcessInputPath(const MediaFile* file){
  return strutil::JoinPaths(FilePostProcessInputDir(file),
                            strutil::Basename(file->path()));
}
string RPCSlaveManagerService::FilePostProcessOutputDir(const MediaFile* file){
  return strutil::JoinPaths(postprocessor_output_dir_,
                            strutil::StringOf(file->id()));
}
string RPCSlaveManagerService::FileProcessingStatusPath(const MediaFile* file) {
  // the ".status" extension is established by transcoder app.
  return file->process_cmd() == PROCESS_CMD_TRANSCODE ?
           FileTranscodeInputPath(file) + ".status" :
         file->process_cmd() == PROCESS_CMD_POST_PROCESS ?
           FilePostProcessInputPath(file) + ".status" :
         "StatusFileUnavailable";
}

int64 RPCSlaveManagerService::ReadIdFromProcessedFile(const string& file_path) {
  const char* subdir = NULL;
  if ( file_path.find(transcoder_input_dir_) == 0 ) {
    subdir = file_path.c_str() + transcoder_input_dir_.size() + 1;
  } else if ( file_path.find(postprocessor_input_dir_) == 0 ) {
    subdir = file_path.c_str() + postprocessor_input_dir_.size() + 1;
  } else {
    LOG_ERROR << "Path: [" << file_path << "] "
        "not in transcoder_input_dir_: [" << transcoder_input_dir_ << "] "
        "nor in postprocessor_input_dir_: [" << postprocessor_input_dir_ << "]";
    return -1;
  }
  int64 id = ::atoll(subdir);
  if ( id == 0 && subdir[0] != '0' ) {
    LOG_ERROR << "Path: [" << file_path << "] not a number subdir: [";
    return -1;
  }
  return id;
}

void RPCSlaveManagerService::CleanDisk(MediaFile& file, bool erase_output) {
  const string file_download_dir = FileDownloadDir(&file);
  const string file_transcode_input_dir = FileTranscodeInputDir(&file);
  const string file_transcode_output_dir = FileTranscodeOutputDir(&file);
  const string file_post_process_input_dir = FilePostProcessInputDir(&file);
  const string file_post_process_output_dir = FilePostProcessOutputDir(&file);
  io::Rm(file_download_dir);
  io::Rm(file_transcode_input_dir);
  io::Rm(file_transcode_output_dir);
  io::Rm(file_post_process_input_dir);
  io::Rm(file_post_process_output_dir);
  if ( file.path() != "") {
    LOG(INFO) << "Erase file: " << file.path();
    io::Rm(file.path());
    file.set_path("");
  }
  if ( erase_output ) {
    const vector<FileOutputDir>& output_dirs = file.output().dirs_;
    for ( uint32 i = 0; i < output_dirs.size(); i++ ) {
      const FileOutputDir& output_dir = output_dirs[i];
      const vector<string>& output_files = output_dir.files_;
      for ( uint32 i = 0; i < output_files.size(); i++ ) {
        const string& output_file = output_files[i];
        LOG(INFO) << "Erase output file: " << output_file;
        io::Rm(output_file);
      }
    }
    file.set_output(kEmptyFileSlaveOutput);
  }
}
void RPCSlaveManagerService::CleanDisk(const StateMediaFile& rpc_file,
                                       bool erase_output) {
  MediaFile tmp(rpc_file.id_.get(),
                rpc_file.slave_id_.get(),
                NULL,
                rpc_file.remote_url_.get(),
                rpc_file.path_.get(),
                "",
                DecodeProcessCmd(rpc_file.process_cmd_.get()),
                rpc_file.process_params_.get(),
                DecodeFState(rpc_file.state_.get()),
                rpc_file.transcoding_status_.get());
  tmp.set_output(rpc_file.output_.get());
  CleanDisk(tmp, erase_output);
}

MediaFile* RPCSlaveManagerService::AddFile(
    int64 id,
    const string& slave_id,
    const string& master_uri,
    const net::HostPort& master_alt_ip,
    const string& remote_url,
    const string& path,
    ProcessCmd process_cmd,
    const map<string, string>& process_params,
    FState state,
    const string& error,
    int64 ts_state_change,
    const FileSlaveOutput& output,
    const FileTranscodingStatus& fts,
    FState& out_error_state,
    string& out_error_description) {
  net::HostPort master_host_port;
  string master_http_path;
  string master_http_user;
  string master_http_pswd;
  rpc::CONNECTION_TYPE master_connection_type;
  rpc::CODEC_ID master_codec;
  bool success = rpc::ParseUri(master_uri, master_host_port, master_http_path,
                               master_http_user, master_http_pswd,
                               master_connection_type, master_codec);
  if(!success) {
    LOG_ERROR << "Failed to parse master address from: [" << master_uri << "]";
    out_error_state = FSTATE_INVALID_PARAMETER;
    out_error_description = "Cannot parse master_uri";
    return NULL;
  }
  if(master_host_port.ip_object().IsInvalid()) {
    // use alternate IP
    master_host_port.set_ip(master_alt_ip.ip_object());
  }

  MediaFileMap::iterator fit = files_.find(id);
  if(fit != files_.end()) {
    LOG_ERROR << "duplicate media file ID: " << id << ", old: " << *fit->second;
    out_error_state = FSTATE_DUPLICATE;
    out_error_description = strutil::StringPrintf(
        "A file with ID: %"PRId64" already exists.", (id));
    return NULL;
  }

  // possibly add a new MasterWrapper
  //
  MasterWrapper* master;
  MasterMap::iterator mit = masters_.find(master_uri);
  if(mit == masters_.end()) {
    master = new MasterWrapper(selector_, net_factory_, master_uri,
                               master_host_port, master_http_path,
                               master_http_user, master_http_pswd,
                               master_connection_type, master_codec);
    bool success = master->Start();
    if(!success) {
      LOG_ERROR << "Cannot start MasterWrapper: " << *master
                << ", for media file ID: " << id;
      out_error_state = FSTATE_INVALID_PARAMETER;
      out_error_description = "Cannot start MasterWrapper.";
      delete master;
      return NULL;
    }
    masters_.insert(make_pair(master_uri, master));
  } else {
    master = mit->second;
  }

  // add the new MediaFile
  //
  MediaFile* file = new MediaFile(id,
                                   slave_id,
                                   master,
                                   remote_url,
                                   path,
                                   "TO BE SET",
                                   process_cmd,
                                   process_params,
                                   state,
                                   kEmptyFileTranscodingStatus);
  if ( path != "" ) {
    // for new files, the path is "". And will be set after the file is
    //  downloaded.
    // for old files (created on Load()), the path is valid.
    file->set_transcoding_status_file(FileProcessingStatusPath(file));
  }
  file->set_error(error);
  file->set_ts_state_change(ts_state_change);
  file->set_output(output);
  file->set_transcoding_status(fts);
  files_.insert(make_pair(id, file));

  // create logical link: master -> file
  //
  master->AddFile(file);

  LOG(INFO) << "Added new MediaFile: " << *file;

  return file;
}

void RPCSlaveManagerService::EraseFile(MediaFile* file, bool erase_output) {
  LOG(INFO) << "Removing MediaFile: " << *file;

  // clean disk
  //
  CleanDisk(*file, erase_output);

  // remove master-file reference and maybe delete master
  //
  MasterWrapper* master = file->master();
  master->RemFile(file);
  TryEraseMaster(master);

  // remove "file" from files_
  //
  int result = files_.erase(file->id());
  CHECK_EQ(result, 1);
  delete file;

  // Save state
  //
  Save();
}

void RPCSlaveManagerService::TryEraseMaster(MasterWrapper* master) {
  if(master->Files().empty() &&
     master->RPCPinCount() == 0) {
    LOG_INFO << "Deleting master: " << *master;
    int result = masters_.erase(master->uri());
    CHECK_EQ(result, 1);
    delete master;
  }
}

void RPCSlaveManagerService::ChangeFileState(MediaFile* file, FState state,
                                             const string& error,
                                             FileState* out_state) {
  // we're already in sync with files_
  CHECK_NOT_NULL(file);
  file->set_state(state);
  file->set_error(error);
  FileState new_state(file->id(),
                      EncodeFState(file->state()),
                      file->error(),
                      file->output(),
                      file->transcoding_status());
  if ( out_state != NULL ) {
    // return new_state
    *out_state = new_state;
  } else {
    // send notification with new_state
    file->master()->RPCPin();
    file->master()->RPCWrapper().NotifyStateChange(
        NewCallback(this,
                    &RPCSlaveManagerService::HandleNotifyStateChangeResult,
                    file->master()),
        file->slave_id(),
        new_state);
  }
  Save();
}

void RPCSlaveManagerService::ReadTranscoderStatus(MediaFile& file) {
  // NOTE: erroneous state files are removed right away.
  //       here we expect to find only good files.
  if ( file.state() == FSTATE_TRANSFERRING ||
       file.state() == FSTATE_TRANSFERRED ||
       file.state() == FSTATE_OUTPUTTING ||
       file.state() == FSTATE_READY ) {
    // ignore transferring, trascoded file
    return;
  }

  if ( file.process_cmd() != PROCESS_CMD_TRANSCODE &&
       file.process_cmd() != PROCESS_CMD_POST_PROCESS ) {
    // ignore non-processing files
    return;
  }

  // find transcoder status file
  if ( !io::Exists(file.transcoding_status_file()) ) {
    LOG_ERROR << "No transcoding_status_file found: ["
              << file.transcoding_status_file() << "]";
    return;
  }

  // Open, read and decode transcoder status file.
  // It should contains a JSON encoded FileTranscodingStatus structure.
  string transcoding_status_str;
  if ( !io::FileInputStream::TryReadFile(
           file.transcoding_status_file().c_str(),
           &transcoding_status_str) ) {
    LOG_ERROR << "Failed to open transcoding_status_file: ["
              << file.transcoding_status_file() << "]"
        ", error: " << GetLastSystemErrorDescription();
    return;
  }
  FileTranscodingStatus transcoding_status;
  if ( !rpc::JsonDecoder::DecodeObject(transcoding_status_str,
                                       &transcoding_status) ) {
    LOG_ERROR << "Failed to read& decode FileTranscodingStatus structure"
              << " from transcoding_status_file: ["
              << file.transcoding_status_file() << "]";
    return;
  }
  file.set_transcoding_status(transcoding_status);
}

///////////////////////////////////////////////////////////////////////////////

void RPCSlaveManagerService::StartTransfer(MediaFile* file, FileState* state) {
  // create download dir
  const string file_download_dir = FileDownloadDir(file);
  if ( io::Exists(file_download_dir) ) {
    LOG_ERROR << "download directory: [" << file_download_dir << "] already"
                 " exists, cleaning up..";
    CleanDisk(*file, true);
  }
  if ( !io::Mkdir(file_download_dir, false) ) {
    LOG_ERROR << "cannot create directory: " << file_download_dir
              << ", error: " << GetLastSystemErrorDescription();
    ChangeFileState(file, FSTATE_DISK_ERROR,
                    "cannot create directory: " + file_download_dir +
                    ", error: " + GetLastSystemErrorDescription(), state);
    // Don't erase files in error state.
    //EraseFile(file, true);
    return;
  }

  // SCP src
  //  This may be a single file (e.g. "whispercast@10.16.200.3:video/file.flv")
  //  Or a multi file (e.g. "whispercast@10.16.200.3:video/f1.flv,
  //                         whispercast@10.16.200.3:video/f2.flv")
  vector<string> scp_args;
  strutil::SplitString(file->remote_url(), ",", &scp_args);

  // SCP dst
  //  This is the download dir.
  scp_args.push_back(file_download_dir);

  // start the Transfer process
  process::Process* process = new process::Process("/usr/bin/scp", scp_args);
  process->SetExitCallback(NewCallback(this,
      &RPCSlaveManagerService::TransferCompleted, file->id(), process));
  if ( !process->Start() ) {
    delete process;
    process = NULL;
    ChangeFileState(file, FSTATE_TRANSFER_ERROR,
                    string("cannot start 'scp' process: ") +
                      GetLastSystemErrorDescription(),
                    state);
    // Don't erase files in error state.
    //EraseFile(file, true);
    return;
  }
  active_processes_.insert(process);

  ChangeFileState(file, FSTATE_TRANSFERRING, "", state);
}
void RPCSlaveManagerService::TransferCompleted(int64 file_id,
                                               process::Process* p,
                                               int scp_exit_status) {
  int p_count = active_processes_.erase(p);
  CHECK_EQ(p_count, 1);
  selector_.DeleteInSelectLoop(p);
  p = NULL;

  synch::MutexLocker lock(&sync_);
  MediaFileMap::iterator it = files_.find(file_id);
  if(it == files_.end()) {
    LOG_ERROR << "No such file_id: " << file_id;
    return;
  }
  MediaFile* file = it->second;
  if(scp_exit_status != 0) {
    ChangeFileState(file, FSTATE_TRANSFER_ERROR,
                    strutil::StringPrintf("scp failed with exit_status: %d",
                                          scp_exit_status),
                    NULL);
    // Don't erase files in error state.
    //EraseFile(file, true);
    return;
  }

  const string file_download_dir = FileDownloadDir(file);

  set<string> down_files;
  if(!DirectoryMonitor::ScanNow(file_download_dir, down_files, NULL)) {
    LOG_ERROR << "Failed to scan downloaded files in dir: ["
              << file_download_dir << "]";
    ChangeFileState(file, FSTATE_DISK_ERROR,
                    strutil::StringPrintf(
                        "Failed to scan download dir: [%s], error: %s",
                        file_download_dir.c_str(),
                        GetLastSystemErrorDescription()),
                    NULL);
    // Don't erase files in error state.
    //EraseFile(file, true);
    return;
  }
  if ( down_files.size() == 1 ) {
    // single file
    file->set_path(*down_files.begin());
    file->set_transcoding_status_file(FileProcessingStatusPath(file));
  } else {
    // multiple files
    file->set_path("MULTIPLE FILES: " + strutil::ToString(down_files));
  }
  ChangeFileState(file, FSTATE_TRANSFERRED, "", NULL);
}

void RPCSlaveManagerService::StartTranscode(MediaFile* file, FileState* state) {
  if ( !io::Exists(file->path()) ) {
    LOG_ERROR << "Cannot transcode inexistent file: " << *file;
    ChangeFileState(file, FSTATE_DISK_ERROR, "Cannot transcode inexistent "
                    "file: [" + file->path() + "]", state);
    // Don't erase files in error state.
    //EraseFile(file, true);
    return;
  }
  const string trans_dir = FileTranscodeInputDir(file);
  if( !io::Mkdir(trans_dir, false) ) {
    LOG_ERROR << "cannot create directory: [" << trans_dir << "]"
                 ", error: " << GetLastSystemErrorDescription();
    ChangeFileState(file, FSTATE_DISK_ERROR,
                    "cannot create directory: [" + trans_dir + "]"
                    ", error: " + GetLastSystemErrorDescription(), state);
    // Don't erase files in error state.
    //EraseFile(file, true);
    return;
  }
  const string trans_file = FileTranscodeInputPath(file);
  if ( 0 != ::symlink(file->path().c_str(), trans_file.c_str()) ) {
    LOG_ERROR << "cannot create symlink from: [" << file->path() << "]"
                 ", to: [" << trans_file << "]"
                 ", error: " << GetLastSystemErrorDescription();
    ChangeFileState(file, FSTATE_DISK_ERROR,
                    "cannot create symlink from: [" + file->path() + "]"
                    ", to: [" + trans_file + "]"
                    ", error: " + GetLastSystemErrorDescription(), state);
    // Don't erase files in error state.
    //EraseFile(file, true);
    return;
  }

  ChangeFileState(file, FSTATE_TRANSCODING, "", state);
}
void RPCSlaveManagerService::TranscodeCompleted(int64 file_id,
                                                const string& error) {
  synch::MutexLocker lock(&sync_);
  MediaFileMap::iterator it = files_.find(file_id);
  if(it == files_.end()) {
    LOG_ERROR << "No such file_id: " << file_id;
    return;
  }
  MediaFile* file = it->second;

  if(error != "") {
    LOG_ERROR << "Failed to transcode file: [" << file->path() << "]"
                 ", error: [" << error << "]";
    ChangeFileState(file, FSTATE_TRANSCODE_ERROR, error, NULL);
    // Don't erase files in error state.
    //EraseFile(file, true);
    return;
  }
  LOG_INFO << "Successfully transcoded file: " << file_id
           << " [" << file->path() << "]";

  // read transcoding status once more. This is the final transcoding status.
  ReadTranscoderStatus(*file);

  // erase original file from "download_dir"
  //
  //io::Rm(FileDownloadDir(file));

  // erase transcoder input dir (contains a symlink to the original file)
  //
  const string file_process_input_dir = FileTranscodeInputDir(file);
  if(!io::Rm(file_process_input_dir)) {
    LOG_ERROR << "Failed to erase process input file dir: "
                 "[" << file_process_input_dir << "]"
                 ", error: " << GetLastSystemErrorDescription();
  }

  // copy transcoded files (the results) from "transcoder_output_dir"
  // to each output directory.
  //
  StartOutput(file, NULL);
}

void RPCSlaveManagerService::StartPostProcess(MediaFile* file,
                                              FileState* state) {
  const string post_proc_dir = FilePostProcessInputDir(file);
  if( !io::Mkdir(post_proc_dir, false) ) {
    LOG_ERROR << "cannot create directory: [" << post_proc_dir << "]"
                 ", error: " << GetLastSystemErrorDescription();
    ChangeFileState(file, FSTATE_DISK_ERROR,
                    "cannot create directory: [" + post_proc_dir + "]"
                    ", error: " + GetLastSystemErrorDescription(), state);
    // Don't erase files in error state.
    //EraseFile(file, true);
    return;
  }
  const string post_proc_file = FilePostProcessInputPath(file);
  if ( 0 != ::symlink(file->path().c_str(), post_proc_file.c_str()) ) {
    LOG_ERROR << "cannot create symlink from: [" << file->path() << "]"
                 ", to: [" << post_proc_file << "]"
                 ", error: " << GetLastSystemErrorDescription();
    ChangeFileState(file, FSTATE_DISK_ERROR,
                    "cannot create symlink from: [" + file->path() + "]"
                    ", to: [" + post_proc_file + "]"
                    ", error: " + GetLastSystemErrorDescription(), state);
    // Don't erase files in error state.
    //EraseFile(file, true);
    return;
  }

  ChangeFileState(file, FSTATE_POST_PROCESSING, "", state);
}
void RPCSlaveManagerService::PostProcessCompleted(int64 file_id,
                                                  const string& error) {
  synch::MutexLocker lock(&sync_);
  MediaFileMap::iterator it = files_.find(file_id);
  if(it == files_.end()) {
    LOG_ERROR << "No such file_id: " << file_id;
    return;
  }
  MediaFile* file = it->second;

  if(error != "") {
    LOG_ERROR << "Failed to post process file: [" << file->path() << "]"
                 ", error: [" << error << "]";
    ChangeFileState(file, FSTATE_POST_PROCESS_ERROR, error, NULL);
    // Don't erase files in error state.
    //EraseFile(file, true);
    return;
  }
  LOG_INFO << "Successfully post processed file: " << file_id;

  // read post processing status once more. This is the final status.
  ReadTranscoderStatus(*file);

  // erase original file from "download_dir"
  //
  //io::Rm(FileDownloadDir(file));

  // erase post processor input dir (contains a symlink to the original file)
  //
  const string file_post_process_input_dir = FilePostProcessInputDir(file);
  if ( !io::Rm(file_post_process_input_dir) ) {
    LOG_ERROR << "Failed to erase process input file dir: "
                 "[" << file_post_process_input_dir << "]";
  }

  // start the output distribution process (copy result files to each
  // output directory)
  //
  StartOutput(file, NULL);
}

void RPCSlaveManagerService::StartOutput(MediaFile* file, FileState* state) {
  const string src_dir = file->process_cmd() == PROCESS_CMD_TRANSCODE ?
                           FileTranscodeOutputDir(file) :
                         file->process_cmd() == PROCESS_CMD_POST_PROCESS ?
                           FilePostProcessOutputDir(file) :
                         file->process_cmd() == PROCESS_CMD_COPY ?
                           FileDownloadDir(file) : "";
  CHECK(src_dir != "") << "Illegal process_cmd: " << file->process_cmd();

  // create the list of 'cp' commands
  vector<string> cps;
  for ( uint32 i = 0; i < output_dirs_.size(); i++ ) {
    cps.push_back("cp " + src_dir + "/* " + output_dirs_[i]);
  }
  // We make a single bash command, using the '&&'.
  const string cmd = strutil::JoinStrings(cps, " && ");

  // start the copy process.
  process::Process* proc = new process::Process("/bin/bash", "-c",
                                                cmd.c_str(), NULL);
  proc->SetExitCallback(NewCallback(this,
      &RPCSlaveManagerService::OutputCompleted, file->id(), proc));
  if ( !proc->Start() ) {
    delete proc;
    const string err = string("cannot start '/bin/bash' process: ") +
                              GetLastSystemErrorDescription();
    LOG_ERROR << err;
    ChangeFileState(file, FSTATE_DISK_ERROR, err, state);
    // Don't erase files in error state.
    //EraseFile(file, true);
    return;
  }

  active_processes_.insert(proc);

  ChangeFileState(file, FSTATE_OUTPUTTING, "", state);
}
void RPCSlaveManagerService::OutputCompleted(int64 file_id,
                                             process::Process* p,
                                             int cp_exit_status) {
  int p_count = active_processes_.erase(p);
  CHECK_EQ(p_count, 1);
  selector_.DeleteInSelectLoop(p);
  p = NULL;

  synch::MutexLocker lock(&sync_);
  MediaFileMap::iterator it = files_.find(file_id);
  if(it == files_.end()) {
    LOG_ERROR << "No such file_id: " << file_id;
    return;
  }
  MediaFile* file = it->second;

  if ( cp_exit_status != 0 ) {
    LOG_ERROR << "cp failed, exit code: " << cp_exit_status
              << ", for file: " << *file;
    ChangeFileState(file, FSTATE_DISK_ERROR,
                    strutil::StringPrintf("cp failed, exit code: %d",
                                          cp_exit_status),
                    NULL);
    // Don't erase files in error state.
    //EraseFile(file, true);
    return;
  }

  //
  // done distributing results to all output directories
  //

  // scan the source directory
  //
  const string src_dir = file->process_cmd() == PROCESS_CMD_TRANSCODE ?
                           FileTranscodeOutputDir(file) :
                         file->process_cmd() == PROCESS_CMD_POST_PROCESS ?
                           FilePostProcessOutputDir(file) :
                         file->process_cmd() == PROCESS_CMD_COPY ?
                           FileDownloadDir(file) : "";
  CHECK(src_dir != "") << "Illegal process_cmd: " << file->process_cmd()
                       << ", on file: " << *file;
  set<string> src_set;
  if(!DirectoryMonitor::ScanNow(src_dir, src_set, NULL)) {
    LOG_ERROR << "Failed to scan result files in dir: [" << src_dir << "]";
    ChangeFileState(file, FSTATE_DISK_ERROR, strutil::StringPrintf(
        "Failed to scan result files in dir: [%s]", src_dir.c_str()), NULL);
    // Don't erase files in error state.
    //EraseFile(file, true);
    return;
  }

  // create the list of output files
  //
  FileSlaveOutput output;
  output.scp_username_ = scp_username_;
  output.ip_ = external_ip_.ToString();
  output.dirs_.ref().resize(output_dirs_.size());
  for ( uint32 i = 0; i < output_dirs_.size(); i++ ) {
    const string& output_dir = output_dirs_[i];
    vector<string> output_files;
    for ( set<string>::const_iterator it = src_set.begin();
          it != src_set.end(); ++it ) {
      const string& src_file = *it;
      output_files.push_back(strutil::JoinPaths(output_dir,
                                                strutil::Basename(src_file)));
    }
    output.dirs_.ref()[i] = FileOutputDir(output_dir,
                                          output_files);
  }

  // set output
  //
  file->set_output(output);
  ChangeFileState(file, FSTATE_READY, "", NULL);

  // do something with distribution source directory
  //
  io::Rm(src_dir);
}

///////////////////////////////////////////////////////////////////////////////

void RPCSlaveManagerService::NotifyNewTranscodedFile(const string& file_path) {
  int64 file_id = ReadIdFromProcessedFile(file_path);
  if(file_id == -1) {
    LOG_ERROR << "Cannot ReadIdFromProcessedFilename: [" << file_path << "]";
    return;
  }
  const string k_error_suffix(".error");
  const string k_done_suffix(".done");
  if(strutil::StrEndsWith(file_path, k_error_suffix)) {
    TranscodeCompleted(file_id, "transcode failed");
  } else if(strutil::StrEndsWith(file_path, k_done_suffix)) {
    TranscodeCompleted(file_id, "");
  } else {
    LOG_ERROR << "Ignoring unrecognized processor flag file:"
                 " [" << file_path << "]";
  }
}

void RPCSlaveManagerService::NotifyNewPostProcessedFile(
    const string& file_path) {
  int64 file_id = ReadIdFromProcessedFile(file_path);
  if(file_id == -1) {
    LOG_ERROR << "Cannot ReadIdFromProcessedFilename: [" << file_path << "]";
    return;
  }
  const string k_error_suffix(".error");
  const string k_done_suffix(".done");
  if(strutil::StrEndsWith(file_path, k_error_suffix)) {
    PostProcessCompleted(file_id, "post process failed");
  } else if(strutil::StrEndsWith(file_path, k_done_suffix)) {
    PostProcessCompleted(file_id, "");
  } else {
    LOG_ERROR << "Ignoring unrecognized post processor flag file:"
                 " [" << file_path << "]";
  }
}

void RPCSlaveManagerService::NotifyUpdateTranscoderStatus() {
  for ( MediaFileMap::iterator it = files_.begin(); it != files_.end(); ++it ) {
    MediaFile& file = *it->second;
    ReadTranscoderStatus(file);
  }
}

void RPCSlaveManagerService::HandleNotifyStateChangeResult(
    MasterWrapper* master, const rpc::CallResult<rpc::Void>& result) {
  synch::MutexLocker lock(&sync_);
  if(!result.success_) {
    LOG_ERROR << "NotifyStateChange failed with error: " << result.error_;
  }
  master->RPCUnpin();
  TryEraseMaster(master);
}

void RPCSlaveManagerService::Save() {
  // create an array of all media files
  vector< StateMediaFile > rpc_files(files_.size());
  uint32 i = 0;
  for(MediaFileMap::const_iterator it = files_.begin();
      it != files_.end(); ++it, ++i) {
    const MediaFile& file = *it->second;
    rpc_files[i] = StateMediaFile(file.id(),
                                  file.slave_id(),
                                  file.master()->uri(),
                                  file.remote_url(),
                                  file.path(),
                                  EncodeProcessCmd(file.process_cmd()),
                                  file.process_params(),
                                  EncodeFState(file.state()),
                                  file.error(),
                                  file.ts_state_change(),
                                  file.output(),
                                  file.transcoding_status());
  }

  io::MemoryStream ms;
  rpc::JsonEncoder encoder(ms);
  encoder.Encode(rpc_files);

  string str_encoded_files;
  ms.ReadString(&str_encoded_files);

  state_keep_user_.SetValue(kStateKeyName, str_encoded_files);
  LOG_INFO << "Save: finished. #" << files_.size() << " files saved.";
}
bool RPCSlaveManagerService::Load() {
  string str_encoded_files;
  bool success = state_keep_user_.GetValue(kStateKeyName, &str_encoded_files);
  if(!success) {
    LOG_ERROR << "Load: key not found: " << kStateKeyName;
    return false;
  }
  io::MemoryStream ms;
  ms.Write(str_encoded_files);

  rpc::JsonDecoder decoder(ms);
  vector< StateMediaFile > rpc_files;
  const rpc::DECODE_RESULT result = decoder.Decode(rpc_files);
  if(result != rpc::DECODE_RESULT_SUCCESS) {
    LOG_ERROR << "Load: failed to decode str_encoded_files:"
                 " [" << str_encoded_files << "]";
    return false;
  }
  LOG_INFO << "Load: load state succeeded, #" << rpc_files.size()
           << " files found, checking disk";

  // look at every file, check physical existence and add to files_
  //
  for(uint32 i = 0, size = rpc_files.size(); i < size; ++i) {
    const StateMediaFile& rpc_file = rpc_files[i];
    // don't check file existence. Just assume the best.

    FState file_state = DecodeFState(rpc_file.state_.get());
    if ( file_state == FSTATE_BAD_SLAVE_STATE ) {
      LOG_WARNING << "Load: unable to decode file state: "
                     "[" << rpc_file.state_.get() << "]"
                     ", ignoring file: " << rpc_file;
      CleanDisk(rpc_file, true);
      continue;
    }

    FState err_state;
    string err_description;
    MediaFile* file = AddFile(rpc_file.id_.get(),
                              rpc_file.slave_id_.get(),
                              rpc_file.master_uri_.get(),
                              net::HostPort(),
                              rpc_file.remote_url_.get(),
                              rpc_file.path_.get(),
                              DecodeProcessCmd(
                                  rpc_file.process_cmd_.get()),
                              rpc_file.process_params_.get(),
                              file_state,
                              rpc_file.error_.get(),
                              rpc_file.ts_state_change_.get(),
                              rpc_file.output_.get(),
                              rpc_file.transcoding_status_.get(),
                              err_state,
                              err_description);
    if ( !file ) {
      LOG_ERROR << "Load: AddFile failed: " << err_description
                << ", ignoring file: " << rpc_file;
      CleanDisk(rpc_file, true);
      continue;
    }
    // Success, resume work on file
    if ( file_state == FSTATE_TRANSFERRING ) {
      LOG_DEBUG << "Load: resuming TRANSFERRING file: " << *file;
      StartTransfer(file, NULL);
    } else if ( file_state == FSTATE_OUTPUTTING ) {
      LOG_DEBUG << "Load: resuming OUTPUTTING file: " << *file;
      StartOutput(file, NULL);
    } else {
      LOG_DEBUG << "Load: nothing to resume for file: " << *file;
    }
  }
  LOG_INFO << "Load: finished. #" << files_.size() << " files added.";
  return true;
}

string RPCSlaveManagerService::ToString(const string& sep) const {
  ostringstream oss;
  oss << "RPCSlaveManagerService{";
  oss << "scp_username_: " << scp_username_ << sep;
  oss << "external_ip_: " << external_ip_.ToString() << sep;
  oss << "download_dir_: " << download_dir_ << sep;
  oss << "transcoder_input_dir_: " << transcoder_input_dir_ << sep;
  oss << "transcoder_output_dir_: " << transcoder_output_dir_ << sep;
  oss << "postprocessor_input_dir_: " << postprocessor_input_dir_ << sep;
  oss << "postprocessor_output_dir_: " << postprocessor_output_dir_ << sep;
  oss << "output_dirs_: " << strutil::ToString(output_dirs_) << sep;
  oss << "files_: #" << files_.size() << "{";
  for ( MediaFileMap::const_iterator it = files_.begin();
        it != files_.end(); ++it ) {
    const MediaFile* file = it->second;
    if ( it != files_.begin() ) {
      oss << ", ";
    }
    oss << file->ToString();
  }
  oss << "}" << sep;
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
  oss << "processor_regexp_: " << processor_regexp_.regex() << sep;
  oss << "transcoder_input_monitor_: "
      << transcoder_input_monitor_.ToString().c_str() << sep;
  oss << "postprocessor_input_monitor_: "
      << postprocessor_input_monitor_.ToString().c_str() << "}";
  return oss.str();
}

/**********************************************************************/
/*              ServiceInvokerSlaveManager methods                 */
/**********************************************************************/
void RPCSlaveManagerService::Copy(
    rpc::CallContext< FileState >* call,
    const string& rpc_file_url,
    int64 rpc_file_id,
    const string& rpc_file_process_cmd,
    const map<string, string>& rpc_file_process_params,
    const string& rpc_master_uri,
    const string& rpc_slave_id) {
  const string& file_url = rpc_file_url;
  const ProcessCmd file_process_cmd = DecodeProcessCmd(rpc_file_process_cmd);
  const int64 file_id = rpc_file_id;
  const string& master_uri = rpc_master_uri;
  const string& slave_id = rpc_slave_id;

  if ( file_process_cmd == static_cast<ProcessCmd>(-1) ) {
    LOG_ERROR << "Invalid ProcessCmd value: " << rpc_file_process_cmd;
    call->Complete(FileState(file_id,
                             EncodeFState(FSTATE_INVALID_PARAMETER),
                             "invalid ProcessCmd value: " +
                             rpc_file_process_cmd,
                             kEmptyFileSlaveOutput,
                             kEmptyFileTranscodingStatus));
    return;
  }

  synch::MutexLocker lock(&sync_);

  FState err_state;
  string err_description;
  MediaFile* file = AddFile(file_id,
                            slave_id,
                            master_uri,
                            call->Transport().peer_address(),
                            file_url,
                            "",
                            file_process_cmd,
                            rpc_file_process_params,
                            FSTATE_IDLE,
                            "",
                            timer::Date::Now(),
                            kEmptyFileSlaveOutput,
                            kEmptyFileTranscodingStatus,
                            err_state,
                            err_description);
  if(file == NULL) {
    LOG_ERROR << "Failed to AddFile: " << err_description;
    call->Complete(FileState(file_id,
                             EncodeFState(err_state),
                             err_description,
                             kEmptyFileSlaveOutput,
                             kEmptyFileTranscodingStatus));
    return;
  }

  FileState result;
  StartTransfer(file, &result);
  call->Complete(result);
}
void RPCSlaveManagerService::Process(rpc::CallContext< FileState >* call,
                                     int64 file_id) {
  synch::MutexLocker lock(&sync_);

  MediaFileMap::iterator it = files_.find(file_id);
  if(it == files_.end()) {
    LOG_ERROR << "No such file_id: " << file_id;
    call->Complete(
        FileState(file_id,
                  EncodeFState(FSTATE_NOT_FOUND),
                  strutil::StringPrintf("No such file_id: %"PRId64"",
                                        (file_id)),
                  kEmptyFileSlaveOutput,
                  kEmptyFileTranscodingStatus));
    return;
  }
  MediaFile* file = it->second;

  if(file->state() != FSTATE_TRANSFERRED) {
    LOG_ERROR << "Cannot start processing busy file: " << *file;
    call->Complete(FileState(file->id(),
                             EncodeFState(FSTATE_TRANSCODE_ERROR),
                             strutil::StringPrintf(
                                 "File_id: %"PRId64" has state: %s",
                                 (file->id()),
                                 FStateName2(file->state()).c_str()),
                             file->output(),
                             file->transcoding_status()));
    return;
  }

  FileState file_new_state;
  if ( file->process_cmd() == PROCESS_CMD_TRANSCODE ) {
    StartTranscode(file, &file_new_state);
  } else if ( file->process_cmd() == PROCESS_CMD_POST_PROCESS ) {
    StartPostProcess(file, &file_new_state);
  } else if ( file->process_cmd() == PROCESS_CMD_COPY ) {
    StartOutput(file, &file_new_state);
  } else {
    LOG_ERROR << "Unknown process_cmd: " << file->process_cmd();
    call->Complete(FileState(file->id(),
                             EncodeFState(FSTATE_INVALID_PARAMETER),
                             strutil::StringPrintf("Unknown process_cmd: %d",
                                                   file->process_cmd()),
                             kEmptyFileSlaveOutput,
                             kEmptyFileTranscodingStatus));
    return;
  }
  call->Complete(file_new_state);
}

void RPCSlaveManagerService::GetFileState(rpc::CallContext< FileState >* call,
                                          int64 file_id) {
  synch::MutexLocker lock(&sync_);
  MediaFileMap::iterator it = files_.find(file_id);
  if(it == files_.end()) {
    LOG_WARNING << "GetFileState(" << file_id << ") => no such file id";
    call->Complete(
        FileState(file_id,
                  EncodeFState(FSTATE_NOT_FOUND),
                  strutil::StringPrintf("No such file_id: %"PRId64"",
                                        (file_id)),
                  kEmptyFileSlaveOutput,
                  kEmptyFileTranscodingStatus));
    return;
  }
  MediaFile* file = it->second;
  LOG_INFO << "GetFileState(" << file_id << ") => " << *file;
  call->Complete(FileState(file->id(),
                           EncodeFState(file->state()),
                           file->error(),
                           file->output(),
                           file->transcoding_status()));
}

void RPCSlaveManagerService::Delete(rpc::CallContext< rpc::Void >* call,
                                    int64 file_id,
                                    bool erase_output) {
  synch::MutexLocker lock(&sync_);
  MediaFileMap::iterator it = files_.find(file_id);
  if(it == files_.end()) {
    LOG_WARNING << "Delete(" << file_id << ") => no such file id";
    call->Complete();
    return;
  }
  MediaFile* file = it->second;
  LOG_INFO << "Delete(" << file_id << boolalpha
           << ", erase_output: " << erase_output << ") => Clearing " << *file;
  EraseFile(file, erase_output);
  file = NULL;
  call->Complete();
}
void RPCSlaveManagerService::DebugToString(
    rpc::CallContext< string >* call) {
  call->Complete(ToString("<br>"));
}

}; // namespace slave
}; // namespace manager
