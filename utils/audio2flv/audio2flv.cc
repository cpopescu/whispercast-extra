// Copyright (c) 2009, Whispersoft s.r.l.
// All rights reserved.
// Author: Catalin Popescu & Cosmin Tudorache

// Packs an audio file into a flv

#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>

#include <whisperlib/common/base/types.h>
#include <whisperlib/common/base/log.h>
#include <whisperlib/common/base/system.h>
#include <whisperlib/common/base/gflags.h>
#include <whisperlib/common/base/errno.h>

#include <whisperstreamlib/base/media_file_reader.h>
#include <whisperstreamlib/aac/aac_tag_splitter.h>
#include <whisperstreamlib/mp3/mp3_frame.h>
#include <whisperstreamlib/flv/flv_tag.h>
#include <whisperstreamlib/flv/flv_file_writer.h>
#include <whisperstreamlib/rtmp/objects/rtmp_objects.h>
#include <whisperstreamlib/rtmp/objects/amf/amf_util.h>

//////////////////////////////////////////////////////////////////////

DEFINE_string(in_path,
              "",
              "The audio file to read. Not modified.");
DEFINE_string(out_path,
              "",
              "The output file.");


static const int kFlushSize = 1<<20;
static const int kDefaultBufferSize = 1 << 18;

//////////////////////////////////////////////////////////////////////

// (2*K)    027 - key
// (2*K + 1)027 - inter
static const uint8 kVP6_KEYFRAME[] = {
  0x14,   0x00,   0x78,   0x46,   0x02,   0x02,   0x02,   0x02,
  0x3f,   0x6a,   0xf9,   0x1f,   0x00,   0x08,   0x9f,   0x40,
  0x10,   0x9f,   0xe6,   0x8f,   0x7f,   0xeb,   0x03,
  0x7f,   0x57,   0x36,   0x00,   0x00,
};

static const uint8 kVP6_INTRAFRAME[] = {
  0x24,   0x00,   0xf8,   0x40,   0x00,   0x00,   0x05,   0x10,   0x00,
};
uint32 next_video_time_ = 27;
int32 next_video_id_ = 0;

void OutputBlankVideo(streaming::FlvFileWriter& writer) {
  scoped_ref<streaming::FlvTag> blank = new streaming::FlvTag(
      0, streaming::kDefaultFlavourMask, 0, streaming::FLV_FRAMETYPE_VIDEO);
  io::MemoryStream data;
  if ( (next_video_id_ % 2) == 0 ) {
    data.Write(kVP6_KEYFRAME, sizeof(kVP6_KEYFRAME));
  } else {
    data.Write(kVP6_INTRAFRAME, sizeof(kVP6_INTRAFRAME));
  }
  streaming::TagReadStatus result =
      blank->mutable_video_body().Decode(data, data.Size());
  CHECK_EQ(result, streaming::READ_OK);
  writer.Write(blank, next_video_time_);
}

//////////////////////////////////////////////////////////////////////

uint8 GetMp3HeaderByte(const streaming::Mp3FrameTag* tag) {
  uint8 mask = 0;
  if ( tag->channel_mode() == streaming::Mp3FrameTag::SINGLE_CHANNEL ) {
    mask |= streaming::FLV_FLAG_SOUND_TYPE_MONO;
  } else {
    mask |= streaming::FLV_FLAG_SOUND_TYPE_STEREO;
  }
  mask |= (streaming::FLV_FLAG_SOUND_SIZE_16_BIT << 1);
  switch ( tag->sampling_rate_hz() ) {
    case    44100: mask |= (streaming::FLV_FLAG_SOUND_RATE_44_KHZ << 2); break;
    case    22050: mask |= (streaming::FLV_FLAG_SOUND_RATE_22_KHZ << 2); break;
    case    11025: mask |= (streaming::FLV_FLAG_SOUND_RATE_11_KHZ << 2); break;
  }
  mask |= (streaming::FLV_FLAG_SOUND_FORMAT_MP3 << 4);
  return mask;
}

void OutputMp3Headings(const streaming::Mp3FrameTag* tag,
                       streaming::FlvFileWriter& writer) {
  scoped_ref<streaming::FlvTag> meta_head(new streaming::FlvTag(
      0,streaming::kDefaultFlavourMask,0,streaming::FLV_FRAMETYPE_METADATA));
  streaming::FlvTag::Metadata& metadata = meta_head->mutable_metadata_body();

  metadata.mutable_name()->set_value(streaming::kOnMetaData);
  metadata.mutable_values()->Set("audiocodecid", new rtmp::CNumber(
      streaming::FLV_FLAG_SOUND_FORMAT_MP3));
  metadata.mutable_values()->Set("audiosamplerate", new rtmp::CNumber(
      tag->sampling_rate_hz()));
  metadata.mutable_values()->Set("audiosamplesize", new rtmp::CNumber(16));
  metadata.mutable_values()->Set("stereo", new rtmp::CBoolean(
      tag->channel_mode() == streaming::Mp3FrameTag::SINGLE_CHANNEL
      ? true : false));
  metadata.mutable_values()->Set("framerate", new rtmp::CNumber(1));
  metadata.mutable_values()->Set("height", new rtmp::CNumber(32));
  metadata.mutable_values()->Set("videocodecid", new rtmp::CNumber(4));
  metadata.mutable_values()->Set("videodatarate", new rtmp::CNumber(600));
  metadata.mutable_values()->Set("width", new rtmp::CNumber(32));
  writer.Write(meta_head, -1);
}

void OutputMp3Tag(const streaming::Mp3FrameTag* tag,
                  streaming::FlvFileWriter& writer) {
  if ( tag->timestamp_ms() > next_video_time_ ) {
    OutputBlankVideo(writer);
    next_video_time_ += 1000;
    next_video_id_++;
  }

  io::MemoryStream data;
  io::NumStreamer::WriteByte(&data, GetMp3HeaderByte(tag));
  data.AppendStreamNonDestructive(&tag->data());

  scoped_ref<streaming::FlvTag> frame(new streaming::FlvTag(
        0, streaming::kDefaultFlavourMask, 0, streaming::FLV_FRAMETYPE_AUDIO));
  streaming::TagReadStatus result =
      frame->mutable_audio_body().Decode(data, data.Size());
  CHECK_EQ(result, streaming::READ_OK);

  writer.Write(frame, tag->timestamp_ms());
}

//////////////////////////////////////////////////////////////////////

uint8 GetAacHeaderByte(const streaming::AacFrameTag* tag) {
  uint8 mask = 0;
  if ( tag->channels() == 2 ) {
    mask |= streaming::FLV_FLAG_SOUND_TYPE_STEREO;
  } else {
    mask |= streaming::FLV_FLAG_SOUND_TYPE_MONO;
  }
  mask |= (streaming::FLV_FLAG_SOUND_SIZE_16_BIT << 1);
  switch ( tag->samplerate() ) {
    case    44100: mask |= (streaming::FLV_FLAG_SOUND_RATE_44_KHZ << 2); break;
    case    22050: mask |= (streaming::FLV_FLAG_SOUND_RATE_22_KHZ << 2); break;
    case    11025: mask |= (streaming::FLV_FLAG_SOUND_RATE_11_KHZ << 2); break;
  }
  mask |= (streaming::FLV_FLAG_SOUND_FORMAT_AAC << 4);
  return mask;
}

void OutputAacHeadings(const streaming::AacFrameTag* tag,
                       streaming::FlvFileWriter& writer) {
  rtmp::CMixedMap values;
  values.Set("audiocodecid", new rtmp::CNumber(
      streaming::FLV_FLAG_SOUND_FORMAT_AAC));
  values.Set("audiosamplerate", new rtmp::CNumber(tag->samplerate()));
  values.Set("audiosamplesize", new rtmp::CNumber(16));
  if ( tag->channels() == 2) {
    values.Set("stereo", new rtmp::CBoolean(true));
  }
  values.Set("framerate", new rtmp::CNumber(1));
  values.Set("height", new rtmp::CNumber(32));
  values.Set("videocodecid", new rtmp::CNumber(7));
  values.Set("videodatarate", new rtmp::CNumber(0));
  values.Set("width", new rtmp::CNumber(32));

  writer.Write(*scoped_ref<streaming::FlvTag>(new streaming::FlvTag(
      0, streaming::kDefaultFlavourMask, 0,
      new streaming::FlvTag::Metadata(streaming::kOnMetaData, values))), -1);

  io::MemoryStream frame;
  io::NumStreamer::WriteByte(&frame, GetAacHeaderByte(tag));
  io::NumStreamer::WriteByte(&frame, 0x00);  // header

  // AAC Main           : start from  binary 00001 0000 0000 000
  // AAC Low complexity : start from  binary 00010 0000 0000 000
  // AAC SSR            : start from  binary 00011 0000 0000 000
  // AAC Long term pred.: start from  binary 00100 0000 0000 000
  // AAC HE             : start from  binary 00101 0000 0000 000

  // We assume AAC MAIN -- which is what we have
  // - can we obtain it from some place ?
  uint16_t pattern = 0x0800;     // AAC MAIN..
  // int16_t pattern = 0x2800;        // AAC HE..
  // uint16_t pattern = 0x1000;     // AAC LC..
  switch ( tag->samplerate() / 2 ) {   // This is how it is at FLV
    case 96000: pattern |= (0 << 7); break;
    case 88200: pattern |= (1 << 7); break;
    case 64000: pattern |= (2 << 7); break;
    case 48000: pattern |= (3 << 7); break;
    case 44100: pattern |= (4 << 7); break;
    case 32000: pattern |= (5 << 7); break;
    case 24000: pattern |= (6 << 7); break;
    case 22050: pattern |= (7 << 7); break;
    case 16000: pattern |= (8 << 7); break;
    case 12000: pattern |= (9 << 7); break;
    case 11025: pattern |= (10 << 7); break;
    case 8000 : pattern |= (11 << 7); break;
    case 7350 : pattern |= (12 << 7); break;
  }
  if ( tag->channels() == 2) {
    pattern |= (2 << 3);
  }
  io::NumStreamer::WriteUInt16(&frame, pattern, common::BIGENDIAN);

  scoped_ref<streaming::FlvTag> flv_tag(new streaming::FlvTag(
        0, streaming::kDefaultFlavourMask, 0, streaming::FLV_FRAMETYPE_AUDIO));
  streaming::TagReadStatus result =
      flv_tag->mutable_audio_body().Decode(frame, frame.Size());
  CHECK_EQ(result, streaming::READ_OK);

  writer.Write(flv_tag, tag->timestamp_ms());
}

void OutputAacTag(const streaming::AacFrameTag* tag,
                  streaming::FlvFileWriter& writer) {
  if ( tag->timestamp_ms() > next_video_time_ ) {
    OutputBlankVideo(writer);
    next_video_time_ += 1000;
    next_video_id_++;
  }

  io::MemoryStream frame;
  io::NumStreamer::WriteByte(&frame, GetAacHeaderByte(tag));
  io::NumStreamer::WriteByte(&frame, 0x01);  // raw data
  frame.AppendStreamNonDestructive(&tag->data());

  scoped_ref<streaming::FlvTag> flv_tag(new streaming::FlvTag(
        0, streaming::kDefaultFlavourMask, 0, streaming::FLV_FRAMETYPE_AUDIO));
  streaming::TagReadStatus result =
      flv_tag->mutable_audio_body().Decode(frame, frame.Size());
  CHECK_EQ(result, streaming::READ_OK);

  writer.Write(flv_tag, tag->timestamp_ms());
}

//////////////////////////////////////////////////////////////////////

int main(int argc, char* argv[]) {
  common::Init(argc, argv);
  CHECK(!FLAGS_in_path.empty()) << " Please specify input file !";
  CHECK(!FLAGS_out_path.empty()) << " Please specify output file !";
  streaming::MediaFileReader reader;
  if ( !reader.Open(FLAGS_in_path) ) {
    LOG_ERROR << "Cannot open input file: [" << FLAGS_in_path << "]";
    common::Exit(1);
  }
  streaming::FlvFileWriter writer(true, true);
  if ( !writer.Open(FLAGS_out_path) ) {
    LOG_ERROR << "Cannot open output file: [" << FLAGS_out_path << "]";
    common::Exit(1);
  }

  bool is_first_tag = false;
  while ( true ) {
    int64 timestamp_ms;
    scoped_ref<streaming::Tag> tag;
    streaming::TagReadStatus result = reader.Read(&tag, &timestamp_ms);
    if ( result == streaming::READ_EOF ) {
      break;
    }
    if ( result != streaming::READ_OK ) {
      LOG_FATAL << "Failed to read next tag, result: "
                << streaming::TagReadStatusName(result);
      common::Exit(1);
    }

    // Process tag
    if ( tag->type() == streaming::Tag::TYPE_AAC ) {
      const streaming::AacFrameTag* aac_tag =
          static_cast<const streaming::AacFrameTag*>(tag.get());
      if ( is_first_tag ) {
        OutputAacHeadings(aac_tag, writer);
        is_first_tag = false;
      }
      if ( !aac_tag->data().IsEmpty() ) {
        OutputAacTag(aac_tag, writer);
      }
    }
    if ( tag->type() == streaming::Tag::TYPE_MP3 ) {
      const streaming::Mp3FrameTag* mp3_tag =
          static_cast<const streaming::Mp3FrameTag*>(tag.get());
      if ( is_first_tag ) {
        OutputMp3Headings(mp3_tag, writer);
        is_first_tag = false;
      }
      if ( !mp3_tag->data().IsEmpty() ) {
        OutputMp3Tag(mp3_tag, writer);
      }
    }
  }
  writer.Close();

  LOG(INFO) << "STATISTICS:\n" << reader.splitter()->stats().ToString();
}
