// Copyright (c) 2009, Whispersoft s.r.l.
// All rights reserved.
//
// Authors: Catalin Popescu
//

#include <string>
#include <whisperlib/common/base/types.h>

namespace util {

// Given a film filename and a timestamp, returns the JPEG filename.
// filename: can be a full path, or just a basename
// returns: the basename for the JPEG file.
string MakeThumbnailName(const string& filename, int64 time_ms);

// Utility for dumping thumbnails from a single media file.
// The file format is not important, it can be: .flv, .mp4, ...
bool ExtractThumbnails(const string& file,
                       int64 start_ms,
                       int64 repeat_ms,
                       int64 end_ms,
                       int jpeg_width, int jpeg_height, int jpeg_quality,
                       const string& output_dir);

} // namespace util
