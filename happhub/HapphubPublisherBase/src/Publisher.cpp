#include "BufferIO.h"
#include "Publisher.h"

extern "C" {
#include "libavutil/opt.h"
#include "libavformat/avformat.h"
}

namespace happhub {

static AVRational __avMicroseconds = {1, 1000000};

Publisher::Publisher() {
  m_szUrl = NULL;

  m_nVideoFormat = AV_PIX_FMT_NONE;
  m_nVideoInWidth = -1;
  m_nVideoInHeight = -1;
  m_nVideoOutWidth = -1;
  m_nVideoOutHeight = -1;
  m_nVideoFramerate = -1;
  m_nVideoBitRate = -1;

  m_nAudioFormat = AV_SAMPLE_FMT_NONE;
  m_nAudioInChannels = -1;
  m_nAudioInSampleRate = -1;
  m_nAudioOutChannels = -1;
  m_nAudioOutSampleRate = -1;
  m_nAudioBitRate = -1;

  m_nMaxVideoLag = 0;
  m_nMaxAudioLag = 0;

  m_bHasEncoders = false;

  m_pThreadCallback = NULL;
  m_pThreadCallbackArg = NULL;

  m_pOutputContext = NULL;

  m_pVideoStream = NULL;
  m_pAudioStream = NULL;

  m_pAudioFrame = NULL;
  m_pVideoFrame = NULL;

  memset(&m_SwsPicture, 0, sizeof(m_SwsPicture));
  m_pSwsContext = NULL;

  m_pSwrContext = NULL;
  memset(&m_pAudioBuffer, 0, sizeof(m_pAudioBuffer));
  m_nAudioBufferSamples = 0;

  m_bStop = false;
  m_bStopped = true;

  m_nLastPushedTimestamp = INT_MIN;
}
Publisher::~Publisher() {
  destroy();
}

int Publisher::create(
  const char* url,
  AVPixelFormat videoFormat,
  int videoInWidth,
  int videoInHeight,
  int videoOutWidth,
  int videoOutHeight,
  int videoFramerate,
  int videoBitRate,

  AVSampleFormat audioFormat,
  int audioInChannels,
  int audioInSampleRate,
  int audioOutChannels,
  int audioOutSampleRate,
  int audioBitRate,

  long maxVideoLag,
  long maxAudioLag,

  const char* szX264Profile,
  const char* szX264Level,
  const char* szX264Preset,
  const char* szX264Tune,

  bool createEncoders,

  ThreadCallback threadCallback,
  void* threadCallbackArg
) {
  int result;

  m_szUrl = strdup(url);
  if (m_szUrl == NULL) {
    LOGE("Publisher: Couldn't allocate the URL");
    result = AVERROR(ENOMEM);
    goto create_done;
  }

  m_nVideoFormat = videoFormat;
  m_nVideoInWidth = videoInWidth;
  m_nVideoInHeight = videoInHeight;
  m_nVideoOutWidth = videoOutWidth;
  m_nVideoOutHeight = videoOutHeight;
  m_nVideoFramerate = videoFramerate;
  m_nVideoBitRate = videoBitRate;

  m_nAudioFormat = audioFormat;
  m_nAudioInChannels = audioInChannels;
  m_nAudioInSampleRate = audioInSampleRate;
  m_nAudioOutChannels = audioOutChannels;
  m_nAudioOutSampleRate = audioOutSampleRate;
  m_nAudioBitRate = audioBitRate;

  m_nMaxVideoLag = maxVideoLag;
  m_nMaxAudioLag = maxAudioLag;

  if ((result = avformat_alloc_output_context2(&m_pOutputContext, 0, "flv", 0)) != 0) {
    LOGE("Publisher: Couldn't allocate the output context");
    goto create_done;
  }

  {
  AVCodec* pCodec = avcodec_find_encoder(AV_CODEC_ID_H264);

  m_pVideoStream = avformat_new_stream(m_pOutputContext, pCodec);
  if (m_pVideoStream == NULL) {
    LOGE("Publisher VIDEO: Couldn't allocate the video stream");
    result = AVERROR(ENOMEM);
    goto create_done;
  }

  m_pVideoStream->codec->pix_fmt = AV_PIX_FMT_NONE;
  if (pCodec && pCodec->pix_fmts && (*pCodec->pix_fmts != AV_PIX_FMT_NONE)) {
    const AVPixelFormat* format = pCodec->pix_fmts;
    while (*format != AV_PIX_FMT_NONE) {
      LOGD("Publisher VIDEO: Available format: %d", *format);
      if (*format == videoFormat) {
        LOGD("Publisher VIDEO: Found format match: %d", *format);
        m_pVideoStream->codec->pix_fmt = *format;
        break;
      }
      ++format;
    }
    if (pCodec && (m_pVideoStream->codec->pix_fmt == AV_PIX_FMT_NONE)) {
      LOGW("Publisher VIDEO: Input pixel format not suitable for the encoder, using transform");
      m_pVideoStream->codec->pix_fmt = *pCodec->pix_fmts;
    }
  }

  m_pVideoStream->codec->strict_std_compliance = FF_COMPLIANCE_EXPERIMENTAL;
  m_pVideoStream->codec->time_base.num = 1;
  m_pVideoStream->codec->time_base.den = 1000000;

  m_pVideoStream->avg_frame_rate.num = videoFramerate;
  m_pVideoStream->avg_frame_rate.den = 1;

  m_pVideoStream->codec->width = videoOutWidth;
  m_pVideoStream->codec->height = videoOutHeight;
  m_pVideoStream->codec->bit_rate = videoBitRate;
  m_pVideoStream->codec->bit_rate_tolerance = 2*videoBitRate;
  m_pVideoStream->codec->rc_min_rate = videoBitRate;
  m_pVideoStream->codec->rc_max_rate = videoBitRate;
  m_pVideoStream->codec->rc_buffer_size = videoBitRate/2;

  if (szX264Profile && (*szX264Profile != '\0')) {
    av_opt_set(m_pVideoStream->codec->priv_data, "profile", szX264Profile, AV_OPT_SEARCH_CHILDREN);
  }
  if (szX264Level && (*szX264Level != '\0')) {
    av_opt_set(m_pVideoStream->codec->priv_data, "level", szX264Level, AV_OPT_SEARCH_CHILDREN);
  }
  if (szX264Preset && (*szX264Preset != '\0')) {
    av_opt_set(m_pVideoStream->codec->priv_data, "preset", szX264Preset, AV_OPT_SEARCH_CHILDREN);
  }
  if (szX264Tune && (*szX264Tune != '\0')) {
    av_opt_set(m_pVideoStream->codec->priv_data, "tune", szX264Tune, AV_OPT_SEARCH_CHILDREN);
  }

  if (pCodec !=  NULL) {
    if ((result = avcodec_open2(m_pVideoStream->codec, pCodec, 0)) != 0) {
      LOGE("Publisher VIDEO: Couldn't open the video codec");
      goto create_done;
    }
    m_pVideoStream->codec->flags |= CODEC_FLAG_GLOBAL_HEADER;
  } else {
    m_pVideoStream->codec->flags |= CODEC_FLAG_GLOBAL_HEADER;

    m_pVideoStream->codec->codec_type = AVMEDIA_TYPE_VIDEO;
    m_pVideoStream->codec->codec_id = AV_CODEC_ID_H264;
    m_pVideoStream->codec->codec_tag = av_codec_get_tag(m_pOutputContext->oformat->codec_tag, m_pVideoStream->codec->codec_id);
  }
  if (createEncoders) {
    if (m_pVideoStream->codec->pix_fmt != m_nVideoFormat || m_nVideoInWidth != m_nVideoOutWidth || m_nVideoInHeight != m_nVideoOutHeight) {
      // TODO: Do cropping to keep the input aspect ration
      m_pSwsContext = sws_getContext(
        m_nVideoInWidth, m_nVideoInHeight, (AVPixelFormat)m_nVideoFormat,
        m_nVideoOutWidth, m_nVideoOutHeight, m_pVideoStream->codec->pix_fmt,
        SWS_POINT,
        NULL,
        NULL,
        NULL
      );
      if (m_pSwsContext == NULL) {
        LOGE("Publisher VIDEO: Couldn't allocate the image converter");
        result = AVERROR(ENOMEM);
        goto create_done;
      }
      if ((result = avpicture_alloc(&m_SwsPicture, m_pVideoStream->codec->pix_fmt, m_nVideoOutWidth, m_nVideoOutHeight)) != 0) {
        LOGE("Publisher VIDEO: Couldn't allocate the intermediate image for the converter");
        goto create_done;
      }
    }

    m_pVideoFrame = avcodec_alloc_frame();
    if (m_pVideoFrame == NULL) {
      LOGE("Publisher VIDEO: Couldn't allocate the video frame");
      result = AVERROR(ENOMEM);
      goto create_done;
    }
  }
  }

  {
  AVCodec* pCodec = avcodec_find_encoder(AV_CODEC_ID_AAC);

  m_pAudioStream = avformat_new_stream(m_pOutputContext, pCodec);
  if (m_pAudioStream == NULL) {
    LOGE("Publisher AUDIO: Couldn't allocate the audio stream");
    result = AVERROR(ENOMEM);
    goto create_done;
  }

  m_pAudioStream->codec->sample_fmt = AV_SAMPLE_FMT_NONE;
  if (pCodec && pCodec->sample_fmts && (*pCodec->sample_fmts != AV_SAMPLE_FMT_NONE)) {
    const AVSampleFormat* format = pCodec->sample_fmts;
    while (*format != AV_SAMPLE_FMT_NONE) {
      if (*format == m_nAudioFormat) {
        m_pAudioStream->codec->sample_fmt = *format;
        break;
      }
      ++format;
    }
    if (pCodec && (m_pAudioStream->codec->sample_fmt == AV_SAMPLE_FMT_NONE)) {
      LOGW("Publisher AUDIO: Input sample format not suitable for the encoder, using transform");
      m_pAudioStream->codec->sample_fmt = *pCodec->sample_fmts;
    }
  }

  m_pAudioStream->codec->strict_std_compliance = FF_COMPLIANCE_EXPERIMENTAL;
  m_pAudioStream->codec->time_base.num = 1;
  m_pAudioStream->codec->time_base.den = 1000000;

  m_pAudioStream->codec->sample_rate = m_nAudioOutSampleRate;
  m_pAudioStream->codec->channels = m_nAudioOutChannels;
  m_pAudioStream->codec->bit_rate = m_nAudioBitRate;

  if (pCodec != NULL) {
    if ((result = avcodec_open2(m_pAudioStream->codec, pCodec, 0)) != 0) {
      LOGE("Publisher VIDEO: Couldn't open the audio codec");
      goto create_done;
    }
    m_pAudioStream->codec->flags |= CODEC_FLAG_GLOBAL_HEADER;
  } else {
    m_pAudioStream->codec->flags |= CODEC_FLAG_GLOBAL_HEADER;

    m_pAudioStream->codec->codec_type = AVMEDIA_TYPE_AUDIO;
    m_pAudioStream->codec->codec_id = AV_CODEC_ID_AAC;
    m_pAudioStream->codec->codec_tag = av_codec_get_tag(m_pOutputContext->oformat->codec_tag, m_pAudioStream->codec->codec_id);
  }
  if (createEncoders) {
    m_pSwrContext = swr_alloc();
    if (m_pSwrContext == NULL) {
      LOGE("Publisher AUDIO: Couldn't allocate the audio converter");
      result = AVERROR(ENOMEM);
      goto create_done;
    }
    av_opt_set_int(m_pSwrContext, "in_channel_layout",  av_get_default_channel_layout(m_nAudioInChannels), 0);
    av_opt_set_int(m_pSwrContext, "out_channel_layout", av_get_default_channel_layout(m_pAudioStream->codec->channels), 0);

    av_opt_set_int(m_pSwrContext, "in_sample_rate", m_nAudioInSampleRate, 0);
    av_opt_set_int(m_pSwrContext, "out_sample_rate", m_pAudioStream->codec->sample_rate, 0);

    av_opt_set_sample_fmt(m_pSwrContext, "in_sample_fmt", (AVSampleFormat)m_nAudioFormat, 0);
    av_opt_set_sample_fmt(m_pSwrContext, "out_sample_fmt", m_pAudioStream->codec->sample_fmt, 0);

    if ((result = swr_init(m_pSwrContext)) != 0) {
      LOGE("Publisher AUDIO: Couldn't initialize the audio converter");
      goto create_done;
    }

    m_pAudioFrame = avcodec_alloc_frame();
    if (m_pAudioFrame == NULL) {
      LOGE("Publisher AUDIO: Couldn't allocate the audio frame");
      result = AVERROR(ENOMEM);
      goto create_done;
    }
  }
  }
  m_bHasEncoders = createEncoders;

  m_pThreadCallback = threadCallback;
  m_pThreadCallbackArg = threadCallbackArg;
  return 0;

create_done:
  free();
  return result;
}

// creates encoders too
int Publisher::create(
  const char* url,
  AVPixelFormat videoFormat,
  int videoInWidth,
  int videoInHeight,
  int videoOutWidth,
  int videoOutHeight,
  int videoFramerate,
  int videoBitRate,

  AVSampleFormat audioFormat,
  int audioInChannels,
  int audioInSampleRate,
  int audioOutChannels,
  int audioOutSampleRate,
  int audioBitRate,

  long maxVideoLag,
  long maxAudioLag,

  const char* szX264Profile,
  const char* szX264Level,
  const char* szX264Preset,
  const char* szX264Tune,

  ThreadCallback threadCallback,
  void* threadCallbackArg
) {
  int result;
  if ((result = create(
    url,
    videoFormat,
    videoInWidth,
    videoInHeight,
    videoOutWidth,
    videoOutHeight,
    videoFramerate,
    videoBitRate,

    audioFormat,
    audioInChannels,
    audioInSampleRate,
    audioOutChannels,
    audioOutSampleRate,
    audioBitRate,

    maxVideoLag,
    maxAudioLag,

    szX264Profile,
    szX264Level,
    szX264Preset,
    szX264Tune,

    true,

    threadCallback,
    threadCallbackArg
  )) == 0) {
    if ((result = run()) == 0)
      return 0;
    free();
  }
  return result;
}
// creates only the muxer
int Publisher::create(
  const char* url,
  AVPixelFormat videoFormat,
  int videoOutWidth,
  int videoOutHeight,
  int videoFramerate,
  int videoBitRate,

  AVSampleFormat audioFormat,
  int audioOutChannels,
  int audioOutSampleRate,
  int audioBitRate,

  const void* videoHeaders,
  int videoHeadersLength,
  const void* audioHeaders,
  int audioHeadersLength,

  long maxVideoLag,
  long maxAudioLag,

  ThreadCallback threadCallback,
  void* threadCallbackArg
) {
  int result;
  if ((result = create(
    url,
    videoFormat,
    -1,
    -1,
    videoOutWidth,
    videoOutHeight,
    videoFramerate,
    videoBitRate,

    audioFormat,
    -1,
    -1,
    audioOutChannels,
    audioOutSampleRate,
    audioBitRate,

    maxVideoLag,
    maxAudioLag,

    NULL,
    NULL,
    NULL,
    NULL,

    false,

    threadCallback,
    threadCallbackArg
  )) == 0) {
    if (audioHeaders != NULL) {
      m_pAudioStream->codec->extradata = (unsigned char*)av_malloc(audioHeadersLength);
      if (m_pAudioStream->codec->extradata == NULL) {
        result = AVERROR(ENOMEM);
        goto create_done;
      }
      m_pAudioStream->codec->extradata_size = audioHeadersLength;
      memcpy(m_pAudioStream->codec->extradata, audioHeaders, audioHeadersLength);
    }
    if (videoHeaders != NULL) {
      m_pVideoStream->codec->extradata = (unsigned char*)av_malloc(videoHeadersLength);
      if (m_pVideoStream->codec->extradata == NULL) {
        result = AVERROR(ENOMEM);
        goto create_done;
      }
      m_pVideoStream->codec->extradata_size = videoHeadersLength;
      memcpy(m_pVideoStream->codec->extradata, videoHeaders, videoHeadersLength);
    }

    if ((result = run()) == 0)
      return 0;

create_done:
    free();
  }
  return result;
}

void Publisher::destroy() {
  {
  MutexLocker lock(m_QueueMutex);
  m_bStop = true;

  m_QueueCondition.signal();
  }

  m_Thread.join();
  m_nLastPushedTimestamp = INT_MIN;

  free();
}

void Publisher::free() {
  if (m_pAudioFrame != NULL) {
    avcodec_free_frame(&m_pAudioFrame);
  }
  if (m_pVideoFrame != NULL) {
    avcodec_free_frame(&m_pVideoFrame);
  }

  if (m_pAudioStream != NULL) {
    avcodec_close(m_pAudioStream->codec);
    m_pAudioStream = NULL;
  }
  if (m_pVideoStream != NULL) {
    avcodec_close(m_pVideoStream->codec);
    m_pVideoStream = NULL;
  }

  if (m_pOutputContext != NULL) {
    if (m_pOutputContext->pb != NULL) {
      avio_closep(&m_pOutputContext->pb);
    }
    av_freep(&m_pOutputContext);
  }

  avpicture_free(&m_SwsPicture);
  memset(&m_SwsPicture, 0, sizeof(m_SwsPicture));

  if (m_pSwsContext != NULL) {
    sws_freeContext(m_pSwsContext);
    m_pSwsContext = NULL;
  }

  if (m_pSwrContext != NULL) {
    swr_free(&m_pSwrContext);
  }
  if (m_nAudioBufferSamples > 0) {
    av_freep(&m_pAudioBuffer[0]);
    m_nAudioBufferSamples = 0;
  }

  m_pThreadCallback = NULL;
}

int Publisher::run() {
  m_bStop = false;
  m_bStopped = false;

  m_sInterruptedCB.callback = avioInterruptCB;
  m_sInterruptedCB.opaque = this;

  if (m_Thread.start(Publisher::thread_entry_point, this)) {
    return 0;
  }
  return AVERROR(ENOMEM);
}

int Publisher::processQueueFrame(Frame* frame) {
  int result = 0;

  long timestampDiff = (m_nLastPushedTimestamp - frame->m_nTimestamp);

  if ((frame->m_Type == PACKET_VIDEO) || (frame->m_Type == ENCODED_VIDEO)) {
    if (timestampDiff > m_nMaxVideoLag) {
      LOGW("Publisher: Dropping video because of lag");

      m_QueueMutex.unlock();
      delete frame;
      m_QueueMutex.lock();

      m_bNeedsVideoKeyframe = true;
      goto process_queue_frame_done;
    } else
    if (m_bNeedsVideoKeyframe && ((frame->m_nFlags & FRAME_KEYFRAME) == 0)) {
      LOGW("Publisher: Dropping video waiting for keyframe");

      m_QueueMutex.unlock();
      delete frame;
      m_QueueMutex.lock();

      goto process_queue_frame_done;
    }
    m_bNeedsVideoKeyframe = false;
  }
  else
  if ((frame->m_Type == PACKET_AUDIO) || (frame->m_Type == ENCODED_AUDIO)) {
    if (timestampDiff > m_nMaxAudioLag) {
      LOGW("Publisher: Dropping audio because of lag");

      m_QueueMutex.unlock();
      delete frame;
      m_QueueMutex.lock();

      m_bNeedsAudioKeyframe = true;
      goto process_queue_frame_done;
    } else
    if (m_bNeedsAudioKeyframe && ((frame->m_nFlags & FRAME_KEYFRAME) == 0)) {
      LOGW("Publisher: Dropping audio waiting for keyframe");

      m_QueueMutex.unlock();
      delete frame;
      m_QueueMutex.lock();

      goto process_queue_frame_done;
    }
    m_bNeedsAudioKeyframe = false;
  }

  result = processFrame(frame);

process_queue_frame_done:
  return result;
}

void* Publisher::thread_entry_point(void* arg) {
  return ((Publisher*)arg)->thread_entry_point();
}
void* Publisher::thread_entry_point() {
  int result = 0;

  AVIOContext* bufferContext = NULL;
  AVIOContext* httpContext = NULL;

  Buffer* buffer = NULL;

  AVDictionary* params = NULL;

  if (m_pThreadCallback != NULL) {
    m_pThreadCallback(m_pThreadCallbackArg, true, 0);
  }

  m_QueueMutex.lock();

  if ((result = buffer_create(&buffer)) != 0) {
    LOGE("Publisher: Couldn't create the buffer for the buffer I/O");
    goto thread_done;
  }

  unsigned char b[4096];
  bufferContext = avio_alloc_context(b, 4096, 1, buffer, 0, buffer_write, buffer_seek);
  if (bufferContext == NULL) {
    result = AVERROR(ENOMEM);

    LOGE("Publisher: Couldn't open the buffer I/O");
    goto thread_done;
  }

  av_dict_set(&params, "chunked_post", "1", 0);
  av_dict_set(&params, "headers", "Content-Type: video/x-flv\r\n", 0);

  if ((result = avio_open2(&m_pOutputContext->pb, m_szUrl, AVIO_FLAG_WRITE, &m_sInterruptedCB, &params)) != 0) {
    LOGE("Publisher: Couldn't open the output");
    av_dict_free(&params);
    goto thread_done;
  }
  av_dict_free(&params);

  httpContext = m_pOutputContext->pb;
  m_pOutputContext->pb = bufferContext;

  if ((result = avformat_write_header(m_pOutputContext, 0)) != 0) {
    m_pOutputContext->pb = httpContext;

    LOGE("Publisher: Couldn't buffer the stream header");
    goto thread_done;
  }

  avio_flush(bufferContext);
  m_pOutputContext->pb = httpContext;

  avio_write(m_pOutputContext->pb, buffer->data, buffer->length);

  m_bNeedsVideoKeyframe = true;
  m_bNeedsAudioKeyframe = true;

  LOGD("Publisher: Started streaming...");
  while ((result == 0) && !m_bStop) {
    m_QueueCondition.wait(m_QueueMutex);

    while ((result == 0) && !m_Queue.empty()) {
      Frame* frame = *m_Queue.begin();
      m_Queue.pop_front();

      result = processQueueFrame(frame);
    }
  }

  // make sure that any pushes are done, but keeping lock order
  m_QueueMutex.unlock();
  m_AudioMutex.lock();
  m_VideoMutex.lock();

  m_bStopped = true;

  m_VideoMutex.unlock();
  m_AudioMutex.unlock();
  m_QueueMutex.lock();

  if (result == 0) {
    // flush the encoders, it it's the case
    int gotPacket;
    if (m_bHasEncoders) {
      if ((result = encodeAndPushAudio(NULL, &gotPacket)) != 0)
        goto thread_terminated;
      if ((result = encodeAndPushVideo(NULL, &gotPacket)) != 0)
        goto thread_terminated;
    }

    // flush the queue
    while ((result == 0) && !m_Queue.empty()) {
      Frame* frame = *m_Queue.begin();
      m_Queue.pop_front();

      result = processQueueFrame(frame);
    }

    
    // flush the muxer
    result = av_interleaved_write_frame(m_pOutputContext, NULL);
    if (result == 0) {
      av_write_trailer(m_pOutputContext);
    }
  }

thread_terminated:
  LOGD("Publisher: Finished streaming...");

thread_done:
  // delete any remaining frames
  while (!m_Queue.empty()) {
    Frame* frame = *m_Queue.begin();
    m_Queue.pop_front();

    m_QueueMutex.unlock();
    delete frame;
    m_QueueMutex.lock();
  }

  m_QueueMutex.unlock();

  if (buffer != NULL) {
    buffer_destroy(&buffer);
    if (bufferContext != NULL) {
      av_freep(&bufferContext);
    }
  }

  if (m_pThreadCallback != NULL) {
    m_pThreadCallback(m_pThreadCallbackArg, false, result);
  }
  return NULL;
}

int Publisher::outputVideo(AVPacket* video) {
  video->stream_index = m_pVideoStream->index;
  if (video->pts != AV_NOPTS_VALUE) {
    video->pts = av_rescale_q(
      video->pts,
      m_pVideoStream->codec->time_base,
      m_pVideoStream->time_base
    );
  }
  if (video->dts != AV_NOPTS_VALUE) {
    video->dts = av_rescale_q(
      video->dts,
      m_pVideoStream->codec->time_base,
      m_pVideoStream->time_base
    );
  }

  int result = av_interleaved_write_frame(m_pOutputContext, video);
  if (result != 0) {
    LOGE("Publisher VIDEO: Failed sending packet, size %d, timestamp: %ld, %s, result: %d", video->size, (long)video->pts, (video->flags & AV_PKT_FLAG_KEY) ? "KEYFRAME" : "", result);
  } else {
    LOGD("Publisher VIDEO: Sent packet, size %d, timestamp: %ld, %s", video->size, (long)video->pts, (video->flags & AV_PKT_FLAG_KEY) ? "KEYFRAME" : "");
  }
  return result;
}
int Publisher::outputAudio(AVPacket* audio) {
  audio->stream_index = m_pAudioStream->index;
  if (audio->pts != AV_NOPTS_VALUE) {
    audio->pts = av_rescale_q(
      audio->pts,
      m_pAudioStream->codec->time_base,
      m_pAudioStream->time_base
    );
  }
  if (audio->dts != AV_NOPTS_VALUE) {
    audio->dts = av_rescale_q(
      audio->dts,
      m_pAudioStream->codec->time_base,
      m_pAudioStream->time_base
    );
  }

  int result = av_interleaved_write_frame(m_pOutputContext, audio);
  if (result != 0) {
    LOGE("Publisher AUDIO: Failed sending packet, size %d, timestamp: %ld, %s, result: %d", audio->size, (long)audio->pts, (audio->flags & AV_PKT_FLAG_KEY) ? "KEYFRAME" : "", result);
  } else {
    LOGD("Publisher AUDIO: Sent packet, size %d, timestamp: %ld, %s", audio->size, (long)audio->pts, (audio->flags & AV_PKT_FLAG_KEY) ? "KEYFRAME" : "");
  }
  return result;
}

static void PacketDestructor(Publisher::FrameType type, void* pOpaque, const void* pData, int nSize, long nTimestamp) {
 av_free_packet((AVPacket*)pOpaque);
 ::free(pOpaque);
}

int Publisher::encodeAndPushVideo(AVFrame* video, int* gotPacket) {
  int result = 0;

  do {
    AVPacket pkt = {0};
    av_init_packet(&pkt);

    if ((result = avcodec_encode_video2(m_pVideoStream->codec, &pkt, video, gotPacket)) == 0) {
      if (*gotPacket) {
        AVPacket* encoded = (AVPacket*)malloc(sizeof(AVPacket));
        if (encoded == NULL) {
          result = -ENOMEM;
          av_free_packet(&pkt);
        } else {
          av_init_packet(encoded);
          *encoded = pkt;

          if ((result = pushPacketVideo(encoded, PacketDestructor, encoded, 0, (long)encoded->pts, 0)) != 0) {
            av_free_packet(&pkt);
            ::free(encoded);
          }
        }
      }
    }
  } while ((video == NULL) && (result == 0) && *gotPacket);
  return result;
}
int Publisher::encodeAndPushAudio(AVFrame* audio, int* gotPacket) {
  int result = 0;

  do {
    AVPacket pkt = {0};
    av_init_packet(&pkt);

    if ((result = avcodec_encode_audio2(m_pAudioStream->codec, &pkt, audio, gotPacket)) == 0) {
      if (*gotPacket) {
        AVPacket* encoded = (AVPacket*)malloc(sizeof(AVPacket));
        if (encoded == NULL) {
          result = -ENOMEM;
          av_free_packet(&pkt);
        } else {
          av_init_packet(encoded);
          *encoded = pkt;

          if ((result = pushPacketAudio(encoded, PacketDestructor, encoded, 0, (long)encoded->pts, 0)) != 0) {
            av_free_packet(&pkt);
            ::free(encoded);
          }
        }
      }
    }
  } while ((audio == NULL) && (result == 0) && *gotPacket);
  return result;
}

int Publisher::pushRawVideo(void* opaque, FrameDestructor destructor, const void* video, int size, long timestamp, unsigned int flags) {
  int result = 0;

  LOGD("Publisher VIDEO: Pushing raw at %ld - %s", timestamp, (flags & FRAME_KEYFRAME) ? "KEYFRAME" : "");
  m_VideoMutex.lock();

  bool stopped;
  m_QueueMutex.lock();
  stopped = m_bStopped;
  m_QueueMutex.unlock();

  // silently ignore any new frame while stopping
  if (!stopped) {
    if (video) {
      if (m_pVideoFrame->extended_data != m_pVideoFrame->data) {
        av_freep(&m_pVideoFrame->extended_data);
      }
      avcodec_get_frame_defaults(m_pVideoFrame);

      avpicture_fill((AVPicture*)m_pVideoFrame, (const unsigned char*)video, (AVPixelFormat)m_nVideoFormat, m_nVideoInWidth, m_nVideoInHeight);
      if ((flags & FRAME_FLIP_Y) != 0) {
        m_pVideoFrame->data[0] += m_pVideoFrame->linesize[0]*(m_nVideoInHeight-1);
        m_pVideoFrame->linesize[0] *= -1;
      }

      if (m_pSwsContext != NULL) {
        sws_scale(m_pSwsContext, m_pVideoFrame->data, m_pVideoFrame->linesize, 0, m_nVideoInHeight, m_SwsPicture.data, m_SwsPicture.linesize);
        avpicture_fill((AVPicture*)m_pVideoFrame, (const unsigned char*)m_SwsPicture.data[0], m_pVideoStream->codec->pix_fmt, m_nVideoOutWidth, m_nVideoOutHeight);

        m_pVideoFrame->width = m_nVideoOutWidth;
        m_pVideoFrame->height = m_nVideoOutHeight;
        m_pVideoFrame->format = m_pVideoStream->codec->pix_fmt;
      } else {
        m_pVideoFrame->width = m_nVideoInWidth;
        m_pVideoFrame->height = m_nVideoInHeight;
        m_pVideoFrame->format = m_nVideoFormat;
      }

      m_pVideoFrame->pts = timestamp;
    }

    if (result == 0) {
      int gotPacket;
      result = encodeAndPushVideo(m_pVideoFrame, &gotPacket);
    }
  }

  m_VideoMutex.unlock();

  if (destructor != NULL) {
    destructor(RAW_VIDEO, opaque, video, size, timestamp);
  }
  LOGD("Publisher VIDEO: ENCODE +++ DONE");
  return result;
}
int Publisher::pushRawAudio(void* opaque, FrameDestructor destructor, const void* audio, int size, long timestamp, unsigned int flags) {
  int result = 0;

  LOGD("Publisher AUDIO: Pushing raw at %ld - %s", timestamp, (flags & FRAME_KEYFRAME) ? "KEYFRAME" : "");
  m_AudioMutex.lock();

  bool stopped;
  m_QueueMutex.lock();
  stopped = m_bStopped;
  m_QueueMutex.unlock();

  // silently ignore any new frame while stopping
  if (!stopped) {
    if (audio) {
      int inBytesPerSample;
      inBytesPerSample = av_get_bytes_per_sample((AVSampleFormat)m_nAudioFormat);

      int inSamples;
      inSamples = size/inBytesPerSample/m_nAudioInChannels;

      unsigned char* inData[SWR_CH_MAX];
      av_samples_fill_arrays(inData, 0, (const unsigned char*)audio, m_nAudioInChannels, inSamples, (AVSampleFormat)m_nAudioFormat, 0);

      int result;
      if ((result = swr_convert(
        m_pSwrContext,
        0,
        0,
        (const unsigned char**)inData,
        inSamples
      )) != 0 ) {
        LOGE("Publisher AUDIO: Resampling failed with %d", result);
        goto push_raw_audio_done;
      }
    }

    int outAvailable;
    outAvailable = (int)swr_get_delay(m_pSwrContext, m_nAudioInSampleRate);

    while ((result == 0) && (outAvailable > 0)) {
      int outSamples = outAvailable;
      if (m_pAudioStream->codec->frame_size > 0) {
        outSamples = m_pAudioStream->codec->frame_size;
        if (outSamples > outAvailable) {
          if (audio != NULL)
            break;
          else
            outSamples = outAvailable;
        }
      }

      if (outSamples > m_nAudioBufferSamples) {
        av_freep(&m_pAudioBuffer[0]);

        if (av_samples_alloc(
          m_pAudioBuffer,
          0,
          m_pAudioStream->codec->channels,
          outSamples,
          m_pAudioStream->codec->sample_fmt,
          0
        ) != 0) {
          LOGW("Publisher AUDIO: Skipping an audio frame, not enough memory allocating output buffer");
          result = AVERROR(ENOMEM);
          goto push_raw_audio_done;
        }
        m_nAudioBufferSamples = outSamples;
      }

      int convertedSamples;
      convertedSamples = swr_convert(
        m_pSwrContext,
        m_pAudioBuffer,
        outSamples,
        0,
        0
      );

      if (m_pAudioFrame->extended_data != m_pAudioFrame->data) {
        av_freep(&m_pAudioFrame->extended_data);
      }
      avcodec_get_frame_defaults(m_pAudioFrame);

      m_pAudioFrame->format = m_nAudioFormat;
      m_pAudioFrame->nb_samples = convertedSamples;

      avcodec_fill_audio_frame(
        m_pAudioFrame,
        m_pAudioStream->codec->channels,
        m_pAudioStream->codec->sample_fmt,
        m_pAudioBuffer[0],
        av_samples_get_buffer_size(
          0,
          m_pAudioStream->codec->channels,
          convertedSamples,
          m_pAudioStream->codec->sample_fmt,
          0
        ),
        0
      );

      int gotPacket;
      if ((result = encodeAndPushAudio(m_pAudioFrame, &gotPacket)) == 0) {
        outAvailable = (int)swr_get_delay(m_pSwrContext, (int)m_nAudioInSampleRate);
      }
    }
  }

push_raw_audio_done:
  m_AudioMutex.unlock();

  if (destructor != NULL) {
    destructor(RAW_AUDIO, opaque, audio, size, timestamp);
  }
  LOGD("Publisher AUDIO: ENCODE --- DONE");
  return result;
}

int Publisher::pushFrame(FrameType type, void* opaque, FrameDestructor destructor, const void* data, int size, long timestamp, unsigned int flags) {
  Frame* frame = new Frame(type, opaque, data, size, timestamp, flags, destructor);
  if (frame != NULL) {
    MutexLocker lock(m_QueueMutex);
    // silently ignore any new frame while stopping
    if (m_bStopped) {
      m_QueueMutex.unlock();
      delete frame;
      m_QueueMutex.lock();
    } else {
      m_Queue.push_back(frame);

      m_nLastPushedTimestamp = timestamp;
      m_QueueCondition.signal();
    }
    return 0;
  }
  return AVERROR(ENOMEM);
}

int Publisher::processEncodedVideo(Frame* frame) {
  LOGD("Publisher VIDEO: Processing encoded, size %d, timestamp: %ld, %s", frame->m_nSize, frame->m_nTimestamp, (frame->m_nFlags & FRAME_KEYFRAME) ? "KEYFRAME" : "");

  AVPacket pkt = {0};
  av_init_packet(&pkt);

  pkt.data = (unsigned char*)frame->m_pData;
  pkt.size = frame->m_nSize;

  pkt.flags = ((frame->m_nFlags & FRAME_KEYFRAME) != 0) ? AV_PKT_FLAG_KEY : 0;

  if (frame->m_nTimestamp > 0) {
    pkt.pts = av_rescale_q(
      frame->m_nTimestamp,
      __avMicroseconds,
      m_pVideoStream->codec->time_base
    );
  }
  // TODO: Android - how can we retrieve the DTS from the MediaCodec output ?
  pkt.dts = AV_NOPTS_VALUE;

  int result = outputVideo(&pkt);
  if (result != 0) {
    av_free_packet(&pkt);
  }
  return result;
}
int Publisher::processEncodedAudio(Frame* frame) {
  LOGD("Publisher AUDIO: Processing encoded, size %d, timestamp: %ld, %s", frame->m_nSize, frame->m_nTimestamp, (frame->m_nFlags & FRAME_KEYFRAME) ? "KEYFRAME" : "");

  AVPacket pkt = {0};
  av_init_packet(&pkt);

  pkt.data = (unsigned char*)frame->m_pData;
  pkt.size = frame->m_nSize;

  pkt.flags = AV_PKT_FLAG_KEY;

  if (frame->m_nTimestamp > 0) {
    pkt.pts = av_rescale_q(
      frame->m_nTimestamp,
      __avMicroseconds,
      m_pAudioStream->codec->time_base
    );
  }
  pkt.dts = AV_NOPTS_VALUE;

  int result = outputAudio(&pkt);
  if (result != 0) {
    av_free_packet(&pkt);
  }
  return result;
}

int Publisher::processPacketVideo(Frame* frame) {
  int result = 0;

  if (frame != NULL) {
    LOGD("Publisher VIDEO: Processing packet, size %d, timestamp: %ld, %s", frame->m_nSize, frame->m_nTimestamp, (frame->m_nFlags & FRAME_KEYFRAME) ? "KEYFRAME" : "");

    AVPacket* pkt = (AVPacket*)frame->m_pData;
    if (pkt != NULL) {
      result = outputVideo(pkt);
      if (result == 0) {
        // it was taken over by the muxer
        frame->m_Destructor = NULL;
      }
    }
  }
  return result;
}
int Publisher::processPacketAudio(Frame* frame) {
  int result = 0;

  if (frame != NULL) {
    LOGD("Publisher AUDIO: Processing packet, size %d, timestamp: %ld, %s", frame->m_nSize, frame->m_nTimestamp, (frame->m_nFlags & FRAME_KEYFRAME) ? "KEYFRAME" : "");

    AVPacket* pkt = (AVPacket*)frame->m_pData;
    if (pkt != NULL) {
      result = outputAudio(pkt);
      if (result == 0) {
        // it was taken over by the muxer
        frame->m_Destructor = NULL;
      }
    }
  }
  return result;
}

int Publisher::avioInterruptCB(void* opaque) {
  Publisher* publisher = (Publisher*)opaque;

  MutexLocker lock(publisher->m_QueueMutex);
  return publisher->m_bStop ? 1 : 0;
}

}
