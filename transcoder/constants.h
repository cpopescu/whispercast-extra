#ifndef CONSTANTS_H_
#define CONSTANTS_H_

#include <string>
#include "auto/types.h"

namespace manager
{

static const int64 INVALID_FILE_ID = -1;

////////////////////////////////////////////////////////////
// FState codes, as agreed in "manager.rpc" :
// These codes are not used anymore. Instead we have string codes, provided
// by EncodeFState/DecodeFState.
//
// These numeric codes are still used in state saving.
// TODO: use string codes in state saving.
//
enum FState {
  /////////////////////////////////////////////////////////////////////////////
  // SLAVE file state codes
  FSTATE_TRANSFERRING, // the file is being downloaded
  FSTATE_TRANSFERRED, // the file is transferred (copied)
  FSTATE_TRANSCODING, // the file is being transcoded
  FSTATE_POST_PROCESSING, // the file is being post processed
  FSTATE_OUTPUTTING, // result is duplicated between output dirs
  FSTATE_DISTRIBUTING, // result is duplicated between slaves
  FSTATE_READY, // the result is ready in all output dirs

  FSTATE_TRANSFER_ERROR, // transfer error
  FSTATE_TRANSCODE_ERROR, // transcode error
  FSTATE_POST_PROCESS_ERROR, // post process error
  FSTATE_NOT_FOUND, // no such file_id
  FSTATE_DUPLICATE, // duplicate file_id,
  FSTATE_INVALID_PARAMETER, // cannot parse master address, ProcessCmd
  FSTATE_DISK_ERROR, // mv, mkdir, cp failed
  /////////////////////////////////////////////////////////////////////////////
  // MASTER file state codes
  FSTATE_IDLE, // we have the file ready for transfer
  FSTATE_BAD_SLAVE_STATE, // the slave announced an invalid file state
};

const std::string& FStateName2(FState file_state);

////////////////////////////////////////////////////////////
//
// These encoding/decoding methods are used for FileState.state_ field
// Complies with FState codes declared in "manager.rpc" :
//

// from FState -> to string
const std::string& EncodeFState(FState state);
// from string -> to FState
// Returns BAD_SLAVE_STATE on failure.
FState DecodeFState(const std::string& state);

////////////////////////////////////////////////////////////
// ProcessCmd codes, for internal use
//
enum ProcessCmd {
  PROCESS_CMD_TRANSCODE,
  PROCESS_CMD_POST_PROCESS,
  PROCESS_CMD_COPY,
};
const std::string& ProcessCmdName(ProcessCmd process_cmd);

///////////////////////////////////////////////////////////
//
// Map ProcessCmd numeric codes to string codes.
// Complies with ProcessCmd codes declared in "manager.rpc" :
//

// from ProcessCmd -> string
const std::string& EncodeProcessCmd(ProcessCmd process_cmd);
// from string -> to ProcessCmd
// Returns -1 on failure.
ProcessCmd DecodeProcessCmd(const std::string& process_cmd);

////////////////////////////////////////////////////////////
// defined in constants.cc
extern const FileTranscodingStatus kEmptyFileTranscodingStatus;
extern const FileSlaveOutput kEmptyFileSlaveOutput;

};

#endif /*CONSTANTS_H_*/
