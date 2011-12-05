// Copyright (c) 2009, Whispersoft s.r.l.
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
// * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
// * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
// * Neither the name of Whispersoft s.r.l. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// Author: Cosmin Tudorache

#include "common/base/types.h"
#include "common/base/log.h"
#include "common/base/re.h"
#include "common/base/system.h"
#include "common/base/timer.h"
#include "common/sync/event.h"

#include "../multi_process.h"

synch::Event* g_process_exit = NULL;

void ProcessExit(bool success, int exit_code) {
  LOG_WARNING << "ProcessExit(" << boolalpha << success << ", " << exit_code << ")";
  g_process_exit->Signal();
}

struct TestProc {
  process::Process* process_;
  int expected_exit_code_;
  TestProc(process::Process* process, int expected_exit_code)
    : process_(process),
      expected_exit_code_(expected_exit_code) {
  }
};

bool Test(int32 min_duration_ms, int32 max_duration_ms,
          TestProc* proc, ...) {
  vector<TestProc*> procs;
  va_list ap;
  ::va_start(ap, proc);
  TestProc* crt = proc;
  while ( crt != NULL ) {
    procs.push_back(crt);
    crt = va_arg(ap, TestProc*);
  }
  ::va_end(ap);

  process::MultiProcess multi_process(process::MultiProcess::SEQUENTIAL_EXECUTION);
  multi_process.set_completion_callback(NewCallback(&ProcessExit));
  for ( uint32 i = 0; i < procs.size(); i++ ) {
    TestProc* proc = procs[i];
    multi_process.Add(proc->process_, proc->expected_exit_code_);
    delete proc;
  }
  procs.clear();

  g_process_exit->Reset();
  if ( !multi_process.Start() ) {
    LOG_ERROR << "Failed to start multi_process";
    return false;
  }
  int64 start_ts = timer::TicksMsec();
  if ( !g_process_exit->Wait(max_duration_ms) ) {
    LOG_ERROR << "Timeout waiting for multi_process exit: " << max_duration_ms << " ms";
    return false;
  }
  int64 end_ts = timer::TicksMsec();
  LOG_INFO << "Test finished in " << (end_ts - start_ts)/1000.0f << " seconds";
  if ( end_ts - start_ts < min_duration_ms ) {
    LOG_ERROR << "Test finished to early, took: " << (end_ts - start_ts) << " , expected min: " << min_duration_ms;
    return false;
  }
  return true;
}

int main(int argc, char* argv[]) {
  common::Init(argc, argv);

  LOG_INFO << "Begin TEST";
  g_process_exit = new synch::Event(false, true);

  CHECK(Test(9000, 14000,
             new TestProc(new process::Process("/bin/sleep", "1", NULL), 0),
             new TestProc(new process::Process("/bin/sleep", "3", NULL), 0),
             new TestProc(new process::Process("/bin/sleep", "5", NULL), 0),
             NULL));
  CHECK(Test(4000, 8000,
             new TestProc(new process::Process("/bin/sleep", "1", NULL), 0),
             new TestProc(new process::Process("/bin/sleep", "3", NULL), 0),
             new TestProc(new process::Process("/bin/cp", "missing file1", "missing file2", NULL), 0),
             new TestProc(new process::Process("/bin/sleep", "5", NULL), 0),
             NULL));
  CHECK(Test(9000, 14000,
             new TestProc(new process::Process("/bin/sleep", "1", NULL), 0),
             new TestProc(new process::Process("/bin/sleep", "3", NULL), 0),
             new TestProc(new process::Process("/bin/cp", "missing file1", "missing file2", NULL), 1),
             new TestProc(new process::Process("/bin/sleep", "5", NULL), 0),
             NULL));

  delete g_process_exit;
  g_process_exit = NULL;
  LOG_INFO << "End TEST";

  common::Exit(0);
}
