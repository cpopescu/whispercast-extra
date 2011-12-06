// Copyright (c) 2009, Whispersoft s.r.l.
// All rights reserved.
//
// Author: Catalin Popescu
//
#include <stdio.h>
#include <whisperlib/common/base/date.h>
#include <whisperlib/common/base/scoped_ptr.h>

#include <whisperlib/common/sync/thread.h>
#include <whisperlib/common/sync/thread_pool.h>
#include <whisperlib/common/base/callback.h>
#include <whisperlib/common/io/ioutil.h>

#include <whisperstreamlib/base/joiner.h>
#include <whisperstreamlib/flv/flv_joiner.h>

#include <whisperlib/common/io/file/file_input_stream.h>


#include "whisperproc_util.h"

/////////////////////////////////////////////////////////////////////////////

DEFINE_string(content_type_file,
              ".content-type",
              "We try to read the content type from the given file."
              "Else, we assume default");
DEFINE_string(default_content_type,
              "video/x-flv",
              "We assume this content type for the files we join, if we "
              "cannot read it from a content type file");
DEFINE_int32(default_buildup_period_min,
             20,
             "With no external indication we buildup every these many "
             "minutes.");
DEFINE_int32(default_buildup_delay_min,
             10,
             "With no external indication we wait this long before starting"
             "buildup");

DEFINE_int32(min_buildup_period_min,
             2,
             "Minimum value for buildup period for buildup files..");

DEFINE_int32(max_buildup_period_min,
             180,
             "Don't buildup longer files then these");

DEFINE_int32(min_buildup_delay_min,
             2,
             "Minimum delay for buildup of published files");

DEFINE_int32(max_buildup_delay_min,
             60,
             "Don't wait more then this time before building up files");
DEFINE_int32(cue_ms_time,
             5000,
             "In joined files we create these many cue points..");
DEFINE_bool(delete_part_files,
            false,
            "If enabled, we delete the files that we joined.");
DEFINE_string(files_to_process,
              "part_.*",
              "Pattern for obtaining the files to process");
DEFINE_string(done_message_file,
              ".message_done",
              "We write a message in this file when processing is done");
DEFINE_string(processed_media_file,
              "media",
             "In each processing directory "
             "we create a processed file w/ this name");
DEFINE_string(files_suffix,
              ".part",
              "We have written  files w/ this suffix");
DEFINE_string(done_file,
              ".done",
              "We mark a directory that is done this file");

DEFINE_string(begin_file,
              ".save_done",
              "We start when we observe this file in a directory");
DEFINE_string(processing_file,
              ".processing",
              "We mark a directory that is under processing w/ this file");
DEFINE_string(base_media_dir,
              "",
              "Media to be processed is under this directory");

DEFINE_int32(look_for_fresh_alarm_sec,
             10,
             "We look for new materials every these many seconds.");
DEFINE_int32(max_num_threads,
             2,
             "We start at most these many new threads");

DEFINE_bool(autogenerate_thumbnails,
             true,
             "If this is on we automatically generate thumbnails for media "
             "we merge");
DEFINE_string(thumbnail_processing_dir,
              "",
              "Where we look for commands to process flvs "
              "for bitmap extraction");
DEFINE_string(thumbnail_output_dir,
              "",
              "Where we output the thumbnails.");
DEFINE_int32(thumbnail_processing_threads,
             2,
             "Processors for thumbnails");
DEFINE_int32(thumbnail_default_quality,
             75,
             "Default quality for thumbnail processing");

//////////////////////////////////////////////////////////////////////

namespace whisperproc {

bool PullAllPreparedBuildupFiles(const string& dir);
bool PrepareFilesForBuildup(thread::ThreadPool* pool,
                            string* dir,
                            string& building_dir,
                            string& processed_dir,
                            int32 buildup_delay_sec,
                            int32 buildup_period_sec);
void ProcessDir(string* dir,
                string* output_file);

string GetContentTypeForDir(const string& dir){
  string content_type = FLAGS_default_content_type;
  string filename = strutil::JoinPaths(dir, FLAGS_content_type_file);
  if ( !io::Exists(filename) ) {
    // This test is just to avoid the LOG_ERROR inside File::Open
    // when 'filename' does not exist.
    return content_type;
  }
  io::File* const infile = new io::File();
  if ( !infile->Open(filename.c_str(),
                     io::File::GENERIC_READ,
                     io::File::OPEN_EXISTING) ) {
    delete infile;
    return content_type;
  }
  io::FileInputStream fis(infile); // 'fis' auto deletes 'infile'
  fis.ReadString(&content_type);
  return content_type;
}

bool DirectoryBuildup(thread::ThreadPool* pool,
                      string* dir,
                      int32 buildup_delay_sec,
                      int32 buildup_period_sec) {
  string processed_dir = strutil::JoinPaths(*dir, "media");
  string building_dir = strutil::JoinPaths(*dir, "building");
  if ( !io::IsDir(building_dir) && !io::Mkdir(building_dir) ) {
    LOG_ERROR << " Error creating build dir: " << building_dir;
    delete dir;
    return false;
  }
  if ( !io::IsDir(processed_dir) && !io::Mkdir(processed_dir) ) {
    LOG_ERROR << " Error creating build dir: " << building_dir;
    delete dir;
    return false;
  }

  if ( io::IsReadableFile(strutil::NormalizePath(building_dir + "/" +
                                                 FLAGS_processing_file)) ) {
    delete dir;
    // still building - cannot start ..
    return false;
  }
  // Pull all the file files from building, just in case we died while
  // preparing the building dir
  PullAllPreparedBuildupFiles(*dir);

  // Determine which files to build - if any
  bool ret = PrepareFilesForBuildup(pool,
                                    dir, building_dir, processed_dir,
                                    buildup_delay_sec,
                                    buildup_period_sec);
  delete dir;
  return ret;
}

void TriggerProcessing(thread::ThreadPool* pool,
                       string* directory,
                       string* output_file) {
  LOG_INFO << "Starting to process: " << *directory;
  Closure* const thread_proc = NewCallback(&ProcessDir,
                                           directory, output_file);
  pool->jobs()->Put(thread_proc);
}

void MarkProcessed(const string& dir,
                   const string done_message) {
  const string processing_file(dir + "/" + FLAGS_processing_file);
  const string done_file(dir + "/" + FLAGS_done_file);
  if ( rename(processing_file.c_str(), done_file.c_str()) ) {
    LOG_ERROR << "Cannot rename the done file !!";
    return;
  }
  const string done_message_file = dir + "/" + FLAGS_done_message_file;
  if ( !io::FileOutputStream::TryWriteFile(
           done_message_file.c_str(),
           done_message) ) {
    LOG_ERROR << " Cannot create message file: " << done_message_file;
  }
}
bool ListFilesToProcess(const string& dir,
                        vector<string>* files) {
  re::RE regex(FLAGS_files_to_process);
  if ( !io::DirList(dir + "/", files, false, &regex) )
    return false;
  for ( int i = 0; i < files->size(); ++i ) {
    if ( !(*files)[i].empty() && (*files)[i][0] != '/' ) {
      (*files)[i] = strutil::JoinPaths(dir, (*files)[i]);
    }
  }
  return true;
}

bool PullAllPreparedBuildupFiles(const string& dir) {
  vector<string> to_move;
  const string building_dir(strutil::JoinPaths(dir, "building"));
  if ( !ListFilesToProcess(building_dir, &to_move) ) {
    LOG_INFO << " Cannot list " << building_dir;
    return false;
  }
  for ( int i = 0; i < to_move.size(); ++i ) {
    const string dest(strutil::JoinPaths(dir, strutil::Basename(to_move[i])));
    if ( rename(to_move[i].c_str(), dest.c_str()) ) {
      LOG_ERROR << " Error renaming: " << to_move[i] << " to " << dest;
    } else {
      LOG_INFO << " Pulled back from processing: " << to_move[i]
               << " => " << dest;
    }
  }
  return true;
}

bool PrepareFilesForBuildup(thread::ThreadPool* pool,
                            string* dir,
                            string& building_dir,
                            string& processed_dir,
                            int32 buildup_delay_sec,
                            int32 buildup_period_sec) {
  vector<string> to_process;
  if ( !ListFilesToProcess(*dir, &to_process) ) {
    LOG_INFO << " Cannot list " << dir;
    return false;
  }
  if ( to_process.empty() ) {
    return false;
  }
  map<int64, string> file_map;
  for ( int i = 0; i < to_process.size(); ++i ) {
    const string num(
        to_process[i].substr(
            to_process[i].size() - FLAGS_files_suffix.length() - 16,
            15));
    errno = 0;  // essential as strtol would not set a 0 errno
    const int64 crt_date = strtoll(num.c_str(), NULL, 10);
    if ( errno || crt_date <= 0 ) {
      LOG_INFO << "Error time detected in: " << to_process[i]
               << " num: " << num << " - skipping file";
    } else  {
      VLOG(LDEBUG) << " Candidates: " << crt_date << " -> " << to_process[i];
      file_map.insert(make_pair(crt_date, to_process[i]));
    }
  }
  // if ( file_map.size() < 2 ) {
  //  return false;
  // }
  // Now is time to look at what we got ..
  int64 start_time_ms = file_map.begin()->first;
  map<int64, string>::reverse_iterator itr = file_map.rbegin();
  itr++;
  const int64 now = timer::Date::Now();
  int64 end_time_ms = itr->first;
  if ( (end_time_ms - start_time_ms) < buildup_period_sec * 1000  &&
       (now - end_time_ms)  < buildup_period_sec * 1000  ) {
    LOG_DEBUG << " Not enough data built in: " << *dir
              << " - " << end_time_ms - start_time_ms
              << " vs: " << buildup_period_sec * 1000
              << " (real time passed: " << (now - end_time_ms);
    return false;
  }
  if ( (end_time_ms - start_time_ms) <
       ((buildup_period_sec + buildup_delay_sec) * 1000) &&
       (now - end_time_ms)  < (buildup_period_sec  + buildup_delay_sec) * 1000  ) {
    LOG_DEBUG << " Not enough delay data built in: " << *dir
              << " - " << end_time_ms - start_time_ms
              << " vs: " << (buildup_period_sec + buildup_delay_sec) * 1000;
    return false;
  }

  // Ok - we got something !!!
  vector<string> to_build;
  bool first = true;
  for ( map<int64, string>::const_iterator it = file_map.begin();
        it != file_map.end(); ++it ) {
    if ( (it->first - start_time_ms) > buildup_period_sec * 1000 ) {
      end_time_ms = it->first;
      break;
    }
    if ( !first && it->second[it->second.size() -
                              FLAGS_files_suffix.length() - 1] == 'N' ) {
      LOG_INFO << " Breaking up per new sequence @: " << it->second;
      end_time_ms = it->first;
      break;
    }
    first = false;
    to_build.push_back(it->second);
  }
  // Get some nice time string
  string start_time_str(
      timer::Date(start_time_ms, true).ToShortString());
  string end_time_str(
      timer::Date(end_time_ms, true).ToShortString());

  if ( !io::IsDir(building_dir) && !io::Mkdir(building_dir) ) {
    LOG_ERROR << " Error creating build dir: " << building_dir;
    return false;
  }
  // Prepare the output_file_name
  string *output_file = new string(
      strutil::JoinPaths(processed_dir,
                         FLAGS_processed_media_file +
                         "_" + start_time_str +
                         "_" + end_time_str));
  for ( int i = 0; i < to_build.size(); ++i ) {
    const string dest(strutil::JoinPaths(building_dir,
                                         strutil::Basename(to_build[i])));
    if ( rename(to_build[i].c_str(), dest.c_str()) ) {
      LOG_ERROR << " Cannot move file to build: "
                << to_build[i] << " => " << dest;
      delete output_file;
      return false;
    }
  }
  const string processing_file(
      strutil::JoinPaths(building_dir, FLAGS_processing_file));
  if ( !io::FileOutputStream::TryWriteFile(processing_file.c_str(),
                                           *output_file) ) {
    LOG_ERROR << " Cannot create processing file: " << processing_file;
    delete output_file;
    return false;
  }
  TriggerProcessing(pool, new string(building_dir), output_file);
  return true;
}

void ProcessDir(string* dir,
                string* output_file) {
  scoped_ptr<string> auto_del_dir(dir);
  scoped_ptr<string> auto_del_output_file(output_file);

  vector<string> files;
  if ( !ListFilesToProcess(*dir, &files) ||
       files.empty() ) {
    LOG_INFO << " Nothing to process in " << *dir;
    MarkProcessed(*dir, "No files to process\n");
    return;
  }

  std::sort(files.begin(),  files.end());

  const string content_type = GetContentTypeForDir(*dir);
  string extension = streaming::GetSmallTypeFromContentType(content_type);
  for ( int i = 0; i < extension.size(); ++i ) {
    if ( extension[i] == '/' ) {
      extension[i] = '.';
    }
  }
  string tmp_output = strutil::JoinPaths(
      *dir,
      string("joining.") + strutil::Basename(*output_file));

  LOG_DEBUG << "Preparing to join: \n" << strutil::JoinStrings(files, "\n")
            << "\nOutput in: " << tmp_output;

  int64 duration = streaming::JoinFlvFiles(files,
                                           tmp_output,
                                           FLAGS_cue_ms_time,
                                           false);
  if ( duration > 0 ) {
    if ( strutil::Basename(*output_file) != FLAGS_processed_media_file ) {
      size_t pos_uscore1 = output_file->rfind('_');
      size_t pos_uscore2 = output_file->rfind('_', pos_uscore1 - 1);
      timer::Date start_time(true);
      timer::Date spec_end_time(true);
      if ( start_time.SetFromShortString(
               output_file->substr(pos_uscore2 + 1,
                                   pos_uscore1 - pos_uscore2 - 1), true) &&
           spec_end_time.SetFromShortString(
               output_file->substr(pos_uscore1 + 1), true) ) {
        timer::Date end_time(start_time.GetTime() + duration, true);
        if ( ::llabs(end_time.GetTime() - spec_end_time.GetTime()) > 1000 ) {
          *output_file = output_file->substr(0, pos_uscore1 + 1) +
                         end_time.ToShortString();
        }
      }
    }
    if ( !io::Rename(tmp_output, (*output_file + "." + extension), true) ) {
      LOG_ERROR << " Cannot rename [" << tmp_output << "]"
                   " to [" << *output_file << "]";
    } else {
      LOG_INFO << *output_file << " finally written.";
    }
    if ( FLAGS_autogenerate_thumbnails ) {
      const string command_file(
          strutil::JoinPaths(
              FLAGS_thumbnail_processing_dir,
              strutil::Basename(*output_file) + ".cmd"));
      string command;
      if ( FLAGS_thumbnail_output_dir != "" ) {
        command = strutil::StringPrintf(
              "{ \"file_name_\": \"%s.%s\", \"jpeg_dir_\": \"%s\"}",
              output_file->c_str(), extension.c_str(),
              strutil::JoinPaths(
                  FLAGS_thumbnail_output_dir,
                  output_file->substr(FLAGS_base_media_dir.size())).c_str());
      } else {
        command = strutil::StringPrintf(
              "{ \"file_name_\": \"%s.%s\"}",
              output_file->c_str(), extension.c_str());
      }
      if ( !io::FileOutputStream::TryWriteFile(
               command_file.c_str(), command) ) {
        LOG_ERROR << " Cannot write jpeg thumbnail command file: "
                  << command_file;
      }
    }
    // rename file ..
    MarkProcessed(*dir, "OK\n");
  } else {
    MarkProcessed(*dir, "ERRORS JOINING\n");
  }
  if ( FLAGS_delete_part_files ) {
    for ( int i = 0; i < files.size(); ++i ) {
      LOG_INFO << "Erasing part file: " << files[i];
      io::Rm(files[i]);
    }
  } else {
    const string done_files_dir = strutil::JoinPaths(*dir, "done_files");
    if ( io::IsDir(done_files_dir) ||
         io::Mkdir(done_files_dir) ) {
      for ( int i = 0; i < files.size(); ++i ) {
        const string fname(
            strutil::JoinPaths(done_files_dir,
                               strutil::Basename(files[i])));
        if ( !io::Rename(files[i], fname, true) ) {
          LOG_ERROR << "Error renaming: [" << files[i] << "] => ["
                    << fname << "]";
        }
      }
    } else {
      LOG_ERROR << "Error creating dir: " << done_files_dir;
    }
  }
  LOG_INFO << "Done processing: " << *dir;
}

bool StartProcessing(thread::ThreadPool* pool,
                     const string& path) {
  if ( pool->jobs()->IsFull() ) {
    LOG_WARNING << " No space in the thread pool !";
    return false;
  }
  LOG_INFO << " --> Processing: [" << path << "]";
  string* directory = new string(strutil::Dirname(path.c_str()));
  const string processing_file(
      strutil::JoinPaths(*directory, FLAGS_processing_file));
  const string normal_path = strutil::NormalizePath(path);
  bool processing = (processing_file == normal_path);
  string* output_file = new string();
  if ( processing &&
       io::FileInputStream::TryReadFile(processing_file.c_str(),
                                        output_file) ) {
    LOG_INFO << " Relinkwish processing with output : " << output_file;
  } else {
    *output_file = strutil::JoinPaths(*directory, FLAGS_processed_media_file);
  }

  string done_content;
  if ( io::FileInputStream::TryReadFile(
           strutil::JoinPaths(*directory, FLAGS_begin_file).c_str(),
           &done_content) ) {
    if ( !done_content.empty() ) {
      vector<string> done_lines;
      strutil::SplitString(done_content, "\n", &done_lines);
      if ( done_lines.size() > 0 && done_lines[0] == "buildup" ) {
        int32 buildup_period_sec = FLAGS_default_buildup_period_min * 60;
        int32 buildup_delay_sec = FLAGS_default_buildup_delay_min * 60;

        if ( done_lines.size() > 1 ) {
          if ( !done_lines[1].empty() ) {
            buildup_period_sec = atoi(done_lines[1].c_str());
          }
          if ( done_lines.size() > 2 ) {
            if ( !done_lines[2].empty() ) {
              buildup_delay_sec = atoi(done_lines[2].c_str());
            }
          }
        }
        VLOG(LDEBUG) << " Buildup check on:  " << *directory
               << " buildup_period_sec: " << buildup_period_sec
               << " buildup_delay_sec: " << buildup_delay_sec
               << " done_content: [" << done_content << "]";
        buildup_period_sec = max(FLAGS_min_buildup_period_min * 60,
                                 min(FLAGS_max_buildup_period_min * 60,
                                     buildup_period_sec));
        buildup_delay_sec = max(FLAGS_min_buildup_delay_min * 60,
                                min(FLAGS_max_buildup_delay_min * 60,
                                    buildup_delay_sec));

        if ( !processing ) {
          delete output_file;
          DirectoryBuildup(pool,
                           directory,
                           buildup_delay_sec,
                           buildup_period_sec);
          return true;   // don't delay other guys because of us..
        }
      }
    }
  }
  if ( processing_file != normal_path ) {
    if ( rename(normal_path.c_str(), processing_file.c_str()) ) {
      LOG_ERROR << "Cannot rename: [" << normal_path << "] to ["
                << processing_file << "].";
      return true;
    }
  }
  TriggerProcessing(pool, directory, output_file);
  return true;
}

string PrepareRegex(const string& s) {
  string sret;
  for ( int i = 0; i < s.size(); i++ ) {
    if ( s[i] == '.' ) sret += '\\';
    sret += s[i];
  }
  sret += "$";
  DLOG_DEBUG << " regex: [" << sret << "] for: [" << s << "]";
  return sret;
}

PostProcessor::PostProcessor()
    : begin_file_re_(PrepareRegex(FLAGS_begin_file)),
      processing_re_(PrepareRegex(FLAGS_processing_file)),
      pool_(FLAGS_max_num_threads,
            FLAGS_max_num_threads + 1) {
}

PostProcessor::~PostProcessor() {
}

void PostProcessor::Start() {
  LOG_INFO << "Starting post - processor...";
  selector_thread_.Start();
  selector_thread_.mutable_selector()->RunInSelectLoop(
      NewCallback(this,
                  &PostProcessor::ProcessProcessing));
  selector_thread_.mutable_selector()->RunInSelectLoop(
      NewCallback(this,
                  &PostProcessor::ProcessFresh, true));
}

bool PostProcessor::FindProcessing(vector<string>* dirs) {
  return io::RecursiveListing(FLAGS_base_media_dir, dirs, &processing_re_);
}
bool PostProcessor::FindFresh(vector<string>* dirs) {
  return io::RecursiveListing(FLAGS_base_media_dir, dirs, &begin_file_re_);
}

void PostProcessor::ProcessProcessing() {
  VLOG(10) << "Looking for processing directories in: ["
          << FLAGS_base_media_dir << "].";
  vector<string> dirs;
  if ( !FindProcessing(&dirs) ) {
    LOG_ERROR << "Cannot list files under: "  << FLAGS_base_media_dir;
    return;
  }
  VLOG(10) << "Found #" << dirs.size() << " processing directories..";
  for ( int i = 0; i < dirs.size(); ++i ) {
    VLOG(10) << "StartProcessing old directory " << i << "/" << dirs.size()
            << " : [" << dirs[i] << "]";
    if ( !whisperproc::StartProcessing(&pool_, dirs[i]) ) {
      LOG_WARNING << " Delaying the processing of " << dirs[i]
                  << " and " << dirs.size() - i << " other files.";
      selector_thread_.mutable_selector()->RegisterAlarm(
          NewCallback(this,
                      &PostProcessor::ProcessProcessing),
          FLAGS_look_for_fresh_alarm_sec * 1000);
        break;
    }
  }
}

void PostProcessor::ProcessFresh(bool look_again) {
  VLOG(10) << "Looking for fresh directories in: ["
          << FLAGS_base_media_dir << "]";
  vector<string> dirs;
  if ( !FindFresh(&dirs) ) {
    LOG_ERROR << "Cannot list files under: "  << FLAGS_base_media_dir;
    return;
  }
  VLOG(10) << "Found #" << dirs.size() << " fresh directories..";
  for ( int i = 0; i < dirs.size(); ++i ) {
    VLOG(10) << "StartProcessing fresh directory " << i << "/" << dirs.size()
            << " : [" << dirs[i] << "]";
    if ( !whisperproc::StartProcessing(&pool_, dirs[i]) ) {
      LOG_WARNING << " Delaying the processing of " << dirs[i]
                  << " and " << dirs.size() - i << " other files.";
      break;
    }
  }
  if ( look_again ) {
    DLOG_DEBUG << " Scheduling again in: " << FLAGS_look_for_fresh_alarm_sec;
    selector_thread_.mutable_selector()->RegisterAlarm(
        NewCallback(this, &PostProcessor::ProcessFresh, true),
        FLAGS_look_for_fresh_alarm_sec * 1000);
  }
}


}
