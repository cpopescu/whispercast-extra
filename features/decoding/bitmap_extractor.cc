// Copyright (c) 2009, Whispersoft s.r.l.
// All rights reserved.
//
// Author: Catalin Popescu
//

#include "bitmap_extractor.h"
#include <whisperlib/common/base/scoped_ptr.h>
#include <whisperstreamlib/flv/flv_coder.h>
#include <whisperstreamlib/f4v/atoms/movie/avcc_atom.h>
#include <whisperstreamlib/f4v/atoms/movie/esds_atom.h>
#include <whisperstreamlib/f4v/f4v_util.h>

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
#if LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(53,0,0)
      dictionary_(NULL),
#endif
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
    int64 tag_ts, vector< scoped_ref<const PictureStruct> >* out) {
  if ( tag->type() == streaming::Tag::TYPE_SOURCE_STARTED ||
       tag->type() == streaming::Tag::TYPE_SOURCE_ENDED ||
       tag->type() == streaming::Tag::TYPE_EOS ) {
    ClearCodec();
    return;
  }
  if ( tag->type() == streaming::Tag::TYPE_COMPOSED ) {
    /*
    const streaming::ComposedTag* ctd =
      static_cast<const streaming::ComposedTag*>(tag);
    if ( ctd->sub_tag_type() != streaming::Tag::TYPE_FLV ) {
      return;
    }
    for ( int i = 0; i < ctd->tags().size(); ++i ) {
      ProcessFlvTag(
          static_cast<const streaming::FlvTag*>(ctd->tags().tag(i).get()),
          tag_ts + ctd->tags().tag(i)->timestamp_ms(),
          out);
    }
    */
    return;
  }
  if ( tag->type() == streaming::Tag::TYPE_FLV ) {
    ProcessFlvTag(static_cast<const streaming::FlvTag*>(tag), tag_ts, out);
  } else if ( tag->type() == streaming::Tag::TYPE_F4V ) {
    ProcessF4vTag(static_cast<const streaming::F4vTag*>(tag), tag_ts, out);
  }
}

void BitmapExtractor::ClearCodec() {
  if ( codec_ctx_ != NULL ) {
    avcodec_close(codec_ctx_);
    codec_ctx_ = NULL;
  }
#if LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(53,0,0)
  if ( dictionary_ != NULL ) {
    av_dict_free(&dictionary_);
    dictionary_ = NULL;
  }
#endif
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
    int64 tag_ts, vector< scoped_ref<const PictureStruct> >* out) {
  if ( error_codec_ ) {
    return;
  }
  if ( flv_tag->body().type() != streaming::FLV_FRAMETYPE_VIDEO ) {
    // TODO: consider also complex media tags..
    return;
  }
  /////////////////////////////////////////////////////////////////////
  // maybe initialize the codec
  if ( codec_ == NULL ) {
    if ( flv_tag->video_body().codec() == streaming::FLV_FLAG_VIDEO_CODEC_AVC &&
         flv_tag->video_body().avc_packet_type() != streaming::AVC_SEQUENCE_HEADER ) {
      LOG_WARNING << "Waiting for AVC Sequence Header, dropping: " << flv_tag->ToString();
      return;
    }
    if ( !InitializeCodec(flv_tag) ) {
      LOG_ERROR << " Error initializing the codec ... ";
      error_codec_ = true;
      return;
    }
    // there's no picture in AVC Sequence Header
    return;
  }
  if ( flv_tag->body().Size() > kMaxFrameSize ) {
    LOG_ERROR << "Frame too long - skipping";
    return;
  }
  io::MemoryStream ms;
  if ( !streaming::FlvCoder::ExtractVideoCodecData(flv_tag, &ms) ) {
    LOG_ERROR << "ExtractVideoCodecData() failed, tag: " << flv_tag->ToString();
    return;
  }
  const int len = ms.Read(frame_buf_, kMaxFrameSize);
  ProcessFrame(len, tag_ts, FlvFlagVideoCodecName(flv_tag->video_body().codec()), out);
}
void BitmapExtractor::ProcessF4vTag(const streaming::F4vTag* f4v_tag,
    int64 tag_ts, vector< scoped_ref<const PictureStruct> >* out) {
  /////////////////////////////////////////////////////////////////////
  // maybe initialize the codec
  if ( codec_ == NULL ) {
    if ( !f4v_tag->is_atom() ||
         f4v_tag->atom()->type() != streaming::f4v::ATOM_MOOV ) {
      LOG_ERROR << "Waiting for MOOV, ignoring tag: " << f4v_tag->ToString();
      return;
    }
    if ( !InitializeCodec(*static_cast<const streaming::f4v::MoovAtom*>(f4v_tag->atom())) ) {
      LOG_ERROR << " Error initializing the codec ... ";
      error_codec_ = true;
      return;
    }
  }

  if ( !f4v_tag->is_frame() ) {
    // ignore Atoms
    return;
  }
  const streaming::f4v::Frame* frame = f4v_tag->frame();
  if ( frame->header().type() != streaming::f4v::FrameHeader::VIDEO_FRAME ) {
    // ignore AUDIO_FRAMEs
    return;
  }

  // Send Packet NALUs, without start codes
  const int len = frame->data().Peek(frame_buf_, kMaxFrameSize);
  ProcessFrame(len, tag_ts, "some_f4v_codec", out);
}

void BitmapExtractor::ProcessFrame(int frame_buf_size, int64 frame_ts,
    const string& codec_name,
    vector< scoped_ref<const PictureStruct> >* out) {
  int bytes_remaining = frame_buf_size;
  int bytes_decoded = 0;
  int frame_finished = 0;
  while ( bytes_remaining > 0 ) {
#if LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(52,72,0)
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
                << codec_name
                << ", bytes_remaining: " << bytes_remaining
                << ", bytes_decoded: " << bytes_decoded;
      return;
    }
    bytes_decoded += cb;
    bytes_remaining -= cb;
    if ( frame_finished ) {
      //LOG_DEBUG << "Frame extracted: " << codec_ctx_->width
      //          << "x" << codec_ctx_->height
      //          << ", from: "
      //          <<  codec_name
      //          << ", ts: " << flv_tag->timestamp_ms() << " ms";
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

      crt_picture_.timestamp_ = frame_ts;
      crt_picture_.is_transposed_ = is_transposed_;
      //DLOG_INFO << "Picture found: " << codec_ctx_->width << "x"
      //          << codec_ctx_->height;
      if ( out != NULL ) {
        out->push_back(new PictureStruct(crt_picture_));
      }
    }
  }
}

bool BitmapExtractor::InitializeCodec(const streaming::FlvTag* flv_tag) {
  scoped_ptr<io::MemoryStream> decoder_extra_data;
  switch ( flv_tag->video_body().codec() ) {
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
      CHECK_EQ(flv_tag->video_body().avc_packet_type(),
               streaming::AVC_SEQUENCE_HEADER);
      codec_id_ = CODEC_ID_H264;
      decoder_extra_data = new io::MemoryStream();
      decoder_extra_data->AppendStreamNonDestructive(flv_tag->Data());
      break;
    default:
      LOG_ERROR << "Unknown codec: "
                << FlvFlagVideoCodecName(flv_tag->video_body().codec())
                << " In tag: " << flv_tag->ToString();
      return false;
  }
  return InitializeCodecInternal(decoder_extra_data.get());
}
bool BitmapExtractor::InitializeCodec(const streaming::f4v::MoovAtom& moov) {
  codec_id_ = CODEC_ID_H264;
  // extract AVCC (H264 decoder configuration)
  io::MemoryStream ms;
  const streaming::f4v::AvccAtom* avcc = streaming::f4v::util::GetAvccAtom(moov);
  if ( avcc != NULL ) {
    streaming::f4v::Encoder f4v_encoder;
    f4v_encoder.WriteAtom(ms, *avcc);
    ms.Skip(8);
  }
  return InitializeCodecInternal(&ms);
}

bool BitmapExtractor::InitializeCodecInternal(const io::MemoryStream* extra_data) {
  codec_ = avcodec_find_decoder(codec_id_);
  if ( codec_ == NULL ) {
    LOG_ERROR << " Cannot open codec id: " << codec_id_;
    return false;
  }
#if LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(53,35,0)
  codec_ctx_ = avcodec_alloc_context3(codec_);
#else
  codec_ctx_ = avcodec_alloc_context();
#endif
  if ( extra_data != NULL ) {
    const uint32 size = extra_data->Size();
    codec_ctx_->extradata_size = size;
    codec_ctx_->extradata = new uint8[size];
    extra_data->Peek(codec_ctx_->extradata, size);
  }
  frame_ = avcodec_alloc_frame();
  LOG_INFO << " Got codec: " << codec_->name
           << " size: " << codec_->priv_data_size
           << " caps: "<< codec_->capabilities;
#if LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(53,0,0)
  if ( avcodec_open2(codec_ctx_, codec_, &dictionary_) < 0 ) {
#else
  if ( avcodec_open(codec_ctx_, codec_) < 0 ) {
#endif
    LOG_ERROR << " Cannot opening codec context: " << codec_id_;
    return false;
  }

  // Inform the codec that we can handle truncated bitstreams -- i.e.,
  // bitstreams where frame boundaries can fall in the middle of packets
  if ( codec_->capabilities & CODEC_CAP_TRUNCATED ) {
    codec_ctx_->flags |= CODEC_FLAG_TRUNCATED;
  }
  LOG_INFO << " Detected codec id: " << codec_id_;
  return true;
}

}
