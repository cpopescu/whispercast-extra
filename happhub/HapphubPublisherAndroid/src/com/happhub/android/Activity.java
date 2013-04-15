package com.happhub.android;

import android.annotation.TargetApi;
import android.app.ActionBar;
import android.os.Build;
import android.view.Window;
import android.view.WindowManager;

public class Activity extends android.app.Activity {
  @Override 
  public void onResume(){
    super.onResume();
    Application.setRunningActivity(this);
  }

  @Override
  public void onPause(){
    super.onPause();
    Application.setRunningActivity(null);
  }
  
  @TargetApi(Build.VERSION_CODES.HONEYCOMB)
  public void setFullscreen(boolean fullscreen) {
    if (fullscreen) {
      requestWindowFeature(Window.FEATURE_NO_TITLE); 
      getWindow().addFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN);
    } else {
      getWindow().clearFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN);
    }
    
    if (((android.os.Build.VERSION.SDK_INT >= android.os.Build.VERSION_CODES.HONEYCOMB))) {
      ActionBar actionBar = getActionBar();
      if (actionBar != null) {
        if (fullscreen) {
          actionBar.hide();
        } else {
          actionBar.show();
        }
      }
    }
  }
}
