// Copyright (c) 2010, Whispersoft s.r.l.

#include "rpc_master_control_service.h"
#include "rpc_master_manager_service.h"
#include "media_file.h"
#include "../common.h"

namespace manager { namespace master {

RPCMasterControlService::RPCMasterControlService(
    net::Selector & selector,
    RPCMasterManagerService & master)
  : ServiceInvokerMasterControl(ServiceInvokerMasterControl::GetClassName()),
    selector_(selector),
    master_(master) {
}

RPCMasterControlService::~RPCMasterControlService() {
}

void RPCMasterControlService::AddFile(
    rpc::CallContext<int64>* call,
    const string& rpc_file_path,
    const string& rpc_process_cmd,
    const map<string, string>& process_params) {
  ProcessCmd process_cmd = DecodeProcessCmd(rpc_process_cmd);
  if ( process_cmd == static_cast<ProcessCmd>(-1) ) {
    LOG_ERROR << "Invalid process_cmd: [" << rpc_process_cmd << "]";
    call->Complete(-1);
    return;
  }
  const int64 file_id = master_.AddMediaFile(rpc_file_path,
                                             process_cmd,
                                             process_params);
  call->Complete(file_id);
}
void RPCMasterControlService::GetFileState(
    rpc::CallContext<FileStateControl>* call, int64 file_id) {
  FullMediaFile full_file = master_.GetMediaFile(file_id);
  if ( full_file.main_ == NULL ) {
    call->Complete(FileStateControl(INVALID_FILE_ID, "", "",
                                    map<string, string>(),
                                    EncodeFState(FSTATE_NOT_FOUND), "",
                                    timer::Date::Now(),
                                    vector<FileSlaveOutput>(0),
                                    kEmptyFileTranscodingStatus));
    return;
  }

  FState main_fstate = full_file.main_->state();
  string main_error = full_file.main_->error();
  bool main_has_error = false;

  vector<FileSlaveOutput> output(1 + full_file.duplicates_.size());
  output[0] = full_file.main_->output();
  for ( uint32 i = 0; i < full_file.duplicates_.size(); i++ ) {
    const MediaFile* f = full_file.duplicates_[i];
    output[1 + i] = f->output();
    if ( main_has_error ) {
      continue;
    }
    if ( f->state() == FSTATE_READY ) {
      continue;
    }
    if ( main_error == "" && f->error() != "" ) {
      main_error = "error on slave: " + f->slave() + ": " + f->error();
    }
    if ( f->state() == FSTATE_IDLE ||
         f->state() == FSTATE_TRANSFERRING ||
         f->state() == FSTATE_TRANSFERRED ||
         f->state() == FSTATE_TRANSCODING ||
         f->state() == FSTATE_POST_PROCESSING ||
         f->state() == FSTATE_OUTPUTTING ) {
      main_fstate = FSTATE_DISTRIBUTING;
      continue;
    }
    main_fstate = f->state();
    main_has_error = true;
  }
  call->Complete(FileStateControl(
                     full_file.main_->id(),
                     full_file.main_->path(),
                     EncodeProcessCmd(full_file.main_->process_cmd()),
                     full_file.main_->process_params(),
                     EncodeFState(main_fstate),
                     main_error,
                     full_file.main_->ts_state_change(),
                     // TODO(cosmin): fix the array of slave output
                     output,
                     full_file.main_->transcoding_status()));
}
void RPCMasterControlService::ListFiles(
    rpc::CallContext< vector<FileStateControl> > * call) {
  std::vector<FullMediaFile> files;
  master_.GetMediaFiles(files);
  vector<FileStateControl> rpc_files(files.size());
  uint32 i = 0;
  for ( std::vector<FullMediaFile>::const_iterator it = files.begin();
        it != files.end(); ++it, ++i ) {
    const FullMediaFile& full_file = *it;

    FState main_fstate = full_file.main_->state();
    string main_error = full_file.main_->error();
    bool main_has_error = false;

    vector<FileSlaveOutput> output(1 + full_file.duplicates_.size());
    output[0] = full_file.main_->output();
    for ( uint32 j = 0; j < full_file.duplicates_.size(); j++ ) {
      const MediaFile* f = full_file.duplicates_[j];
      output[1 + j] = f->output();
      if ( main_has_error ) {
        continue;
      }
      if ( f->state() == FSTATE_READY ) {
        continue;
      }
      if ( main_error == "" && f->error() != "" ) {
        main_error = "error on slave: " + f->slave() + ": " + f->error();
      }
      if ( f->state() == FSTATE_IDLE ||
           f->state() == FSTATE_TRANSFERRING ||
           f->state() == FSTATE_TRANSFERRED ||
           f->state() == FSTATE_TRANSCODING ||
           f->state() == FSTATE_POST_PROCESSING ||
           f->state() == FSTATE_OUTPUTTING ) {
        main_fstate = FSTATE_DISTRIBUTING;
        continue;
      }
      main_fstate = f->state();
      main_has_error = true;
    }
    rpc_files[i] = FileStateControl(
        full_file.main_->id(),
        full_file.main_->path(),
        EncodeProcessCmd(full_file.main_->process_cmd()),
        full_file.main_->process_params(),
        EncodeFState(main_fstate),
        main_error,
        full_file.main_->ts_state_change(),
        // TODO(cosmin): fix the array of slave output
        output,
        full_file.main_->transcoding_status());
  }
  call->Complete(rpc_files);
}
void RPCMasterControlService::DebugDumpFiles(
    rpc::CallContext< vector<FileStateControl> > * call) {
  std::vector<const MediaFile*> files;
  master_.DebugDumpFiles(files);
  vector<FileStateControl> rpc_files(files.size());
  for ( uint32 i = 0; i < files.size(); i++ ) {
    const MediaFile* file = files[i];

    vector<FileSlaveOutput> output(1);
    output[0] = file->output();

    rpc_files[i] = FileStateControl(
        file->id(),
        file->path(),
        EncodeProcessCmd(file->process_cmd()),
        file->process_params(),
        EncodeFState(file->state()),
        file->error(),
        file->ts_state_change(),
        // TODO(cosmin): fix the array of slave output
        output,
        file->transcoding_status());
  }
  call->Complete(rpc_files);
}

void RPCMasterControlService::Complete(rpc::CallContext<rpc::Void> * call,
                                       int64 file_id) {
  master_.Delete(file_id, false, false);
  call->Complete();
}

void RPCMasterControlService::Delete(rpc::CallContext<rpc::Void> * call,
                                     int64 file_id,
                                     bool erase_master_original,
                                     bool erase_slave_output) {
  master_.Delete(file_id, erase_master_original, erase_slave_output);
  call->Complete();
}

void RPCMasterControlService::Retry(rpc::CallContext<string> * call,
                                    int64 file_id) {
  const FState file_state = master_.Retry(file_id);
  call->Complete(string(EncodeFState(file_state)));
}

}   // namespace master
}   // namespace manager
