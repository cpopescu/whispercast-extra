package com.happhub.android.media;

import android.annotation.TargetApi;
import android.media.MediaCodec;
import android.media.MediaCodecInfo;
import android.media.MediaCodecList;
import android.media.MediaFormat;
import android.os.Build;
import android.util.Log;

@TargetApi(Build.VERSION_CODES.JELLY_BEAN)
public class CodecVideo extends CodecBase {
  protected AVPixelFormat mFormat = null;
  
  protected int mWidth = 0;
  protected int mHeight = 0;
  
  protected CodecVideo(MediaCodec codec) {
    super(codec);
  }
  
  public AVPixelFormat getFormat() {
    return mFormat;
  }
  
  public int getWidth() {
    return mWidth;
  }
  public int getHeight() {
    return mHeight;
  }
  
  static
  private AVPixelFormat getAVPixelFormat(int codecFormat) {
    switch (codecFormat) {
      case MediaCodecInfo.CodecCapabilities.COLOR_FormatYUV420SemiPlanar:
        return AVPixelFormat.AV_PIX_FMT_NV21;
      case MediaCodecInfo.CodecCapabilities.COLOR_Format32bitARGB8888:
        return AVPixelFormat.AV_PIX_FMT_ARGB;
      case MediaCodecInfo.CodecCapabilities.COLOR_Format32bitBGRA8888:
        return AVPixelFormat.AV_PIX_FMT_BGRA;
      case MediaCodecInfo.CodecCapabilities.COLOR_Format24bitRGB888:
        return AVPixelFormat.AV_PIX_FMT_RGB24;
      case MediaCodecInfo.CodecCapabilities.COLOR_Format24bitBGR888:
        return AVPixelFormat.AV_PIX_FMT_BGR24;
    }
    return null;
  }
  
  static
  private CodecVideo tryCreateCodec(
    String mediaType,
    String name,
    AVPixelFormat[] prefferedVideoFormat,
    MediaCodecInfo codecInfo,
    MediaFormat mediaFormat
  ) {
    Log.d("HAPPHUB/Codec", "VIDEO: Trying MediaCodec '" + name + "' for '" + mediaType + "'");

    int videoFormatCodec = -1;
    try {
      MediaCodecInfo.CodecCapabilities caps = codecInfo.getCapabilitiesForType(mediaType);
      for (int j = 0; j < caps.profileLevels.length; ++j) {
        Log.d("HAPPHUB/Codec", " VIDEO: Profile/Level: " +caps.profileLevels[j].profile+"/"+caps.profileLevels[j].level);
      }
      for (int prefferedOnly = (prefferedVideoFormat != null) ? 0 : 1; (prefferedOnly < 2); ++prefferedOnly) {
        if (prefferedOnly == 0) {
          for (int i = 0; i < caps.colorFormats.length; ++i) {
            Log.d("HAPPHUB/Codec", "VIDEO: Checking format: " +caps.colorFormats[i]);
            AVPixelFormat videoFormat = getAVPixelFormat(videoFormatCodec = caps.colorFormats[i]);
            if (videoFormat != null) {
              Log.d("HAPPHUB/Codec", "VIDEO: Got format: " +videoFormat+"("+videoFormatCodec+")");
              for (int j = 0; j < prefferedVideoFormat.length; ++j) {
                if (videoFormat == prefferedVideoFormat[j]) {
                  CodecVideo codec = new CodecVideo(MediaCodec.createByCodecName(name));
                  try {
                    Log.d("HAPPHUB/Codec", "VIDEO: Checking format: " +videoFormat+"("+videoFormatCodec+"), step 1");
                    codec.mFormat = videoFormat;
                    mediaFormat.setInteger(MediaFormat.KEY_COLOR_FORMAT, videoFormatCodec);
                    
                    codec.mCodec.configure(mediaFormat, null, null, codecInfo.isEncoder() ? MediaCodec.CONFIGURE_FLAG_ENCODE : 0);
                    codec.mCodec.start();
                    
                    Log.d("HAPPHUB/Codec", "VIDEO: Using format " +videoFormat+"("+videoFormatCodec+"), step 2");
                    return codec;
                  } catch(Exception e) {
                    Log.w("HAPPHUB/Codec", "VIDEO: Failed format: " +videoFormat+"("+videoFormatCodec+"), step 2 ("+e.toString()+")");
                    codec.destroy();
                  }
                }
              }
            } else {
              Log.d("HAPPHUB/Codec", "VIDEO: No FFMPEG equivalent for color format: " + caps.colorFormats[i]);
            }
          }
        } else {
          for (int i = 0; i < caps.colorFormats.length; ++i) {
            Log.d("HAPPHUB/Codec", "VIDEO: Checking format: " +caps.colorFormats[i]);
            AVPixelFormat videoFormat = getAVPixelFormat(videoFormatCodec = caps.colorFormats[i]);
            if (videoFormat != null) {
              CodecVideo codec = new CodecVideo(MediaCodec.createByCodecName(name));
              try {
                Log.d("HAPPHUB/Codec", "VIDEO: Checking format " +videoFormat+"("+videoFormatCodec+"), step 2");
                codec.mFormat = videoFormat;
                mediaFormat.setInteger(MediaFormat.KEY_COLOR_FORMAT, videoFormatCodec);
                
                codec.mCodec.configure(mediaFormat, null, null, codecInfo.isEncoder() ? MediaCodec.CONFIGURE_FLAG_ENCODE : 0);
                codec.mCodec.start();
                
                Log.d("HAPPHUB/Codec", "VIDEO: Using format " +videoFormat+"("+videoFormatCodec+"), step 2");
                return codec;
              } catch(Exception e) {
                Log.w("HAPPHUB/Codec", "VIDEO: Failed format " +videoFormat+"("+videoFormatCodec+"), step 2 ("+e.toString()+")");
                codec.destroy();
              }
            }
          }
        }
      }
      return null;
    } catch(Exception e) {
    }
    return null;
  }
  
  static
  protected CodecVideo create(
    String mediaType,
    String[] prefferedName,
    AVPixelFormat[] prefferedVideoFormat,
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
                      CodecVideo codec = tryCreateCodec(mediaType, name, prefferedVideoFormat, codecInfo, mediaFormat);
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
                        CodecVideo codec =  tryCreateCodec(mediaType, name, prefferedVideoFormat, codecInfo, mediaFormat);
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
      Log.w("HAPPHUB/Codec", "No MediaCodec support on your device");
    }
    return null;
  }

  static
  public CodecVideo create(
    String mediaType,
    String[] prefferedName,
    AVPixelFormat[] prefferedVideoFormat,
    boolean isEncoder,
    Publisher.VideoParameters video
  ) {
    if (android.os.Build.VERSION.SDK_INT < android.os.Build.VERSION_CODES.JELLY_BEAN) {
      return null;
    }
    
    MediaFormat format = MediaFormat.createVideoFormat(
      mediaType,
      video.outWidth,
      video.outHeight
    );
    format.setInteger(MediaFormat.KEY_BIT_RATE, video.bitrate);
    
    format.setInteger(MediaFormat.KEY_FRAME_RATE, video.framerate);
    format.setInteger(MediaFormat.KEY_I_FRAME_INTERVAL, video.framerate);
    
    format.setInteger("stride", video.outWidth);
    format.setInteger("slice-height", video.outHeight);
    
    CodecVideo codec = create(mediaType, prefferedName, prefferedVideoFormat, isEncoder, format);
    if (codec != null) {
      codec.mWidth = video.outWidth;
      codec.mHeight = video.outHeight;
    }
    return codec;
  }

  public void destroy() {
    super.destroy();
    
    mFormat = null;
      
    mWidth = 0;
    mHeight = 0;
  }
}
 