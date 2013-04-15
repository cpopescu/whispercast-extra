package com.happhub.android;

public class ProgressHandlerProxy implements ProgressHandler {
  private boolean mSet = false;
  private boolean mShow = false;
  
  private ProgressHandler mProgressHandler = null;
  
  public ProgressHandler getProgressHandler() {
    return mProgressHandler;
  }
  public void setProgressHandler(ProgressHandler progressHandler) {
    mProgressHandler = progressHandler;
    if (mSet) {
      if (mProgressHandler != null) {
        mSet = false;
        mProgressHandler.showProgress(mShow);
      }
    }
  }
  
  @Override
  public void showProgress(boolean show) {
    if (mProgressHandler != null) {
      mProgressHandler.showProgress(show);
    } else {
      mSet = true;
      mShow = show;
    }
  }
}
