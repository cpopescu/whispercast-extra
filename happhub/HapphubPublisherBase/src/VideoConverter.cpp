#include "VideoConverter.h"

namespace happhub {

VideoConverter::VideoConverter() {
  m_nInFormat = 0;
  m_nInWidth = 0;
  m_nInHeight = 0;

  m_nOutFormat = 0;
  m_nOutWidth = 0;
  m_nOutHeight = 0;

  m_pSwsContext = 0;
}
VideoConverter::~VideoConverter() {
  destroy();
}

int VideoConverter::create(
  int inFormat,
  int inWidth,
  int inHeight,

  int outFormat,
  int outWidth,
  int outHeight
) {
  m_nInFormat = inFormat;
  m_nInWidth = inWidth;
  m_nInHeight = inHeight;

  m_nOutFormat = outFormat;
  m_nOutWidth = outWidth;
  m_nOutHeight = outHeight;

  // TODO: Do cropping to keep the input aspect ration
  m_pSwsContext = sws_getContext(
    m_nInWidth, m_nInHeight, (AVPixelFormat)m_nInFormat,
    m_nOutWidth, m_nOutHeight, (AVPixelFormat)m_nOutFormat,
    SWS_POINT,
    NULL,
    NULL,
    NULL
  );
  if (m_pSwsContext == 0) {
    LOGE("VIDEO: Couldn't allocate the image converter");
    goto done;
  }
  return 0;

done:
  free();
  return -1;
}
void VideoConverter::destroy() {
  free();
}

void VideoConverter::free() {
  if (m_pSwsContext != 0) {
    sws_freeContext(m_pSwsContext);
    m_pSwsContext = 0;
  }
}

int VideoConverter::getInputSize() {
  return avpicture_get_size((AVPixelFormat)m_nInFormat, m_nInWidth, m_nInHeight);
}
int VideoConverter::getOutputSize() {
  return avpicture_get_size((AVPixelFormat)m_nOutFormat, m_nOutWidth, m_nOutHeight);
}

int VideoConverter::convert(void* in, void* out) {
  LOGV("VIDEO: Scaling from (%dx%d, %d) to (%dx%d, %d)", m_nInWidth, m_nInHeight, m_nInFormat, m_nOutWidth, m_nOutHeight, m_nOutFormat);

  AVPicture inPicture;
  avpicture_fill(&inPicture, (const unsigned char*)in, (AVPixelFormat)m_nInFormat, m_nInWidth, m_nInHeight);
  AVPicture outPicture;
  avpicture_fill(&outPicture, (const unsigned char*)out, (AVPixelFormat)m_nOutFormat, m_nOutWidth, m_nOutHeight);

  sws_scale(
    m_pSwsContext,
    inPicture.data,
    inPicture.linesize,
    0,
    m_nInHeight,
    outPicture.data,
    outPicture.linesize
  );

  return 0;
}

}
