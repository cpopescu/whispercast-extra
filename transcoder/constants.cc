
#include <string>
#include <whisperlib/common/base/log.h>
#include "constants.h"

namespace manager {

const char* FStateName(FState state) {
  switch(state) {
    CONSIDER(FSTATE_PENDING)
    CONSIDER(FSTATE_PROCESSING)
    CONSIDER(FSTATE_SLAVE_READY)
    CONSIDER(FSTATE_DISTRIBUTING)
    CONSIDER(FSTATE_DONE)
    CONSIDER(FSTATE_ERROR)
  }
  LOG_FATAL << "Invalid file state: " << state;
  return "Unknown";
};

//////////////////////////////////////////////////////////////////////////
// These strings are used in RPC transactions. Declared in manager.rpc.
// It's better if they don't change.
// But, if you have to change something here:
//  - check both encoder & decoder,
//  - check JS clients too.
//
namespace {
static const string kPendingStr("Pending");
static const string kProcessingStr("Processing");
static const string kSlaveReadyStr("SlaveReady");
static const string kDistributingStr("Distributing");
static const string kDoneStr("Done");
static const string kErrorStr("Error");
}

const string& EncodeFState(FState state) {
  switch(state) {
    case FSTATE_PENDING:      return kPendingStr;
    case FSTATE_PROCESSING:   return kProcessingStr;
    case FSTATE_SLAVE_READY:  return kSlaveReadyStr;
    case FSTATE_DISTRIBUTING: return kDistributingStr;
    case FSTATE_DONE:         return kDoneStr;
    case FSTATE_ERROR:        return kErrorStr;
  }
  LOG_FATAL << "Illegal file state: " << state;
  static const string str("Unknown");
  return str;
}

FState DecodeFState(const string& state) {
  if ( state == kPendingStr )       { return FSTATE_PENDING; }
  if ( state == kProcessingStr )    { return FSTATE_PROCESSING; }
  if ( state == kSlaveReadyStr )    { return FSTATE_SLAVE_READY; }
  if ( state == kDistributingStr )  { return FSTATE_DISTRIBUTING; }
  if ( state == kDoneStr )          { return FSTATE_DONE; }
  if ( state == kErrorStr )         { return FSTATE_ERROR; }
  LOG_ERROR << "Invalid File State: [" << state << "]";
  return FSTATE_ERROR;
}

//extern, declared in constants.h
const TranscodingStatus kEmptyTranscodingStatus(
    0.0f, vector<string>());

};
