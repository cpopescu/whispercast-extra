package com.happhub.android.publisher;

import java.lang.ref.WeakReference;
import java.util.Map;

import android.app.Notification;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.content.Context;
import android.content.Intent;
import android.media.RingtoneManager;
import android.util.Log;

import com.happhub.Model.User;
import com.happhub.android.Application;
import com.happhub.android.media.Publisher;
import com.happhub.android.rest.Result;

public class Service extends com.happhub.android.rest.Service {
  private static final String URL = "http://192.168.0.100:8080";
  
  private Happhub mHapphub = new Happhub(URL);
  
  static
  private class ServicePublisher extends com.happhub.android.media.Publisher {
    static
    private long mVibratePattern[] = {0, 300};
    
    private WeakReference<Service> mService = null;
    private Notification mNotification = null;
    
    private ServicePublisher(Service service) {
      mService = new WeakReference<Service>(service);
    }
    
    @Override
    protected void notifyListeners(int what, int arg) {
      super.notifyListeners(what, arg);

      Service service = mService.get();
      if (service != null) {
        Context context = service.getApplicationContext();
        
        if (what == Publisher.NOTIFY_STATE_CHANGED) {
          Publisher.State state = getState();
          if (state == Publisher.State.PUBLISHING) {
            if (mNotification == null) {
              Intent intent = new Intent(context, PublishActivity.class);
              intent.addFlags(Intent.FLAG_ACTIVITY_CLEAR_TOP | Intent.FLAG_ACTIVITY_SINGLE_TOP);
        
              mNotification = new Notification.Builder(context).
              setContentTitle(context.getText(R.string.app_name)).
              setContentText(context.getText(R.string.publisher_running)).
              setSmallIcon(R.drawable.ic_launcher).
              setContentIntent(PendingIntent.getActivity(context, 0, intent, 0)).
              setOngoing(true).
              setUsesChronometer(true).
              setLights(0xff00ff00, 1000, 1000).
              build();
              
              service.startForeground(1, mNotification);
            }
          }
          else {
            service.stopForeground(true);
            mNotification = null;
          }
          
          if (arg != 0) {
          }
        }
        else
        if (what == Publisher.NOTIFY_ERROR) {
          Intent intent = new Intent(context, PublishActivity.class);
          intent.addFlags(Intent.FLAG_ACTIVITY_CLEAR_TOP | Intent.FLAG_ACTIVITY_SINGLE_TOP);

          Notification notification = new Notification.Builder(context).
          setContentTitle(context.getText(R.string.app_name)).
          setContentText(String.format(context.getString(R.string.publisher_error), arg)).
          setSmallIcon(R.drawable.ic_launcher).
          setContentIntent(PendingIntent.getActivity(context, 0, intent, 0)).
          setLights(0xffff0000, 1000, 1000).
          setSound(RingtoneManager.getDefaultUri(RingtoneManager.TYPE_NOTIFICATION)).
          setVibrate(mVibratePattern).
          build();
          
          ((NotificationManager)context.getSystemService(NOTIFICATION_SERVICE)).notify(2, notification);
          if (Application.getRunningActivity() == null) {
          } else {
            Application.toast(Application.getRunningActivity(), String.format(context.getString(R.string.publisher_error), arg));
          }
        }
      }
    }
  }
  private Publisher mPublisher = null;
  
  public Publisher getPublisher() {
    if (mPublisher == null) {
      mPublisher = new ServicePublisher(this);
    }
    return mPublisher;
  }

  public Integer login(final Listener<User> listener, final String email, final String password) {
    return execute(new Task<User>() {
      {
        mTaskId = Integer.valueOf(nextTaskId());
        mListener = new WeakReference<Listener<User>>(listener);
      }
      protected Result<User> doInBackground(Void... p) {
        return mHapphub.login(email, password);
      }
    });
  }
  
  public Integer logout(final Listener<User> listener) {
    return execute(new Task<User>() {
      {
        mTaskId = Integer.valueOf(nextTaskId());
        mListener = new WeakReference<Listener<User>>(listener);
      }
      protected Result<User> doInBackground(Void... p) {
        return mHapphub.logout();
      }
    });
  }
  
  public <T> Integer get(final Listener<T> listener, final String url, final Class<?> clss, final Map<String, String> params) {
    return execute(new Task<T>() {
      {
        mTaskId = Integer.valueOf(nextTaskId());
        mListener = new WeakReference<Listener<T>>(listener);
      }
      protected Result<T> doInBackground(Void... p) {
        return mHapphub.get(url, clss, params);
      }
    });
  }

  @Override
  public void onCreate() {
    Log.v("HAPPHUB", "Service created");
    super.onCreate();
  }
  @Override
  public void onDestroy() {
    if (mPublisher != null) {
      mPublisher.destroy();
      mPublisher = null;
    }
    
    super.onDestroy();
    Log.v("HAPPHUB", "Service destroyed");
    
  }
}