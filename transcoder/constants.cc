
#include <string>
#include "common/base/log.h"
#include "constants.h"

namespace manager
{

#define CONSIDER_STR(name) \
  case name: { static const std::string str(#name); return str; }

// TODO(cosmin): re
const std::string & FStateName2(FState state) {
  switch(state) {
    CONSIDER_STR(FSTATE_TRANSFERRING)
    CONSIDER_STR(FSTATE_TRANSFERRED)
    CONSIDER_STR(FSTATE_TRANSCODING)
    CONSIDER_STR(FSTATE_POST_PROCESSING)
    CONSIDER_STR(FSTATE_OUTPUTTING)
    CONSIDER_STR(FSTATE_DISTRIBUTING)
    CONSIDER_STR(FSTATE_READY)
    CONSIDER_STR(FSTATE_TRANSFER_ERROR)
    CONSIDER_STR(FSTATE_TRANSCODE_ERROR)
    CONSIDER_STR(FSTATE_POST_PROCESS_ERROR)
    CONSIDER_STR(FSTATE_NOT_FOUND)
    CONSIDER_STR(FSTATE_DUPLICATE)
    CONSIDER_STR(FSTATE_INVALID_PARAMETER)
    CONSIDER_STR(FSTATE_DISK_ERROR)
    CONSIDER_STR(FSTATE_IDLE)
    CONSIDER_STR(FSTATE_BAD_SLAVE_STATE)
    default:
      LOG_FATAL << "Invalid file state: " << state;
      static const std::string str("Unknown");
      return str;
  }
};

//////////////////////////////////////////////////////////////////////////
// These strings are used in RPC transactions. Declared in manager.rpc.
// It's better if they don't change.
// But, if you have to change something here:
//  - check both encoder & decoder,
//  - check JS clients too.
//
namespace {
static const std::string kTransferringStr("Transferring");
static const std::string kTransferredStr("Transferred");
static const std::string kTranscodingStr("Transcoding");
static const std::string kPostProcessingStr("PostProcessing");
static const std::string kOutputtingStr("Outputting");
static const std::string kDistributingStr("Distributing");
static const std::string kReadyStr("Ready");
static const std::string kTransferErrorStr("TransferError");
static const std::string kTranscodeErrorStr("TranscodeError");
static const std::string kPostProcessErrorStr("PostProcessError");
static const std::string kNotFoundStr("NotFound");
static const std::string kDuplicateStr("Duplicate");
static const std::string kInvalidParameterStr("InvalidParameter");
static const std::string kDiskErrorStr("DiskError");
static const std::string kIdleStr("Idle");
static const std::string kBadSlaveStateStr("BadSlaveState");
}

const std::string& EncodeFState(FState state) {
  switch(state) {
    case FSTATE_TRANSFERRING:       { return kTransferringStr; }
    case FSTATE_TRANSFERRED:        { return kTransferredStr; }
    case FSTATE_TRANSCODING:        { return kTranscodingStr; }
    case FSTATE_POST_PROCESSING:    { return kPostProcessingStr; }
    case FSTATE_OUTPUTTING:         { return kOutputtingStr; }
    case FSTATE_DISTRIBUTING:       { return kDistributingStr; }
    case FSTATE_READY:              { return kReadyStr; }
    case FSTATE_TRANSFER_ERROR:     { return kTransferErrorStr; }
    case FSTATE_TRANSCODE_ERROR:    { return kTranscodeErrorStr; }
    case FSTATE_POST_PROCESS_ERROR: { return kPostProcessErrorStr; }
    case FSTATE_NOT_FOUND:          { return kNotFoundStr; }
    case FSTATE_DUPLICATE:          { return kDuplicateStr; }
    case FSTATE_INVALID_PARAMETER:  { return kInvalidParameterStr; }
    case FSTATE_DISK_ERROR:         { return kDiskErrorStr; }
    case FSTATE_IDLE:               { return kIdleStr; }
    case FSTATE_BAD_SLAVE_STATE:    { return kBadSlaveStateStr; }
    default:
      LOG_FATAL << "Invalid file state: " << state;
      static const std::string str("Unknown");
      return str;
  }
}

FState DecodeFState(const std::string& state) {
  if ( state == kTransferringStr )     { return FSTATE_TRANSFERRING; }
  if ( state == kTransferredStr )      { return FSTATE_TRANSFERRED; }
  if ( state == kTranscodingStr )      { return FSTATE_TRANSCODING; }
  if ( state == kPostProcessingStr )   { return FSTATE_POST_PROCESSING; }
  if ( state == kOutputtingStr )       { return FSTATE_OUTPUTTING; }
  if ( state == kDistributingStr )     { return FSTATE_DISTRIBUTING; }
  if ( state == kReadyStr )            { return FSTATE_READY; }
  if ( state == kTransferErrorStr )    { return FSTATE_TRANSFER_ERROR; }
  if ( state == kTranscodeErrorStr )   { return FSTATE_TRANSCODE_ERROR; }
  if ( state == kPostProcessErrorStr ) { return FSTATE_POST_PROCESS_ERROR; }
  if ( state == kNotFoundStr )         { return FSTATE_NOT_FOUND; }
  if ( state == kDuplicateStr )        { return FSTATE_DUPLICATE; }
  if ( state == kInvalidParameterStr ) { return FSTATE_INVALID_PARAMETER; }
  if ( state == kDiskErrorStr )        { return FSTATE_DISK_ERROR; }
  if ( state == kIdleStr )             { return FSTATE_IDLE; }
  return FSTATE_BAD_SLAVE_STATE;
}

const std::string& ProcessCmdName(ProcessCmd process_cmd) {
  switch ( process_cmd ) {
    CONSIDER_STR(PROCESS_CMD_TRANSCODE)
    CONSIDER_STR(PROCESS_CMD_POST_PROCESS)
    CONSIDER_STR(PROCESS_CMD_COPY)
    default:
      LOG_FATAL << "Invalid ProcessCmd value: " << process_cmd;
      static const std::string str("UNKNOWN");
      return str;
  }
}

//////////////////////////////////////////////////////////////////////////
// These strings are used in RPC transactions. Declared in manager.rpc.
// It's better if they don't change.
// But, if you have to change something here:
//  - check both encoder & decoder,
//  - check JS clients too.
//
namespace {
static const std::string kProcessCmdTranscode("transcode");
static const std::string kProcessCmdPostProcess("post_process");
static const std::string kProcessCmdCopy("copy");
}

const std::string& EncodeProcessCmd(ProcessCmd process_cmd) {
  switch ( process_cmd ) {
    case PROCESS_CMD_TRANSCODE: return kProcessCmdTranscode;
    case PROCESS_CMD_POST_PROCESS: return kProcessCmdPostProcess;
    case PROCESS_CMD_COPY: return kProcessCmdCopy;
    default:
      LOG_FATAL << "Invalid ProcessCmd value: " << process_cmd;
      static const string str("unknown");
      return str;
  }
}
ProcessCmd DecodeProcessCmd(const std::string& process_cmd) {
  if ( process_cmd == kProcessCmdTranscode ) { return PROCESS_CMD_TRANSCODE; }
  if ( process_cmd == kProcessCmdPostProcess ) { return PROCESS_CMD_POST_PROCESS; }
  if ( process_cmd == kProcessCmdCopy ) { return PROCESS_CMD_COPY; }
  return static_cast<ProcessCmd>(-1);
}

//extern, declared in constants.h
const FileTranscodingStatus kEmptyFileTranscodingStatus(
    "", vector<OutFileTranscodingStatus>(0));
const FileSlaveOutput kEmptyFileSlaveOutput(
    "", "", vector<FileOutputDir>(0));

};
