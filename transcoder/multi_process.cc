// Copyright (c) 2009, Whispersoft s.r.l.
// All rights reserved.
//
// Author: Cosmin Tudorache
//
//////////////////////////////////////////////////////////////////////

#include "multi_process.h"

namespace process {

MultiProcess::MultiProcess(ExecutionPattern execution_pattern)
  : execution_pattern_(execution_pattern),
    processes_(),
    completion_callback_(NULL),
    parallel_success_(true),
    parallel_exit_code_(-1),
    sync_() {
}

MultiProcess::~MultiProcess() {
  StopAndClear();
  CHECK(processes_.empty());
  if ( completion_callback_ != NULL && !completion_callback_->is_permanent() ) {
    delete completion_callback_;
    completion_callback_ = NULL;
  }
}

void MultiProcess::set_completion_callback(CompletionCallback* callback) {
  completion_callback_ = callback;
}

void MultiProcess::Add(Process* process, int expected_exit_code) {
  ProcessState* ps = new ProcessState(process,
                                      expected_exit_code,
                                      processes_.size());
  processes_.push_back(ps);
}

bool MultiProcess::Start() {
  CHECK_NOT_NULL(completion_callback_) << "Missing completion_callback_";
  if ( processes_.empty() ) {
    LOG_ERROR << "No processes to start";
    return false;
  }

  for ( int32 i = 0; i < processes_.size(); i++ ) {
    ProcessState* ps = processes_[i];
    ps->process_->SetExitCallback(NewCallback(
        this, &MultiProcess::ProcessExit, ps));
  }

  if ( execution_pattern_ == SEQUENTIAL_EXECUTION ) {
    ProcessState* ps = processes_[0];
    if ( !ps->process_->Start() ) {
      LOG_ERROR << "Failed to start process: " << ps->ToString();
      return false;
    }
    processes_[0]->is_running_ = true;
    return true;
  }
  if ( execution_pattern_ == PARALLEL_EXECUTION ) {
    parallel_success_ = true;
    parallel_exit_code_ = 0;
    for ( int32 i = 0; i < processes_.size(); i++ ) {
      ProcessState* ps = processes_[i];
      if ( ps->process_->Start() ) {
        ps->is_running_ = true;
      }
    }
    return true;
  }
  LOG_FATAL << "Illegal execution_pattern_: " << execution_pattern_;
  return false;
}

void MultiProcess::StopAndClear() {
  synch::MutexLocker lock(&sync_);
  for ( int32 i = 0; i < processes_.size(); i++ ) {
    ProcessState* ps = processes_[i];
    if ( ps->is_running_ ) {
      ps->process_->Kill();
      ps->is_running_ = false;
    }
    delete ps;
  }
  processes_.clear();
}

void MultiProcess::ProcessExit(ProcessState* ps, int exit_code) {
  synch::MutexLocker lock(&sync_);

  // integrity check
  CHECK_EQ(processes_[ps->index_], ps);

  ps->is_running_ = false;

  if ( execution_pattern_ == PARALLEL_EXECUTION ) {
    // maybe update parallel execution success & exit_code
    if ( exit_code != ps->expected_exit_code_ && parallel_success_ ) {
      parallel_success_ = false;
      parallel_exit_code_ = exit_code;
    }
    // if at least 1 process is still running, wait for it
    for ( int32 i = 0; i < processes_.size(); i++ ) {
      ProcessState* ps = processes_[i];
      if ( ps->is_running_ ) {
        return;
      }
    }
    LOG_INFO << "All processes finished execution";
    RunCompletionCallback(parallel_success_, parallel_exit_code_);
    return;
  }
  if ( execution_pattern_ == SEQUENTIAL_EXECUTION ) {
    // if last process failed, stop execution
    if ( exit_code != ps->expected_exit_code_ ) {
      LOG_ERROR << "Process execution failed: " << ps->ToString()
                << ", returned exit_code: " << exit_code;
      RunCompletionCallback(false, exit_code);
      return;
    }
    // run next process
    int next_index = ps->index_ + 1;
    if ( next_index < processes_.size() ) {
      ProcessState* ps = processes_[next_index];
      if ( !ps->process_->Start() ) {
        LOG_ERROR << "Failed to start process: " << ps->ToString();
        RunCompletionCallback(false, -1);
        return;
      }
      ps->is_running_ = true;
      return;
    }
    LOG_INFO << "All processes finished execution";
    RunCompletionCallback(true, 0);
    return;
  }
  LOG_FATAL << "Illegal execution_pattern_: " << execution_pattern_;
}

void MultiProcess::RunCompletionCallback(bool success, int exit_code) {
  bool is_permanent = completion_callback_->is_permanent();
  completion_callback_->Run(success, exit_code);
  if ( !is_permanent ) {
    completion_callback_ = NULL;
  }
}

}
