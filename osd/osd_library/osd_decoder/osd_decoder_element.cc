// Copyright 2009 WhisperSoft s.r.l.
// Author: Cosmin Tudorache

#include <whisperlib/net/rpc/lib/codec/json/rpc_json_decoder.h>
#include <whisperstreamlib/flv/flv_tag.h>
#include <whisperstreamlib/rtmp/objects/rtmp_objects.h>

#include "osd/base/flv_to_osd_decoder.h"
#include "osd/osd_library/osd_decoder/osd_decoder_element.h"

namespace {

/////////////////////////////////////////////////////////////////////////
//
// A pair of callbacks (upstream, downstream) that injects osd
// tags.
//
class OsdCallbackData : public streaming::FilteringCallbackData {
 public:
  OsdCallbackData()
    : streaming::FilteringCallbackData(),
      decoder_() {
  }
  virtual ~OsdCallbackData() {
  }

 protected:

  ///////////////////////////////////////////////////////////////////////

  virtual void FilterTag(const streaming::Tag* tag,
                         int64 timestamp_ms,
                         TagList* out) {
    if ( tag->type() != streaming::Tag::TYPE_FLV ) {
      // not a FLV tag
      // forward tag
      out->push_back(FilteredTag(tag, timestamp_ms));
      return;
    }
    const streaming::FlvTag* flv_tag =
        static_cast<const streaming::FlvTag*>(tag);
    scoped_ref<streaming::OsdTag> osd = decoder_.Decode(*flv_tag);
    if ( osd.get() == NULL ) {
      // decoding failed. Probably not an OSD metadata tag.
      // forward tag
      out->push_back(FilteredTag(tag, timestamp_ms));
      return;
    }

    osd->set_flavour_mask(tag->flavour_mask());
    DLOG_DEBUG << "Decoded and replaced flv tag with"
               << " OSD (" << flv_tag->ToString() << "): "
               << osd->ToString();
    // forward tag
    out->push_back(FilteredTag(osd.get(), timestamp_ms));
    return;
  }

 private:
  streaming::FlvToOsdDecoder decoder_;

  DISALLOW_EVIL_CONSTRUCTORS(OsdCallbackData);
};
}

////////////////////////////////////////////////////////////////////////////

namespace streaming {

const char OsdDecoderElement::kElementClassName[] = "osd_decoder";

OsdDecoderElement::OsdDecoderElement(const char* name,
                                     const char* id,
                                     ElementMapper* mapper,
                                     net::Selector* selector)
    : FilteringElement(kElementClassName, name, id, mapper, selector) {
}
OsdDecoderElement::~OsdDecoderElement() {
}

FilteringCallbackData* OsdDecoderElement::CreateCallbackData(
    const char* media_name,
    streaming::Request* req) {
  return new OsdCallbackData();
}
}
