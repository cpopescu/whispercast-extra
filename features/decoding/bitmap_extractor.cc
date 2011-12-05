// Copyright (c) 2009, Whispersoft s.r.l.
// All rights reserved.
//
// Author: Catalin Popescu
//

#include "bitmap_extractor.h"
#include <whisperstreamlib/flv/flv_coder.h>

namespace feature {

BitmapExtractor::BitmapExtractor(
    int width,
    int height,
    int pix_format)
    : width_(width),
      height_(height),
      pix_format_(pix_format),
      error_codec_(false),
      codec_id_(CODEC_ID_NONE),
      is_transposed_(false),
      codec_(NULL),
      codec_ctx_(NULL),
      frame_(NULL),
      sws_ctx_rescale_(NULL),
      sws_ctx_convert_(NULL) {
  void* allocated = NULL;
  CHECK(!posix_memalign(&allocated,
                        kFrameBufAllign, kMaxFrameSize));
  frame_buf_ = reinterpret_cast<uint8*>(allocated);
}

BitmapExtractor::~BitmapExtractor() {
  ClearCodec();
  free(frame_buf_);
}

void BitmapExtractor::GetNextPictures(const streaming::Tag* tag,
    vector< scoped_ref<const PictureStruct> >* out) {
  if ( tag->type() == streaming::Tag::TYPE_SOURCE_STARTED ||
       tag->type() == streaming::Tag::TYPE_SOURCE_ENDED ||
       tag->type() == streaming::Tag::TYPE_EOS ) {
    ClearCodec();
    return;
  }
  if ( tag->type() == streaming::Tag::TYPE_COMPOSED ) {
    const streaming::ComposedTag* ctd =
      static_cast<const streaming::ComposedTag*>(tag);
    if ( ctd->sub_tag_type() != streaming::Tag::TYPE_FLV ) {
      return;
    }
    for ( int i = 0; i < ctd->tags().size(); ++i ) {
      ProcessFlvTag(
          static_cast<const streaming::FlvTag*>(ctd->tags().tag(i).get()),
          tag->timestamp_ms(), out);
    }
    return;
  }
  if ( tag->type() != streaming::Tag::TYPE_FLV ) {
    return;
  }
  ProcessFlvTag(static_cast<const streaming::FlvTag*>(tag), 0, out);
}

void BitmapExtractor::ClearCodec() {
  if ( codec_ctx_ != NULL ) {
    avcodec_close(codec_ctx_);
    codec_ctx_ = NULL;
  }
  if ( frame_ != NULL ) {
    av_free(frame_);
    frame_ = NULL;
  }
  if ( crt_picture_.frame_allocated_ ) {
    avpicture_free(&crt_picture_.frame_);
    avpicture_free(&crt_picture_.rescaled_yuv_frame_);
    crt_picture_.frame_allocated_ = false;
  }
  if ( sws_ctx_rescale_ != NULL ) {
    sws_freeContext(sws_ctx_rescale_);
    sws_ctx_rescale_ = NULL;
  }
  if ( sws_ctx_convert_ != NULL ) {
    sws_freeContext(sws_ctx_convert_);
    sws_ctx_convert_ = NULL;
  }
  crt_picture_.width_ = -1;
  crt_picture_.height_ = -1;
  error_codec_ = false;
  codec_id_ = CODEC_ID_NONE;
  is_transposed_ = false;
  codec_ = NULL;
}

void BitmapExtractor::ProcessFlvTag(const streaming::FlvTag* flv_tag,
    int64 timestamp_base_ms, vector< scoped_ref<const PictureStruct> >* out) {
  if ( error_codec_ ) {
    return;
  }
  if ( flv_tag->body().type() != streaming::FLV_FRAMETYPE_VIDEO ) {
    // TODO: consider also complex media tags..
    return;
  }
  if ( codec_ == NULL ) {
    if ( !InitializeCodec(flv_tag) ) {
      LOG_ERROR << " Error initializing the codec ... ";
      error_codec_ = true;
      return;
    }
  }
  if ( flv_tag->body().Size() > kMaxFrameSize ) {
    LOG_ERROR << " Incredibly long frame - skipping";
    return;
  }
  // we pull the video bytes, passing over the header
  scoped_ref<streaming::FlvTag> tmp_flv_tag =
      new streaming::FlvTag(*flv_tag, -1, true);
  const int len = streaming::FlvCoder::ExtractVideoCodecData(
      tmp_flv_tag.get(), frame_buf_, kMaxFrameSize);
  if ( len < 0 ) {
    LOG_ERROR << " Invalid frame for " << tmp_flv_tag->ToString();
    return;
  }
  if ( len == 0 ) {
    LOG_ERROR << " Oversized frame for " << tmp_flv_tag->ToString();
    return;
  }

  int bytes_remaining = len;
  int bytes_decoded = 0;
  int frame_finished = 0;
  while ( bytes_remaining > 0 ) {
#if LIBAVCODEC_VERSION_MAJOR >= 52 && LIBAVCODEC_VERSION_MINOR >= 72
    AVPacket pkt;
    av_init_packet(&pkt);
    pkt.data = const_cast<uint8*>(frame_buf_) + bytes_decoded;
    pkt.size = bytes_remaining;
    const int cb = avcodec_decode_video2(
        codec_ctx_, frame_,
        &frame_finished, &pkt);
#else
    const int cb = avcodec_decode_video(
        codec_ctx_, frame_,
        &frame_finished,
        const_cast<uint8*>(frame_buf_) + bytes_decoded,
        bytes_remaining);
#endif
    if ( cb < 0 ) {
      LOG_ERROR << " Error decoding bytes for codec: "
                << FlvFlagVideoCodecName(flv_tag->video_body().video_codec())
                << ", bytes_remaining: " << bytes_remaining
                << ", bytes_decoded: " << bytes_decoded;
      return;
    }
    bytes_decoded += cb;
    bytes_remaining -= cb;
    if ( frame_finished ) {
      DLOG(10) << " Frame extracted : " << codec_ctx_->width
               << "x" << codec_ctx_->height
               << " from: "
               <<  FlvFlagVideoCodecName(flv_tag->video_body().video_codec());
      if ( crt_picture_.width_ != codec_ctx_->width ||
           crt_picture_.height_ != codec_ctx_->height ) {
        if ( sws_ctx_rescale_ != NULL ) {
          sws_freeContext(sws_ctx_rescale_);
          sws_ctx_rescale_ = NULL;
        }
        if ( crt_picture_.frame_allocated_ ) {
          avpicture_free(&crt_picture_.frame_);
          avpicture_free(&crt_picture_.rescaled_yuv_frame_);
          crt_picture_.frame_allocated_ = false;
        }
        crt_picture_.width_ = crt_picture_.height_ = -1;
        int dest_width = width_;
        int dest_height = height_;
        if ( dest_width <= 0 ) {
          if ( dest_height <= 0 )  {
            dest_width = codec_ctx_->width;
            dest_height = codec_ctx_->height;
          } else {
            dest_width = codec_ctx_->width * dest_height / codec_ctx_->height;
          }
        } else if ( dest_height <= 0 ) {
          dest_height = codec_ctx_->height * dest_width / codec_ctx_->width;
        }
        crt_picture_.pix_format_ = pix_format_;
        avpicture_alloc(&crt_picture_.frame_, (PixelFormat)pix_format_,
                        dest_width, dest_height);
        avpicture_alloc(&crt_picture_.rescaled_yuv_frame_, PIX_FMT_YUV420P,
                        dest_width, dest_height);
        crt_picture_.frame_allocated_ = true;


        sws_ctx_rescale_ = sws_getContext(
            codec_ctx_->width, codec_ctx_->height,
            codec_ctx_->pix_fmt,
            dest_width, dest_height,
            static_cast<PixelFormat>(PIX_FMT_YUV420P),
            SWS_BICUBIC,
            NULL, NULL, NULL);
        if ( sws_ctx_rescale_ == NULL ) {
          LOG_ERROR << " Error creating sws_context !";
          return;
        }
        crt_picture_.width_ = dest_width;
        crt_picture_.height_ = dest_height;
      }
      sws_scale(sws_ctx_rescale_,
                frame_->data,  frame_->linesize,
                0, codec_ctx_->height,
                crt_picture_.rescaled_yuv_frame_.data,
                crt_picture_.rescaled_yuv_frame_.linesize);
      if ( sws_ctx_convert_ == NULL ) {
        sws_ctx_convert_ = sws_getContext(
            crt_picture_.width_, crt_picture_.height_,
            static_cast<PixelFormat>(PIX_FMT_YUV420P),
            crt_picture_.width_, crt_picture_.height_,
            static_cast<PixelFormat>(PIX_FMT_RGB24),
            SWS_BICUBIC,
            NULL, NULL, NULL);
      }
      sws_scale(sws_ctx_convert_,
                crt_picture_.rescaled_yuv_frame_.data,
                crt_picture_.rescaled_yuv_frame_.linesize,
                0, crt_picture_.height_,
                crt_picture_.frame_.data,
                crt_picture_.frame_.linesize);

      crt_picture_.timestamp_ = timestamp_base_ms + flv_tag->timestamp_ms();
      crt_picture_.is_transposed_ = is_transposed_;
      DLOG(10) << " Picture found: " << codec_ctx_->width << "x"
               << codec_ctx_->height;
      out->push_back(new PictureStruct(crt_picture_));
    }
  }
}

bool BitmapExtractor::InitializeCodec(const streaming::FlvTag* flv_tag) {
  switch ( flv_tag->video_body().video_codec() ) {
    case streaming::FLV_FLAG_VIDEO_CODEC_H263:
      codec_id_ = CODEC_ID_FLV1;
      break;
    case streaming::FLV_FLAG_VIDEO_CODEC_VP6:
#ifdef CODEC_ID_VP6F
      codec_id_ = CODEC_ID_VP6F;
#else
      codec_id_ = CODEC_ID_VP6;
      is_transposed_ = true;   // for this guys frames come transposed ..
#endif
      break;
    case streaming::FLV_FLAG_VIDEO_CODEC_VP6_WITH_ALPHA:
#ifdef CODEC_ID_VP6F
      codec_id_ = CODEC_ID_VP6F;
#else
      codec_id_ = CODEC_ID_VP6;
      is_transposed_ = true;   // for this guys frames come transposed ..
#endif
      break;
    case streaming::FLV_FLAG_VIDEO_CODEC_AVC:
      codec_id_ = CODEC_ID_H264;
      break;
    default:
      LOG_ERROR << " Unknown codec do decode: "
                << FlvFlagVideoCodecName(flv_tag->video_body().video_codec())
                << " For: " << flv_tag->ToString();
      return false;
  }
  codec_ = avcodec_find_decoder(codec_id_);
  if ( codec_ == NULL ) {
    LOG_ERROR << " Cannot open codec id: " << codec_id_
              << " for flv type: "
              << FlvFlagVideoCodecName(flv_tag->video_body().video_codec());
    return false;
  }
  codec_ctx_ = avcodec_alloc_context();
  frame_ = avcodec_alloc_frame();
  LOG_INFO << " Got codec: " << codec_->name
           << " size: " << codec_->priv_data_size
           << " caps: "<< codec_->capabilities;

  if ( avcodec_open(codec_ctx_, codec_) < 0 ) {
    LOG_ERROR << " Cannot opening codec context: " << codec_id_
              << " for flv type: "
              << FlvFlagVideoCodecName(flv_tag->video_body().video_codec());
    return false;
  }

  // Inform the codec that we can handle truncated bitstreams -- i.e.,
  // bitstreams where frame boundaries can fall in the middle of packets
  if ( codec_->capabilities & CODEC_CAP_TRUNCATED ) {
    codec_ctx_->flags |= CODEC_FLAG_TRUNCATED;
  }
  LOG_INFO << " Detected codec id: " << codec_id_
           << " from: " <<  FlvFlagVideoCodecName(flv_tag->video_body().video_codec());
  return true;
}

}
