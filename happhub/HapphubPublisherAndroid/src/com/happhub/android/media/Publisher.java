package com.happhub.android.media;

import java.nio.ByteBuffer;
import java.util.HashSet;
import java.util.List;
import java.util.Set;

import com.happhub.android.Utils;

import android.app.Activity;
import android.content.Context;
import android.content.SharedPreferences;
import android.graphics.ImageFormat;
import android.hardware.Camera;
import android.net.wifi.WifiManager;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.os.PowerManager;
import android.util.Log;
import android.view.Surface;
import android.view.View;
import android.view.ViewGroup;
import android.widget.RelativeLayout;

public class Publisher extends Handler {
  static
  public class Setup {
    public int videoWidth = -1;
    public int videoHeight = -1;
    public int videoFramerate = -1;
    public int videoBitRate = -1;
    public long videoMaxLag = -1;
    
    public int audioChannels = -1;
    public int audioSampleRate = -1;
    public int audioBitRate = -1;
    public long audioMaxLag = -1;
  };
  
  static
  public class Options {
    public int cameraIndex = 0;
    
    public CaptureMode captureMode = CaptureMode.SOFTWARE;
    public boolean enforceAudioVideoSync = false;
    
    public boolean load(Bundle bundle) {
      if (bundle != null) {
        int cameraIndex_ = bundle.getInt("publisher_camera", 0);
        CaptureMode captureMode_ = Publisher.CaptureMode.valueOf(bundle.getString("publisher_capture_mode", "SOFTWARE"));
        boolean enforceAudioVideoSync_ = Boolean.valueOf(bundle.getBoolean("publisher_enforce_audio_video_sync", false));
        
        boolean changed = (cameraIndex_ != cameraIndex) || (captureMode_ != captureMode) || (enforceAudioVideoSync_ != enforceAudioVideoSync);
        
        cameraIndex = cameraIndex_;
        captureMode = captureMode_;
        enforceAudioVideoSync = enforceAudioVideoSync_;
        
        return changed;
      }
      return false;
    }
    public boolean load(SharedPreferences bundle) {
      if (bundle != null) {
        int cameraIndex_ = Integer.valueOf(bundle.getString("publisher_camera", "0"));
        CaptureMode captureMode_ = Publisher.CaptureMode.valueOf(bundle.getString("publisher_capture_mode", "SOFTWARE"));
        boolean enforceAudioVideoSync_ = Boolean.valueOf(bundle.getBoolean("publisher_enforce_audio_video_sync", false));
        
        boolean changed = (cameraIndex_ != cameraIndex) || (captureMode_ != captureMode) || (enforceAudioVideoSync_ != enforceAudioVideoSync);
        
        cameraIndex = cameraIndex_;
        captureMode = captureMode_;
        enforceAudioVideoSync = enforceAudioVideoSync_;
        
        return changed;
      }
      return false;
    }
    
    public boolean save(Bundle bundle) {
      if (bundle != null) {
        bundle.putInt("publisher_camera", cameraIndex);
        bundle.putString("publisher_capture_mode", captureMode.name());
        bundle.putBoolean("publisher_enforce_audio_video_sync", enforceAudioVideoSync);
        
        return true;
      }
      return false;
    }
    
    public Options clone() {
      Options options = new Options();
      options.cameraIndex = this.cameraIndex;
      options.captureMode = this.captureMode;
      options.enforceAudioVideoSync = this.enforceAudioVideoSync;
      
      return options;
    }
  }
  
  static
  public class VideoParameters {
    public AVPixelFormat format;
    public int inWidth;
    public int inHeight;
    public int outWidth;
    public int outHeight;
    public int framerate;
    public int bitrate;
    public long maxLag;
  }
  static
  public class AudioParameters {
    AVSampleFormat format;
    public int channels;
    public int samplerate;
    public int bitrate;
    public long maxLag;
    // hackish - this is setup by the grabber
    public int bufferSize;
  }
  
  public interface VideoGrabber {
    public int create(Publisher publisher, VideoParameters parameters, Camera camera);
    public void destroy();
    
    public View createView(Context context);
    public void destroyView();
    
    public AVPixelFormat getFormat();
    
    public int startPublishing();
    public void stopPublishing();
  }
  public interface AudioGrabber {
    public int create(Publisher publisher, AudioParameters parameters);
    public void destroy();
    
    public AVSampleFormat getFormat();
    
    public int startPublishing();
    public void stopPublishing();
    
    // adjusts the given timestamp (as microseconds in System.nanoTime() reference frame)
    public long adjustTimestamp(long timestamp);
  }
  
  public interface PublisherImpl {
    public int create(Publisher publisher, String url, VideoParameters video, AudioParameters audio);
    public void destroy();
    
    public int pushVideo(ByteBuffer video, long timestamp, long timeout) throws InterruptedException;
    public int pushAudio(ByteBuffer audio, long timestamp, long timeout) throws InterruptedException;
    
    public void onNativeStart();
    public void onNativeStop(int result);
  }
  
  public static final int NOTIFY_STATE_CHANGED = 1;
  public static final int NOTIFY_ERROR = 2;
  public static final int NOTIFY_FOCUS_MODE = 3;
  
  private static final int MESSAGE_ERROR = 1;
  
  public enum CaptureMode {
    SOFTWARE,
    HARDWARE
  };
  public enum FocusMode {
    FIXED,
    CONTINUOUS,
    MACRO
  };
  
  public interface NotificationListener {
    public void onPublisherNotification(Publisher publisher, int what, int arg);
  }
  
  private Set<NotificationListener> mStateListeners = new HashSet<NotificationListener>();
  
  private ViewGroup mViewGroup = null;
  private View.OnLayoutChangeListener mViewGroupLayoutListener = null; 
  private View mView = null;

  private String mURL = null;
  
  private Setup mSetup = null;
  private Options mOptions = null;
  
  private AudioParameters mAudio = null;
  private VideoParameters mVideo = null;
  
  private boolean mInitialized = false;
  private boolean mStarted = false;
  
  private boolean mHasFlashlight = false;
  private boolean mHasZoom = false;
  
  private boolean mFlashlight = false;
  
  private int mZoom = 0;
  private int mMaxZoom = 0;

  private FocusMode mFocusMode = FocusMode.CONTINUOUS;
  
  private boolean mLocked = false;
  private PowerManager.WakeLock mWakeLock = null;
  private WifiManager.WifiLock mWifiLock = null;
  
  private Camera mCamera = null;
  
  private VideoGrabber mVideoGrabber = null;
  private AudioGrabber mAudioGrabber = null;
  
  private long mFirstTimestamp = -1;
  
  private long mLastVideoTimestamp = -1;
  private long mLastAudioTimestamp = -1;
  
  private PublisherImpl mPublisherImpl = null;
  
  public void addStateListener(NotificationListener listener) {
    mStateListeners.add(listener);
  }
  public void removeStateListener(NotificationListener listener) {
    mStateListeners.remove(listener);
  }
  
  protected void notifyListeners(int what, int arg) {
    for (NotificationListener listener : mStateListeners) {
      listener.onPublisherNotification(this, what, arg);
    }
  }
  
  public enum State {
    NONE,
    PREVIEWING,
    PUBLISHING
  }
  private State mState = State.NONE;
  
  private void setState(State state) {
    if (state != mState) {
      mState = state;
      notifyListeners(NOTIFY_STATE_CHANGED, 0);
    }
  }
  public State getState() {
    return mState;
  }
  
  public Setup getSetup() {
    return mSetup; 
  }
  public Options getOptions() {
    return mOptions; 
  }
  
  public int getInWidth() {
    return (mVideo != null) ? mVideo.inWidth : 0;
  }
  public int getInHeight() {
    return (mVideo != null) ? mVideo.inHeight : 0;
  }
  
  public int getOutWidth() {
    return (mVideo != null) ? mVideo.outWidth : 0;
  }
  public int getOutHeight() {
    return (mVideo != null) ? mVideo.outHeight : 0;
  }
  
  public void updateOrientation(Activity activity) {
    if (mCamera != null) {
      android.hardware.Camera.CameraInfo info = new android.hardware.Camera.CameraInfo();
      android.hardware.Camera.getCameraInfo(0, info);
      
      int rotation = activity.getWindowManager().getDefaultDisplay().getRotation();
      
      int degrees = 0;
      switch (rotation) {
        case Surface.ROTATION_0:
          degrees = 0;
          break;
        case Surface.ROTATION_90:
          degrees = 90;
          break;
        case Surface.ROTATION_180:
          degrees = 180;
          break;
        case Surface.ROTATION_270:
          degrees = 270;
          break;
      }
  
      int result;
      if (info.facing == Camera.CameraInfo.CAMERA_FACING_FRONT) {
          result = (info.orientation + degrees) % 360;
          result = (360 - result) % 360;  // compensate the mirror
      } else {  // back-facing
          result = (info.orientation - degrees + 360) % 360;
      }
      
      mCamera.setDisplayOrientation(result);
    }
  }
  
  public boolean hasFlashlight() {
    return mHasFlashlight; 
  }
  
  public boolean getFlashlight() {
    return mFlashlight;
  }
  public void setFlashlight(boolean flashlight) {
    if (flashlight != mFlashlight) {
      mFlashlight = flashlight;
      if (mCamera != null) {
        try {
          Camera.Parameters parameters = mCamera.getParameters();
          parameters.setFlashMode(mFlashlight ? Camera.Parameters.FLASH_MODE_TORCH : Camera.Parameters.FLASH_MODE_OFF);
          mCamera.setParameters(parameters);
        } catch(Exception e) {
        }
      }
    }
  }
  
  public boolean hasZoom() {
    return mHasZoom; 
  }
  
  public int getMaxZoom() {
    return mMaxZoom;
  }
  
  public int getZoom() {
    return mZoom;
  }
  public void setZoom(int zoom) {
    zoom = Math.max(0, Math.min(zoom, mMaxZoom));
    if (zoom != mZoom) {
      mZoom = zoom;
      if (mCamera != null) {
        try {
          Camera.Parameters parameters = mCamera.getParameters();
          parameters.setZoom(mZoom);
          mCamera.setParameters(parameters);
        } catch(Exception e) {
        }
      }
    }
  }
  
  private boolean updateCameraFocusMode(Camera.Parameters parameters, FocusMode focusMode) {
    List<String> focusModes = parameters.getSupportedFocusModes();
    
    String desiredFocusMode;
    switch (focusMode) {
      case CONTINUOUS:
        desiredFocusMode = Camera.Parameters.FOCUS_MODE_CONTINUOUS_VIDEO;
        break;
      case MACRO:
        desiredFocusMode = Camera.Parameters.FOCUS_MODE_MACRO;
        break;
      default:
        desiredFocusMode = Camera.Parameters.FOCUS_MODE_FIXED;
    }
    
    String cameraMode = null;
    for (String s : focusModes) {
      Log.d("HAPPHUB/Publisher", "Focus mode available: "+s);
      if (s.compareTo(desiredFocusMode) == 0) {
        cameraMode = s;
        break;
      } else
      if (s.compareTo(Camera.Parameters.FOCUS_MODE_FIXED) == 0) {
        cameraMode = s;
      }
    }
    Log.d("HAPPHUB/Publisher", "Focus mode chosen: "+cameraMode);
    
    if (cameraMode != null) {
      parameters.setFocusMode(cameraMode);
      return true;
    }
    return false;
  }
  
  public FocusMode getFocusMode() {
    return mFocusMode;
  }
  public void setFocusMode(FocusMode focusMode) {
    if (focusMode != mFocusMode) {
      if (mCamera != null) {
        try {
          Camera.Parameters parameters = mCamera.getParameters();
          if (updateCameraFocusMode(parameters, focusMode)) {
            mCamera.setParameters(parameters);
            
            mFocusMode = focusMode;
            notifyListeners(NOTIFY_FOCUS_MODE, 0);
          }
        } catch(Exception e) {
        }
      }
    }
  }
  
  private void updateView() {
    if (mView != null) {
      mViewGroup.removeOnLayoutChangeListener(mViewGroupLayoutListener);
      mViewGroupLayoutListener = null;
      
     int width = this.getInWidth();
      int height = this.getInHeight();
      
      int layoutWidth = mViewGroup.getWidth();
      int layoutHeight = mViewGroup.getHeight();
      
      RelativeLayout.LayoutParams lp = new RelativeLayout.LayoutParams(0, 0);
      if (width > height) {
        lp.height = layoutHeight;
        lp.width = (layoutHeight*width)/height;
        lp.leftMargin = lp.rightMargin = (layoutWidth - lp.width)/2;  
      } else {
        lp.width = layoutWidth;
        lp.height = (layoutWidth*height)/width;
        lp.topMargin = lp.bottomMargin = (layoutHeight - lp.height)/2;  
      }
      mView.setLayoutParams(lp);
      mView.requestLayout();
    }
  }
  
  private void createView(Context context) {
    if (mView == null) {
      if ((mViewGroup != null) && (mVideoGrabber != null)) {
        mView = mVideoGrabber.createView(context);
        
        mViewGroup.addView(mView, 0);
        updateView();
        
        mViewGroupLayoutListener = new View.OnLayoutChangeListener() {
          @Override
          public void onLayoutChange(View v, int left, int top, int right,
              int bottom, int oldLeft, int oldTop, int oldRight, int oldBottom) {
            updateView();
          }
        };
        mViewGroup.addOnLayoutChangeListener(mViewGroupLayoutListener);
      }
    }
  }
  private void destroyView() {
    if (mView != null) {
      mViewGroup.removeView(mView);
      mView = null;
    }
  }
  
  public boolean create(Context context, ViewGroup group, String url, Setup setup, Options options) {
    int result = 0;
    
    try {
      mViewGroup = group;
      
      mHasFlashlight = false;
    
      mURL = url;
      
      mSetup = setup;
      mOptions = options;
      
      mVideo = new VideoParameters();
      mVideo.outWidth = setup.videoWidth;
      mVideo.outHeight = setup.videoHeight;
      mVideo.framerate = setup.videoFramerate;
      mVideo.bitrate = setup.videoBitRate;
      mVideo.maxLag = setup.videoMaxLag;
      
      mAudio = new AudioParameters();
      mAudio.format = AVSampleFormat.AV_SAMPLE_FMT_S16;
      mAudio.channels = setup.audioChannels;
      mAudio.samplerate = setup.audioSampleRate;
      mAudio.bitrate = setup.audioBitRate;
      mAudio.maxLag = setup.audioMaxLag;
      
      if ((result = initialize(context)) == 0) {
        mLocked = false;
        
        mWakeLock = ((PowerManager)context.getSystemService(Context.POWER_SERVICE)).newWakeLock(PowerManager.PARTIAL_WAKE_LOCK, "Happhub");
        mWifiLock = ((WifiManager)context.getSystemService(Context.WIFI_SERVICE)).createWifiLock("Happhub");
        
        return true;
      }
    } catch(Exception e) {
      result = AVError.UNKNOWN;
    }
    
    stop();
    release(result);
    
    mWifiLock = null;
    mWakeLock = null;
    
    mLocked = false;
    
    return false;
  }
  public void destroy() {
    stop();
    release(0);
    
    mWifiLock = null;
    mWakeLock = null;
    
    mLocked = false;
  }
  
  public void attach(Context context, ViewGroup group) {
    mViewGroup = group;
    createView(context);
  }
  public void detach(Context context) {
    destroyView();
    mViewGroup = null;
  }
  
  private int initialize(Context context) {
    int result = 0;
    
    try {
      if (mCamera != null) {
        release(0);
      }
      
      mCamera = Camera.open(mOptions.cameraIndex);
      if (mCamera != null) {
        mHasFlashlight = (mCamera.getParameters().getFlashMode() != null);
        
        try {
          Camera.Parameters parameters = mCamera.getParameters();
          
          List<Camera.Size> sizes = parameters.getSupportedPreviewSizes();
  
          Camera.Size previewSize = null;
          for (Camera.Size size : sizes) {
            Log.d("HAPPHUB/Publisher", "Preview size available: "+size.width+"x"+size.height);
            if (size.width < mVideo.outWidth || size.height < mVideo.outHeight) {
              continue;
            }
            if (previewSize == null) {
              previewSize = size;
            } else
            if (size.width > previewSize.width || size.height > previewSize.height) {
              continue;
            }
            previewSize = size;
          }
          Log.d("HAPPHUB/Publisher", "Preview size chosen: "+previewSize.width+"x"+previewSize.height);
          
          if (previewSize != null) {
            updateCameraFocusMode(parameters, mFocusMode);
            
            parameters.setPreviewSize(previewSize.width, previewSize.height);
            parameters.setPreviewFormat(ImageFormat.NV21);
            
            mVideo.inWidth = parameters.getPreviewSize().width;
            mVideo.inHeight = parameters.getPreviewSize().height;
            
            int[] fpsRange = null;
            mVideo.framerate *= 1000;
            
            List<int[]> fpsRanges = parameters.getSupportedPreviewFpsRange();
            for (int[] r : fpsRanges) {
              fpsRange = r;
              Log.d("HAPPHUB/Publisher", "FPS range available: ["+(fpsRange[Camera.Parameters.PREVIEW_FPS_MIN_INDEX]/1000)+"-"+fpsRange[Camera.Parameters.PREVIEW_FPS_MAX_INDEX]/1000+"]");
              
              if (mVideo.framerate <= fpsRange[Camera.Parameters.PREVIEW_FPS_MAX_INDEX]) {
                break;
              }
            }
            Log.d("HAPPHUB/Publisher", "FPS range chosen: ["+(fpsRange[Camera.Parameters.PREVIEW_FPS_MIN_INDEX]/1000)+"-"+fpsRange[Camera.Parameters.PREVIEW_FPS_MAX_INDEX]/1000+"]");
            
            mVideo.framerate /= 1000;
            /*
            parameters.setPreviewFpsRange(fpsRange[Camera.Parameters.PREVIEW_FPS_MAX_INDEX], fpsRange[Camera.Parameters.PREVIEW_FPS_MAX_INDEX]);
            */
  
            if (mHasFlashlight) {
              parameters.setFlashMode(mFlashlight ? Camera.Parameters.FLASH_MODE_TORCH : Camera.Parameters.FLASH_MODE_OFF);
            }
            if (mHasZoom) {
              parameters.setZoom(mZoom);
            }
            mCamera.setParameters(parameters);
  
            mAudioGrabber = new SoftwareAudioGrabber();
            if ((result = mAudioGrabber.create(this,  mAudio)) == 0) {
              if (mOptions.captureMode == CaptureMode.HARDWARE) {
                mVideoGrabber = new HardwareVideoGrabber();
              } else {
                mVideoGrabber = new SoftwareVideoGrabber();
              }
              
              if ((result = mVideoGrabber.create(this, mVideo, mCamera)) == 0) {
                mVideo.format = mVideoGrabber.getFormat();
                
                createView(context);
                
                mCamera.startPreview();
                Log.v("HAPPHUB/Publisher", "Publisher initialized");
                
                mInitialized = true;
                setState(State.PREVIEWING);
                
                return 0;
              }
            }
          }
        } catch(Exception e) {
          Log.d("HAPPHUB/Publisher", "Exception in initialize(): " + e.toString());
        }
      }
    } catch(Exception e) {
      result = AVError.UNKNOWN;
    }
    
    release(result);
    return result;
  }
  private void release(int error) {
    destroyView();
    
    if (mVideoGrabber != null) {
      mVideoGrabber.destroy();
      mVideoGrabber = null;
    }
    if (mAudioGrabber != null) {
      mAudioGrabber.destroy();
      mAudioGrabber = null;
    }
    
    if (mCamera != null) {
      doStop();
      
      mCamera.release();
      mCamera = null;
    }
    
    mHasFlashlight = false;
    
    mInitialized = false;
    setState(State.NONE);
    
    if (error != 0) {
      notifyListeners(NOTIFY_ERROR, error);
    }
  }
  
  private void doStart() {
    int result = 0;
    
    if (mWakeLock != null) {
      if (mWifiLock != null) {
        mWifiLock.acquire();
      }
      mWakeLock.acquire();
      mLocked = true;
    }
    
    PublisherImpl publisherImpl = null;
    try {
      publisherImpl = new HardwarePublisher();
      result = publisherImpl.create(this, mURL, mVideo, mAudio);
      
      if (result == 0) {
        result = mVideoGrabber.startPublishing();
        if (result == 0) {
          result = mAudioGrabber.startPublishing();
        }

        if (result == 0) {
          synchronized(this) {
            mFirstTimestamp = -1;
            
            mLastVideoTimestamp = -1;
            mLastAudioTimestamp = -1;
            
            mPublisherImpl = publisherImpl;
            
            this.notifyAll();
          }
          
          setState(State.PUBLISHING);
          return;
        }
      }
    } catch(Exception e) {
      result = AVError.UNKNOWN;
      
      if (publisherImpl != null) {
        publisherImpl.destroy();
      }
    }
    
    mStarted = false;
    doStop();
    
    notifyListeners(NOTIFY_ERROR, result);
  }
  public void start() {
    if (!mStarted) {
      mStarted = true;
      if (mInitialized) {
        doStart();
      }
    }
  }
  
  private void doStop() {
    PublisherImpl publisherImpl;
    synchronized(this) {
      publisherImpl = mPublisherImpl;
      mPublisherImpl = null;
      
      this.notifyAll();
    }
    
    if (mAudioGrabber != null) {
      mAudioGrabber.stopPublishing();
    }
    if (mVideoGrabber != null) {
      mVideoGrabber.stopPublishing();
    }
    
    if (publisherImpl != null) {
      publisherImpl.destroy();
    }
    
    if (mLocked) {
      if (mWifiLock != null) {
        mWifiLock.acquire();
      }
      mWakeLock.release();
      mLocked = false;
    }
    if (mInitialized) {
      setState(State.PREVIEWING);
    } else {
      setState(State.NONE);
    }
  }
  public void stop() {
    if (mStarted) {
      mStarted = false;
      doStop();
    }
  }

  public void postError(int error, Object source) {
    Message msg = this.obtainMessage();
    msg.what = MESSAGE_ERROR;
    msg.arg1 = error;
    msg.obj = source;
    sendMessage(msg);
  }
  
  @Override
  public void handleMessage (Message msg) {
    if (msg.what == MESSAGE_ERROR) {
      stop();
      notifyListeners(NOTIFY_ERROR, msg.arg1);
    }
  }
  
  private PublisherImpl getPublisher(long timeout) throws InterruptedException {
    PublisherImpl publisherImpl = null;
    
    synchronized(this) {
      publisherImpl = mPublisherImpl;
      if ((publisherImpl == null) && (timeout != 0)) {
        if (timeout < 0) {
          this.wait();
        } else {
          this.wait(timeout/1000L/1000L);
        }
        publisherImpl = mPublisherImpl;
      }
    }
    
    return publisherImpl;
  }
  
  protected void pushVideo(ByteBuffer video, long timestamp, long timeout) throws InterruptedException {
    int result = 0;
    
    PublisherImpl publisherImpl = getPublisher(timeout);
    if (publisherImpl != null) {
      boolean drop = false;
      
      synchronized(this) {
        if (mOptions.enforceAudioVideoSync) {
          timestamp = mAudioGrabber.adjustTimestamp(timestamp);
        }
        
        if (mFirstTimestamp == -1) {
          mFirstTimestamp = 
          mLastVideoTimestamp = 
          mLastAudioTimestamp = timestamp;
        }
        if (timestamp < mLastVideoTimestamp) {
          Log.w("HAPPHUB/Publisher", "VIDEO: Frame dropped "+Utils.timestampAsString(timestamp)+"/"+Utils.timestampAsString(mLastVideoTimestamp));
          drop = true;
        } else {
          mLastVideoTimestamp = timestamp;
          timestamp -= mFirstTimestamp; 
        }
      }
      
      if (!drop) {
        result = publisherImpl.pushVideo(video, timestamp, timeout);
      }
    }
    
    if (result != 0) {
      postError(result, publisherImpl);
    }
  }
  protected void pushAudio(ByteBuffer audio, long timestamp, long timeout) throws InterruptedException {
    int result = 0;
    
    PublisherImpl publisherImpl = getPublisher(timeout);
    if (publisherImpl != null) {
      boolean drop = false;
      
      synchronized(this) {
        if (mFirstTimestamp == -1) {
          mFirstTimestamp = 
          mLastVideoTimestamp = 
          mLastAudioTimestamp = timestamp;
        }
        if (timestamp < mLastAudioTimestamp) {
          Log.w("HAPPHUB/Publisher", "AUDIO: Frame dropped "+Utils.timestampAsString(timestamp)+"/"+Utils.timestampAsString(mLastAudioTimestamp));
          drop = true;
        } else {
          timestamp -= mFirstTimestamp; 
        }
      }
      
      if (!drop) {
        result = publisherImpl.pushAudio(audio, timestamp, timeout);
      }
    }
    
    if (result != 0) {
      postError(result, publisherImpl);
    }
  }
  
  static {
    System.loadLibrary("stlport_shared");
    System.loadLibrary("avutil");
    System.loadLibrary("avcodec");
    System.loadLibrary("avformat");
    System.loadLibrary("swscale");
    System.loadLibrary("swresample");
    System.loadLibrary("publisher");
  }
}
