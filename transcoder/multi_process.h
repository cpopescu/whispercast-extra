// Copyright (c) 2009, Whispersoft s.r.l.
// All rights reserved.
//
// Author: Cosmin Tudorache
//
//////////////////////////////////////////////////////////////////////

#ifndef MULTI_PROCESS_H
#define MULTI_PROCESS_H

#include <whisperlib/common/base/types.h>
#include <whisperlib/common/base/callback.h>
#include <whisperlib/common/base/strutil.h>
#include <whisperlib/common/sync/process.h>
#include <whisperlib/common/sync/mutex.h>
#include <whisperlib/net/base/selector.h>

namespace process {

class MultiProcess {
public:
  enum ExecutionPattern {
    // execute processes one after the other. Exit code is the first fail, or 0.
    SEQUENTIAL_EXECUTION,
    // execute all processes in parallel. Exit code is the first fail, or 0.
    PARALLEL_EXECUTION,
  };

private:
  struct ProcessState {
    Process* process_;
    int expected_exit_code_;
    bool is_running_;
    int index_; // index inside vector 'processes_'
    ProcessState(Process* process,
                 int expected_exit_code,
                 int index)
      : process_(process),
        expected_exit_code_(expected_exit_code),
        is_running_(false),
        index_(index) {
    }
    string ToString() const {
      return strutil::StringPrintf(
          "{process_: 0x%p, expected_exit_code_: %d"
          ", is_running_: %s, index_: %d}",
          process_, expected_exit_code_,
          is_running_ ? "true" : "false", index_);
    }
  };

public:
  MultiProcess(ExecutionPattern execution_pattern);
  virtual ~MultiProcess();

  // Set a callback to be run when execution finishes.
  // You receive:
  //  - on success: true, 0
  //  - on failure: false, the exit code of the last failed process
  // If the 'callback' is permanent we never delete it;
  // else we either run it or delete it.
  typedef Callback2<bool, int> CompletionCallback;
  void set_completion_callback(CompletionCallback* callback);

  // The 'process' must be dynamically allocated.
  // We take ownership of 'process' and delete it on completion.
  void Add(Process* process, int expected_exit_code);

  // Start executing the processes. The 'completion_callback_' must be set.
  // Returns: true on success. The 'completion_callback_' will be later called.
  //          false on failure. The'completion_callback_' will not be called.
  bool Start();
  // If nothing is running, then it just clears 'processes_' .
  // Else, it kills the current running process(es), no other process is run,
  // the 'completion_callback_' is not run but possibly deleted.
  void StopAndClear();

private:
  // Called with every process exit.
  void ProcessExit(ProcessState* ps, int exit_code);

  // Helper for running 'completion_callback_' .
  void RunCompletionCallback(bool success, int exit_code);

private:
  // how to execute the processes
  const ExecutionPattern execution_pattern_;

  // array of processes
  typedef vector<ProcessState*> ProcessVector;
  ProcessVector processes_;

  CompletionCallback* completion_callback_;

  bool parallel_success_;
  int parallel_exit_code_;

  synch::Mutex sync_;
};

}

#endif  // MULTI_PROCESS_H
