// Copyright 2009 WhisperSoft s.r.l.
// Author: Catalin Popescu

#ifndef __MEDIA_OSD_ENCODER_ELEMENT_H__
#define __MEDIA_OSD_ENCODER_ELEMENT_H__

#include <whisperstreamlib/base/filtering_element.h>

namespace streaming {

// A filtering element that encodes the osd tags into flv tags, replacing
// the osd tags w/ these flv tags.
// All other tags are left untouched.
//
class OsdEncoderElement : public FilteringElement {
 public:
  OsdEncoderElement(const string& name,
                    ElementMapper* mapper,
                    net::Selector* selector);
  virtual ~OsdEncoderElement();

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

  DISALLOW_EVIL_CONSTRUCTORS(OsdEncoderElement);
};

}
#endif  // __MEDIA_OSD_ENCODER_ELEMENT_H__
