package com.happhub.android;

import android.app.Activity;
import android.content.Context;
import android.widget.Toast;

public class Application extends android.app.Application {
  static
  protected Activity mRunningActivity = null;
  
  public Application() {
  }
  
  static
  public void setRunningActivity(Activity activity) {
    mRunningActivity = activity;
  }
  static
  public Activity getRunningActivity() {
    return mRunningActivity;
  }

  /*
  static
  public void notificationClear(Context context, int id) {
    NotificationManager notificationManager = (NotificationManager)context.getSystemService(Context.NOTIFICATION_SERVICE);
    notificationManager.cancel(id);
  }
  
  static
  public void notification(Context context, int id, int flags, String title, String message, int iconId) {
    NotificationManager notificationManager = (NotificationManager)context.getSystemService(Context.NOTIFICATION_SERVICE);
    notificationManager.cancel(id);
   
    Intent intent = new Intent(context, context.getClass());
    intent.putExtra("notification", true);
    
    TaskStackBuilder stackBuilder = TaskStackBuilder.create(context);
    stackBuilder.addParentStack(context.getClass());
    stackBuilder.addNextIntent(intent);
    
    PendingIntent pendingIntent = stackBuilder.getPendingIntent(0, PendingIntent.FLAG_UPDATE_CURRENT);
    
    Notification notification = new Notification.Builder(context).
    setContentTitle(title).
    setContentText(message).
    setSmallIcon(iconId).
    setContentIntent(pendingIntent).
    setLargeIcon(BitmapFactory.decodeResource(context.getResources(), iconId)).
    build();
    
    notification.flags |= flags;
    notificationManager.notify(id, notification);
  }
  static
  public void notification(Context context, int id, int flags, String title, int messageId, int iconId) {
    if (context != null) {
      notification(context, id, flags, title, context.getString(messageId), iconId);
    }
  }
  static
  public void notification(Context context, int id, int flags, int titleId, String message, int iconId) {
    notification(context, id, flags, context.getString(titleId), message, iconId);
  }
  static
  public void notification(Context context, int id, int flags, int titleId, int messageId, int iconId) {
    if (context != null) {
      notification(context, id, flags, context.getString(titleId), context.getString(messageId), iconId);
    }
  }
  */
  
  static
  public Toast toast(Context context, String message) {
    Toast toast = Toast.makeText(context, message, Toast.LENGTH_SHORT);
    toast.show();
    return toast;
  }
  static
  public Toast toast(Context context, int messageId) {
    if (context != null) {
      return toast(context, context.getString(messageId));
    }
    return null;
  }
}
