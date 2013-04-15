package com.happhub.android.media;

import java.nio.ByteBuffer;

import com.happhub.android.Utils;

import android.annotation.TargetApi;
import android.media.MediaCodec;
import android.os.Build;
import android.util.Log;

@TargetApi(Build.VERSION_CODES.JELLY_BEAN)
public class CodecBase {
  static
  public class InputBuffer {
    public int index;
    public ByteBuffer data;
  }
  static
  public class OutputBuffer {
    public int index;
    public ByteBuffer data;
    public MediaCodec.BufferInfo info;
  }
  
  protected MediaCodec mCodec = null;
  
  protected int mInputIndex = -1;
  
  protected CodecBase(MediaCodec codec) {
    mCodec = codec;
  }
  
  protected void destroy() {
    if (mCodec != null) {
      mCodec.flush();
      mCodec.stop();
      
      mCodec.release();
      mCodec = null;
    }
  }
  
  public int push(ByteBuffer data, long timestamp, int flags) {
    int result = 0;
    
    try {
      while (data.remaining() > 0) {
        ByteBuffer dst = null;
        
        if (mInputIndex < 0) {
          mInputIndex = mCodec.dequeueInputBuffer(0);
          if (mInputIndex >= 0) {
            dst = mCodec.getInputBuffers()[mInputIndex];
            dst.rewind();
          }
        } else {
          dst = mCodec.getInputBuffers()[mInputIndex];
        }
        
        if (dst != null) {
          int offset = dst.position();
          
          int size = Math.min(dst.remaining(), data.remaining());
          dst.put(data.array(), data.position(), size);
          
          data.position(data.position()+size);
          
          if (dst.remaining() == 0) {
            mCodec.queueInputBuffer(mInputIndex, offset, size, timestamp, flags);
            mInputIndex = -1;
          }
        } else {
          break;
        }
      }
    } catch(Exception e) {
      result = AVError.UNKNOWN;
    }
    Log.d("HAPPHUB/Codec", "BUFFER: Provided input at " + Utils.timestampAsString(timestamp)); 
    return result;
  }
  
  public OutputBuffer dequeueOutputBuffer(long timeout) {
    MediaCodec.BufferInfo info = new MediaCodec.BufferInfo();
    
    int index = mCodec.dequeueOutputBuffer(info, timeout);
    if (index == MediaCodec.INFO_OUTPUT_BUFFERS_CHANGED) {
      index = mCodec.dequeueOutputBuffer(info, 0);
    }
    if (index < 0) {
      return null;
    }
    
    Log.d("HAPPHUB/Codec", "BUFFER: Got output at " +
      Utils.timestampAsString(info.presentationTimeUs) +
      " OFFSET: " + info.offset +
      " SIZE: " + info.size +
      (((info.flags & MediaCodec.BUFFER_FLAG_CODEC_CONFIG) != 0) ? " CONFIG" : "") +
      (((info.flags & MediaCodec.BUFFER_FLAG_END_OF_STREAM) != 0) ? " EOS" : "") +
      (((info.flags & MediaCodec.BUFFER_FLAG_SYNC_FRAME) != 0) ? " KEYFRAME" : "")
    );
    
    OutputBuffer buffer = new OutputBuffer();
    buffer.index = index;
    buffer.data = mCodec.getOutputBuffers()[index];
    buffer.info = info;

    return buffer;
  }
  public void releaseOutputBuffer(int index) {
    if (index >= 0) {
      mCodec.releaseOutputBuffer(index, false);
    }
  }
}
 