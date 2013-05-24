// Copyright 2009 WhisperSoft s.r.l.
// Author: Cosmin Tudorache

#ifndef __MEDIA_OSD_DECODER_ELEMENT_H__
#define __MEDIA_OSD_DECODER_ELEMENT_H__

#include <whisperstreamlib/base/filtering_element.h>
#include "osd/base/osd_tag.h"

namespace streaming {

// A filtering element that decodes flv tags into internal osd tags.
// All tags that cannot be decoded are left untouched.
// Decoded flv tags are removed from the stream.
// The newly created OSD tags have the flv tag's timestamp.
//
class OsdDecoderElement : public FilteringElement {
 public:
  OsdDecoderElement(const string& name,
                    ElementMapper* mapper,
                    net::Selector* selector);
  virtual ~OsdDecoderElement();

  static const char kElementClassName[];

  virtual bool Initialize() {
    return true;
  }

 protected:
  //////////////////////////////////////////////////////////////////
  //
  // FilteringElement methods
  //
  virtual FilteringCallbackData* CreateCallbackData(const string& media_name,
                                                    Request* req);

 private:
  const std::string media_name_;
  ProcessingCallback* process_tag_callback_;

  DISALLOW_EVIL_CONSTRUCTORS(OsdDecoderElement);
};

}
#endif  // __MEDIA_OSD_DECODER_ELEMENT_H__
