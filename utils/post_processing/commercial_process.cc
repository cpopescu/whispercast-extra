// Copyright 2009 WhisperSoft s.r.l.
// Author: Catalin Popescu & Cosmin Tudorache
//
// ./commercial_process
//      --infile=../media/www/media/Carrots\ Promo\ lung.720x480.500kbps.h264.flv
//      --outfile=../media/www/media/carrots-promo-long.720x480.500kbps.h264.flv
//      --overlay_url=http://carrots.ro/
//      --alsologtostderr
//
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
#include <whisperstreamlib/flv/flv_tag.h>
#include <whisperstreamlib/flv/flv_file_writer.h>
#include <whisperlib/common/io/buffer/memory_stream.h>

#include <whisperstreamlib/rtmp/objects/rtmp_objects.h>
#include <whisperstreamlib/rtmp/objects/amf/amf_util.h>
#include "osd/base/osd_tag.h"
#include "osd/base/osd_to_flv_encoder.h"

//////////////////////////////////////////////////////////////////////

DEFINE_string(infile, "", "Input file, not modified.");
DEFINE_string(outfile, "", "Output file.");
DEFINE_bool(erase_duration, true,
            "If on, we erase the duration from the metadata.");
DEFINE_string(overlay_url, "",
              "You can set here a url as an overlay for the comercial.");

//////////////////////////////////////////////////////////////////////

static const int kDefaultBufferSize = 2048;
int main(int argc, char* argv[]) {
  common::Init(argc, argv);

  streaming::MediaFileReader reader;
  if ( !reader.Open(FLAGS_infile) ) {
    LOG_ERROR << "Cannot open input file: [" << FLAGS_infile << "]";
    common::Exit(1);
  }
  if ( reader.splitter()->media_format() != streaming::MFORMAT_FLV ) {
    LOG_ERROR << "Input file: [" << FLAGS_infile << "] is not a FLV file"
                 ", splitter: " << reader.splitter()->media_format_name();
    common::Exit(1);
  }
  int64 timestamp_ms;
  scoped_ref<streaming::Tag> header_tag;
  if ( reader.Read(&header_tag, &timestamp_ms) != streaming::READ_OK ) {
    LOG_ERROR << "Failed to read header tag";
    common::Exit(1);
  }
  if ( header_tag->type() != streaming::Tag::TYPE_FLV_HEADER ) {
    LOG_ERROR << "First tag is not the header: " << header_tag->ToString();
    common::Exit(1);
  }
  const streaming::FlvHeader& header =
      static_cast<const streaming::FlvHeader&>(*header_tag.get());

  streaming::FlvFileWriter writer(header.has_video(), header.has_audio());
  if ( !writer.Open(FLAGS_outfile) ) {
    LOG_ERROR << "Cannot open output file: [" << FLAGS_outfile << "]";
    common::Exit(1);
  }

  while ( true ) {
    int64 timestamp_ms;
    scoped_ref<streaming::Tag> tag;
    streaming::TagReadStatus err = reader.Read(&tag, &timestamp_ms);
    if ( err == streaming::READ_EOF ) {
      break;
    }
    if ( err != streaming::READ_OK ) {
      LOG_ERROR << "Error reading next tag: "
                << streaming::TagReadStatusName(err);
      return 1;
    }

    // process tag
    if ( tag->type() != streaming::Tag::TYPE_FLV ) {
      LOG_ERROR << "Unexpected tag: " << tag->ToString();
      continue;
    }
    streaming::FlvTag* flv_tag =
        const_cast<streaming::FlvTag*>(
            static_cast<const streaming::FlvTag*>(tag.get()));
    bool insert_overlay = false;
    if ( flv_tag->body().type() == streaming::FLV_FRAMETYPE_METADATA ) {
      streaming::FlvTag::Metadata& metadata = flv_tag->mutable_metadata_body();
      if ( metadata.name().value() == streaming::kOnMetaData ) {
        insert_overlay = true;
        if ( FLAGS_erase_duration ) {
          metadata.mutable_values()->Erase("duration");
        }
      }
    }
    writer.Write(*flv_tag, -1);

    if ( insert_overlay && !FLAGS_overlay_url.empty() ) {
      CreateOverlayParams params;
      params.id = "preloaded_commercial";
      string s = strutil::StringPrintf(
          "<a href=\"%s\" target=\"_blank\"><font size=\"120\">",
          FLAGS_overlay_url.c_str());
      for ( int i = 0; i < 100;  ++i ) {
        s += "&nbsp;-------------------------------------------------------------------------------------------------------<br/>";
      }
      s += "</font></a>";
      params.content = s;
      params.fore_color = PlayerColor("#000000", 0.0);
      params.back_color = PlayerColor("#000000", 0.0);
      params.link_color = PlayerColor("#000000", 0.0);
      params.hover_color = PlayerColor("#000000", 0.0);
      params.x_location = 0;
      params.y_location = 0;
      params.width = 1;
      params.show = true;
      scoped_ref<streaming::OsdTag> overlay =
          new streaming::OsdTag(0, streaming::kDefaultFlavourMask,
                                streaming::OsdTag::CREATE_OVERLAY, params);
      scoped_ref<streaming::FlvTag> flv_osd_tag =
          streaming::OsdToFlvEncoder().Encode(*overlay.get());
      writer.Write(*flv_osd_tag.get(), -1);
    }
  }

  if ( !FLAGS_overlay_url.empty() ) {
    DestroyOverlayParams params;
    params.id = "preloaded_commercial";
    scoped_ref<streaming::OsdTag> overlay =
        new streaming::OsdTag(0, streaming::kDefaultFlavourMask,
                              streaming::OsdTag::DESTROY_OVERLAY, params);
    scoped_ref<streaming::FlvTag> flv_osd_tag =
        streaming::OsdToFlvEncoder().Encode(*overlay.get());
    writer.Write(*flv_osd_tag.get(), -1);
  }

  writer.Close();
  common::Exit(0);
}
