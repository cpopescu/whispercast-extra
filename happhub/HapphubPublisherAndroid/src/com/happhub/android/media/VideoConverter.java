package com.happhub.android.media;

import java.nio.ByteBuffer;

public class VideoConverter {
  native
  protected long converterCreate(
    int inFormat,
    int inWidth,
    int inHeight,
    int outFormat,
    int outWidth,
    int outHeight
  );

  native
  protected void converterDestroy(long converter);

  native
  protected int converterGetInputSize(long converter);
  native
  protected int converterGetOutputSize(long converter);

  native
  protected int converterConvert(long converter, ByteBuffer in, ByteBuffer out);
  
  protected long mConverter = 0;

  public int create(
    AVPixelFormat inFormat,
    int inWidth,
    int inHeight,
    AVPixelFormat outFormat,
    int outWidth,
    int outHeight
  ) {
    mConverter = converterCreate(
      inFormat.ordinal(),
      inWidth,
      inHeight,
      outFormat.ordinal(),
      outWidth,
      outHeight
    );
    if (mConverter != 0) {
      return 0;
    }
    return -1;
  }
  public void destroy() {
    if (mConverter != 0) {
      converterDestroy(mConverter);
      mConverter = 0;
    }
  }
  
  public int getInputSize() {
    if (mConverter != 0) {
      return converterGetInputSize(mConverter);
    }
    return -1;
  }
  public int getOutputSize() {
    if (mConverter != 0) {
      return converterGetOutputSize(mConverter);
    }
    return -1;
  }
  
  public int convert(ByteBuffer in, ByteBuffer out) {
    if (mConverter != 0) {
      return converterConvert(mConverter, in, out);
    }
    return -1;
  }
}
