// Copyright (c) 2009, Whispersoft s.r.l.
// All rights reserved.
//
// Author: Catalin Popescu
//
#include <stdio.h>
extern "C" {
#include <jpeglib.h>
}
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/types.h>

#include <whisperlib/common/io/file/file_input_stream.h>
#include <whisperlib/common/io/buffer/memory_stream.h>
#include <whisperstreamlib/base/media_file_reader.h>

#include "jpeg_util.h"

namespace util {

bool JpegSave(const feature::BitmapExtractor::PictureStruct* pic,
              int quality,
              const string& filename) {
  struct jpeg_compress_struct cinfo;
  struct jpeg_error_mgr jerr;

  cinfo.err = jpeg_std_error(&jerr);
  jpeg_create_compress(&cinfo);

  FILE* outfile;
  if ( (outfile = ::fopen(filename.c_str(), "wb") ) == NULL) {
    LOG_ERROR << " Cannot open JPEG output file: " << filename;
    return false;
  }
  jpeg_stdio_dest(&cinfo, outfile);

  cinfo.image_width = pic->width_;
  cinfo.image_height = pic->height_;
  if ( pic->pix_format_ == PIX_FMT_RGB24 ) {
    cinfo.input_components = 3;
    cinfo.in_color_space = JCS_RGB;
  } else if ( pic->pix_format_ == PIX_FMT_YUV444P ||
              pic->pix_format_ == PIX_FMT_YUV420P ) {
    cinfo.input_components = 3;
    cinfo.in_color_space = JCS_YCbCr;
  } else {
    LOG_ERROR << " JpegSaver does not know input pixel format: "
              << pic->pix_format_;
    return false;
  }

  jpeg_set_defaults(&cinfo);

  DCHECK(quality >= 0 && quality <= 100) << " Invalid quality: " << quality;

  jpeg_set_quality(&cinfo, quality, false);

  jpeg_start_compress(&cinfo, true);

  JSAMPROW row_pointer[1];
  const int row_stride = pic->frame_.linesize[0];

  while ( cinfo.image_height > cinfo.next_scanline )  {
    const int start =
        pic->is_transposed_
        ? (cinfo.image_height - cinfo.next_scanline - 1) * row_stride
        : cinfo.next_scanline * row_stride;
    row_pointer[0] = reinterpret_cast<const JSAMPROW>(
        pic->frame_.data[0] + start);
    jpeg_write_scanlines(&cinfo, row_pointer, 1);
  }

  jpeg_finish_compress(&cinfo);
  ::fclose(outfile);

  jpeg_destroy_compress(&cinfo);

  return true;
}

bool JpegLoad(const string& filename,
              feature::BitmapExtractor::PictureStruct* pic) {
  LOG_FATAL << "JpegLoad not implemented..";
  return false;
}

}
