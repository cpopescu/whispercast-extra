// Copyright 2009 WhisperSoft s.r.l.
// Author: Catalin Popescu

#include <whisperstreamlib/flv/flv_tag.h>
#include "osd/base/osd_tag.h"
#include "osd/base/osd_to_flv_encoder.h"
#include "osd/osd_library/osd_encoder/osd_encoder_element.h"

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
        osd_to_flv_encoder_() {
  }
  virtual ~OsdCallbackData() {
  }

 protected:
  ///////////////////////////////////////////////////////////////////////

  virtual void FilterTag(const streaming::Tag* tag,
                         int64 timestamp_ms,
                         TagList* out) {
    if ( tag->type() == streaming::Tag::TYPE_OSD ) {
      scoped_ref<streaming::FlvTag> flv_tag =
          osd_to_flv_encoder_.Encode(
              static_cast<const streaming::OsdTag &>(*tag));
      flv_tag->set_attributes(streaming::Tag::ATTR_METADATA);
      flv_tag->set_flavour_mask(tag->flavour_mask());
      // forward tag
      out->push_back(FilteredTag(flv_tag.get(), timestamp_ms));
      return;
    }
    // default: forward tag
    out->push_back(FilteredTag(tag, timestamp_ms));
  }
 private:
  // Knows how to encode OSD tags in FLV metadata tags.
  streaming::OsdToFlvEncoder osd_to_flv_encoder_;
  DISALLOW_EVIL_CONSTRUCTORS(OsdCallbackData);
};
}

////////////////////////////////////////////////////////////////////////////

namespace streaming {

const char OsdEncoderElement::kElementClassName[] = "osd_encoder";

OsdEncoderElement::OsdEncoderElement(const char* name,
                                     const char* id,
                                     ElementMapper* mapper,
                                     net::Selector* selector)
    : FilteringElement(kElementClassName, name, id, mapper, selector) {
}
OsdEncoderElement::~OsdEncoderElement() {
}

FilteringCallbackData* OsdEncoderElement::CreateCallbackData(
    const char* media_name,
    streaming::Request* req) {
  // We basically impose FLV
  if ( req->caps().tag_type_ != Tag::TYPE_FLV ) {
    LOG_ERROR << "Refusing NON FLV request: " << req->ToString();
    return NULL;
  }
  req->mutable_caps()->tag_type_ = Tag::TYPE_FLV;
  return new OsdCallbackData();
}
}
