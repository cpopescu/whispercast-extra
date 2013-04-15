package com.happhub.android.media;

public enum AVSampleFormat {
  AV_SAMPLE_FMT_U8,          ///< unsigned 8 bits
  AV_SAMPLE_FMT_S16,         ///< signed 16 bits
  AV_SAMPLE_FMT_S32,         ///< signed 32 bits
  AV_SAMPLE_FMT_FLT,         ///< float
  AV_SAMPLE_FMT_DBL,         ///< double

  AV_SAMPLE_FMT_U8P,         ///< unsigned 8 bits, planar
  AV_SAMPLE_FMT_S16P,        ///< signed 16 bits, planar
  AV_SAMPLE_FMT_S32P,        ///< signed 32 bits, planar
  AV_SAMPLE_FMT_FLTP,        ///< float, planar
  AV_SAMPLE_FMT_DBLP;        ///< double, planar
  
  static
  int sampleSizeInBytes(AVSampleFormat format) {
    switch (format) {
      case AV_SAMPLE_FMT_U8:
        return 1;
      case AV_SAMPLE_FMT_S16:
        return 2;
      case AV_SAMPLE_FMT_S32:
        return 4;
      case AV_SAMPLE_FMT_FLT:
        return 4;
      case AV_SAMPLE_FMT_DBL:
        return 8;

      case AV_SAMPLE_FMT_U8P:
        return 1;
      case AV_SAMPLE_FMT_S16P:
        return 2;
      case AV_SAMPLE_FMT_S32P:
        return 4;
      case AV_SAMPLE_FMT_FLTP:
        return 4;
      case AV_SAMPLE_FMT_DBLP:
        return 8;
    }
    return -1;
  }
}
