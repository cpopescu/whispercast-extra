// Copyright (c) 2009, Whispersoft s.r.l.
// All rights reserved.
//
// Authors: Catalin Popescu
//

#include <whisperstreamlib/base/media_file_reader.h>
#include "features/util/flv_thumbnailer.h"
#include "features/util/jpeg_util.h"

namespace util {

bool FlvExtractThumbnails(const string& flv_file,
                          int64 start_ms, int64 repeat_ms, int64 end_ms,
                          int jpeg_width, int jpeg_height, int jpeg_quality,
                          const string& output_dir) {
  feature::BitmapExtractor extractor(jpeg_width, jpeg_height, PIX_FMT_RGB24);

  io::Mkdir(output_dir, true);
  streaming::MediaFileReader reader;
  if ( !reader.Open(flv_file, streaming::TagSplitter::TS_FLV) ) {
    LOG_ERROR << "Cannot open for thumbnailing: " << output_dir;
    return false;
  }
  if ( reader.splitter()->type() != streaming::TagSplitter::TS_FLV ) {
    LOG_ERROR << "Not a FLV file: " << output_dir;
    return false;
  }

  int64 next_snapshot_ms = start_ms;
  while ( true ) {
    scoped_ref<streaming::Tag> tag;
    streaming::TagReadStatus err = reader.Read(&tag);
    if ( err == streaming::READ_EOF ) {
      break;
    }
    if ( err == streaming::READ_SKIP ) {
      continue;
    }
    if ( err != streaming::READ_OK ) {
      LOG_ERROR << "Failed to read next tag, result: "
                << streaming::TagReadStatusName(err);
      break;
    }

    vector<scoped_ref<const feature::BitmapExtractor::PictureStruct> > pics;
    extractor.GetNextPictures(tag.get(), &pics);
    for ( uint32 i = 0; i < pics.size(); i++ ) {
      const feature::BitmapExtractor::PictureStruct* pic = pics[i].get();
      if ( pic->timestamp_ < next_snapshot_ms ) {
        continue;
      }
      const string filename = strutil::JoinPaths(
          output_dir,
          strutil::StringPrintf("%s-%010"PRId64".jpg",
              strutil::Basename(output_dir).c_str(),
              pic->timestamp_));
      if ( !JpegSave(pic, jpeg_quality, filename) ) {
        return false;
      }
      LOG_INFO << "Snapshot at: " << next_snapshot_ms
               << ", Output File: [" << filename << "], next snapshot at: "
               << (next_snapshot_ms + repeat_ms) << " ms";
      next_snapshot_ms += repeat_ms;
      if ( next_snapshot_ms > end_ms ) {
        return true;
      }
    }
  };

  return true;
}

} // namespace util
