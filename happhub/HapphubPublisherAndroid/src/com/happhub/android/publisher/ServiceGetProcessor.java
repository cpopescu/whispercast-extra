package com.happhub.android.publisher;

import java.util.Map;

import android.util.Log;

import com.happhub.android.rest.Processor;

public class ServiceGetProcessor<T> extends Processor<T> {
  private String mUrl = null;
  private Class<?> mDataClass = null;
  private Map<String, String> mParams = null; 

  public void setup(String url, Class<?> clss, Map<String, String> params) {
    mUrl = url;
    mDataClass = clss;
    mParams = params;
  }
  
  @Override
  protected boolean onExecute() {
    Log.v("HAPPHUB", "Executing " + this);
    
    if (mUrl != null) {
      if (isBound()) {
        if (mTaskId == null) {
          mTaskId = ((Service)mServiceConnection.getService()).get(this, mUrl, mDataClass, mParams);
          return false;
        }
      }
      return true;
    }
    return false;
  }
}
