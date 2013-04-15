package com.happhub.android.rest;

public class DataHandlerProxy<T> implements DataHandler<T> {
  private boolean mSet = false;
  
  private Result<T> mData = null;
  private boolean mReportErrors = true;
  
  private DataHandler<T> mDataHandler = null;
  
  public DataHandler<T> getDataHandler() {
    return mDataHandler;
  }
  public void setDataHandler(DataHandler<T> dataHandler) {
    mDataHandler = dataHandler;
    if (mSet) {
      if (mDataHandler != null) {
        mSet = false;
        mDataHandler.onDataChanged(mData, mReportErrors);
      }
    }
  }
  
  @Override
  public void onDataChanged(Result<T> data, boolean reportErrors) {
    if (mDataHandler != null) {
      mDataHandler.onDataChanged(data, reportErrors);
    } else {
      mSet = true;
      mData = data;
      mReportErrors = reportErrors;
    }
  }
};