#include <whisperlib/common/io/ioutil.h>
#include <whisperlib/net/rpc/lib/types/rpc_all_types.h>
#include <whisperlib/net/rpc/lib/codec/json/rpc_json_encoder.h>
#include <whisperlib/net/rpc/lib/codec/json/rpc_json_decoder.h>
#include <whisperlib/net/rpc/lib/rpc_util.h>
#include "slave_controller.h"
#include "../common.h"

namespace manager { namespace master {

const string SlaveController::kStateFiles("slave_controller_files");

SlaveController::SlaveController(net::Selector& selector,
                                 net::NetFactory& net_factory,
                                 RPCMasterManagerService& master,
                                 const net::HostPort& host_port,
                                 const string& rpc_http_path,
                                 const string& rpc_http_auth_user,
                                 const string& rpc_http_auth_pswd,
                                 rpc::CONNECTION_TYPE rpc_connection_type,
                                 rpc::CodecId rpc_codec_id,
                                 uint32 max_paralel_processing_file_count,
                                 io::StateKeeper& state_keeper)
  : selector_(selector),
    net_factory_(net_factory),
    master_(master),
    name_(host_port.ToString()),
    pending_files_(),
    files_(),
    max_paralel_processing_file_count_(max_paralel_processing_file_count),
    host_port_(host_port),
    rpc_http_path_(rpc_http_path),
    rpc_http_auth_user_(rpc_http_auth_user),
    rpc_http_auth_pswd_(rpc_http_auth_pswd),
    rpc_connection_type_(rpc_connection_type),
    rpc_codec_id_(rpc_codec_id),
    rpc_connection_(NULL),
    rpc_client_wrapper_(NULL),
    maybe_grab_media_file_alarm_(selector),
    state_keep_user_(&state_keeper,
                     rpc::CreateUri(host_port,
                                    rpc_http_path,
                                    "",
                                    "",
                                    rpc_connection_type,
                                    rpc_codec_id),
                     0) {
  maybe_grab_media_file_alarm_.Set(NewPermanentCallback(this,
      &SlaveController::MaybeGrabMediaFile), true, 3000, true, false);
}
SlaveController::~SlaveController() {
  CHECK_NULL(rpc_connection_);
  CHECK_NULL(rpc_client_wrapper_);
  CHECK(files_.empty());
  CHECK(pending_files_.empty());
}
const string& SlaveController::name() const {
  return name_;
}
const net::HostPort& SlaveController::host_port() const {
  return host_port_;
}
bool SlaveController::Start() {
  CHECK_NULL(rpc_connection_) << "duplicate Start()";
  CHECK_NULL(rpc_client_wrapper_) << "duplicate Start()";
  if(rpc_connection_type_ == rpc::CONNECTION_TCP) {
    rpc_connection_ = new rpc::ClientConnectionTCP(selector_, net_factory_,
        net::PROTOCOL_TCP, host_port_, rpc_codec_id_);
  } else if(rpc_connection_type_ == rpc::CONNECTION_HTTP) {
    rpc::ClientConnectionHTTP* rpc_http_connection =
      new rpc::ClientConnectionHTTP(selector_, net_factory_, net::PROTOCOL_TCP,
          http::ClientParams(), host_port_, rpc_codec_id_, rpc_http_path_);
    rpc_http_connection->SetHttpAuthentication(rpc_http_auth_user_,
                                               rpc_http_auth_pswd_);
    rpc_connection_ = rpc_http_connection;
  } else {
    LOG_FATAL << "No such rpc_connection_type_: " << rpc_connection_type_;
  }
  rpc_client_wrapper_ = new ServiceWrapperSlaveManager(*rpc_connection_, "SlaveManager");
  maybe_grab_media_file_alarm_.Start();
  return true;
}
void SlaveController::Stop() {
  // The reverse of Start()
  maybe_grab_media_file_alarm_.Stop();
  delete rpc_client_wrapper_;
  rpc_client_wrapper_ = NULL;
  delete rpc_connection_;
  rpc_connection_ = NULL;

  Save();

  for ( CMediaFileMap::iterator it = files_.begin();
        it != files_.end(); ++it ) {
    CMediaFile* cfile = it->second;
    delete cfile->file_;
    delete cfile;
  }
  files_.clear();
  for ( PendingFileMap::iterator it = pending_files_.begin();
        it != pending_files_.end(); ++it ) {
    MediaFile* file = it->second;
    delete file;
  }
  pending_files_.clear();
}

const MediaFile* SlaveController::GetMediaFile(const string& file_id) const {
  PendingFileMap::const_iterator pit = pending_files_.find(file_id);
  if ( pit != pending_files_.end() ) {
    const MediaFile* file = pit->second;
    return file;
  }
  CMediaFileMap::const_iterator wit = files_.find(file_id);
  if ( wit != files_.end() ) {
    const CMediaFile* cfile = wit->second;
    const MediaFile* file = cfile->file_;
    return file;
  }
  return NULL;
}
void SlaveController::GetMediaFiles(vector<const MediaFile*>* out) const {
  out->reserve(out->size() + pending_files_.size() + files_.size());
  for ( PendingFileMap::const_iterator it = pending_files_.begin();
        it != pending_files_.end(); ++it ) {
    const MediaFile* file = it->second;
    out->push_back(file);
  }
  for ( CMediaFileMap::const_iterator it = files_.begin();
        it != files_.end(); ++it ) {
    const CMediaFile* cfile = it->second;
    const MediaFile* file = cfile->file_;
    out->push_back(file);
  }
}
void SlaveController::QueueMediaFile(MediaFile* file) {
  CMediaFileMap::iterator it = files_.find(file->id());
  CHECK(it == files_.end()) << "duplicate file id"
                               ", working file: " << *it->second->file_
                            << ", pending: " << *file;

  pair<PendingFileMap::iterator, bool> result =
    pending_files_.insert(make_pair(file->id(), file));
  CHECK(result.second) << "QueueMediaFile duplicate file"
                          ", old: " << *result.first->second
                       << ", new: " << *file;
}

void SlaveController::NotifyMediaFileStateChange(
    const FileState& rpc_file_state) {
  LOG_DEBUG << "NotifyMediaFileStateChange rpc_file_state: " << rpc_file_state;
  const string& file_id = rpc_file_state.id_.get();
  const FState state = DecodeFState(rpc_file_state.state_.get());
  const string& error = rpc_file_state.error_.get();
  const TranscodingStatus& transcoding_status =
      rpc_file_state.transcoding_status_.get();

  // look only in processing files
  CMediaFileMap::iterator it = files_.find(file_id);
  if(it == files_.end()) {
    LOG_ERROR << "No such working file_id: " << file_id;
    return;
  }
  CMediaFile& cfile = *it->second;
  cfile.file_->set_transcoding_status(transcoding_status);
  if ( cfile.file_->state() == state &&
       cfile.file_->error() == error ) {
    LOG_DEBUG << "No significant changes to MediaFile, skip ChangeFileState";
    return;
  }
  ChangeFileState(cfile, state, error);
}
void SlaveController::NotifyMediaFileError(const string& file_id,
                                           const string & error) {
  LOG_DEBUG << "NotifyMediaFileError file_id: " << file_id
            << " error: [" << error << "]";
  // look only in processing files
  CMediaFileMap::iterator it = files_.find(file_id);
  if(it == files_.end()) {
    LOG_ERROR << "No such working file_id: " << file_id;
    return;
  }
  CMediaFile& cfile = *it->second;
  ChangeFileState(cfile, cfile.file_->state(), error);
}
void SlaveController::NotifyMediaFileError(const string& file_id,
                                           FState state,
                                           const string& error) {
  LOG_DEBUG << "NotifyMediaFileError file_id: " << file_id
            << " state: " << state << "(" << FStateName(state) << ")"
               " error: [" << error << "]";
  // look only in processing files
  CMediaFileMap::iterator it = files_.find(file_id);
  if(it == files_.end()) {
    LOG_ERROR << "No such working file_id: " << file_id;
    return;
  }
  CMediaFile& cfile = *it->second;
  ChangeFileState(cfile, state, error);
}

MediaFile* SlaveController::DeleteMediaFile(const string& file_id) {
  LOG_DEBUG << "DeleteMediaFile file_id: " << file_id;
  PendingFileMap::iterator pit = pending_files_.find(file_id);
  if ( pit != pending_files_.end() ) {
    MediaFile* file = pit->second;
    pending_files_.erase(pit);
    return file;
  }
  CMediaFileMap::iterator wit = files_.find(file_id);
  if ( wit == files_.end() ) {
    LOG_ERROR << "No such file_id: " << file_id;
    return NULL;
  }
  CMediaFile* cfile = wit->second;
  MediaFile* file = cfile->file_;
  RemFile(cfile);
  cfile = NULL;
  rpc_client_wrapper_->DelFile(
      NewCallback(this, &SlaveController::HandleDelFileResult, file->id()),
      file->id());
  return file;
}

SlaveController::CMediaFile* SlaveController::AddFile(MediaFile* file) {
  CMediaFileMap::iterator prev = files_.find(file->id());
  if(prev != files_.end()) {
    LOG_ERROR << "Duplicate MediaFile. Old: " << *prev->second->file_ << " New: " << *file;
    return NULL;
  }

  CMediaFile* cfile = new CMediaFile(selector_, file);
  cfile->alarm_.Set(NewPermanentCallback(this,
      &SlaveController::ProcessFile, file->id()), true,
      DelayProcessFile(file->state()), true, true);
  files_.insert(make_pair(file->id(), cfile));

  LOG(INFO) << "Added new MediaFile: " << *file;
  Save();
  return cfile;
}
void SlaveController::RemFile(CMediaFile* file) {
  LOG(INFO) << "Removing MediaFile: " << *file->file_;
  int result = files_.erase(file->file_->id());
  CHECK_EQ(result, 1);
  file->alarm_.Clear();
  // do NOT directly delete, because we are called from Alarm notify callback!
  selector_.DeleteInSelectLoop(file);
  file = NULL;
  Save();
}

//static
int64 SlaveController::DelayProcessFile(FState state) {
  switch(state) {
    case FSTATE_PENDING:           return 5000;
    case FSTATE_PROCESSING:        return 5000;
    case FSTATE_SLAVE_READY:       return 5000;
    case FSTATE_DISTRIBUTING:      return 5000;
    case FSTATE_DONE:              return 60000;
    case FSTATE_ERROR:             return 60000;
    default:
      LOG_FATAL << "Invalid file state: " << state;
      return 0;
  }
}
void SlaveController::ChangeFileState(CMediaFile& cfile,
                                      FState state,
                                      const string& error) {
  cfile.file_->set_state(state, error);
  cfile.file_->set_ts_state_change(timer::Date::Now());
  LOG(INFO) << "New file state: " << *cfile.file_;
  ScheduleProcessFile(cfile);
  Save();
}
void SlaveController::ScheduleProcessFile(CMediaFile& cfile) {
  cfile.alarm_.ResetTimeout(DelayProcessFile(cfile.file_->state()));
}
void SlaveController::ProcessFile(string file_id) {
  CMediaFileMap::iterator it = files_.find(file_id);
  if(it == files_.end()) {
    LOG_FATAL << "No such file_id: " << file_id;
    return;
  }
  CMediaFile* cfile = it->second;
  MediaFile* file = cfile->file_;

  // ProcessFile should be called ONLY with connected RPC
  CHECK_NOT_NULL(rpc_connection_);
  CHECK_NOT_NULL(rpc_client_wrapper_);

  switch(file->state()) {
    case FSTATE_PENDING:
      {
        rpc_client_wrapper_->AddFile(
            NewCallback(this, &SlaveController::HandleAddFileResult, file->id()),
            IsScpPath(file->path()) ?
                file->path() :
                MakeScpPath(master_.scp_username(),
                            master_.external_ip(), file->path()),
            file->id(),
            file->process_params(),
            master_.master_uri(),
            name_);
        // RPC call pending. File state will be changed on RPC reponse.
        return;
      }
    case FSTATE_PROCESSING:
      // transcode in progress, keep checking file state
      return;
    case FSTATE_SLAVE_READY:
      // transcode done. The slave finished it's job; time to distribute
      {
        LOG_FATAL << "TODO(cosmin): implement file distribution";
        return;
      }
    case FSTATE_DISTRIBUTING:
      // result is duplicated between output dirs
      return;
    case FSTATE_DONE:        // file done
      return;
    case FSTATE_ERROR:       // something failed: scp, transcoding, ...
      // Do nothing, the file would stay in this state forever.
      // Or until the user calls CompleteMediaFile or RetryMediaFile
      // through the MasterControl service.
      return;
  }
  LOG_FATAL << "Invalid file state: " << *file;
  return;
}

void SlaveController::MaybeGrabMediaFile() {
  // count processing files
  uint32 processing_file_count = 0;
  for ( CMediaFileMap::iterator it = files_.begin(); it != files_.end(); ++it ) {
    CMediaFile& cfile = *it->second;
    MediaFile& file = *cfile.file_;
    if ( file.state() == FSTATE_PROCESSING ) {
      // count only working files. Exclude files in final state (good or error).
      processing_file_count++;
    }
  }
  if(processing_file_count >= max_paralel_processing_file_count_) {
    LOG_INFO << "MaybeGrabMediaFile => too many processing files: " << processing_file_count;
    return;
  }

  MediaFile* file = NULL;
  if ( !pending_files_.empty() ) {
    // grab a local pending file
    PendingFileMap::iterator it = pending_files_.begin();
    file = it->second;
    pending_files_.erase(it);
  } else {
    // grab a new file from master manager
    file = master_.GrabMediaFile(this);
  }
  if ( file == NULL ) {
    LOG_DEBUG << "MaybeGrabMediaFile => NULL";
    return;
  }
  file->set_slave(SlaveURI());
  LOG_INFO << "MaybeGrabMediaFile => " << *file;

  // add the new file
  CMediaFile* cfile = AddFile(file);
  CHECK_NOT_NULL(cfile) << "Failed to AddFile: " << *file;
}

void SlaveController::HandleAddFileResult(
    string file_id, const rpc::CallResult<string>& result) {
  if ( !result.success_ ) {
    LOG_ERROR << "Failed RPC AddFile: " << result.error_;
    NotifyMediaFileError(file_id, "Failed RPC AddFile: " + result.error_);
    return;
  }
  if ( result.result_ != "" ) {
    NotifyMediaFileError(file_id, result.result_);
  }
}
void SlaveController::HandleDelFileResult(
    string file_id, const rpc::CallResult<rpc::Void>& result) {
  if ( !result.success_ ) {
    LOG_ERROR << "Failed RPC DelFile: " << result.error_;
    return;
  }
}
void SlaveController::HandleGetFileStateResult(
    string file_id, const rpc::CallResult<FileState>& result) {
  if ( !result.success_ ) {
    LOG_ERROR << "Failed RPC GetFileState: " << result.error_;
    NotifyMediaFileError(file_id, "Failed RPC GetFileState: " + result.error_);
    return;
  }
  NotifyMediaFileStateChange(result.result_);
}

void SlaveController::Save() {
  vector<StateMediaFile> rpc_files(files_.size());
  uint32 i = 0;
  for( CMediaFileMap::const_iterator it = files_.begin();
       it != files_.end(); ++it, ++i ) {
    const CMediaFile& cfile = *it->second;
    const MediaFile& file = *cfile.file_;
    rpc_files[i] = StateMediaFile(file.id(),
                                  file.path(),
                                  file.process_params(),
                                  EncodeFState(file.state()),
                                  file.error());
  }
  state_keep_user_.SetValue(kStateFiles,
                            rpc::JsonEncoder::EncodeObject(rpc_files));
}

bool SlaveController::Load() {
  CHECK(files_.empty());
  string str_encoded_files;
  if(!state_keep_user_.GetValue(kStateFiles, &str_encoded_files)) {
    LOG_ERROR << "Load: key not found: " << kStateFiles;
    return false;
  }
  vector<StateMediaFile> rpc_files;
  if ( !rpc::JsonDecoder::DecodeObject(str_encoded_files, &rpc_files) ) {
    LOG_ERROR << "Load: failed to decode str_encoded_files: ["
              << str_encoded_files << "]";
    return false;
  }
  for(int i = 0, size = rpc_files.size(); i < size; ++i) {
    const StateMediaFile& rpc_file = rpc_files[i];
    // don't check file physical existence. Just assume the best.

    // add the new file
    MediaFile* file = new MediaFile(
        rpc_file.id_.get(),
        rpc_file.path_.get(),
        rpc_file.process_params_.get(),
        DecodeFState(rpc_file.state_.get()),
        rpc_file.error_.get(),
        0,
        SlaveURI(),
        kEmptyTranscodingStatus);
    CMediaFile* cfile = AddFile(file);
    if ( cfile == NULL ) {
      LOG_ERROR << "Load: Ignoring duplicate file: " << *file;
      delete file;
      file = NULL;
    }
  }
  return true;
}

string SlaveController::ToString() const {
  ostringstream oss;
  oss << "SlaveController{host_port_=" << host_port_
      << ", rpc_codec_id_=" << rpc_codec_id_
      << "(" << rpc::CodecName(rpc_codec_id_) << ")"
      << ", rpc_connected_=" << (rpc_client_wrapper_ ? "true" : "false")
      << ", files_=#" << files_.size() << "{";
  for ( CMediaFileMap::const_iterator it = files_.begin();
        it != files_.end(); ) {
    MediaFile& file = *it->second->file_;
    oss << file;
    if ( ++it != files_.end() ) {
      oss << ", ";
    }
  }
  oss << "}}";
  return oss.str();
}

ostream& operator<<(ostream& os, const SlaveController& slave) {
  return os << slave.ToString();
}
}
}
