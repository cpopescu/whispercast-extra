// Copyright (c) 2013, Whispersoft s.r.l.
// All rights reserved.
// Author: Cosmin Tudorache

// Converts a media file from one format to another. No transcoding.
// The input file is read using the whispercast's MediaFileReader.
// The output file is written using LibAV: avcodec & avformat libs.

#ifndef UINT64_C
#define UINT64_C(value) value ## ULL
#endif

extern "C" {
#pragma GCC diagnostic ignored "-Wall"
#include __AV_FORMAT_INCLUDE_FILE
#include __AV_CODEC_INCLUDE_FILE
#pragma GCC diagnostic warning "-Wall"
}

#include <whisperlib/common/base/types.h>
#include <whisperlib/common/base/log.h>
#include <whisperlib/common/base/gflags.h>
#include <whisperlib/common/base/errno.h>

#include <whisperstreamlib/base/media_file_reader.h>
#include <whisperstreamlib/base/media_file_writer.h>

//////////////////////////////////////////////////////////////////////

DEFINE_string(in,
              "",
              "The media file to read."
              " The file's extension determines the input format.");
DEFINE_string(out,
              "",
              "The output file to write."
              " The file's extension determines the output format.");

//////////////////////////////////////////////////////////////////////

char str_err[255] = {0,};
const char* av_err2str(int err) {
  av_strerror(err, str_err, sizeof(str_err));
  return str_err;
}
const char* CodecIDName(CodecID id) {
  AVCodec* a = avcodec_find_decoder(id);
  return a == NULL ? "Unknown" : a->name;
}

void DestructPacket(AVPacket* pkt) {
  delete pkt->data;
  pkt->data = NULL;
}

bool WriteTag(AVFormatContext *oc, AVStream *st, const streaming::Tag* tag,
    int64 ts, AVBitStreamFilterContext* filter) {
  if ( tag->Data() == NULL ) {
    LOG_WARNING << "Ignoring tag: " << tag->ToString();
    return true;
  }
  //LOG_WARNING << "WriteTag @" << ts << " " << tag->ToString()
  //            << " AVStream time_base: "
  //            << st->time_base.num << "/" << st->time_base.den;
  AVPacket pkt = {0}; // data and size must be 0;
  av_init_packet(&pkt);
  if ( tag->can_resync() ) {
    pkt.flags |= AV_PKT_FLAG_KEY;
  }
  pkt.pos = -1;
  pkt.stream_index = st->index;
  pkt.size = tag->Data()->Size();
  pkt.data = new uint8[pkt.size];
  tag->Data()->Peek(pkt.data, pkt.size);
  pkt.destruct = &DestructPacket;
  pkt.dts = ts * st->time_base.den / st->time_base.num / 1000;
  if ( tag->is_audio_tag() ) {
    pkt.pts = 0x8000000000000000LL;
  } else {
    pkt.pts = pkt.dts + tag->composition_offset_ms() * st->time_base.den / st->time_base.num / 1000;
  }
  pkt.duration = tag->duration_ms();

  // maybe apply a filter to this frame
  if ( filter != NULL ) {
    uint8_t* new_data = NULL;
    int new_size = 0;
    int result = av_bitstream_filter_filter(filter, st->codec, NULL,
                                            &new_data, &new_size,
                                            pkt.data, pkt.size,
                                            pkt.flags & AV_PKT_FLAG_KEY);
    if ( result < 0 ) {
      LOG_ERROR << "av_bitstream_filter_filter() failed, " << av_err2str(result);
      return false;
    }
    delete pkt.data;
    pkt.data = new_data;
    pkt.size = new_size;
  }

  //int ret = av_interleaved_write_frame(oc, &pkt);
  int ret = av_write_frame(oc, &pkt);
  if ( ret != 0 ) {
    LOG_ERROR << "av_interleaved_write_frame() failed: " << av_err2str(ret);
    return false;
  }
  return true;
}

AVStream* AddVideoStream(AVFormatContext *oc, const streaming::MediaInfo::Video& inf) {
  CodecID cid = CODEC_ID_NONE;
  switch ( inf.format_ ) {
    case streaming::MediaInfo::Video::FORMAT_H263: cid = CODEC_ID_H263; break;
    case streaming::MediaInfo::Video::FORMAT_H264: cid = CODEC_ID_H264; break;
    case streaming::MediaInfo::Video::FORMAT_ON_VP6: cid = CODEC_ID_VP6; break;
  };

  AVStream *video_st = avformat_new_stream(oc, 0);
  AVCodecContext *pcc = video_st->codec;
  avcodec_get_context_defaults3(pcc, NULL);
  pcc->codec_type = AVMEDIA_TYPE_VIDEO;
  pcc->codec_id = cid;
  pcc->width = inf.width_;
  pcc->height = inf.height_;
  pcc->time_base.num = 1;
  pcc->time_base.den = 1000;

  // set video codec extradata
  if ( inf.format_ == streaming::MediaInfo::Video::FORMAT_H264 &&
       !inf.h264_avcc_.empty() ) {
    video_st->codec->extradata_size = inf.h264_avcc_.size();
    video_st->codec->extradata = new uint8[video_st->codec->extradata_size + FF_INPUT_BUFFER_PADDING_SIZE];
    ::memcpy(video_st->codec->extradata,
             inf.h264_avcc_.c_str(),
             video_st->codec->extradata_size);
    video_st->codec->flags |= CODEC_FLAG_GLOBAL_HEADER;
  }

  return video_st;
}

AVStream* AddAudioStream(AVFormatContext *oc, const streaming::MediaInfo::Audio& inf) {
  CodecID cid = CODEC_ID_NONE;
  switch ( inf.format_ ) {
    case streaming::MediaInfo::Audio::FORMAT_AAC: cid = CODEC_ID_AAC; break;
    case streaming::MediaInfo::Audio::FORMAT_MP3: cid = CODEC_ID_MP3; break;
  }

  AVStream *audio_st = avformat_new_stream(oc, 0);
  AVCodecContext *pcc = audio_st->codec;
  avcodec_get_context_defaults3(pcc, NULL);
  pcc->codec_type = AVMEDIA_TYPE_AUDIO;
  pcc->codec_id = cid;
  pcc->time_base.num = 1;
  pcc->time_base.den = 1000;
  pcc->sample_rate = inf.sample_rate_;
  pcc->frame_size = 1024;
  pcc->sample_fmt = AV_SAMPLE_FMT_S16;
  pcc->channels = inf.channels_;

  // set audio codec extradata
  if ( inf.format_ == streaming::MediaInfo::Audio::FORMAT_AAC ) {
    audio_st->codec->extradata_size = 2;
    audio_st->codec->extradata = new uint8[2 + FF_INPUT_BUFFER_PADDING_SIZE];
    ::memcpy(audio_st->codec->extradata, inf.aac_config_, 2);
    audio_st->codec->flags |= CODEC_FLAG_GLOBAL_HEADER;
  }

  return audio_st;
}

#define STRINGISIZE(x) #x
#define STRINGISIZE2(x) "[ " STRINGISIZE(x) "] "

int IOWrite(void* opaque, uint8_t* buf, int buf_size) {
  io::File* writer = reinterpret_cast<io::File*>(opaque);
  return writer->Write(buf, buf_size);
}

int main(int argc, char* argv[]) {
  // Register all formats and codecs
  av_register_all();

  common::Init(argc, argv);
  LOG_WARNING << "Including avformat: " STRINGISIZE2(__AV_FORMAT_INCLUDE_FILE);
  LOG_WARNING << "Including avcodec: " STRINGISIZE2(__AV_CODEC_INCLUDE_FILE);
  if ( FLAGS_in.empty() ) {
    LOG_ERROR << "Please specify input file !";
    common::Exit(1);
  }
  if ( FLAGS_out.empty() ) {
    LOG_ERROR << "Please specify output file !";
    common::Exit(1);
  }

  streaming::MediaFileReader reader;
  if ( !reader.Open(FLAGS_in) ) {
    LOG_ERROR << "Cannot open input file: [" << FLAGS_in << "]";
    common::Exit(1);
  }

  /* auto detect the output format from the filename  */
  AVOutputFormat *fmt = av_guess_format(NULL, FLAGS_out.c_str(), NULL);
  if ( fmt == NULL ) {
    LOG_ERROR << "Could not deduce output format from file extension: " << FLAGS_out;
    common::Exit(1);
  }

  /* allocate the output media context */
  AVFormatContext *oc = avformat_alloc_context();
  if ( oc == NULL ) {
    LOG_ERROR << "avformat_alloc_context() failed";
    common::Exit(1);
  }
  oc->oformat = fmt;
  snprintf(oc->filename, sizeof(oc->filename), "%s", FLAGS_out.c_str());

  LOG_WARNING << "## Detected Output format"
                 ", video: " << CodecIDName(fmt->video_codec)
              << ", audio: " << CodecIDName(fmt->audio_codec);

  ////////////////////////
  // Create internal Buffer for FFmpeg:

  io::File writer;
  if ( !writer.Open(FLAGS_out, io::File::GENERIC_WRITE, io::File::CREATE_ALWAYS) ) {
    LOG_ERROR << "Cannot open output file: [" << FLAGS_out << "]"
                 ", error: " << GetLastSystemErrorDescription();
    common::Exit(1);
  }

  static const uint32 kIOBufSize = 32 * 1024;
  uint8* io_buf = (uint8*)av_malloc(kIOBufSize * sizeof(uint8));

  // Allocate the AVIOContext:
  // The fourth parameter (pStream) is a user parameter which will be passed to our callback functions
  oc->pb = avio_alloc_context(io_buf, kIOBufSize, // internal Buffer and its size
                              1,                  // bWriteable (1=true,0=false)
                              &writer,            // user data ; will be passed to our callback functions
                              NULL,               // Read callback, not needed
                              &IOWrite,           // Write callback
                              NULL);              // Seek Callback, not needed

  ////////////////////////
  /* open the output file, if needed */
  //if ( !(fmt->flags & AVFMT_NOFILE) ) {
  //  if ( avio_open(&oc->pb, FLAGS_out.c_str(), URL_WRONLY) < 0 ) {
  //    LOG_ERROR << "Could not open '" << FLAGS_out.c_str() << "'";
  //    common::Exit(1);
  //  }
  //}
  //////////////////////////

  AVStream* video_st = NULL;
  AVStream* audio_st = NULL;
  AVBitStreamFilterContext* video_filter = NULL;

  uint32 audio_tag_count = 0;
  uint32 video_tag_count = 0;

  while ( true ) {
    int64 timestamp_ms = 0;
    scoped_ref<streaming::Tag> tag;
    streaming::TagReadStatus result = reader.Read(&tag, &timestamp_ms);
    if ( result == streaming::READ_EOF ) {
      break;
    }
    if ( result == streaming::READ_SKIP ) {
      continue;
    }
    if ( result != streaming::READ_OK ) {
      LOG_ERROR << "Failed to read next tag, result: "
                << streaming::TagReadStatusName(result);
      common::Exit(1);
    }
    if ( tag->type() == streaming::Tag::TYPE_MEDIA_INFO ) {
      LOG_WARNING << "Setting MediaInfo: " << tag->ToString();
      const streaming::MediaInfo& info =
          static_cast<const streaming::MediaInfoTag*>(tag.get())->info();
      if ( info.has_video() ) {
        video_st = AddVideoStream(oc, info.video());
        if ( video_st == NULL ) {
          LOG_ERROR << "Could not allocate video stream";
          common::Exit(1);
        }
        fmt->video_codec = video_st->codec->codec_id;
        if ( info.video().format_ == streaming::MediaInfo::Video::FORMAT_H264 &&
             !info.video().h264_nalu_start_code_ ) {
          video_filter = av_bitstream_filter_init("h264_mp4toannexb");
          if ( video_filter == NULL ) {
            LOG_ERROR <<  "Cannot open the 'h264_mp4toannexb' filter";
            common::Exit(1);
          }
        }
      }
      if ( info.has_audio() ) {
        audio_st = AddAudioStream(oc, info.audio());
        if ( audio_st == NULL ) {
          LOG_ERROR << "Could not allocate audio stream";
          common::Exit(1);
        }
        fmt->audio_codec = audio_st->codec->codec_id;
      }

      av_dump_format(oc, 0, FLAGS_out.c_str(), 1);

      /* write the stream header, if any */
      avformat_write_header(oc, NULL);

      continue;
    }

    if ( tag->is_audio_tag() ) {
      if ( audio_st == NULL ) {
        LOG_ERROR << "No audio in media info, cannot serialize tag: " << tag->ToString();
        common::Exit(1);
      }

      if ( !WriteTag(oc, audio_st, tag.get(), timestamp_ms, NULL) ) {
        LOG_ERROR << "WriteTag() failed";
        common::Exit(1);
      }
      audio_tag_count++;
      continue;
    }
    if ( tag->is_video_tag() ) {
      if ( video_st == NULL ) {
        LOG_ERROR << "No video in media info, cannot serialize tag: " << tag->ToString();
        common::Exit(1);
      }
      if ( !WriteTag(oc, video_st, tag.get(), timestamp_ms, video_filter) ) {
        LOG_ERROR << "WriteTag() failed";
        common::Exit(1);
      }
      video_tag_count++;
      continue;
    }
    LOG_WARNING << "Ignoring tag: " << tag->ToString();
  }
  LOG_INFO << "EOF on input file. Total: " << audio_tag_count << " audio + "
           << video_tag_count << " video tags written.";

  //writer.Close();

  /* write the trailer, if any.  the trailer must be written
   * before you close the CodecContexts open when you wrote the
   * header; otherwise write_trailer may try to use memory that
   * was freed on av_codec_close() */
  av_write_trailer(oc);

  // free video filter
  if ( video_filter != NULL ) {
    av_bitstream_filter_close(video_filter);
    video_filter = NULL;
  }

  /* free the streams */
  for ( uint32 i = 0; i < oc->nb_streams; i++ ) {
    av_freep(&oc->streams[i]->codec);
    av_freep(&oc->streams[i]);
  }

  /////////////////////////////////////////////
  //if (!(fmt->flags & AVFMT_NOFILE)) {
  //  /* close the output file */
  //  avio_close(oc->pb);
  //}
  /////////////////////////////////////////////

  av_free(oc->pb);
  av_free(io_buf);
  av_free(oc);

  writer.Close();
  reader.Close();

  LOG_INFO << "Success.";
}
