// Copyright (c) 2009, Whispersoft s.r.l.
// All rights reserved.
//
// Authors: Catalin Popescu
//

#include <string>
#include <whisperlib/common/base/types.h>

namespace util {

//
// Utility for dumping thumbnails for a flv file.
//
bool FlvExtractThumbnails(const string& flv_file,
                          int64 start_ms,
                          int64 repeat_ms,
                          int64 end_ms,
                          int jpeg_width, int jpeg_height, int jpeg_quality,
                          const string& output_dir);

} // namespace util
