package com.happhub.android;

import android.content.Intent;
import android.os.IBinder;

public class Service extends android.app.Service {
  public class Binder extends android.os.Binder {
    public Service getService() {
      return Service.this;
    }
  };

  private IBinder mBinder = new Binder();
  
  @Override
  public IBinder onBind(Intent arg0) {
    return mBinder;
  }
}