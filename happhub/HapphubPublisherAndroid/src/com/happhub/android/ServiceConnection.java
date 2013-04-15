package com.happhub.android;

import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.os.IBinder;

public class ServiceConnection implements android.content.ServiceConnection {
  public interface Client {
    public void onServiceChanged();
  }
  
  private Client mClient = null;
  private Service mService = null;
  
  private boolean mBound = false;
  
  public ServiceConnection(Client client) {
    mClient = client;
  }
  
  public Service getService() {
    return mService;
  }
  
  public void bind(Context context, Class<?> service) {
    context.bindService(new Intent(context, service), this, Context.BIND_AUTO_CREATE);
    mBound = true;
  }
  public void unbind(Context context) {
    if (mBound) {
      context.unbindService(this);
      mBound = false;
      
      setService(null);
    }
  }
  
  private void setService(Service service) {
    if (service == mService) {
      return;
    }
    if (mService != null && service != null) {
      throw new IllegalStateException();
    }
    
    mService = service;
    if (mClient != null) {
      mClient.onServiceChanged();
    }
  }
  
  @Override
  public void onServiceConnected(ComponentName className, IBinder service) {
    setService(((Service.Binder)service).getService());
  }
  @Override
  public void onServiceDisconnected(ComponentName className) {
    setService(null);
  }
}
