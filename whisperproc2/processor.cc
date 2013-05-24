// Copyright (c) 2009, Whispersoft s.r.l.
// All rights reserved.
//
// Author: Catalin Popescu & Cosmin Tudorache
//
#include <stdio.h>
#include <whisperlib/common/base/date.h>
#include <whisperlib/common/io/ioutil.h>

#include <whisperstreamlib/base/joiner.h>
#include <whisperstreamlib/base/saver.h>
#include <whisperstreamlib/base/time_range.h>
#include <whisperstreamlib/flv/flv_joiner.h>
#include <whisperstreamlib/f4v/f4v_joiner.h>

#include "processor.h"

namespace whisperproc {

Processor::Processor(const string& media_dir,
                     streaming::MediaFormat output_format,
                     int64 output_media_ms,
                     int64 max_delay_ms,
                     int64 cue_ms,
                     int64 periodic_scan_ms,
                     bool delete_part_files)
  : media_dir_(media_dir),
    output_format_(output_format),
    output_media_ms_(output_media_ms),
    max_delay_ms_(max_delay_ms),
    cue_ms_(cue_ms),
    periodic_scan_ms_(periodic_scan_ms),
    delete_part_files_(delete_part_files),
    info_file_re_(streaming::Saver::kMediaInfoFile),
    part_file_re_(streaming::Saver::kFileRE),
    thread_(NewCallback(this, &Processor::ThreadRun)),
    stop_event_(false, true) {
}
Processor::~Processor() {
  Stop();
}
bool Processor::Start() {
  CHECK(!thread_.IsRunning()) << " multiple Start()";
  if ( !io::IsDir(media_dir_) ) {
    LOG_ERROR << "Invalid media_dir: [" << media_dir_ << "]";
    return false;
  }
  switch ( output_format_ ) {
    // accepted formats
    case streaming::MFORMAT_FLV:
    case streaming::MFORMAT_F4V:
      break;
    // unsupported formats
    case streaming::MFORMAT_AAC:
    case streaming::MFORMAT_MP3:
    case streaming::MFORMAT_MTS:
    case streaming::MFORMAT_INTERNAL:
    case streaming::MFORMAT_RAW:
      LOG_ERROR << "Invalid output format: "
                << streaming::MediaFormatName(output_format_);
      return false;
  };
  thread_.SetJoinable();
  thread_.Start();
  return true;
}

void Processor::Stop() {
  if ( stop_event_.is_signaled() ) {
    return;
  }
  stop_event_.Signal();
  thread_.Join();
}

void Processor::ThreadRun() {
  LOG_WARNING << "Processor thread running..";
  while ( true ) {
    vector<string> info_files;
    io::DirList(media_dir_, io::LIST_FILES | io::LIST_RECURSIVE, &info_file_re_,
        &info_files);
    LOG_INFO << "Scan found " << info_files.size() << " saver locations";
    for ( uint32 i = 0; i < info_files.size(); i++ ) {
      ProcessDirectory(strutil::Dirname(info_files[i]));
    }
    LOG_INFO << "Waiting " << periodic_scan_ms_ << " ms before next scan..";
    if ( stop_event_.Wait(periodic_scan_ms_) ) {
      break;
    }
  }
  LOG_WARNING << "Processor thread ended.";
}

void Processor::GetProcessableFiles(const string& dir, vector<string>* out,
    int64* out_next_file_ts) {
  vector<string> files;
  const string d = strutil::JoinPaths(media_dir_, dir);
  io::DirList(d, io::LIST_FILES, &part_file_re_, &files);
  if ( files.empty() ) {
    return;
  }

  sort(files.begin(), files.end());

  const string& first_file = files[0];
  int64 first_file_ts = 0;
  if ( !streaming::Saver::ParseFilename(first_file, &first_file_ts, NULL) ) {
    LOG_ERROR << "Saver::ParseFilename failed for file: [" << first_file << "]";
    return;
  }
  const string& last_file = files[files.size()-1];
  int64 last_file_ts = 0;
  if ( !streaming::Saver::ParseFilename(last_file, &last_file_ts, NULL) ) {
    LOG_ERROR << "Saver::ParseFilename failed for file: [" << last_file << "]";
    return;
  }

  // if enough files were found, or the files are too old => then gather
  // the necessary files for a single composition
  if ( last_file_ts - first_file_ts > output_media_ms_ ||
       timer::Date::Now() - last_file_ts > max_delay_ms_ ) {
    for ( uint32 i = 0; i < files.size(); i++ ) {
      int64 ts = 0;
      bool is_new_chunk = false;
      if ( !streaming::Saver::ParseFilename(files[i], &ts, &is_new_chunk) ) {
        LOG_ERROR << "Saver::ParseFilename failed, file: [" << files[i] << "]";
        continue;
      }
      *out_next_file_ts = is_new_chunk ? 0 : ts;

      // if a new chunk starts or enough files were gathered, then stop
      if ( (is_new_chunk && i > 0) ||
           (ts - first_file_ts > output_media_ms_) ) {
        break;
      }

      out->push_back(files[i]);
    }
  }
}

bool Processor::ProcessDirectoryOnce(string dir) {
  vector<string> files;
  int64 next_file_ts = 0;
  GetProcessableFiles(dir, &files, &next_file_ts);
  if ( files.empty() ) {
    // not enough files for a composition
    LOG_INFO << "Process dir: [" << dir << "] => nothing to process";
    return false;
  }

  int64 begin_file_ts = 0;
  streaming::Saver::ParseFilename(files[0], &begin_file_ts, NULL);

  vector<string> input_paths;
  for ( uint32 i = 0; i < files.size(); i++ ) {
    input_paths.push_back(strutil::JoinPaths(media_dir_,
                           strutil::JoinPaths(dir, files[i])));
  }

  const string output_dir = strutil::JoinPaths(media_dir_,
                              strutil::JoinPaths(dir, "media"));
  io::Mkdir(output_dir);

  const string output_path = strutil::JoinPaths(output_dir,
      streaming::MakeTimeRangeFile(begin_file_ts, next_file_ts,
          output_format_));

  LOG_INFO << "Process dir: [" << dir << "] => composing files: "
           << strutil::ToString(files) << ", output file: " << output_path;

  // compose all given files. Returns the output file's length in ms.
  int64 ms = -1;
  if ( output_format_ == streaming::MFORMAT_FLV ) {
    ms = streaming::JoinFilesIntoFlv(input_paths, output_path, cue_ms_);
  } else if ( output_format_ == streaming::MFORMAT_F4V ) {
    ms = streaming::JoinFilesIntoF4v(input_paths, output_path);
  } else {
    LOG_FATAL << "Illegal output_format: " << (int)output_format_
              << " should have been filtered out on Start()";
  }
  if ( ms < 0 ) {
    LOG_ERROR << "Process dir: [" << dir << "]: JoinFiles failed"
                 ", abandoning directory..";
    return false;
  }
  if ( ms == 0 ) {
    // the output file contains only metadata. Useless file.
    LOG_INFO << "Process dir: [" << dir << "]: discarding output file: ["
             << output_path << "] because it has 0 ms";
    io::Rm(output_path);
  }

  if ( ms > 0 && next_file_ts == 0 ) {
    // The output file is correct (has a positive milliseconds length).
    // But the next part file is a New Chunk, so it's timestamp is irrelevant
    // (there's probably a gap between the last composed part file and the next
    // part file). Use the composed file size in ms.
    const int64 end_file_ts = begin_file_ts + ms;
    io::Rename(output_path,
        strutil::JoinPaths(strutil::Dirname(output_path),
            streaming::MakeTimeRangeFile(begin_file_ts, end_file_ts,
                output_format_)),
        false);
  }

  // move or erase the part files that have been composed
  for ( uint32 i = 0; i < input_paths.size(); i++ ) {
    if ( delete_part_files_ ) {
      io::Rm(input_paths[i]);
    } else {
      const string done_dir = strutil::JoinPaths(
          strutil::Dirname(input_paths[i]), "done_files");
      io::Mkdir(done_dir);
      io::Rename(input_paths[i],
          strutil::JoinPaths(done_dir,strutil::Basename(input_paths[i])),
          true);
    }
  }
  return true;
}
void Processor::ProcessDirectory(string dir) {
  // process the same directory multiple times
  // (each processing outputs a single media file)
  while ( !stop_event_.is_signaled() && ProcessDirectoryOnce(dir) ) {}
}

}
