package com.happhub.android.rest;

import com.happhub.android.ProgressHandler;
import com.happhub.android.ServiceConnection;

abstract
public class Processor<T> implements ServiceConnection.Client, Service.Listener<T> {
  protected ServiceConnection mServiceConnection = null; 
  protected ProgressHandler mProgressHandler = null;
  
  protected Integer mTaskId = null;
  
  protected boolean mIsDirty = false;
  protected boolean mHasError = false;
  
  protected DataHandler<T> mDataHandler = null;
  protected Result<T> mData = null;
  
  public void setServiceConnection(ServiceConnection serviceConnection) {
    mServiceConnection = serviceConnection;
    onServiceChanged();
  }
  public ServiceConnection getServiceConnection() {
    return mServiceConnection;
  }
  
  public void setProgressHandler(ProgressHandler progressHandler) {
    mProgressHandler = progressHandler;
    updateProgress();
  }
  public ProgressHandler getProgressHandler() {
    return mProgressHandler;
  }
  
  public void setDataHandler(DataHandler<T> dataHandler) {
    mDataHandler = dataHandler;
    if (mDataHandler != null) {
      mDataHandler.onDataChanged(mData, true);
    }
  }
  public DataHandler<T> getDataHandler() {
    return mDataHandler;
  }
  
  protected void setData(Result<T> data, boolean force) {
    if (force || (data != mData)) {
      mData = data;
      if (mDataHandler != null) {
        mDataHandler.onDataChanged(mData, true);
      }
    }
  }
  public Result<T> getData() {
    return mData;
  }
  
  protected void updateProgress() {
    if (mProgressHandler != null) {
      mProgressHandler.showProgress(isRunning());
    }
  }
  
  public boolean isDirty() {
    return mIsDirty;
  }
  public boolean hasError() {
    return mHasError;
  }
  
  public boolean isBound() {
    return (mServiceConnection != null) && (mServiceConnection.getService() != null);
  }
  public boolean isRunning() {
    return (mIsDirty && !mHasError)|| (isBound() && (mTaskId != null));
  }
  
  public void cleanup() {
    setServiceConnection(null);
    
    setProgressHandler(null);
    setDataHandler(null);
  }
  
  abstract
  protected boolean onExecute();
  
  public void execute() {
    mHasError = false;
    
    mIsDirty = onExecute();
    updateProgress();
  }
  
  @Override
  public void onServiceChanged() {
    mHasError = false;
    
    Service service = (mServiceConnection != null) ? (Service)mServiceConnection.getService() : null;
    if (service != null) {
      if (mTaskId != null) {
        mTaskId = service.reattach(mTaskId, this);
      }
      if (mTaskId == null) {
        if (mIsDirty) {
          execute();
          return;
        }
      }
    }
    updateProgress();
  }
  
  @Override
  public void onServiceCallComplete(int taskId, Result<T> result) {
    if (taskId == mTaskId) {
      mTaskId = null;
      if ((result == null) || (result.getStatus() != 0)) {
        mHasError = true;
        mIsDirty = true;
      }
      
      if (mIsDirty && !mHasError) {
        execute();
      } else {
        setData(result, false);
        updateProgress();
      }
    }
  }
}
