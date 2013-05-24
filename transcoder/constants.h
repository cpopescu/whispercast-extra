#ifndef CONSTANTS_H_
#define CONSTANTS_H_

#include <string>
#include "auto/types.h"

namespace manager
{

////////////////////////////////////////////////////////////
// FState codes, as agreed in "manager.rpc" :
// These codes are internally used.
// When saving to state or reporting through RPC, we have string codes,
// provided by EncodeFState/DecodeFState.
//
enum FState {
  /////////////////////////////////////////////////////////////////////////////
  // SLAVE file state codes
  FSTATE_PENDING,       // the file is waiting on the slave for processing
  FSTATE_PROCESSING,    // the file is being transcoded
  FSTATE_SLAVE_READY,   // the results are ready on the slave

  /////////////////////////////////////////////////////////////////////////////
  // MASTER file state codes
  FSTATE_DISTRIBUTING, // the result files are being copied to output dirs
  FSTATE_DONE,         // the result files are ready in all output dirs

  FSTATE_ERROR,
};

const char* FStateName(FState file_state);

////////////////////////////////////////////////////////////
//
// These encoding/decoding methods are used for FileState.state_ field
// Complies with FState codes declared in "manager.rpc" :
//

// from FState -> to string
const string& EncodeFState(FState state);
// from string -> to FState
// Returns FSTATE_ERROR on failure.
FState DecodeFState(const string& state);

////////////////////////////////////////////////////////////
// defined in constants.cc
extern const TranscodingStatus kEmptyTranscodingStatus;


};

#endif /*CONSTANTS_H_*/
