package com.happhub.android.publisher;

import org.apache.http.HttpStatus;

import com.happhub.android.rest.Result;

import android.content.Context;
import android.content.Intent;

public class Application extends com.happhub.android.Application {
  static
  public void restError(Context context, int status, Exception e) {
    if (status == HttpStatus.SC_FORBIDDEN || status == 1) {
      toast(context, R.string.login_error_failed);
    } else {
      toast(context, R.string.connection_error);
    }
  }
  static
  public void restError(Context context, Result<?> data) {
    restError(context, data.getStatus(), data.getException());
  }
  
  @Override
  public void onCreate() {
    super.onCreate();
    
    startService(new Intent(this, Service.class));
  }
  @Override
  public void onTerminate() {
    super.onTerminate();
    
    stopService(new Intent(this, Service.class));
  }
}
