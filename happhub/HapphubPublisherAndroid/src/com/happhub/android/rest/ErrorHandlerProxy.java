package com.happhub.android.rest;

public class ErrorHandlerProxy implements ErrorHandler {
  private boolean mSet = false;
  
  private int mStatus = 0;
  private Exception mException = null;
  private boolean mReportErrors = true;
  
  private ErrorHandler mErrorHandler = null;
  
  public ErrorHandler getErrorHandler() {
    return mErrorHandler;
  }
  public void setErrorHandler(ErrorHandler errorHandler) {
    mErrorHandler = errorHandler;
    if (mSet) {
      if (mErrorHandler != null) {
        mSet = false;
        mErrorHandler.onError(mStatus, mException, mReportErrors);
      }
    }
  }
  
  @Override
  public void onError(int status, Exception exception, boolean reportErrors) {
    if (mErrorHandler != null) {
      mErrorHandler.onError(status, exception, reportErrors);
    } else {
      mSet = true;
      
      mStatus = status;
      mException = exception;
      mReportErrors = reportErrors;
    }
  }
}
