// Copyright (c) 2009, Whispersoft s.r.l.
// All rights reserved.
//
// Authors: Cosmin Tudorache
//
// Utility that scans a directory tree for .flv files and outputs .jpg
// thumbnails into a similar tree structure.
// For now it runs once and then terminates. It might be useful transforming it
// into a permanently monitoring daemon.
//

#ifndef UINT64_C
#define UINT64_C(value) value ## ULL
#endif

extern "C" {
#pragma GCC diagnostic ignored "-Wall"
#include __AV_CODEC_INCLUDE_FILE
#include __SW_SCALE_INCLUDE_FILE
//#include "/usr/include/libavcodec/avcodec.h"
//#include "/usr/include/libswscale/swscale.h"
#pragma GCC diagnostic warning "-Wall"
}

#include <whisperlib/common/base/types.h>
#include <whisperlib/common/base/log.h>
#include <whisperlib/common/base/system.h>
#include <whisperlib/common/base/gflags.h>
#include <whisperlib/common/base/errno.h>
#include <whisperlib/common/sync/thread_pool.h>

#include <whisperstreamlib/base/media_file_reader.h>
#include "features/util/file_thumbnailer.h"
#include "features/util/rtmp_thumbnailer.h"

//////////////////////////////////////////////////////////////////////

DEFINE_string(input,
              "",
              "File, RTMP stream, or Directory containing the media files.");
DEFINE_string(output_dir,
              "",
              "Directory where the thumbnails are written. Will be created.");
DEFINE_int32(threads,
             2,
             "The number of processing threads.");
DEFINE_int32(start_ms,
             0,
             "Extract first thumbnail at this time in file.");
DEFINE_int32(repeat_ms,
             5000,
             "Repeat extracting thumbnails every so often.");
DEFINE_int32(end_ms,
             kMaxInt32,
             "Stop extracting thumbnails at this time in file.");
DEFINE_int32(width,
             0,
             "Width of the output JPEGs. 0 = use film width.");
DEFINE_int32(height,
             0,
             "Height of the output JPEGs. 0 = use film height.");
DEFINE_int32(quality,
             75,
             "JPEG quality [1-99].");

//////////////////////////////////////////////////////////////////////

string ThumbsDoneFile(const string& file) {
  return file + ".thumbs_done";
}

// file: path to input file
// out_dir: path to the directory that receives the thumbnails
// skip_processed:
//   true => does nothing if file is already processed
//   false => ignores the possibility of file being already processed
void ProcessMediaFile(string file, string out_dir, bool skip_processed) {
  const string done_file = ThumbsDoneFile(strutil::JoinPaths(
      out_dir, strutil::Basename(file)));

  if ( skip_processed && io::IsReadableFile(done_file) ) {
    LOG_INFO << "Nothing to do for: [" << file << "]";
    // already processed
    return;
  }

  // proceed with thumbnails extraction
  if ( !util::ExtractThumbnails(
          file,
          FLAGS_start_ms,
          FLAGS_repeat_ms,
          FLAGS_end_ms,
          FLAGS_width,
          FLAGS_height,
          FLAGS_quality,
          out_dir) ) {
    LOG_ERROR << "Error processing thumbnails for file: [" << file << "]";
    return;
  }
  io::Touch(done_file);
}

int main(int argc, char* argv[]) {
#if LIBAVCODEC_VERSION_INT <= AV_VERSION_INT(53,34,0)
  avcodec_init();
#endif
  avcodec_register_all();

  common::Init(argc, argv);

  if ( !io::Mkdir(FLAGS_output_dir, false) ) {
    LOG_ERROR << "Cannot create output_dir: [" << FLAGS_output_dir << "]";
    common::Exit(1);
  }

  // maybe process a RTMP stream
  if ( strutil::StrIStartsWith(FLAGS_input, "rtmp://" ) ) {
    util::RtmpExtractThumbnails(FLAGS_input,
                                FLAGS_start_ms,
                                FLAGS_repeat_ms,
                                FLAGS_end_ms,
                                FLAGS_width,
                                FLAGS_height,
                                FLAGS_quality,
                                FLAGS_output_dir,
                                "");
    common::Exit(0);
  }

  // maybe process a single file
  if ( io::IsReadableFile(FLAGS_input) ) {
    ProcessMediaFile(FLAGS_input, FLAGS_output_dir, false);
    common::Exit(0);
  }

  /////////////////////////////////////////////////////////

  // or process a full directory tree
  if ( !io::IsDir(FLAGS_input) ) {
    LOG_ERROR << "Invalid input directory: [" << FLAGS_input << "]";
    common::Exit(1);
  }

  // scan input dir
  re::RE flv_re("\\.flv");
  vector<string> media_files;
  if ( !io::DirList(FLAGS_input, io::LIST_FILES |
                                 io::LIST_RECURSIVE, &flv_re, &media_files) ) {
    LOG_ERROR << "io::DirList failed for directory: " << FLAGS_input;
    common::Exit(1);
  }
  LOG_INFO << "Found #" << media_files.size() << " processable files";

  // process each file (skip files which were already processed)
  thread::ThreadPool tp(FLAGS_threads, FLAGS_threads * 4);
  tp.Start();
  for ( uint32 i = 0; i < media_files.size(); i++ ) {
    while ( tp.jobs()->IsFull() ) {
      ::sleep(1);
    }
    tp.jobs()->Put(NewCallback(&ProcessMediaFile,
        strutil::JoinPaths(FLAGS_input, media_files[i]),
        strutil::Dirname(strutil::JoinPaths(FLAGS_output_dir, media_files[i])),
        true));
  }
  tp.Stop();

  common::Exit(0);
}
