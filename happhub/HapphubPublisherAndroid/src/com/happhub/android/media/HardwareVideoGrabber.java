package com.happhub.android.media;

import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.FloatBuffer;

import javax.microedition.khronos.egl.EGL10;
import javax.microedition.khronos.egl.EGL11;
import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.egl.EGLContext;
import javax.microedition.khronos.egl.EGLDisplay;
import javax.microedition.khronos.egl.EGLSurface;
import javax.microedition.khronos.opengles.GL10;
import javax.microedition.khronos.opengles.GL11;

import android.opengl.GLES11;
import android.opengl.GLES11Ext;
import android.opengl.GLSurfaceView;

import android.content.Context;
import android.graphics.SurfaceTexture;
import android.hardware.Camera;

import android.util.Log;
import android.view.View;

class HardwareVideoGrabber implements Publisher.VideoGrabber {
  private Publisher mPublisher = null;
  private Camera mCamera = null;
  
  private int mWidth = 0;
  private int mHeight = 0;
  
  private EGL10 mEgl = null;
  private EGLDisplay mEglDisplay = null;
  private EGLConfig mEglConfig = null;
  private EGLContext mEglContext = null;
  private EGLSurface mEglSurface = null;
  
  private int mTextureId = -1;
  private SurfaceTexture mTexture = null;
  
  private int mViewTextureId = -1;
  
  private long mTextureTimestampRef = -1;
  private long mTextureTimestampRealRef = -1;
  
  private ByteBuffer mBuffer = null;
  
  private SurfaceTexture.OnFrameAvailableListener mTextureFrameAvailableListener = new SurfaceTexture.OnFrameAvailableListener() {
    @Override
    public void onFrameAvailable(SurfaceTexture surfaceTexture) {
      Publisher publisher = null;
     long timestamp = 0;
     
      synchronized(HardwareVideoGrabber.this) {
         makeCurrent(true);
         mTexture.attachToGLContext(mTextureId);
        
         mTexture.updateTexImage();
        
         if (mTextureTimestampRef == -1) {
           mTextureTimestampRef = mTexture.getTimestamp(); 
           mTextureTimestampRealRef = System.nanoTime(); 
         }
         
         if (mBuffer != null) {
           if (mPublisher != null) {
             publisher = mPublisher;
             
             GLES11.glDrawArrays(GLES11.GL_TRIANGLE_STRIP, 0, 4);
             checkGLError();
             
             GLES11.glReadPixels(0, 0, mWidth, mHeight, GL11.GL_RGBA, GL11.GL_UNSIGNED_BYTE, mBuffer);
             checkGLError();
             
             timestamp = (mTextureTimestampRealRef + (mTexture.getTimestamp()-mTextureTimestampRef))/1000L;
           }
         }
        
         mTexture.detachFromGLContext();
         makeCurrent(false);
        
         if (mView != null) {
           mView.requestRender();
         }
       }
      
       if (publisher != null) {
         try {
           publisher.pushVideo(mBuffer, timestamp, 0);
         } catch(InterruptedException e) {
         }
       }
    }
  };
  
  ByteBuffer mVerCoords = null;
  ByteBuffer mTexCoords = null;
  
  private GLSurfaceView mView = null;
  
  private int checkGLError() {
    int code = GLES11.glGetError();
    if (code != GLES11.GL_NO_ERROR) {
      Log.e("HAPPHUB/HardwareGrabber", "OpenGL error: " + code);
    }
    return code;
  }
  
  private ByteBuffer createFloatBuffer(float[] values) {
    ByteBuffer bb = ByteBuffer.allocateDirect(values.length * 4);
    bb.order(ByteOrder.nativeOrder());
    FloatBuffer fb = bb.asFloatBuffer();
    fb.put(values);
    fb.position(0);
    return bb;
  }
  
  private int createEGL() {
    mEgl = (EGL10)EGLContext.getEGL();
    if (mEgl != null) {
      mEglDisplay = mEgl.eglGetDisplay(EGL10.EGL_DEFAULT_DISPLAY);
      if (mEglDisplay != null) {
        int[] version = new int[2];
        if (mEgl.eglInitialize(mEglDisplay, version)) {
          EGLConfig[] configurations = new EGLConfig[1];
          int[] configurationsCount = new int[1];
          
          int configAttributes[] = {
            EGL10.EGL_RED_SIZE, 8,
            EGL10.EGL_GREEN_SIZE, 8,
            EGL10.EGL_BLUE_SIZE, 8,
            EGL10.EGL_ALPHA_SIZE, 8,
            EGL10.EGL_SURFACE_TYPE, EGL10.EGL_PBUFFER_BIT,
            EGL10.EGL_NONE
          };
          if (mEgl.eglChooseConfig(mEglDisplay, configAttributes, configurations, 1, configurationsCount)) {
            mEglConfig = configurations[0];
            
            mEglContext = mEgl.eglCreateContext(mEglDisplay, mEglConfig, EGL10.EGL_NO_CONTEXT, null);
            if (mEglContext != null) {
              int surfaceAttributes[] = {
                EGL10.EGL_WIDTH, mWidth,
                EGL10.EGL_HEIGHT, mHeight,
                EGL10.EGL_NONE
              };
              
              mEglSurface = mEgl.eglCreatePbufferSurface(mEglDisplay, mEglConfig, surfaceAttributes);
              if (mEglSurface != null) {
                makeCurrent(true);
                
                int[] textureId = new int[1];
                GLES11.glGenTextures(1, textureId, 0);
                checkGLError();
                
                mTextureId = textureId[0];
                setupGL(mTextureId, false);
                
                GLES11.glViewport(0, 0, mWidth, mHeight);
                checkGLError();
                
                makeCurrent(false);
                return 0;
              }
            }
          }
        }
      }
    }
    
    destroyEGL();
    return AVError.UNKNOWN;
  }
  private void destroyEGL() {
    if (mEglSurface != null) {
      mEgl.eglMakeCurrent(mEglDisplay, EGL11.EGL_NO_SURFACE, EGL11.EGL_NO_SURFACE, EGL11.EGL_NO_CONTEXT);
    }
    
    if (mEglSurface != null) {
      mEgl.eglDestroySurface(mEglDisplay, mEglSurface);
      mEglSurface = null;
    }
    if (mEglContext != null) {
      mEgl.eglDestroyContext(mEglDisplay, mEglContext);
      mEglContext = null;
    }
    if (mEglDisplay != null) {
      mEgl.eglTerminate(mEglDisplay);
      mEglDisplay = null;
    }
    
    mEglConfig = null;
    mEgl = null;
  }
  
  void setupGL(int textureId, boolean invertY) {
    if (invertY) {
      GLES11.glOrthof(0, 1, 1, 0, -1, 1);
    } else {
      GLES11.glOrthof(0, 1, 0, 1, -1, 1);
    }
    checkGLError();
    
    GLES11.glBindTexture(GLES11Ext.GL_TEXTURE_EXTERNAL_OES, textureId);
    checkGLError();
    
    GLES11.glTexParameterf(GLES11Ext.GL_TEXTURE_EXTERNAL_OES, GLES11.GL_TEXTURE_MIN_FILTER, GLES11.GL_LINEAR);        
    checkGLError();
    GLES11.glTexParameterf(GLES11Ext.GL_TEXTURE_EXTERNAL_OES, GLES11.GL_TEXTURE_MAG_FILTER, GLES11.GL_LINEAR);
    checkGLError();
    GLES11.glTexParameteri(GLES11Ext.GL_TEXTURE_EXTERNAL_OES, GLES11.GL_TEXTURE_WRAP_S, GLES11.GL_CLAMP_TO_EDGE);
    checkGLError();
    GLES11.glTexParameteri(GLES11Ext.GL_TEXTURE_EXTERNAL_OES, GLES11.GL_TEXTURE_WRAP_T, GLES11.GL_CLAMP_TO_EDGE);
    checkGLError();
    
    GLES11.glEnable(GLES11Ext.GL_TEXTURE_EXTERNAL_OES);
    checkGLError();
    
    GLES11.glEnableClientState(GLES11.GL_VERTEX_ARRAY);
    checkGLError();
    GLES11.glEnableClientState(GLES11.GL_TEXTURE_COORD_ARRAY);
    checkGLError();
   
    GLES11.glVertexPointer(3, GLES11.GL_FLOAT, 0, mVerCoords);
    checkGLError();
    GLES11.glTexCoordPointer(2, GLES11.GL_FLOAT, 0, mTexCoords);
    checkGLError();
  }
  
  private int makeCurrent(boolean current) {
    if (mEgl != null) {
      if (mEgl.eglMakeCurrent(mEglDisplay, current ? mEglSurface : EGL11.EGL_NO_SURFACE,  current ? mEglSurface : EGL11.EGL_NO_SURFACE, current ? mEglContext : EGL11.EGL_NO_CONTEXT)) {
        return 0;
      }
      
      int code = mEgl.eglGetError();
      if (code == EGL11.EGL_CONTEXT_LOST) {
        return createEGL();
      }
    }
    return AVError.UNKNOWN;
  }
  
  @Override
  synchronized
  public int create(Publisher publisher, Publisher.VideoParameters parameters, Camera camera) {
    int result = 0;
    
    try {
      mPublisher = publisher;
      
      mWidth = parameters.inWidth;
      mHeight = parameters.inHeight;
      
      mCamera = camera;
      
      mVerCoords = createFloatBuffer(new float[] {
        0, 1, 0,
        1, 1, 0,
        0, 0, 0,
        1, 0, 0
      });
      mTexCoords = createFloatBuffer(new float[] {
        0, 1,
        1, 1,
        0, 0,
        1, 0
      });
  
      if ((result = createEGL()) == 0) {
        mTexture = new SurfaceTexture(mTextureId);
        mTexture.detachFromGLContext();
        
        mTexture.setOnFrameAvailableListener(mTextureFrameAvailableListener);
        
        mCamera.setPreviewTexture(mTexture);
        return 0;
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
    stopPublishing();
    
    if (mTexture != null) {
      mTexture.setOnFrameAvailableListener(null);
      mTexture = null;
    }

    destroyEGL();
    
    mVerCoords = null;
    mTexCoords = null;
    
    mPublisher = null;
  }

  @Override
  synchronized
  public View createView(Context context) {
    mView = new GLSurfaceView(context);
    mView.setRenderer(new GLSurfaceView.Renderer() {
      @Override
      public void onSurfaceCreated(GL10 gl, EGLConfig config) {
        synchronized(HardwareVideoGrabber.this) {
          int[] textureId = new int[1];
          GLES11.glGenTextures(1, textureId, 0);
          checkGLError();
          
          mViewTextureId = textureId[0];
          setupGL(mViewTextureId, true);
        }
      }
      
      @Override
      public void onSurfaceChanged(GL10 gl, int width, int height) {
        GLES11.glViewport(0, 0, width, height);
        checkGLError();
      }
      
      @Override
      public void onDrawFrame(GL10 gl) {
        synchronized(HardwareVideoGrabber.this) {
          if (mTexture != null) {
            mTexture.attachToGLContext(mViewTextureId);
            
            GLES11.glDrawArrays(GLES11.GL_TRIANGLE_STRIP, 0, 4);
            checkGLError();
            
            mTexture.detachFromGLContext();
          }
        }
      }
    });
    
    mView.setRenderMode(GLSurfaceView.RENDERMODE_WHEN_DIRTY);
    return mView;
  }

  @Override
  synchronized
  public void destroyView() {
    mView = null;
  }
  
  @Override
  public AVPixelFormat getFormat() {
    return AVPixelFormat.AV_PIX_FMT_RGBA;
  }

  @Override
  synchronized
  public int startPublishing() {
    try {
      mBuffer = ByteBuffer.allocateDirect(mWidth*mHeight*4); // RGBA
    } catch(Exception e) {
      return AVError.ENOMEM;
    }
    return 0;
  }
  @Override
  synchronized
  public void stopPublishing() {
    mBuffer = null;
  }
}
