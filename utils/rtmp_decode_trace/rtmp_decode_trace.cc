// Copyright 2009 WhisperSoft s.r.l.
// Author: Catalin Popescu

// An utility used to decode a trace of an rtmp conversation. You can collect
// it w/ tcp dump and extract the specific data w/ retrace_conversation
// (See net/util/retrace_conversation.cc on how to do that).
// From the outfile you obtain, you can just run it through
// this utility.

#include <stdio.h>

#include <whisperlib/common/base/types.h>
#include <whisperlib/common/base/log.h>
#include <whisperlib/common/base/system.h>
#include <whisperlib/common/base/gflags.h>
#include <whisperlib/common/base/errno.h>
#include <whisperlib/common/base/strutil.h>
#include <whisperlib/common/base/scoped_ptr.h>

#include <whisperlib/common/io/file/file.h>
#include <whisperlib/common/io/file/file_input_stream.h>
#include <whisperlib/common/io/file/file_output_stream.h>
#include <whisperstreamlib/rtmp/objects/amf/amf0_util.h>
#include <whisperstreamlib/rtmp/objects/amf/amf_util.h>

#include <whisperstreamlib/rtmp/rtmp_protocol.h>
#include <whisperstreamlib/rtmp/rtmp_coder.h>
#include <whisperstreamlib/rtmp/rtmp_util.h>
#include <whisperstreamlib/rtmp/events/rtmp_event.h>
#include <whisperstreamlib/rtmp/events/rtmp_event_notify.h>

#include <whisperstreamlib/flv/flv_tag.h>
#include <whisperstreamlib/flv/flv_file_writer.h>
#include <whisperstreamlib/flv/flv_coder.h>

DEFINE_string(infile, "", "File to read");
DEFINE_bool(handshake, true, "Decode handshake before decoding events.");
DEFINE_string(outfile, "", "Write media to this output FLV file");
DEFINE_string(packetfile, "", "Write packets as C code in this file");
DEFINE_int32(skip, 0, "If set, we skip these many bytes from input."
                      " You may want to use --handshake=false.");
DEFINE_bool(use_positions, false, "Read positions/size chunks from infile.");

DEFINE_bool(log_handshake, false, "Log handshake data.");
DEFINE_bool(log_events, true, "Log rtmp events");
DEFINE_bool(log_tags, true, "Log flv tags extracted from rtmp events");
DEFINE_bool(log_tag_data, false, "Log binary media inside flv tags");

// Logging convention:
// - log tag binary data on LDEBUG
// - log tags on LINFO
// - log rtmp events on LWARNING
// - log rtmp handshake on LWARNING

string MakeCArray(const void* pbuffer, int size) {
  const uint8* buffer = reinterpret_cast<const uint8*>(pbuffer);
  string s;
  s.reserve(size * 8 + (size / 64) * 10);
  for ( int i = 0; i < size; i++ ) {
    if ( i % 64 == 0 ) {
      s += "\n";
    }
    s += strutil::StringPrintf("  0x%02x, ",
                               static_cast<int>(buffer[i] & 0xff));
  }
  return "{ \n" + s + "\n};\n";
}

class ChunkReader {
 public:
  explicit ChunkReader(io::File* infile)
    : infile_(infile),
      pos_(0) {
  }
  bool GetNext(io::MemoryStream* ms) {
    if ( !FLAGS_use_positions ) {
      int32 read = infile_->Read(ms, 1024);
      return read > 0;
    }

    if ( infile_->Read(&pos_, sizeof(pos_)) < sizeof(pos_) ) {
      return false;
    }
    int32 size = 0;
    if ( infile_->Read(&size, sizeof(size)) < sizeof(size) ) {
      return false;
    }
    return infile_->Read(ms, size) == size;
  }
  int64 pos() const { return pos_; }
 protected:
  io::File* infile_;
  int64 pos_;
};

int main(int argc, char* argv[]) {
  common::Init(argc, argv);
  CHECK(!FLAGS_infile.empty()) << "Please provide an infile";

  io::File infile;
  if ( !infile.Open(FLAGS_infile.c_str(),
                    io::File::GENERIC_READ,
                    io::File::OPEN_EXISTING) ) {
    LOG_ERROR << "Failed to open input file: [" << FLAGS_infile << "]";
    common::Exit(1);
  }

  // First skip the handshake part..
  infile.SetPosition(FLAGS_skip, io::File::FILE_SET);

  ChunkReader cr(&infile);
  io::MemoryStream decode_buffer;

  streaming::FlvFileWriter out(true, true);
  if ( !FLAGS_outfile.empty() &&
       !out.Open(FLAGS_outfile) ) {
    LOG_ERROR << "Failed to open output file: [" << FLAGS_outfile << "]";
    common::Exit(1);
  }

  io::FileOutputStream* packetfile = NULL;
  if ( !FLAGS_packetfile.empty() ) {
    packetfile = new io::FileOutputStream(
      io::File::CreateFileOrDie(FLAGS_packetfile.c_str()));
  }

  if ( FLAGS_handshake ) {
    // We always decode half of the handshake. The captured rtmp conversation
    // is one sided.
    while ( decode_buffer.Size() < 2 * rtmp::kHandshakeSize + 1 ) {
      CHECK(cr.GetNext(&decode_buffer));
    }
    int8 first_byte = io::NumStreamer::ReadByte(&decode_buffer);
    if ( first_byte != rtmp::kHandshakeLeadByte ) {
      LOG_FATAL << strutil::StringPrintf("Invalid first byte: 0x%02x"
          ", expected: 0x%02x", first_byte, rtmp::kHandshakeLeadByte);
    }
    string s1;
    decode_buffer.ReadString(&s1, rtmp::kHandshakeSize);
    string s2;
    decode_buffer.ReadString(&s2, rtmp::kHandshakeSize);
    if ( FLAGS_log_handshake ) {
      LOG_INFO << "=========================================================";
      LOG_INFO << "HANDSHAKE 1:";
      LOG_INFO << strutil::PrintableDataBufferHexa(s1.data(), s1.size());
      LOG_INFO << "=========================================================";
      LOG_INFO << "HANDSHAKE 2:";
      LOG_INFO << strutil::PrintableDataBufferHexa(s2.data(), s1.size());
    }
  }

  rtmp::ProtocolData protocol;
  rtmp::Coder coder(&protocol, 4 << 20);

  int32 size = 0;
  int64 timestamp = 0;
  int packet_num = 0;
  decode_buffer.MarkerSet();
  while ( true ) {
    const int32 start_decode_size = decode_buffer.Size();
    rtmp::Event* event = NULL;
    rtmp::AmfUtil::ReadStatus err = coder.Decode(
      &decode_buffer, rtmp::AmfUtil::AMF0_VERSION, &event);
    size += (start_decode_size - decode_buffer.Size());
    if ( err == rtmp::AmfUtil::READ_NO_DATA ) {
      if ( !cr.GetNext(&decode_buffer) ) {
        break;
      }
      continue;
    }
    if ( err != rtmp::AmfUtil::READ_OK ) {
      LOG_ERROR << "Decoding error: " << rtmp::AmfUtil::ReadStatusName(err)
                << " @ position: " << size
                << ". Maybe the 'handshake' flag was incorrect.";
      break;
    }
    CHECK_NOT_NULL(event);
    scoped_ptr<rtmp::Event> auto_del_event(event);

    if ( size > 0 ) {
      int32 initial_size = decode_buffer.Size();
      decode_buffer.MarkerRestore();
      CHECK_EQ(initial_size + size, decode_buffer.Size());
      string data;
      CHECK_EQ(decode_buffer.ReadString(&data, size), size);
      if ( packetfile ) {
        packetfile->WriteString(
          strutil::StringPrintf("// @%"PRId64" => %s\n",
                                (cr.pos()),
                                event->ToString().c_str()));
        packetfile->WriteString(
          strutil::StringPrintf("uint8 packet_%d[] = ", packet_num));
        packetfile->WriteString(MakeCArray(data.data(), data.size()));
      }
      CHECK_EQ(decode_buffer.Size(), initial_size);
      packet_num++;
      decode_buffer.MarkerSet();
      size = 0;
    }

    if ( event->header()->is_timestamp_relative() ) {
      timestamp += event->header()->timestamp_ms();
    } else {
      timestamp = event->header()->timestamp_ms();
    }

    if ( FLAGS_use_positions ) {
      LOG_WARNING << "@ " << cr.pos()
                  << " ===================================================";
    } else {
      LOG_WARNING << "=======================================================";
    }
    LOG_WARNING << "EVENT: " << event->ToString();
    if ( event->event_type() == rtmp::EVENT_CHUNK_SIZE ) {
      const int chunk_size =
        static_cast<rtmp::EventChunkSize*>(event)->chunk_size();
      protocol.set_read_chunk_size(chunk_size);
    }

    if ( event->event_type() == rtmp::EVENT_MEDIA_DATA ) {
      rtmp::BulkDataEvent* bd  = static_cast<rtmp::BulkDataEvent*>(event);
      rtmp::ProtocolData protocol2;
      while ( !bd->data().IsEmpty() ) {
        rtmp::Header* const header = new rtmp::Header(&protocol2);
        rtmp::AmfUtil::ReadStatus err2 = header->ReadMediaFromMemoryStream(
            bd->mutable_data(), rtmp::AmfUtil::AMF0_VERSION);
        if ( err2 != rtmp::AmfUtil::READ_OK ) {
          LOG_ERROR << "Error decoding header in media buffer w/ "
                    << bd->data().Size() << " left";
          delete header;
          break;
        }
        rtmp::Event* const event2 = rtmp::Coder::CreateEvent(header);
        if ( event2 == NULL ) {
          LOG_ERROR << "Error creating event from header: "
                    << header->ToString();
          delete header;
          break;
        }
        err2 = event2->ReadFromMemoryStream(bd->mutable_data(),
                                            rtmp::AmfUtil::AMF0_VERSION);
        if ( err2 != rtmp::AmfUtil::READ_OK ) {
          LOG_ERROR << "Error decoding event after header: "
                    << header->ToString();
          delete event2;
          break;
        }
        const int32 len = io::NumStreamer::ReadInt32(bd->mutable_data(),
                                                     common::BIGENDIAN);
        LOG_WARNING << "INNER EVENT, len: " << len
                    << " -- " << event2->ToString();
        delete event2;
      }
    }

    vector<scoped_ref<streaming::FlvTag> > tags;
    rtmp::ExtractFlvTags(*event, timestamp, &tags);
    for ( uint32 i = 0; i < tags.size(); i++ ) {
      // Log tag
      if ( FLAGS_log_tags ) {
        LOG_INFO << "Tag: " << tags[i]->ToString();
      }
      // Log tag data
      const io::MemoryStream* tag_data =
          tags[i]->body().type() == streaming::FLV_FRAMETYPE_AUDIO ?
              &tags[i]->audio_body().data() :
          tags[i]->body().type() == streaming::FLV_FRAMETYPE_VIDEO ?
              &tags[i]->video_body().data() : NULL;
      if ( FLAGS_log_tag_data && tag_data != NULL ) {
        LOG_DEBUG << "TagData: " << tag_data->DumpContentHex();
      }
      // Output to FLV file
      if ( out.IsOpen() ) {
        out.Write(tags[i], -1);
      }
    }
  }
  // Even if 'out' has not been Open, it's still ok to Close().
  out.Close();
  delete packetfile;
  return 0;
}
