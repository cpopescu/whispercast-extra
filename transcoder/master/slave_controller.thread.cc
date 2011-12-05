
namespace manager { namespace master {

SlaceController::SlaceController(net::Selector & selector
                                 RPCMasterManagerService & master,
                                 const net::HostPort & host_port,
                                 RPC_CODEC_ID rpc_codec_id,
                                 uint32 max_paralel_processing_file_count)
  : selector_(selector),
    master_(master),
    thread_(NULL),
    end_(false),
    files_(),
    sync_(false, true),
    max_paralel_processing_file_count_(max_paralel_processing_file_count),
    host_port_(host_port),
    rpc_codec_id_(rpc_codec_id),
    rpc_connection_(NULL),
    rpc_client_wrapper_(NULL),
    rpc_connect_completed_callback_(NewPermanentCallback(this, SlaceController::RpcConnectCompleted)) {
}
SlaveController::~SlaveController() {
  Stop();
  RpcClear();
  delete rpc_connect_completed_callback_;
  rpc_connect_completed_callback_ = NULL;
}
bool SlaveController::Start() {
  CHECK(!thread_) << "already running";
  end_ = false;
  thread_ = new thread::Thread(NewCallback(this, &SlaveController::Run));

  const size_t stack_size = PTHREAD_STACK_MIN + (1 << 18);
  CHECK(thread_->SetStackSize(stack_size));
  CHECK(thread_->SetJoinable(true));
  CHECK(thread_->Start());
  return true;
}
void SlaveController::Stop() {
  if(!thread_) {
    return;
  }
  end_ = true;
  thread_->Join();
  delete thread_;
  thread_ = NULL;
}

void SlaveController::NotifyMediaFileStateChange(int64 file_id, MasterFileState state, const std::string & error) {
  synch::MutexLocker lock(&sync_);
  MediaFileMap::iterator it = files_.find(file_id);
  if(it == files_.end()) {
    LOG_ERROR << "No such file_id: " << file_id;
    return;
  }
  MediaFile & file = *it->second;
  ChangeFileState(file, state, error);
}

void SlaveController::RpcConnect() {
  RpcClear();
  CHECK_NULL(rpc_connection_);
  CHECK_NULL(rpc_client_wrapper_);

  rpc_connection_ = new RPCClientConnectionTCP(selector_, false, rpc_codec_id_);
  bool success = rpc_connection_->Open(host_port_, rpc_connect_completed_callback_);
  if(!success) {
    LOG_ERROR << "Cannot open rpc_connection to host_port: " << host_port_ << " error: " << rpc_connection_->Error();
    RpcClear();
    return;
  }
  return;
}
void SlaveController::RpcConnectCompleted() {
  CHECK_NOT_NULL(rpc_connection_);
  if(!rpc_connection_->IsOpen()) {
    RpcClear();
    return;
  }
  CHECK_NULL(rpc_client_wrapper_);
  rpc_client_wrapper_ = new RPCServiceWrapperSlaveManager(*rpc_connection_);
}
void SlaveController::RpcClear() {
  if(rpc_client_wrapper_) {
    delete rpc_client_wrapper_;
    rpc_client_wrapper_ = NULL;
  }
  if(rpc_connection_) {
    delete rpc_connection_;
    rpc_connection_ = NULL;
  }
}

//static
int64 SlaveController::DelayProcessFile(MasterFileState state) {
  switch(state) {
    case IDLE:              return 0;
    case S_TRANSFERRING:    return 10000;
    case S_TRANSCODING:     return 10000;
    case S_TRANSFERRED:     return 0;
    case S_TRANSCODED:      return 60000;
    case CONNECTION_ERROR:  return 10000;
    case S_NOT_FOUND:       return 0;
    case S_TRANSFER_ERROR:  return 0;
    case S_TRANSCODE_ERROR: return 0;
    default:
      LOG_ERROR << "Invalid file state: " << file;
  }
}
void SlaveController::ChangeFileState(MediaFile & file, MasterFileState state, const std::string & error) {
  int64 ts_now = timer::TicksMsec();
  int64 ts_next_process = ts_now + DelayProcessFile(state);
  file.set_state(state, error);
  file.set_ts_state_change(ts_now);
  file.set_ts_next_process(ts_next_process);
}
void SlaveController::ProcessFile(MediaFile & file) {
  CHECK(RpcIsConnected());
  CHECK(rpc_client_wrapper_);
  switch(file.state()) {
    case IDLE:
      {
        // Start copy
        bool success = rpc_client_wrapper_->Copy(NewCallback(this, &RPCMasterManagerService::HandleCopyResult, (int64)file.id()), RPCString(file.path()), RPCBigInt(file.id()));
        if(!success) {
          LOG_ERROR << "Failed to start Copy RPC: " << rpc_slave->Error();
          ChangeFileState(file, CONNECTION_ERROR, slave->error());
          return;
        }
        ChangeFileState(file, S_TRANSFERRING, "");
        return;
      }
    // actions in progress
    case S_TRANSFERRING:
    case S_TRANSCODING:
      {
        bool success = rpc_client_wrapper_->GetFileState(NewCallback(this, &RPCMasterManagerService::HandleGetFileStateResult, file.id()), file.id());
        if(!success) {
          LOG_ERROR << "Failed to start GetFileState RPC: " << rpc_slave->Error();
          ChangeFileState(file, CONNECTION_ERROR, slave->error());
          return;
        }
        // schedule a new TRANSFERRING / TRANSCODING check.
        // This will probably be overwritten by the HandleGetFileStateResult asynchronous call.
        ChangeFileState(file, file.state(), "");
        return;
      }
    case S_TRANSFERRED:
      {
        bool success = rpc_client_wrapper_->Process(NewCallback(this, &RPCMasterManagerService::HandleProcessResult, file.id()), file.id());
        if(!success) {
          LOG_ERROR << "Failed to start Copy RPC: " << rpc_slave->Error();
          ChangeFileState(file, CONNECTION_ERROR, slave->error());
          return;
        }
        ChangeFileState(file, S_TRANSCODING);
        return;
      }
    case S_TRANSCODED: // file done, but keep checking
      {
        bool success = rpc_client_wrapper_->GetFileState(NewCallback(this, &RPCMasterManagerService::HandleGetFileStateResult, file.id()), file.id());
        if(!success) {
          LOG_ERROR << "Failed to start GetFileState RPC: " << rpc_slave->Error();
          ChangeFileState(file, CONNECTION_ERROR, slave->error());
          return;
        }
        ChangeFileState(file, file.state(), "");
        return;
      }
    case CONNECTION_ERROR: // unknown file state due to broken slave connection
      {
        bool success = rpc_client_wrapper_->GetFileState(NewCallback(this, &RPCMasterManagerService::HandleGetFileStateResult, file.id()), file.id());
        if(!success) {
          LOG_ERROR << "Failed to start GetFileState RPC: " << rpc_connection_->Error();
          ChangeFileState(file, CONNECTION_ERROR, rpc_connection_->error());
          return;
        }
        ChangeFileState(file, file.state(), "");
        return;
      }
    // fatal errors, restart whole process
    case S_NOT_FOUND:
    case S_TRANSFER_ERROR:
    case S_TRANSCODE_ERROR:
      {
        LOG_WARNING << "File in fatal error state: " << file << " switching to IDLE.";
        ChangeFileState(file, IDLE, "");
        return;
      }
    default:
      {
        LOG_FATAL << "Invalid file state: " << file;
        return;
      }
  }
}

void SlaveController::Run() {
  while(!end_) {
    const int64 ticks_start = timer::TicksMsec();
    const int64 ticks_now = ticks_start;

    if(rpc_connection_ == NULL && rpc_client_wrapper_ == NULL) {
      RpcConnect();
    } else {
      synch::MutexLocker lock(&sync_);

      // process files
      //
      uint32 processing_count = 0;
      for(std::list<MediaFile*>::iterator it = files_.begin(); it != files_.end(); ++it) {
        MediaFile & file = **it;
        if(file.ts_next_process() < ticks_now)
        {
          // time to process file
          //
          ProcessFile(file);
          if(!rpc_connection_->IsOpen()) {
            break;
          }
        }

        if(file.state() != TRANSCODED) {
          processing_count++;
        }
      }

      // check for broken rpc
      //
      if(!rpc_connection_->IsOpen()) {
        RpcClear();
        for(std::list<MediaFile*>::iterator it = files_.begin(); it != files_.end(); ++it) {
          MediaFile & file = **it;
          ChangeFileState(file, CONNECTION_ERROR, "RPC connection error");
        }
        processing_count = files_.size();
      }

      // maybe grab a new file
      //
      if(processing_count < max_paralel_processing_file_count_) {
        MediaFile * file = master_.GrabMediaFile();
        if(file) {
          files_.push_back(file);
        }
      }
    }

    const int64 ticks_end = timer::TicksMsec();
    CHECK_GE(ticks_end, ticks_start);
    const int64 duration = ticks_end - ticks_start;

    if(duration < 1000) {
      // delay between iterations
      timer::SleepMsec(1000 - duration);
    }
  }
}

}; };
