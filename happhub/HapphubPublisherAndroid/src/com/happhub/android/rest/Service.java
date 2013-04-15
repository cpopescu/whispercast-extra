package com.happhub.android.rest;

import java.lang.ref.WeakReference;

import android.os.AsyncTask;
import android.util.SparseArray;

public class Service extends com.happhub.android.Service {
  public interface Listener<T> {
    public void onServiceCallComplete(int taskId, Result<T> result);
  };
  
  abstract
  protected class Task<T> extends AsyncTask<Void, Void, Result<T>> {
    protected WeakReference<Listener<T>> mListener = null;
    protected Integer mTaskId = null;
    
    @Override
    protected void onPostExecute(Result<T> result) {
      Listener<T> listener = mListener.get();
      if (listener != null) {
        listener.onServiceCallComplete(mTaskId.intValue(), result);
        Service.this.mTasks.delete(mTaskId.intValue());
      }
    }
  };
  
  @SuppressWarnings("rawtypes")
  private SparseArray<Task> mTasks = new SparseArray<Task>(); 
  private int mNextTaskId = 0;
  
  protected int nextTaskId() {
    return ++mNextTaskId;
  }
  
  protected <T> Integer execute(Task<T> task) {
    int taskId = task.mTaskId.intValue(); 
    mTasks.put(taskId, task);
    task.execute();
    
    return taskId;
  }
  
  public <T> Integer reattach(int taskId, Listener<T> listener) {
    @SuppressWarnings("unchecked")
    Task<T> task = (Task<T>)mTasks.get(taskId);
    
    if (task != null) {
      task.mListener = new WeakReference<Listener<T>>(listener);
      return taskId;
    }
    return null;
  }
}