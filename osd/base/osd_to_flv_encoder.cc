// Copyright 2009 WhisperSoft s.r.l.
// Author: Cosmin Tudorache

#include <whisperlib/net/rpc/lib/codec/json/rpc_json_encoder.h>
#include <whisperstreamlib/base/consts.h>
#include <whisperstreamlib/rtmp/objects/rtmp_objects.h>
#include <whisperstreamlib/flv/flv_tag.h>
#include "osd/base/osd_to_flv_encoder.h"

namespace streaming {

OsdToFlvEncoder::OsdToFlvEncoder()
  : crt_osd_tag_index_(0),
    synch_() {
}

OsdToFlvEncoder::~OsdToFlvEncoder() {
}

scoped_ref<FlvTag> OsdToFlvEncoder::Encode(const OsdTag& osd) {
  const char* fname = osd.fname();
  const int64 crt_time_offset_ms = osd.timestamp_ms();

  // encode fparams rpc::Object into a Json string
  //
  io::MemoryStream tmp;
  rpc::JsonEncoder encoder(tmp);
  switch ( osd.ftype() ) {
    case streaming::OsdTag::CREATE_OVERLAY:
      encoder.Encode(
          static_cast<const CreateOverlayParams&>(osd.fparams()));
      break;
    case streaming::OsdTag::DESTROY_OVERLAY:
      encoder.Encode(
          static_cast<const DestroyOverlayParams&>(osd.fparams()));
      break;
    case streaming::OsdTag::SHOW_OVERLAYS:
      encoder.Encode(
          static_cast<const ShowOverlaysParams&>(osd.fparams()));
      break;
    case streaming::OsdTag::CREATE_CRAWLER:
      encoder.Encode(
          static_cast<const CreateCrawlerParams&>(osd.fparams()));
      break;
    case streaming::OsdTag::DESTROY_CRAWLER:
      encoder.Encode(
          static_cast<const DestroyCrawlerParams&>(osd.fparams()));
      break;
    case streaming::OsdTag::SHOW_CRAWLERS:
      encoder.Encode(
          static_cast<const ShowCrawlersParams&>(osd.fparams()));
      break;
    case streaming::OsdTag::ADD_CRAWLER_ITEM:
      encoder.Encode(
          static_cast<const AddCrawlerItemParams&>(osd.fparams()));
      break;
    case streaming::OsdTag::REMOVE_CRAWLER_ITEM:
      encoder.Encode(
          static_cast<const RemoveCrawlerItemParams&>(osd.fparams()));
      break;
    case streaming::OsdTag::SET_PICTURE_IN_PICTURE:
      encoder.Encode(
          static_cast<const SetPictureInPictureParams &>(osd.fparams()));
      break;
    case streaming::OsdTag::SET_CLICK_URL:
      encoder.Encode(
          static_cast<const SetClickUrlParams &>(osd.fparams()));
      break;
    case streaming::OsdTag::CREATE_MOVIE:
      encoder.Encode(
          static_cast<const CreateMovieParams&>(osd.fparams()));
      break;
    case streaming::OsdTag::DESTROY_MOVIE:
      encoder.Encode(
          static_cast<const DestroyMovieParams&>(osd.fparams()));
      break;
    default:
      LOG_FATAL << "Unknown OSD function type: " << osd.ftype();
  };
  string fparams;
  tmp.ReadString(&fparams);

  // compose Flv Metadata tag
  //

  // part of our encoding convention
  rtmp::CMixedMap* parameters = new rtmp::CMixedMap();
  parameters->Set("target",     new rtmp::CString("flash"));
  parameters->Set("function",   new rtmp::CString(fname));
  parameters->Set("parameters", new rtmp::CString(fparams));

  // part of stadard flv onCuePoint tag
  rtmp::CMixedMap map;
  map.Set("type", new rtmp::CString("actionscript"));
  map.Set("time", new rtmp::CNumber(crt_time_offset_ms/1000.0));
                               // use tag's timestamp
  map.Set("name", new rtmp::CString(
      strutil::StringPrintf("w%"PRIu64"", GenNextOsdTagIndex())));
  map.Set("parameters", parameters);

  return new streaming::FlvTag(0,
      streaming::kDefaultFlavourMask, crt_time_offset_ms,
      new streaming::FlvTag::Metadata("onCuePoint", map));
}

// TODO(cpopescu): Fix this dependency on syncing
uint64 OsdToFlvEncoder::GenNextOsdTagIndex() {
  synch::MutexLocker lock(&synch_);
  return crt_osd_tag_index_++;
}
}
