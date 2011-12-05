// Copyright (c) 2009, Whispersoft s.r.l.
// All rights reserved.
//
// Author: Catalin Popescu
//

#ifndef __FEATURE_JPEG_UTIL_H__
#define __FEATURE_JPEG_UTIL_H__

#include <whisperstreamlib/flv/flv_tag.h>
#include <whisperstreamlib/flv/flv_tag_splitter.h>

#include  "features/decoding/bitmap_extractor.h"

namespace util {

bool JpegSave(const feature::BitmapExtractor::PictureStruct* pic,
              int quality,   // 0 .. 100
              const string& filename);

bool JpegLoad(const string& filename,
              feature::BitmapExtractor::PictureStruct* pic);

}

#endif
