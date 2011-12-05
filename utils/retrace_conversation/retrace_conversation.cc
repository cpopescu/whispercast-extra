// Copyright 2009 WhisperSoft s.r.l.
// Author: Catalin Popescu
//
// Collect your trace w. something like:
//   tcpdump -nn -s 0 -S -c 2000 -i eth0 -w /tmp/filedump "port 1935"
//
// tcpdump flags:
//    -nn - no DNS lookup
//    -s 0 - no limit on dumped packet size
//    -S - print absolute TCP numbers (not that important actually)
//    -c 2000 - collect only 2000 packets
//    -i eth0 - specify interface
//    -w /tmp/filedump - dump the trace to that file
//    "port 1935" - collect only TCP conversations on port 1935
//
// Then dump the TCP data sent from one of the sides w/
//   ./retrace_conversation --infile=/tmp/filedump
//                          --srcaddr=192.168.2.18
//                          --outfile=/tmp/sent_by_192.168.2.18
//
#include <whisperlib/common/base/types.h>
#include <whisperlib/common/base/log.h>
#include <whisperlib/common/base/system.h>
#include <whisperlib/common/base/gflags.h>
#include <whisperlib/common/base/errno.h>
#include <whisperlib/common/base/strutil.h>

#include <whisperlib/common/io/file/file.h>
#include <whisperlib/common/io/file/file_input_stream.h>
#include <whisperlib/common/io/file/file_output_stream.h>
#include <whisperlib/common/io/num_streaming.h>
#include <whisperlib/net/base/address.h>

//////////////////////////////////////////////////////////////////////

DEFINE_string(infile,
              "",
              "File to read");
DEFINE_string(srcaddr,
              "",
              "Source IP address to trace");
DEFINE_int32(srcport,
             0,
             "Source TCP port to trace");
DEFINE_int32(dstport,
             0,
             "Dest TCP port to trace");
DEFINE_string(outfile,
              "",
              "Where to write the contents set by conditions");
DEFINE_bool(use_positions,
            false,
            "Write position/size chunks to outfile.");

//////////////////////////////////////////////////////////////////////

static const int kTcpProtocol = 6;
common::ByteOrder g_pcap_bo = common::kByteOrder;

// Header of the tcpdump file
struct PCapHeader {
  uint32 magic_number;   // magic number
  uint16 version_major;  // major version number
  uint16 version_minor;  // minor version number
  int32  thiszone;       // GMT to local correction
  uint32 sigfigs;        // accuracy of timestamps
  uint32 snaplen;        // max length of captured packets, in octets
  uint32 network;        // data link type

  int Read(io::InputStream* in) {
    CHECK_GE(in->Readable(), 24);
    magic_number = io::IONumStreamer::ReadInt32(in, g_pcap_bo);
    version_major = io::IONumStreamer::ReadInt16(in, g_pcap_bo);
    version_minor = io::IONumStreamer::ReadInt16(in, g_pcap_bo);
    thiszone = io::IONumStreamer::ReadInt32(in, g_pcap_bo);
    sigfigs = io::IONumStreamer::ReadInt32(in, g_pcap_bo);
    snaplen = io::IONumStreamer::ReadInt32(in, g_pcap_bo);
    network = io::IONumStreamer::ReadInt32(in, g_pcap_bo);
    return 24;
  }
};

// IP and TCP headers
struct IpHeader {
  uint32 first;
  uint32 second;
  uint32 third;
  uint32 source_ip;
  uint32 dest_ip;

  int protocol;
  int Read(io::InputStream* in) {
    CHECK_GE(in->Readable(), 5 * sizeof(uint32));
    first = io::IONumStreamer::ReadInt32(in, common::BIGENDIAN);
    second = io::IONumStreamer::ReadInt32(in, common::BIGENDIAN);
    third = io::IONumStreamer::ReadInt32(in, common::BIGENDIAN);
    source_ip = io::IONumStreamer::ReadInt32(in, common::BIGENDIAN);
    dest_ip = io::IONumStreamer::ReadInt32(in, common::BIGENDIAN);
    protocol = ((third & 0x00ff0000) >> 16 ) & 0xff;
    return 5 * sizeof(uint32);
  }
};

struct TcpHeader {
  uint16 src_port;
  uint16 dest_port;
  uint32 second;
  uint32 third;
  uint32 fourth;
  uint32 fifth;
  int Read(io::InputStream* in) {
    CHECK_GE(in->Readable(), sizeof(uint32) * 5);
    src_port  = io::IONumStreamer::ReadInt16(in, common::BIGENDIAN);
    dest_port  = io::IONumStreamer::ReadInt16(in, common::BIGENDIAN);
    second = io::IONumStreamer::ReadInt32(in, common::BIGENDIAN);
    third = io::IONumStreamer::ReadInt32(in, common::BIGENDIAN);
    fourth = io::IONumStreamer::ReadInt32(in, common::BIGENDIAN);
    fifth = io::IONumStreamer::ReadInt32(in, common::BIGENDIAN);
    int header_len = (fourth >> 28) & 0x0f;

    CHECK_GE(in->Readable(), (header_len - 5) * sizeof(uint32));
    in->Skip((header_len - 5) * sizeof(uint32));
    return header_len * sizeof(uint32);
  }
};

struct LibcapEntry {
  // The libcap header
  uint32 ts_sec;         // timestamp seconds
  uint32 ts_usec;        // timestamp microseconds
  uint32 incl_len;       // number of octets of packet saved in file
  uint32 orig_len;       // actual length of packet
  // Ethernet
  char dst[6];           // destination hwaddr
  char src[6];           // src hwaddr
  uint16 frame_type;     // frametype. One of ETHERTYPE_ARP, ETHERTYPE_IP, etc
  IpHeader ip_header;
  TcpHeader tcp_header;
  char data[65536];      // 16384]; // min 64
  int data_len;

  int Read(io::InputStream* in) {
    CHECK_GE(in->Readable(), 4 * sizeof(uint32) + 12 + 2);
    ts_sec = io::IONumStreamer::ReadInt32(in, g_pcap_bo);
    ts_usec = io::IONumStreamer::ReadInt32(in, g_pcap_bo);
    incl_len = io::IONumStreamer::ReadInt32(in, g_pcap_bo);
    orig_len = io::IONumStreamer::ReadInt32(in, g_pcap_bo);

    CHECK_GE(in->Read(dst, sizeof(dst)), sizeof(dst));
    CHECK_GE(in->Read(src, sizeof(src)), sizeof(src));
    frame_type = io::IONumStreamer::ReadInt16(in, g_pcap_bo);
    if ( in->Readable() < incl_len ) {
      return -1;
    }
    CHECK_EQ(incl_len, orig_len);
    const int ip_len = ip_header.Read(in);
    data_len = 0;
    if ( ip_header.protocol == kTcpProtocol ) {
      const int tcp_len = tcp_header.Read(in);
      data_len = (incl_len - ip_len - tcp_len -
                  sizeof(dst) - sizeof(src) - sizeof(frame_type));
      if ( data_len > 0 ){
        CHECK_GE(in->Readable(), data_len);
        CHECK_GE(sizeof(data), data_len);
        in->Read(data, data_len);
      }
    } else {
      in->Skip(incl_len - ip_len -
               sizeof(dst) - sizeof(src) - sizeof(frame_type));
    }
    return data_len;
  }
};

/*
class TcpdumpFileReader {
 public:
  TcpdumpFileReader();
  ~TcpdumpFileReader();
  // Open a tcpdump file for read. Returns success.
  // If it's not a valid file, it returns false.
  bool Open(const string& filename);
  void Close();
  // Reads the next LibcapEntry from input file, and returns it in 'out'.
  // Returns: true on success, false on error/EOF.
  bool Read(LibcapEntry* out);
 private:
  File infile_;
  common::ByteOrder byte_order_;
};
*/

int main(int argc, char* argv[]) {
  common::Init(argc, argv);
  CHECK(!FLAGS_infile.empty()) << " Please provide an infile";
  CHECK(!FLAGS_outfile.empty()) << " Please provide an outfile";
  CHECK(FLAGS_srcport > 0 || FLAGS_dstport > 0)
    << "You must capture conversation one way only."
       " TCP packets from client & server are interleaved.";
  io::File* const infile = io::File::OpenFileOrDie(FLAGS_infile.c_str());
  int32 magic = 0;
  CHECK_EQ(infile->Read(&magic, sizeof(magic)), sizeof(magic));
  infile->SetPosition(0);
  CHECK(magic == 0xa1b2c3d4 || magic == 0xd4c3b2a1)
    << " Wrong magic: " << hex << magic << dec;
  g_pcap_bo = magic == 0xa1b2c3d4 ? common::kByteOrder
              : common::kByteOrder == common::LILENDIAN ? common::BIGENDIAN
              : common::LILENDIAN;

  io::FileInputStream fis(infile);
  net::IpAddress src_addr;
  if ( !FLAGS_srcaddr.empty() )
    src_addr = net::IpAddress(FLAGS_srcaddr.c_str());
  PCapHeader header;
  header.Read(&fis);
  io::FileOutputStream fout(io::File::CreateFileOrDie(FLAGS_outfile.c_str()));
  int64 total_bytes = 0;

  while ( !fis.IsEos() ) {
    LibcapEntry entry;
    if ( entry.Read(&fis) < 0 ) {
      break;
    }
    LOG_INFO << net::IpAddress(entry.ip_header.source_ip)
         << ":" << entry.tcp_header.src_port << " => "
             << net::IpAddress(entry.ip_header.dest_ip)
             << ":" << entry.tcp_header.dest_port;
    if ( entry.data_len > 0 &&
         entry.ip_header.protocol == kTcpProtocol &&
         ((entry.ip_header.source_ip == src_addr.ipv4() &&
           !src_addr.IsInvalid()) ||
          ((entry.tcp_header.src_port == FLAGS_srcport) &&
           FLAGS_srcport > 0) ||
          ((entry.tcp_header.dest_port == FLAGS_dstport) &&
           FLAGS_dstport > 0)) ) {
      LOG_INFO << " Writing packet containing " << entry.data_len << " bytes.";
      total_bytes += entry.data_len;
      if ( FLAGS_use_positions ) {
        const int64 position = infile->Position();
        const int32 size = entry.data_len;
        CHECK_EQ(fout.Write(&position, sizeof(position)), sizeof(position));
        CHECK_EQ(fout.Write(&size, sizeof(size)), sizeof(size));
      }
      CHECK_EQ(fout.Write(entry.data, entry.data_len),
               entry.data_len);
    }
  }
  LOG_INFO << " TOTAL: " << total_bytes << " bytes.";
}
