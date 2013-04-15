package com.happhub.android.media;

import java.nio.ByteBuffer;

import android.media.AudioFormat;
import android.media.AudioRecord;

public class SoftwareAudioGrabber implements Publisher.AudioGrabber {
  private Publisher mPublisher = null;
  
  private int mChannels = -1;
  private int mSampleRate = -1;
  private int mSampleSize = -1;
  
  private Thread mThread = null; 
  private AudioRecord mAudioRecord = null;
  
  private long mTimestampRef = 0;
  private long mSampleCount = 0;
  
  private long mRunningTimestampCurrent = 0;
  private long mRunningTimestampRef = 0;
  
  private ByteBuffer mBuffer = null;
  private int mBufferSize = 0;
  
  private boolean mShutdown = false;
  
  @Override
  synchronized
  public int create(Publisher publisher, Publisher.AudioParameters parameters) {
    int result = 0;
    
    try {
      mPublisher = publisher;
      
      mChannels = parameters.channels;
      mSampleRate = parameters.samplerate;
      mSampleSize = 2; // AudioFormat.ENCODING_PCM_16BIT
      
      mBufferSize = 2*AudioRecord.getMinBufferSize(mSampleRate, (mChannels == 1) ? AudioFormat.CHANNEL_IN_MONO : AudioFormat.CHANNEL_IN_STEREO, AudioFormat.ENCODING_PCM_16BIT);
      mBuffer = ByteBuffer.allocateDirect(mBufferSize);
      
      parameters.bufferSize = mBufferSize;
      
      return 0;
    } catch(Exception e) {
      result = AVError.UNKNOWN;
    }
    
    destroy();
    return result;
  }

  @Override
  synchronized
  public void destroy() {
    stopPublishing();
    
    mBufferSize = 0;
    mBuffer = null;
    
    mChannels = -1;
    mSampleRate = -1;
    mSampleSize = -1;
    
    mTimestampRef = 0;
    mSampleCount = 0;
    
    mPublisher = null;
  }

  @Override
  public AVSampleFormat getFormat() {
    return AVSampleFormat.AV_SAMPLE_FMT_S16;
  }

  @Override
  public long adjustTimestamp(long timestamp) {
    synchronized(this) {
      return mRunningTimestampCurrent+ (timestamp-mRunningTimestampRef);
    }
  }

  @Override
  synchronized
  public int startPublishing() {
    int result = 0;
    
    try {
      mAudioRecord = new AudioRecord(
        android.media.MediaRecorder.AudioSource.CAMCORDER,
        mSampleRate,
        (mChannels == 1) ? AudioFormat.CHANNEL_IN_MONO : AudioFormat.CHANNEL_IN_STEREO,
        AudioFormat.ENCODING_PCM_16BIT,
        mBufferSize
      );
      
      mShutdown = false;
      mThread = new Thread(new Runnable() {
        @Override
        public void run() {
          android.os.Process.setThreadPriority(android.os.Process.THREAD_PRIORITY_URGENT_AUDIO);

          int result = 0;
          
          try {
            mAudioRecord.startRecording();
            
            synchronized(SoftwareAudioGrabber.this) {
              mTimestampRef = System.nanoTime()/1000L;
              mSampleCount = 0;
              
              mRunningTimestampRef = mTimestampRef;
              mRunningTimestampCurrent = mTimestampRef;
              
              SoftwareAudioGrabber.this.notifyAll();
            }
  
            while (result == 0) {
              synchronized(SoftwareAudioGrabber.this) {
                if (mShutdown) {
                  break;
                }
              }
              
              int audioSize = mBuffer.capacity();
              
              mBuffer.rewind();
              mBuffer.limit(audioSize);
              
              int remaining = audioSize;
              while (remaining > 0) {
                int audioRead = mAudioRecord.read(mBuffer, Math.min(remaining, mBufferSize));
                if (audioRead > 0) {
                  remaining -= audioRead;
                } else {
                  result = AVError.UNKNOWN;
                  break;
                }
              }
              
              if (result == 0) {
                long timestamp = mTimestampRef + ((1000000L*mSampleCount)/mSampleRate);
                
                try {
                  mPublisher.pushAudio(mBuffer, timestamp, -1);
                } catch(Exception e) {
                  synchronized(SoftwareAudioGrabber.this) {
                    mTimestampRef = System.nanoTime()/1000L;
                    mSampleCount = 0;
                  }
                }
                
                synchronized(SoftwareAudioGrabber.this) {
                  mSampleCount += (audioSize-remaining)/mChannels/mSampleSize;
                  
                  mRunningTimestampRef = System.nanoTime()/1000L;
                  mRunningTimestampCurrent = mTimestampRef + ((1000000L*mSampleCount)/mSampleRate);
                }
              }
            }
          } catch(Exception e) {
            result = AVError.UNKNOWN;
          }
          
          if (mAudioRecord.getRecordingState() == AudioRecord.RECORDSTATE_RECORDING) {
            mAudioRecord.stop();
          }
          mAudioRecord.release();
          
          if (result != 0) {
            mPublisher.postError(result, SoftwareAudioGrabber.this);
          }
        }
      });
      
      mThread.start();
      this.wait();
      
      return 0;
    } catch (Exception e) {
      result = AVError.UNKNOWN;
    }
    
    stopPublishing();
    return result;
  }
  @Override
  public void stopPublishing() {
    Thread thread = null;
    synchronized(this) {
      mShutdown = true;
      thread = mThread;
    }
    
    if (thread != null) {
      while (thread.isAlive()) {
        thread.interrupt();
        try {
          thread.join();
        } catch(Exception e) {
        }
      }
    }
    
    synchronized(this) {
      mThread = null;
    }
  }
}
