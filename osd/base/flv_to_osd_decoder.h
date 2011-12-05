// Copyright 2009 WhisperSoft s.r.l.
// Author: Cosmin Tudorache

#ifndef __MEDIA_OSD_FLV_TO_OSD_DECODER_H__
#define __MEDIA_OSD_FLV_TO_OSD_DECODER_H__

#include <whisperlib/common/sync/mutex.h>

#include <whisperstreamlib/flv/flv_tag.h>
#include "osd/base/osd_tag.h"

namespace streaming {
class FlvToOsdDecoder {
 public:
  FlvToOsdDecoder();
  virtual ~FlvToOsdDecoder();

  //  [Thread safe]
  //
  //  Decode an Osd tag from a flv Metadata tag.
  // Returns:
  //  A new OsdTag on success. Or a NULL result on failure.
  scoped_ref<OsdTag> Decode(const streaming::FlvTag& flv);
 private:
  DISALLOW_EVIL_CONSTRUCTORS(FlvToOsdDecoder);
};
}

#endif  // __MEDIA_OSD_FLV_TO_OSD_DECODER_H__
