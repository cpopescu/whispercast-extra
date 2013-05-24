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
    rpc::CallContext<rpc::Void>* call,
    const string& file_id,
    const string& file_path,
    const map<string, string>& process_params) {
  master_.AddMediaFile(file_id, file_path, process_params);
  call->Complete();
}
void RPCMasterControlService::DelFile(rpc::CallContext<rpc::Void> * call,
                                      const string& file_id,
                                      bool erase_master_original,
                                      bool erase_slave_output) {
  master_.Delete(file_id, erase_master_original, erase_slave_output);
  call->Complete();
}
void RPCMasterControlService::GetFileState(
    rpc::CallContext<FileStateControl>* call, const string& file_id) {
  const MediaFile* file = master_.GetMediaFile(file_id);
  if ( file == NULL ) {
    call->Complete(FileStateControl("", "",
                                    map<string, string>(),
                                    EncodeFState(FSTATE_ERROR),
                                    "File not found",
                                    timer::Date::Now(),
                                    kEmptyTranscodingStatus));
    return;
  }
  call->Complete(FileStateControl(
                     file->id(),
                     file->path(),
                     file->process_params(),
                     EncodeFState(file->state()),
                     file->error(),
                     file->ts_state_change(),
                     file->transcoding_status()));
}
void RPCMasterControlService::ListFiles(
    rpc::CallContext< vector<FileStateControl> > * call) {
  std::vector<const MediaFile*> files;
  master_.GetMediaFiles(&files);
  vector<FileStateControl> rpc_files(files.size());
  uint32 i = 0;
  for ( std::vector<const MediaFile*>::const_iterator it = files.begin();
        it != files.end(); ++it, ++i ) {
    const MediaFile* file = *it;

    rpc_files[i] = FileStateControl(
        file->id(),
        file->path(),
        file->process_params(),
        EncodeFState(file->state()),
        file->error(),
        file->ts_state_change(),
        file->transcoding_status());
  }
  call->Complete(rpc_files);
}

}   // namespace master
}   // namespace manager
