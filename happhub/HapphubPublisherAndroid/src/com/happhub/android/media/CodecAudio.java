package com.happhub.android.media;

import android.annotation.TargetApi;
import android.media.MediaCodec;
import android.media.MediaCodecInfo;
import android.media.MediaCodecList;
import android.media.MediaFormat;
import android.os.Build;
import android.util.Log;

@TargetApi(Build.VERSION_CODES.JELLY_BEAN)
public class CodecAudio extends CodecBase {
  private AVSampleFormat mFormat = null;
  
  private int mChannels = -1;
  private int mSampleRate = -1;
  
  private CodecAudio(MediaCodec codec) {
    super(codec);
  }
  
  public AVSampleFormat getFormat() {
    return mFormat;
  }
  
  public int getChannels() {
    return mChannels;
  }
  public int getSampleRate() {
    return mSampleRate;
  }
  
  static
  private CodecAudio tryCreateCodec(
    String mediaType,
    String name,
    AVSampleFormat[] prefferedAudioFormat,
    MediaCodecInfo codecInfo,
    MediaFormat mediaFormat
  ) {
    Log.d("HAPPHUB/Codec", "AUDIO: Trying MediaCodec '" + name + "' for '" + mediaType + "'");

    try {
      CodecAudio codec = new CodecAudio(MediaCodec.createByCodecName(name));
      try {
        // TODO: Clarify the enumeration of accepted audio sample formats...
        Log.d("HAPPHUB/Codec", "AUDIO: Checking format "+AVSampleFormat.AV_SAMPLE_FMT_S16);
        codec.mFormat = AVSampleFormat.AV_SAMPLE_FMT_S16;
        
        codec.mCodec.configure(mediaFormat, null, null, codecInfo.isEncoder() ? MediaCodec.CONFIGURE_FLAG_ENCODE : 0);
        codec.mCodec.start();
        
        return codec;
      } catch(Exception e) {
        codec.destroy();
      }
      return null;
    } catch(Exception e) {
    }
    return null;
  }
  
  static
  private CodecAudio create(
    String mediaType,
    String[] prefferedName,
    AVSampleFormat[] prefferedAudioFormat,
    boolean isEncoder,
    MediaFormat mediaFormat
  ) {
    if (((android.os.Build.VERSION.SDK_INT >= android.os.Build.VERSION_CODES.JELLY_BEAN))) {
      for (int prefferedOnly = 0; prefferedOnly < 2; ++prefferedOnly) {
        int count = MediaCodecList.getCodecCount();
        for (int index = 0; index < count; ++index) {
          MediaCodecInfo codecInfo = MediaCodecList.getCodecInfoAt(index);
          if (codecInfo.isEncoder() == isEncoder) {
            String[] types = codecInfo.getSupportedTypes();
            for (String type: types) {
              String name = (String)codecInfo.getName();
              
              Log.d("HAPPHUB/Codec", "Enumerating MediaCodec '" + name + "' for '" + type + "'");
              if (mediaType.compareTo(type) == 0) {
                if (prefferedOnly == 0) {
                  for (String p: prefferedName) {
                    if (name.startsWith(p)) {
                      CodecAudio codec = tryCreateCodec(mediaType, name, prefferedAudioFormat, codecInfo, mediaFormat);
                      if (codec != null) {
                        Log.d("HAPPHUB/Codec", "Using MediaCodec '" + name + "' for '" + mediaType + "'");
                        return codec;
                      }
                    }
                  }
                } else {
                  if (prefferedOnly == 0) {
                    for (String p: prefferedName) {
                      if (!name.startsWith(p)) {
                        CodecAudio codec = tryCreateCodec(mediaType, name, prefferedAudioFormat, codecInfo, mediaFormat);
                        if (codec != null) {
                          Log.d("HAPPHUB/Codec", "Using MediaCodec '" + name + "' for '" + mediaType + "'");
                          return codec;
                        }
                      }
                    }
                  }
                }
              }
            }
          }
        }
      }
      Log.w("HAPPHUB/Codec", "MediaCodec for '" + mediaType + "' not found");
    } else {
      Log.w("HAPPHUB/Codec", "No MediaCodec support on the device");
    }
    return null;
  }

  static
  public CodecAudio create(
    String mediaType,
    String[] prefferedName,
    AVSampleFormat[] prefferedAudioFormat,
    boolean isEncoder,
    Publisher.AudioParameters audio
  ) {
    if (android.os.Build.VERSION.SDK_INT < android.os.Build.VERSION_CODES.JELLY_BEAN) {
      return null;
    }
    
    MediaFormat format = MediaFormat.createAudioFormat(
      mediaType,
      audio.samplerate,
      audio.channels
    );
    
    format.setInteger(MediaFormat.KEY_BIT_RATE, audio.bitrate);
    format.setInteger(MediaFormat.KEY_MAX_INPUT_SIZE, 8192);
    
    format.setInteger(MediaFormat.KEY_AAC_PROFILE, MediaCodecInfo.CodecProfileLevel.AACObjectLC);
    format.setInteger(MediaFormat.KEY_IS_ADTS, 0);
    
    CodecAudio codec = create(mediaType, prefferedName, prefferedAudioFormat, isEncoder, format);
    if (codec != null) {
      codec.mChannels = audio.channels;
      codec.mSampleRate = audio.samplerate;
    }
    return codec;
  }

  public void destroy() {
    super.destroy();
    mFormat = null;
    
    mChannels = -1;
    mSampleRate = -1;
  }
}
