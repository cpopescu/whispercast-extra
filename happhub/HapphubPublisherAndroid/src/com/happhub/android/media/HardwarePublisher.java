package com.happhub.android.media;

import java.nio.ByteBuffer;

import android.annotation.TargetApi;
import android.media.MediaCodec;
import android.os.Build;
import android.util.Log;

import com.happhub.android.Utils;

@TargetApi(Build.VERSION_CODES.JELLY_BEAN)
public class HardwarePublisher implements Publisher.PublisherImpl {
  native
  private long publisherCreate(
    String url,
    
    int videoFormat,
    int videoWidth,
    int videoHeight,
    int videoFramerate,
    int videoBitRate,
    
    int audioFormat,
    int audioChannels,
    int audioSampleRate,
    int audioBitRate,
    
    byte[] videoHeaders,
    byte[] audioHeaders,
    
    long maxVideoLag,
    long maxAudioLag
  );
  native
  private void publisherDestroy(long publisher);
  
  native
  private int publisherPushVideo(long publisher, long opaque, ByteBuffer video, int offset, int size, long timestamp, boolean keyframe);
  native
  private int publisherPushAudio(long publisher, long opaque, ByteBuffer audio, int offset, int size, long timestamp);
  
  private Publisher mPublisher = null;
  
  private String mURL = null;
  
  private Publisher.VideoParameters mVideo = null;
  private Publisher.AudioParameters mAudio = null;
  
  private CodecVideo mVideoCodec = null;
  private CodecAudio mAudioCodec = null;
  
  private byte[] mVideoHeaders = null;
  private byte[] mAudioHeaders = null;
  
  private VideoConverter mVideoConverter = null;
  private ByteBuffer mVideoConverted = null;
  
  private long mPublisherJNI = 0;
  
  private int open() {
    if (mPublisherJNI != 0) {
      publisherDestroy(mPublisherJNI);
    }
    
    mPublisherJNI = publisherCreate(
      mURL,
      mVideoCodec.getFormat().ordinal(),
      mVideoCodec.getWidth(),
      mVideoCodec.getHeight(),
      mVideo.framerate,
      mVideo.bitrate,
      
      mAudio.format.ordinal(),
      mAudio.channels,
      mAudio.samplerate,
      mAudio.bitrate,
      
      mVideoHeaders,
      mAudioHeaders,
      
      mVideo.maxLag,
      mAudio.maxLag
    );
    if (mPublisherJNI != 0) {
      return 0;
    }
    return -1;
  }

  @Override
  synchronized
  public int create(Publisher publisher, String url, Publisher.VideoParameters video, Publisher.AudioParameters audio) {
    int result = 0;

    try {
      mPublisher = publisher;
      
      mURL = url;
      
      mVideo = video;
      mAudio = audio;
      
      mVideoCodec = CodecVideo.create(
        "video/avc",
        new String[]{"OMX."},
        new AVPixelFormat[] {
          video.format
        },
        true,
        video
      );
      mAudioCodec = CodecAudio.create(
        "audio/mp4a-latm",
        new String[]{"OMX.google.aac."},
        new AVSampleFormat[] {
          audio.format
        },
        true,
        audio
      );
      
      if (mAudioCodec != null && mVideoCodec != null) {
        boolean needsConversion =
        (mVideoCodec.getFormat() != video.format) ||
        (mVideoCodec.getWidth() != video.inWidth) || 
        (mVideoCodec.getHeight() != video.inHeight); 
        
        if (needsConversion) {
          mVideoConverter = new VideoConverter();
          result = mVideoConverter.create(
            video.format,
            video.inWidth,
            video.inHeight,
            mVideoCodec.getFormat(),
            mVideoCodec.getWidth(),
            mVideoCodec.getHeight()
          );
          if (result == 0) {
            mVideoConverted = ByteBuffer.allocateDirect(mVideoConverter.getOutputSize());
          }
        }
        return 0;
      } else {
        result = AVError.UNKNOWN;
      }
    } catch(Exception e) {
      result = AVError.UNKNOWN;
    }
    
    destroy();
    return result;
  }

  @Override
  synchronized
  public void destroy() {
    if (mPublisherJNI != 0) {
      publisherDestroy(mPublisherJNI);
      mPublisherJNI = 0;
    }
    
    mAudioHeaders = null;
    mVideoHeaders = null;
    
    if (mVideoConverter != null) {
      mVideoConverter.destroy();
      mVideoConverter = null;
    }
    
    if (mVideoCodec != null) {
      mVideoCodec.destroy();
      mVideoCodec = null;
    }
    if (mAudioCodec != null) {
      mAudioCodec.destroy();
      mAudioCodec = null;
    }
    
    mPublisher = null;
  }
  
  private int sendVideoFrame() {
    // wait for the audio stream headers
    synchronized(mPublisher) {
      if (mAudioHeaders == null && mVideoHeaders != null) {
        return 1;
      }
    }
    
    int result = 0;
    
    CodecBase.OutputBuffer output = mVideoCodec.dequeueOutputBuffer(0);
    if (output != null) {
      Log.v("HAPPHUB/HardwarePublisher", "VIDEO: Produced" +
        " TIMESTAMP: " + Utils.timestampAsString(output.info.presentationTimeUs) +
        " SIZE: " + output.info.size +
        (((output.info.flags & MediaCodec.BUFFER_FLAG_SYNC_FRAME) != 0) ? " KEYFRAME" : "") +
        (((output.info.flags & MediaCodec.BUFFER_FLAG_CODEC_CONFIG) != 0) ? " CONFIG" : "")
      );

      if ((output.info.flags & MediaCodec.BUFFER_FLAG_CODEC_CONFIG) != 0) {
        synchronized(mPublisher) {
          mVideoHeaders = new byte[output.info.size];
          output.data.get(mVideoHeaders, output.info.offset, output.info.size);
          
          if (mAudioHeaders != null && mVideoHeaders != null) {
            result = open();
          }
        }
      } else {
        if (mPublisherJNI != 0) {
          try {
            ByteBuffer d = ByteBuffer.allocateDirect(output.info.size);
            output.data.position(output.info.offset);
            output.data.limit(output.info.offset+output.info.size);
            d.put(output.data);
            
            result = publisherPushVideo(mPublisherJNI, 0, d, 0, output.info.size, output.info.presentationTimeUs, (output.info.flags & MediaCodec.BUFFER_FLAG_SYNC_FRAME) != 0);
          } catch(Exception e) {
            Log.w("HAPPHUB/HardwarePublisher", "VIDEO: Exception while pushing to the native publisher: " + e.toString());
          }
        }
      }
      mVideoCodec.releaseOutputBuffer(output.index);
    }
    return (output == null) ? 1 : result;
  }
  private long audiots = -1;
  private int sendAudioFrame() {
    // wait for the video stream headers
    synchronized(mPublisher) {
      if (mAudioHeaders != null && mVideoHeaders == null) {
        return 1;
      }
    }
    
    int result = 0;
  
    CodecBase.OutputBuffer output = mAudioCodec.dequeueOutputBuffer(0);
    if (output != null) {
      Log.v("HAPPHUB/HardwarePublisher", "AUDIO: Produced" +
        " TIMESTAMP: " + Utils.timestampAsString(output.info.presentationTimeUs) +
        " SIZE: " + output.info.size +
        (((output.info.flags & MediaCodec.BUFFER_FLAG_SYNC_FRAME) != 0) ? " KEYFRAME" : "") +
        (((output.info.flags & MediaCodec.BUFFER_FLAG_CODEC_CONFIG) != 0) ? " CONFIG" : "")
      );

      if ((output.info.flags & MediaCodec.BUFFER_FLAG_CODEC_CONFIG) != 0) {
        synchronized(mPublisher) {
          audiots = output.info.presentationTimeUs;
          mAudioHeaders = new byte[output.info.size];
          output.data.get(mAudioHeaders, output.info.offset, output.info.size);
          
          if (mAudioHeaders != null && mVideoHeaders != null) {
            result = open();
          }
        }
      } else {
        if (mPublisherJNI != 0) {
          try {
            if (audiots <= output.info.presentationTimeUs) {
              audiots = output.info.presentationTimeUs;
              
              ByteBuffer d = ByteBuffer.allocateDirect(output.info.size);
              output.data.position(output.info.offset);
              output.data.limit(output.info.offset+output.info.size);
              d.put(output.data);
              
              result = publisherPushAudio(mPublisherJNI, 0, d, 0, output.info.size, output.info.presentationTimeUs);
            } else {
              Log.e("HAPPHUB/HardwarePublisher", "AUDIO: SKIPPING");
            }
          } catch(Exception e) {
            Log.w("HAPPHUB/HardwarePublisher", "AUDIO: Exception while pushing to the native publisher: " + e.toString());
          }
        }
      }
      mAudioCodec.releaseOutputBuffer(output.index);
    }
    return (output == null) ? 1 : result;
  }
  
  @Override
  public int pushVideo(ByteBuffer video, long timestamp, long timeout) throws InterruptedException {
    int result = 0;
    
    Log.d("HAPPHUB/HardwarePublisher", "VIDEO: Pushing at "+Utils.timestampAsString(timestamp));
    
    if (video != null) {
      ByteBuffer src;
      if (mVideoConverter != null) {
        result = mVideoConverter.convert(video, mVideoConverted);
        src = mVideoConverted;
      } else {
        src = video;
      }

      if (result == 0) {
        src.rewind();
        while (src.remaining() > 0) {
          if ((result = mVideoCodec.push(src, timestamp, 0)) != 0) {
            break;
          }
          
          if (src.remaining() > 0) {
            Log.w("HAPPHUB/HardwarePublisher", "VIDEO: No input buffer, trying to make space");
            result = ((result = sendVideoFrame()) == 1) ? 0 : result; 
            if (result != 0) {
              break;
            }
            
            if ((result = mVideoCodec.push(src, timestamp, 0)) != 0) {
              break;
            }
            if (src.remaining() > 0) {
              Log.e("HAPPHUB/HardwarePublisher", "VIDEO: No input buffer, bailing out");
              // TODO: Should this result in an error or just a dropped frame ?
              result = 1;
              break;
            }
          }
        }
      }
    }
    
    // drain the video out queue
    while (result == 0) {
      result = sendVideoFrame();
    }
    return (result == 1) ? 0 : result;
  }
  @Override
  public int pushAudio(ByteBuffer audio, long timestamp, long timeout) throws InterruptedException {
    int result = 0;
    
    Log.d("HAPPHUB/HardwarePublisher", "AUDIO: Pushing at "+Utils.timestampAsString(timestamp));
    
    audio.rewind();
    while (audio.remaining() > 0) {
      if ((result = mAudioCodec.push(audio, timestamp, 0)) != 0) {
        break;
      }
      
      if (audio.remaining() > 0) {
        Log.w("HAPPHUB/HardwarePublisher", "AUDIO: No input buffer, trying to make space");
        result = ((result = sendAudioFrame()) == 1) ? 0 : result; 
        if (result != 0) {
          break;
        }
        
        if ((result = mAudioCodec.push(audio, timestamp, 0)) != 0) {
          break;
        }
        if (audio.remaining() > 0) {
          Log.e("HAPPHUB/HardwarePublisher", "AUDIO: No input buffer, bailing out");
          // TODO: Should this result in an error or just a dropped frame ?
          result = 1;
          break;
        }
      }
    }
    
    // drain the audio out queue
    while (result == 0) {
      result = sendAudioFrame();
    }
    return (result == 1) ? 0 : result;
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
