// Copyright (c) 2009, Whispersoft s.r.l.
// All rights reserved.
//
// Author: Mihai Ianculescu
//
//////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <whisperlib/common/base/common.h>
#include <whisperlib/common/base/errno.h>
#include <whisperlib/common/base/signal_handlers.h>
#include <whisperlib/common/base/system.h>
#include <whisperlib/common/base/re.h>
#include <whisperlib/common/base/gflags.h>

#include <whisperlib/common/io/ioutil.h>
#include <whisperlib/common/app/app.h>

#include <whisperlib/net/rpc/lib/codec/json/rpc_json_encoder.h>
#include <whisperlib/net/rpc/lib/rpc_util.h>

#include <whispermedialib/media/streaming.h>

#include <whispermedialib/media/pipeline.h>
#include <whispermedialib/media/pipeline_runner.h>

#include <whispermedialib/media/adapter_audio.h>
#include <whispermedialib/media/adapter_video.h>
#include <whispermedialib/media/encoder_audio.h>
#include <whispermedialib/media/encoder_video.h>
#include <whispermedialib/media/source_file.h>
#include <whispermedialib/media/sink_file.h>

#include <whisperstreamlib/flv/flv_joiner.h>
#include <whisperstreamlib/flv/flv_joiner.h>

#include "private/features/util/flv_thumbnailer.h"

#include "../auto/types.h"

//////////////////////////////////////////////////////////////////////

// our namespace
using namespace media;

DEFINE_string(transcode_input_dir, "",
              "The directory in which the transcoder will look "
              "for files to transcode.");
DEFINE_string(transcode_output_dir, "",
              "The directory in which the transcoder will put the "
              "transcoded files.");
DEFINE_string(postprocess_input_dir, "",
              "The directory in which the transcoder will look "
              "for files for postprocess.");
DEFINE_string(postprocess_output_dir, "",
              "The directory in which the transcoder will put "
              "the postprocessed files.");
DEFINE_string(include_mask, "\\.*$",
              "A regular expression used to select the actual "
              "source files' names.");
DEFINE_string(exclude_mask, "",
              "A regular expression used to exclude source files's names.");

DEFINE_string(format, "",
              "As: <unique suffix>:<encoder description>;...\n"
              "Defines the formats to transcode to.");

DEFINE_int32(wait_duration_ms, 1000,
             "The interval, in miliseconds, at which the transcoder "
             "will recheck the source "
             "directory for new files.");

DEFINE_bool(enable_multipass, false,
            "If true, a multipass encoding approach will be "
            "used, where it's the case.");

DEFINE_bool(enable_flv_postprocess, true,
            "Enables the post processing of FLV files."
            " The post process is done by flv_tag_joiner.");

DEFINE_bool(enable_flv_thumbnail, true,
            "Enables the generation of thumbnails for FLV files.");

DEFINE_int32(flv_thumbnail_start_ms,
             5000,
             "Extract the first thumbnail at this file time.");
DEFINE_int32(flv_thumbnail_repeat_ms,
             120000,
             "Extract thumbnails every so milliseconds.");
DEFINE_int32(flv_thumbnail_end_ms,
             kMaxInt32,
             "Stop extracting thumbnails at this time.");
DEFINE_int32(flv_thumbnail_quality,
             75,
             "Quality for thumbnail processing.");

DEFINE_int32(flv_thumbnail_width,
             0,
             "Width of the thumbnail (0 - use original if height not set, "
             "or use corresponding width to keep aspect ratio - if set)");
DEFINE_int32(flv_thumbnail_height,
             0,
             "Height of the thumbnail (0 - use original, if width not set, "
             "or use corresponding height to keep aspect ratio - if set)");

//////////////////////////////////////////////////////////////////////

class App : public app::App {
 public:
  App(int &argc, char **&argv) :
      app::App(argc, argv) {

    include_re_ = NULL;
    exclude_re_ = NULL;
    runner_ = NULL;

    current_step_ = 0;
    current_pipeline_ = 0;

    message_callback_ = NULL;

    google::SetUsageMessage("The Whispercast media transcoder.");
  }
  virtual ~App() {
    CHECK_NULL(message_callback_);

    CHECK_NULL(current_pipeline_);
    CHECK(current_step_ == 0);

    CHECK_NULL(runner_);
    CHECK_NULL(exclude_re_);
    CHECK_NULL(include_re_);
  }

 protected:
  void ComputeProgress() {
    if (current_pipeline_) {
      GstFormat fmt = GST_FORMAT_TIME;
      gint64 pos, len;
      float progress;

      GstElement *pipeline = GST_ELEMENT(current_pipeline_->pipeline());
      if (gst_element_query_position(pipeline, &fmt, &pos) &&
        gst_element_query_duration(pipeline, &fmt, &len)) {
        progress = (1.0f*pos)/len;

        for (std::map<std::string, Status>::iterator it = status_.begin();
          it != status_.end(); ++it) {
          if (it->second.step_ == current_step_) {
            it->second.duration_ = len/1000;
            it->second.progress_ =
              (1.0f*current_step_-1)/it->second.total_steps_ +
              progress/it->second.total_steps_;
          }
        }
      }
      if (!OutputStatus()) {
        current_pipeline_->Stop();
      }
    }
  }
  bool OutputStatus() {
    LOG(10) << "STATUS -----------------------------------------";

    // Create RPC FileTranscodingStatus structure
    //
    FileTranscodingStatus rpc_status;
    rpc_status.filename_in_ = current_source_;
    rpc_status.out_.ref().resize(status_.size());
    uint32 i = 0;
    for (std::map<std::string, Status>::iterator it = status_.begin();
         it != status_.end(); ++it, ++i) {

      LOG(10) << "STATUS [" << it->first << "]: (" <<
      it->second.width_ << "x" << it->second.height_ << ", " <<
      it->second.framerate_n_ << "/" << it->second.framerate_d_ << ", " <<
      it->second.filename_out_ << ") - " << it->second.progress_;

      const Status& status = it->second;
      rpc_status.out_.ref()[i] =
          OutFileTranscodingStatus(status.filename_out_,
                                   status.progress_,
                                   status.width_,
                                   status.height_,
                                   status.framerate_n_,
                                   status.framerate_d_,
                                   status.bitrate_,
                                   status.duration_,
                                   status.messages_,
                                   status.result_);
    }

    // Save RPC FileTranscodingStatus structure to a temporary file
    //
    /*
    char tmp_filename[] = "media_transcoder_XXXXXX";
    if ( ::mktemp(tmp_filename) == NULL ) {
      LOG_ERROR << "::mktemp failed: " << GetLastSystemErrorDescription();
      return;
    }
    */
    const string tmp_filename =
        strutil::StringPrintf("%s.status.%d",
                              current_source_.c_str(), ::getpid());
    io::Rm(tmp_filename); // just to make sure

    if ( !io::FileOutputStream::TryWriteFile(
             tmp_filename.c_str(),
             rpc::JsonEncoder::EncodeObject(rpc_status)) ) {
      LOG_ERROR << "Failed to write status file: [" << tmp_filename << "]"
                << ", error: " << GetLastSystemErrorDescription();
      return false;
    }
    // Move temporary file over regular file
    //
    const string status_filename = current_source_ + ".status";
    if ( !io::Rename(tmp_filename, status_filename, true) ) {
      LOG_ERROR << "Failed to overwrite status file: ["
                << status_filename << "]";
      return false;
    }
    return true;
  }
  std::string BuildPipeline() {
    PipelineDescription description;
    description
        << FileSource("source", NULL)
        << "decodebin name=d"
        << PipelineDescription::end;

    int encoders = 0;

    bool has_audio = false;
    bool has_video = false;

    int index = 0;
    for (std::map<std::string, EncoderDef>::iterator it = formats_.begin();
      it != formats_.end(); ++it,++index) {
      const string sink_name(strutil::StringPrintf("sink%d", index));

      std::map<std::string, Status>::iterator status_it =
        status_.find(it->first);
      CHECK(status_it != status_.end());

      bool has_encoder = false;
      if (current_step_ > status_it->second.total_steps_) {
        continue;
      }

      switch (it->second.type_) {
        case Stream::FLV:
        {
          has_audio =
          has_video = true;

          const string muxer_name(strutil::StringPrintf("flv%d", index));
          const string muxer(string("ffmux_flv name=") + muxer_name);

          description
              << muxer.c_str()
              << PipelineDescription::end
              << PipelineDescription::ref("vt")
              << "queue"
              << VideoAdapter()
              << VideoEncoder(NULL,
                              it->second.video_params_.type_,
                              it->second.video_params_.width_,
                              it->second.video_params_.height_,
                              it->second.video_params_.framerate_n_,
                              it->second.video_params_.framerate_d_,
                              it->second.video_params_.bitrate_,
                              it->second.video_params_.gop_size_,
                              it->second.video_params_.strict_rc_,
                              current_step_,
                              (status_it->second.filename_out_
                               + ".multipass").c_str(),
                              NULL)
              << PipelineDescription::ref(muxer_name.c_str())
              << PipelineDescription::end;

          description
              << PipelineDescription::ref("at")
              << "queue"
              << AudioAdapter()
              << AudioEncoder(NULL,
                              it->second.audio_params_.type_,
                              it->second.audio_params_.samplerate_,
                              it->second.audio_params_.samplewidth_,
                              it->second.audio_params_.bitrate_,
                              true,
                              NULL
                              )
              << PipelineDescription::ref(muxer_name.c_str())
              << PipelineDescription::end
              << PipelineDescription::ref(muxer_name.c_str())
              << FileSink(sink_name.c_str(), false, NULL)
              << PipelineDescription::end;
        }
        has_encoder = true;
        break;
      case Stream::MP3:
      case Stream::AAC:
        {
          has_audio = true;

          description
              << PipelineDescription::ref("at")
              << "queue"
              << AudioAdapter()
              << AudioEncoder(NULL,
                              it->second.audio_params_.type_,
                              it->second.audio_params_.samplerate_,
                              it->second.audio_params_.samplewidth_,
                              it->second.audio_params_.bitrate_,
                              false,
                              NULL
                              )
              << FileSink(sink_name.c_str(), false, NULL)
              << PipelineDescription::end;
        }
        has_encoder = true;
        break;
      default:
        ;
      }

      if (has_encoder) {
        status_it->second.type_ = it->second.type_;
        status_it->second.step_ = current_step_;
        encoders++;
      }
    }

    if (encoders > 0) {
      if (has_audio) {
        description
            << PipelineDescription::ref("d")
            << "queue"
            << "audioconvert"
            << "tee name=at"
            << PipelineDescription::end;
      }
      if (has_video) {
        description
            << PipelineDescription::ref("d")
            << "queue"
            << "ffmpegcolorspace ! ffdeinterlace"
            << "tee name=vt"
            << PipelineDescription::end;
      }
      return description.description();
    }
    return "";
  }

  int Initialize() {
    // initialize streaming (this will also call common::Init())
    media::Init(argc_, argv_);

    CHECK(io::IsDir(FLAGS_transcode_input_dir));
    CHECK(io::IsDir(FLAGS_transcode_output_dir));
    CHECK_NE(FLAGS_transcode_input_dir, FLAGS_transcode_output_dir);
    CHECK(io::IsDir(FLAGS_postprocess_input_dir));
    CHECK(io::IsDir(FLAGS_postprocess_output_dir));
    CHECK_NE(FLAGS_postprocess_input_dir, FLAGS_postprocess_output_dir);


    include_re_ = new re::RE(
        PrepareRegex(FLAGS_include_mask),
        REG_EXTENDED);
    exclude_re_ = new re::RE(
        PrepareRegex(
        FLAGS_exclude_mask+
        (FLAGS_exclude_mask.empty() ? "" : "|")+
        "(\\.(done|error|processing|multipass|status))$"),
        REG_EXTENDED);

    CHECK(!FLAGS_transcode_input_dir.empty());
    if (FLAGS_transcode_input_dir[FLAGS_transcode_input_dir.length()-1] != '/')
      FLAGS_transcode_input_dir += '/';

    if (FLAGS_transcode_output_dir.empty())
      FLAGS_transcode_output_dir = FLAGS_transcode_input_dir;

    if (FLAGS_transcode_output_dir[
            FLAGS_transcode_output_dir.length()-1] != '/')
      FLAGS_transcode_output_dir += '/';

    if ( FLAGS_postprocess_input_dir != "" &&
         !strutil::StrEndsWith(FLAGS_postprocess_input_dir, "/") )
      FLAGS_postprocess_input_dir += '/';

    if (FLAGS_postprocess_output_dir.empty())
      FLAGS_postprocess_output_dir = FLAGS_postprocess_input_dir;

    if (FLAGS_postprocess_output_dir[
            FLAGS_postprocess_output_dir.length() - 1] != '/')
      FLAGS_postprocess_output_dir += '/';

    CHECK(!FLAGS_format.empty());
    vector<string> formats;
    strutil::SplitBracketedString(
        FLAGS_format.c_str(), ';', '(', ')', &formats);
    for ( int i = 0; i < formats.size(); ++i ) {
      vector<string> elements;
      strutil::SplitBracketedString(
          formats[i].c_str(), ':', '(', ')', &elements);
      CHECK_EQ(elements.size(), 2);

      EncoderDef def;
      CHECK(ParseEncoderParameters(elements[1].c_str(),
        def.type_, def.audio_params_, def.video_params_,
        def.audio_port_, def.video_port_
      ));

      formats_[elements[0]] = def;
    }

    message_callback_ = NewPermanentCallback(this,
                                             &App::PipelineMessageCallback);

    runner_  = new PipelineRunner("Transcoder");
    return 0;
  }

  void Run() {
    CHECK(runner_->Start());

    for (bool run = true; run;) {
      run = runner_->Iterate(FLAGS_wait_duration_ms);
      m_mutex.Lock();
      if ( current_step_ != 0 ||
           runner_->state() != PipelineRunner::Running ) {
        ComputeProgress();
        m_mutex.Unlock();
        continue;
      }

      do {
        std::string source = FindSource(FLAGS_transcode_input_dir);
        if ( source != "" ) {
          LOG_DEBUG << "Queing [" << source
                    << "] into the transcoding process..";
          if (AddSource(source)) {
            LOG_DEBUG << "Queing [" << source << "] succeeded";
            break;
          }
          LOG_ERROR << "Queing [" << source << "] failed";
        }

        if ( FLAGS_postprocess_input_dir != "" ) {
          source = FindSource(FLAGS_postprocess_input_dir);
          if ( source != "" ) {
            current_source_ = strutil::JoinPaths(FLAGS_postprocess_input_dir,
                                                 source);
            LOG_DEBUG << "Postprocess START, file ["
                      << current_source_ << "]";

            const std::string out_file = strutil::JoinPaths(
                FLAGS_postprocess_output_dir,
                strutil::CutExtension(source) + ".flv");

            const std::string out_thumbnails = strutil::JoinPaths(
                FLAGS_postprocess_output_dir,
                strutil::CutExtension(source) + ".flv.thumbs");

            status_.clear();
            std::map<std::string, Status>::iterator status_it =
            status_.insert(std::make_pair("", Status())).first;

            status_it->second.filename_out_ = out_file;
            status_it->second.type_ = Stream::FLV;

            int64 duration = -1;

            const std::string current_source_tmp =
              current_source_ + ".processing";
            const std::string out_file_tmp = out_file + ".tmp";

            if (OutputStatus()) {

              if ( !io::Rename(current_source_, current_source_tmp, false) ||
                  !io::Mkdir(strutil::Dirname(out_file), true) ||
                  ((duration = Postprocess(current_source_tmp,
                                           out_file_tmp)) < 0)) {
                LOG_ERROR << "Postprocess ERROR, file ["
                          << current_source_ << "]";
                io::Rename(current_source_tmp,
                           current_source_+".error",
                           false);
                break;
              }
            } else {
                LOG_ERROR << "Postprocess ABORTED, file ["
                          << current_source_ << "]";
                io::Rename(current_source_tmp,
                           current_source_+".aborted",
                           false);
                break;
            }

            status_it->second.progress_ = 1.0f;
            status_it->second.duration_ = duration;
            if (OutputStatus()) {
              if ( !io::Rename(out_file_tmp, out_file, false) ||
               !io::Rename(current_source_tmp,
                      current_source_ + ".done", true) ) {
                LOG_ERROR << "Postprocess ERROR, file ["
                          << current_source_ << "]";
                io::Rename(current_source_tmp,
                           current_source_+".error",
                           false);
              } else {
                if (FLAGS_enable_flv_thumbnail) {
                  if (!io::Mkdir(out_thumbnails, true)) {
                    LOG_ERROR << "Thumbnail ERROR, file ["
                              << current_source_ << "], cannot create dir [" <<
                              out_thumbnails << "]";
                  } else
                  if (!Thumbnails(out_file, out_thumbnails)) {
                    LOG_ERROR << "Thumbnail ERROR, file ["
                              << current_source_ << "], failed";
                  } else {
                    LOG_DEBUG << "Thumbnails DONE, out file ["
                              << out_file << "]";
                  }
                }
                LOG_DEBUG << "Postprocess DONE, out file [" << out_file << "]";
              }
            } else {
              LOG_ERROR << "Postprocess ABORTED, file ["
                << current_source_ << "]";
              io::Rename(current_source_tmp, current_source_+".aborted", false);
            }
            break;
          }
        }

      } while ( false );

      m_mutex.Unlock();
    }
  }
  void Stop() {
    m_mutex.Lock();
    runner_->Stop();
    m_mutex.Unlock();
  }

  void Shutdown() {
    delete message_callback_;
    message_callback_ = NULL;

    delete runner_;
    runner_ = NULL;

    delete exclude_re_;
    exclude_re_ = NULL;
    delete include_re_;
    include_re_ = NULL;
  }

  std::string PrepareRegex(const std::string& s) {
    string sret(s);
    return sret;
  }

  std::string FindSource(const std::string& source_dir) {
    LOG_DEBUG << "Looking for new files in [" << source_dir << "]...";

    std::vector<std::string> sources;
    if ( !io::RecursiveListing(source_dir, &sources, include_re_) ) {
      LOG_WARNING << "Listing files in [" << source_dir << "] failed, ignoring...";
      return "";
    }

    include_re_->MatchEnd();

    while ( !sources.empty() ) {
      std::string source = sources.back();
      sources.pop_back();

      if (io::IsDir(source.c_str()))
        continue;

      source = source.substr(source_dir.length());

      exclude_re_->MatchEnd();
      if (exclude_re_->Matches(source))
        continue;

      if (blacklisted_.find(source) != blacklisted_.end()) {
        LOG_ERROR << "Source [" << source << "] has failed once, "
        "we will not process it again.";
        continue;
      }

      return source;
    }
    return "";
  }

  void PipelineMessageCallback(Pipeline *pipeline, Pipeline::Message::Kind message) {
    CHECK(pipeline == current_pipeline_);

    if (message == Pipeline::Message::Terminated) {
      const GError *error = pipeline->error();
      LOG_DEBUG << "Transcoding pipeline result: " << (error ? error->message : "");

      bool success = (error == NULL);
      if (success) {
        for (std::map<std::string, EncoderDef>::iterator it = formats_.begin();
          it != formats_.end(); ++it) {
          std::map<std::string, Status>::iterator status_it =
            status_.find(it->first);
          CHECK(status_it != status_.end());

          if (status_it->second.step_ == current_step_) {
            if (status_it->second.step_ == status_it->second.total_steps_) {
              string crt_filename = status_it->second.filename_out_ + ".tmp";
              const string& out_filename = status_it->second.filename_out_;
              if (status_it->second.type_ == Stream::FLV &&
                  FLAGS_enable_flv_postprocess) {
                const string tmp_filename = status_it->second.filename_out_ + ".join";
                int64 duration = Postprocess(crt_filename, tmp_filename);
                status_it->second.duration_ = duration;
                io::Rm(crt_filename);
                crt_filename = tmp_filename;
              }
              io::Rename(crt_filename, out_filename, true);
              status_it->second.progress_ = 1.0f;
            }
          }
        }
      }
      else {
        for (std::map<std::string, EncoderDef>::iterator it = formats_.begin(); it != formats_.end(); ++it) {
          std::map<std::string, Status>::iterator status_it =
            status_.find(it->first);
          CHECK(status_it != status_.end());

          if (status_it->second.step_ == current_step_) {
            unlink((status_it->second.filename_out_+".tmp").c_str());
          }
        }
      }

      runner_->RemovePipeline(pipeline);

      pipeline->Destroy();
      delete pipeline;

      current_pipeline_ = NULL;

      DonePipeline(success ? 0 : -1);
    }
  }

  bool AddSource(const std::string& source) {
    // NOTE:
    //  ! source is relative to FLAGS_transcode_input_dir
    //  ! source is something like: ..subdirs../file.flv
    // e.g.
    //  If FLAGS_transcode_input_dir = "/tmp/input/"
    //  - For file: "/tmp/input/media.flv"
    //    source = "media.flv"
    //  - For file: "/tmp/input/a/b/spiderman.flv"
    //    source = "a/b/spiderman.flv"

    std::string current = strutil::JoinPaths(FLAGS_transcode_input_dir, source);

    LOG_DEBUG << "Checking '" << current << "'...";
    if (rename(current.c_str(), (current +".processing").c_str()) != 0) {
      LOG_ERROR << "Couldn't rename '" << current << "'"
                   " to '" << current << ".processing'"
                   ", error: " << GetLastSystemErrorDescription()
                   << "; source will not be processed.";
      blacklisted_.insert(source);
      return false;
    }

    std::string match;
    CHECK(include_re_->MatchNext(source, &match));
    include_re_->MatchEnd();

    current_source_ = current;

    LOG_INFO << "Processing '" + current_source_ + "'...";

    // initialize the status
    status_.clear();
    for (std::map<std::string, EncoderDef>::iterator it = formats_.begin();
      it != formats_.end(); ++it) {

      string destination = strutil::JoinPaths(FLAGS_transcode_output_dir,
          strutil::CutExtension(source) + "." + it->first);

      std::string dirname = strutil::Dirname(destination);
      if (!dirname.empty()) {
        if (!io::CreateRecursiveDirs(dirname.c_str())) {
          LOG_ERROR << "Couldn't create the output directory '" << dirname <<
          "' for '" << current << "', source will not be processed.";

          blacklisted_.insert(source);
          return false;
        }
      }

      std::map<std::string, Status>::iterator status_it =
      status_.insert(std::make_pair(it->first, Status())).first;

      status_it->second.filename_out_ = destination;

      switch (it->second.type_) {
        case Stream::FLV:
          status_it->second.width_ = it->second.video_params_.width_;
          status_it->second.height_ = it->second.video_params_.height_;
          status_it->second.framerate_n_ =
            it->second.video_params_.framerate_n_;
          status_it->second.framerate_d_ =
            it->second.video_params_.framerate_d_;
          status_it->second.bitrate_ =
            it->second.video_params_.bitrate_;
          switch (it->second.video_params_.type_) {
            case Encoder::Video::VP6:
              status_it->second.total_steps_ = FLAGS_enable_multipass ? 2 : 1;
              break;
            case Encoder::Video::H264:
              status_it->second.total_steps_ = FLAGS_enable_multipass ? 3 : 1;
              break;
            default:
              status_it->second.total_steps_ = 1;
          }
          break;
        case Stream::MP3:
        case Stream::AAC:
          status_it->second.bitrate_ =
            it->second.audio_params_.bitrate_;
          status_it->second.total_steps_ = 1;
          break;
        default:
          ;
      }
    }

    CHECK(current_step_ == 0);
    if (ContinueSource()) {
      return true;
    }

    DoneSource(0);
    return false;
  }
  bool ContinueSource() {
    current_step_++;

    CHECK_NULL(current_pipeline_);
    std::string description = BuildPipeline();
    if (!description.empty()) {
      current_pipeline_ = new Pipeline("Transcoder");
      CHECK(current_pipeline_->Create(description.c_str()));

      runner_->AddPipeline(current_pipeline_,
          PipelineRunner::PipelineParams(0, 15000, message_callback_));

      GstElement *element =
        gst_bin_get_by_name(GST_BIN(current_pipeline_->pipeline()), "source");
      CHECK_NOT_NULL(element);

      g_object_set(element,
          "location", (current_source_+".processing").c_str(), NULL);
      gst_object_unref(element);

      int index = 0;
      for (std::map<std::string, EncoderDef>::iterator it = formats_.begin();
        it != formats_.end(); ++it,++index) {
        const string  sink_name(strutil::StringPrintf("sink%d", index));

        GstElement *element =
          gst_bin_get_by_name(
              GST_BIN(current_pipeline_->pipeline()), sink_name.c_str());
        if (element == NULL) {
          continue;
        }

        std::map<std::string, Status>::iterator status_it =
          status_.find(it->first);
        CHECK(status_it != status_.end());

        string output = (status_it->second.filename_out_+".tmp");

        g_object_set(element,
            "location", output.c_str(), NULL);
        gst_object_unref(element);

        LOG_INFO << "Output " << Stream::GetName(it->second.type_) <<
          " into '" << output << "', step " << current_step_;
      }

      if (OutputStatus()) {
        runner_->SyncPipeline(current_pipeline_);
        return true;
      }
    }
    return false;
  }

  void DonePipeline(int result) {
    if (result != 0) {
      DoneSource(result);
      return;
    }

    if (ContinueSource()) {
      return;
    }
    DoneSource(0);
  }
  void DoneSource(int result) {
    for (std::map<std::string, Status>::iterator it = status_.begin();
      it != status_.end(); ++it) {
      it->second.result_ = result;
      unlink((it->second.filename_out_+".multipass").c_str());
    }

    bool status_written = OutputStatus();
    if (status_written && (result == 0)) {
      LOG_INFO << "Processing of '" + current_source_ + "' SUCCEEDED...";
      rename((current_source_+".processing").c_str(),
             (current_source_+".done").c_str());
    }
    else
    if (status_written) {
      if (runner_->state() != PipelineRunner::Running) {
        LOG_ERROR << "Processing of '" + current_source_ + "' WAS ABORTED...";
        rename((current_source_+".processing").c_str(),
               current_source_.c_str());
      } else {
        LOG_ERROR << "Processing of '" + current_source_ + "' FAILED...";
        rename((current_source_+".processing").c_str(),
               (current_source_+".error").c_str());
      }
    }
    else {
      LOG_ERROR << "Processing of '" << current_source_
                << "' WAS PROBABLKY ABORTED BY CALLER...";
      rename((current_source_+".processing").c_str(),
             (current_source_+".aborted").c_str());
    }

    current_step_ = 0;
  }

  int64 Postprocess(const string& in_file, const string& out_file) {
    LOG_INFO << "Running JoinFlvFiles on: [" << in_file << "]";

    vector<string> input_files;
    input_files.push_back(in_file);

    int64 duration = streaming::JoinFlvFiles(input_files, out_file, 1, false);
    if (duration < 0) {
      LOG_ERROR << "JoinFlvFiles failed, for input file: [" << in_file << "]";
      return duration;
    }
    LOG_INFO << "JoinFlvFiles succeeded";
    return duration;
  }

  bool Thumbnails(const string& in_file, const string& out_dir) {
    LOG_INFO << " Generating thumbnail(s) for: [" << in_file << "]";
    if ( !util::FlvExtractThumbnails(
              in_file,
              FLAGS_flv_thumbnail_start_ms,
              FLAGS_flv_thumbnail_repeat_ms,
              FLAGS_flv_thumbnail_end_ms,
              FLAGS_flv_thumbnail_width,
              FLAGS_flv_thumbnail_height,
              FLAGS_flv_thumbnail_quality,
              out_dir) ) {
      LOG_ERROR << " Error processing thumbnails for file: "
                << in_file;
      return false;
    }
    LOG_INFO << "Thumbnail generation succeeded";
    return true;
  }

private:
  synch::Mutex m_mutex;

  re::RE* include_re_;
  re::RE* exclude_re_;
  PipelineRunner *runner_;
  Callback2<Pipeline*, Pipeline::Message::Kind> *message_callback_;

  struct EncoderDef {
    Stream::Type type_;
    Encoder::Audio::Params audio_params_;
    Encoder::Video::Params video_params_;
    int audio_port_;
    int video_port_;
  };
  std::map<std::string, EncoderDef> formats_;

  struct Status {
    string filename_out_;
    float progress_;

    int width_;
    int height_;

    int framerate_n_;
    int framerate_d_;

    float bitrate_;

    int64 duration_;

    std::vector<string> messages_;

    int result_;

    Stream::Type type_;

    int total_steps_;
    int step_;

    Status() :
      progress_(0),
      width_(-1),
      height_(-1),
      framerate_n_(-1),
      framerate_d_(-1),
      bitrate_(-1),
      duration_(-1),
      result_(0),
      type_(Stream::Unknown),
      total_steps_(0),
      step_(0) {
    }
  };
  std::map<std::string, Status> status_;

  std::string current_source_;
  int32 current_step_;
  Pipeline *current_pipeline_;

  std::set<string> blacklisted_;
};

int main (int argc, char *argv[]) {
  return common::Exit(App(argc, argv).Main());
}

///////////////////////////////////////////////////////////////////////////////
