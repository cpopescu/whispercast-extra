// Copyright 2009 WhisperSoft s.r.l.
// Author: Cosmin Tudorache

#ifndef __MEDIA_OSD_OSD_TO_FLV_ENCODER_H__
#define __MEDIA_OSD_OSD_TO_FLV_ENCODER_H__

#include <whisperlib/common/sync/mutex.h>
#include "osd/base/osd_tag.h"


namespace streaming {
class FlvTag;

class OsdToFlvEncoder {
 public:
  OsdToFlvEncoder();
  virtual ~OsdToFlvEncoder();

  //  [Thread safe]
  //
  //  Encode an Osd tag inside a flv Metadata tag.
  // Returns:
  //  A flv metadata tag.
  scoped_ref<FlvTag> Encode(const streaming::OsdTag& osd);

 private:
  // generates the next Osd tag id. Used on flv onCuePoint encoding.
  uint64 GenNextOsdTagIndex();

 private:
  // used for generating unique IDs for Osd encoded tags.
  uint64 crt_osd_tag_index_;
  // synchrnonyze access to crt_osd_tag_index_
  synch::Mutex synch_;

  DISALLOW_EVIL_CONSTRUCTORS(OsdToFlvEncoder);
};
}
#endif  // __MEDIA_OSD_OSD_TO_FLV_ENCODER_H__
