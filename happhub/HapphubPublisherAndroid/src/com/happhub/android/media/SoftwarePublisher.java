package com.happhub.android.media;

import java.nio.ByteBuffer;

public class SoftwarePublisher implements Publisher.PublisherImpl {
  native
  private long publisherCreate(
    String url,
    
    int videoFormat,
    int videoInWidth,
    int videoInHeight,
    int videoOutWidth,
    int videoOutHeight,
    int videoFramerate,
    int videoBitRate,
    
    int audioFormat,
    int audioChannels,
    int audioSampleRate,
    int audioBitRate,
    
    long maxVideoLag,
    long maxAudioMaxLag,
    
    String x264Profile,
    String x264Level,
    String x264Preset,
    String x264Tune
  );
  native
  private void publisherDestroy(long publisher);
  
  native
  private int publisherPushVideo(long publisher, long opaque, ByteBuffer video, int offset, int size, long timestamp);
  native
  private int publisherPushAudio(long publisher, long opaque, ByteBuffer audio, int offset, int size, long timestamp);
  
  private Publisher mPublisher = null;
  private long mPublisherJNI = 0;
  
  @Override
  public int create(Publisher publisher, String url, Publisher.VideoParameters video, Publisher.AudioParameters audio) {
    mPublisher = publisher;
    mPublisherJNI = publisherCreate(
      url,
      video.format.ordinal(),
      video.inWidth,
      video.inHeight,
      video.outWidth,
      video.outHeight,
      video.framerate,
      video.bitrate,
      
      audio.format.ordinal(),
      audio.channels,
      audio.samplerate,
      audio.bitrate,
      
      video.maxLag,
      audio.maxLag,
      
      "baseline",
      "3.1",
      "",
      ""
    );
    if (mPublisherJNI != 0) {
      return 0;
    }
    return AVError.UNKNOWN;
  }
  @Override
  public void destroy() {
    if (mPublisherJNI != 0) {
      publisherDestroy(mPublisherJNI);
      mPublisherJNI = 0;
    }
    mPublisher = null;
  }
  
  @Override
  public int pushVideo(ByteBuffer video, long timestamp, long timeout) throws InterruptedException {
    return AVError.UNKNOWN;
  }
  @Override
  public int pushAudio(ByteBuffer audio, long timestamp, long timeout) throws InterruptedException {
    return AVError.UNKNOWN;
  }
  
  @Override
  public void onNativeStart() {
  }
  @Override
  public void onNativeStop(int result) {
    if (result != 0) {
      synchronized(this) {
        if (mPublisher != null) {
          mPublisher.postError(result, this);
        }
      }
    }
  }
}
