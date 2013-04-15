package com.happhub.android.media;

import java.nio.ByteBuffer;

import android.content.Context;
import android.graphics.ImageFormat;
import android.graphics.SurfaceTexture;
import android.hardware.Camera;
import android.view.TextureView;
import android.view.TextureView.SurfaceTextureListener;
import android.view.View;

public class SoftwareVideoGrabber implements Publisher.VideoGrabber, Camera.PreviewCallback {
  private Publisher mPublisher = null;
  private Camera mCamera = null;
  
  private AVPixelFormat mFormat = null;
  
  private int mWidth = 0;
  private int mHeight = 0;
  
  private SurfaceTexture mSurface = null;
  private TextureView mView = null;
  
  private ByteBuffer mBuffer = null;
  
  @Override
  synchronized
  public int create(Publisher publisher, Publisher.VideoParameters parameters, Camera camera) {
    int result = 0;
    
    try {
      mPublisher = publisher; 
    
      mCamera = camera;
      switch (mCamera.getParameters().getPreviewFormat()) {
        case ImageFormat.NV21:
          mFormat = AVPixelFormat.AV_PIX_FMT_NV21;
          break;
        default:
          mFormat = null;
      }
      
      mWidth = mCamera.getParameters().getPreviewSize().width;
      mHeight = mCamera.getParameters().getPreviewSize().height;
      
      mCamera.setPreviewCallbackWithBuffer(this);
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
    if (mCamera != null) {
      mCamera.setPreviewCallbackWithBuffer(null);
      mCamera = null;
    }
    mSurface = null;
    
    mPublisher = null; 
  }

  @Override
  public View createView(Context context) {
    mView = new TextureView(context);
    
    if (mSurface != null) {
      mView.setSurfaceTexture(mSurface);
    }
    
    mView.setSurfaceTextureListener(new SurfaceTextureListener() {
      @Override
      public void onSurfaceTextureAvailable(SurfaceTexture surface, int width, int height) {
        mSurface = surface;
        if (mCamera != null) {
          try {
            mCamera.setPreviewTexture(surface);
          } catch(Exception e) {
          }
        }
      }
      @Override
      public boolean onSurfaceTextureDestroyed(SurfaceTexture surface) {
        return false;
      }
      @Override
      public void onSurfaceTextureSizeChanged(SurfaceTexture surface, int width, int height) {
      }
      @Override
      public void onSurfaceTextureUpdated(SurfaceTexture surface) {
      }
    });
    return mView;
  }

  @Override
  public void destroyView() {
    mView = null;
  }
  
  @Override
  public AVPixelFormat getFormat() {
    return mFormat;
  }
  
  @Override
  synchronized
  public void onPreviewFrame(byte[] data, Camera camera) {
    try {
      if (mBuffer != null) {
        mPublisher.pushVideo(mBuffer, System.nanoTime()/1000L, -1);
      }
    } catch(InterruptedException e) {
    }
    
    if (mBuffer != null) {
      mCamera.addCallbackBuffer(mBuffer.array());
    }
  }

  @Override
  synchronized
  public int startPublishing() {
    int result = 0;
    
    try {
      mBuffer = ByteBuffer.allocateDirect((mWidth*mHeight*ImageFormat.getBitsPerPixel(mCamera.getParameters().getPreviewFormat()))/8);
      if (mBuffer != null) {
        mCamera.addCallbackBuffer(mBuffer.array());
        return 0;
      }
    } catch(Exception e) {
      result = AVError.ENOMEM; 
    }
    return result;
  }
  @Override
  synchronized
  public void stopPublishing() {
    mBuffer = null;
  }
}
