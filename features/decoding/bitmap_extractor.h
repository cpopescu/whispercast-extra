// Copyright (c) 2009, Whispersoft s.r.l.
// All rights reserved.
//
// Author: Catalin Popescu
//

#include <string>

#include <whisperlib/common/base/types.h>
#include <whisperlib/common/base/callback.h>

#ifndef UINT64_C
#define UINT64_C(value) value ## ULL
#endif

extern "C" {
#pragma GCC diagnostic ignored "-Wall"
#include __AV_CODEC_INCLUDE_FILE
#include __SW_SCALE_INCLUDE_FILE
//#include "/usr/include/libavcodec/avcodec.h"
//#include "/usr/include/libswscale/swscale.h"
#pragma GCC diagnostic warning "-Wall"
}
#include <whisperstreamlib/flv/flv_tag.h>

#ifndef __FEATURE_BITMAP_EXTRACTOR_H__
#define __FEATURE_BITMAP_EXTRACTOR_H__

// NEED TO DO before everything: av_register_all();

namespace feature {

class BitmapExtractor {
 public:
  struct PictureStruct : public RefCounted {
    AVPicture rescaled_yuv_frame_;
    AVPicture frame_;
    bool frame_allocated_;
    int width_;
    int height_;
    int pix_format_;
    int64 timestamp_;
    bool is_transposed_;
    PictureStruct()
        : RefCounted(NULL),
          frame_allocated_(false),
          width_(-1),
          height_(-1),
          pix_format_(-1),
          timestamp_(0),
          is_transposed_(false) {
    }
    PictureStruct(const PictureStruct& other)
        : RefCounted(NULL),
          frame_allocated_(true),
          width_(other.width_),
          height_(other.height_),
          pix_format_(other.pix_format_),
          timestamp_(other.timestamp_),
          is_transposed_(other.is_transposed_) {
      avpicture_alloc(&frame_, (PixelFormat)pix_format_, width_, height_);
      av_picture_copy(&frame_, &other.frame_, (PixelFormat)pix_format_,
          width_, height_);

      avpicture_alloc(&rescaled_yuv_frame_, PIX_FMT_YUV420P, width_, height_);
      av_picture_copy(&rescaled_yuv_frame_, &other.rescaled_yuv_frame_,
                      PIX_FMT_YUV420P, width_, height_);
    }
    ~PictureStruct() {
      if ( frame_allocated_ ) {
        avpicture_free(&frame_);
        avpicture_free(&rescaled_yuv_frame_);
        frame_allocated_ = false;
      }
    }
  };

  BitmapExtractor(int width = 0,
                  int height = 0,
                  int pix_format = PIX_FMT_RGB24);
  ~BitmapExtractor();

  void Reset() {
    ClearCodec();
  }
  // Extract 0 or more pictures from the given tag.
  // If tag is FlvTag, then 0 or 1 picture is extracted.
  // If tag is ComposedTag, then multiple pictures are extracted.
  void GetNextPictures(const streaming::Tag* tag,
                       vector< scoped_ref<const PictureStruct> >* out);
 private:
  void ClearCodec();
  void ProcessFlvTag(const streaming::FlvTag* flv_tag, int64 timestamp_base_ms,
                     vector< scoped_ref<const PictureStruct> >* out);
  bool InitializeCodec(const streaming::FlvTag* flv_tag);

  const int width_;
  const int height_;
  const int pix_format_;
  bool error_codec_;
  CodecID codec_id_;
  bool is_transposed_;
  int32 skip_size_;
  AVCodec* codec_;
  AVCodecContext* codec_ctx_;
  AVFrame* frame_;
  PictureStruct crt_picture_;
  SwsContext* sws_ctx_rescale_;
  SwsContext* sws_ctx_convert_;

  uint8* frame_buf_;
  static const int kFrameBufAllign = 64;
  static const int kMaxFrameSize = 1<<20;

  DISALLOW_EVIL_CONSTRUCTORS(BitmapExtractor);
};

}

#endif  // __FEATURE_BITMAP_EXTRACTOR_H__