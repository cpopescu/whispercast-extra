#ifndef SLAVE_CONTROLLER_H_
#define SLAVE_CONTROLLER_H_

namespace manager { namespace master {

class SlaveController {
public:
  SlaveController(net::Selector & selector,
                  RPCMasterManagerService & master,
                  const net::HostPort & host_port,
                  RPC_CODEC_ID rpc_codec_id);
  virtual ~SlaveController();
  
  bool Start();
  void Stop();
  bool IsRunning() const;
  
  void NotifyMediaFileStateChange(int64 file_id, MasterFileState state, const std::string & error);
  
private:
  void RpcConnect();
  void RpcConnectCompleted();
  void RpcClear();
  
  static int64 DelayProcessFile(MasterFileState state);
  void ChangeFileState(MediaFile & file, MasterFileState state, const std::string & error);
  void ProcessFile(MediaFile & file);
  
  void Run();
  
private:
  net::Selector & selector_;
  RPCMasterManagerService & master_;

  // inner worker thread
  thread::Thread * thread_;
  // signals internal thread to terminate 
  bool end_;

  // all files on this slave. Some are processing, some are processed.
  // Mapped by file_id.
  typedef std::map<int64, MediaFile*> MediaFileMap;
  MediaFileMap files_;
  // synchronize access to files_
  synch::Mutext sync_;
  
  // how many files can we start processing in paralel on the slave
  const uint32 max_paralel_processing_file_count_;
  
  // slave host port
  const net::HostPort host_port_;
  
  // rpc stuff
  const RPC_CODEC_ID rpc_codec_id_;
  RPCClientConnectionTCP * rpc_connection_;
  RPCServiceWrapperSlaveManager * rpc_client_wrapper_;
  Closure * rpc_connect_completed_callback_;
};

}; }; // namespace master // namespace manager

#endif /*SLAVE_CONTROLLER_H_*/
