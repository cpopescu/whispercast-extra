package com.happhub.android;

import android.annotation.TargetApi;
import android.app.Activity;
import android.os.Build;
import android.view.Menu;
import android.view.MenuItem;
import android.view.Window;

public class ProgressHandlerOnActionBar implements ProgressHandler {
  private Activity mActivity = null;
  
  private int mMenuItemId = 0;
  private int mMenuItemViewId = 0;
  
  private Menu mMenu = null;
  
  private boolean mShow = false;
  
  public Activity getActivity() {
    return mActivity;
  }
  
  @TargetApi(Build.VERSION_CODES.HONEYCOMB)
  public void setActivity(Activity activity, int menuId, int actionViewId) {
    mActivity = activity;
    
    mMenuItemId = menuId;
    mMenuItemViewId = actionViewId;
    
    if (mActivity != null) {
      if (((android.os.Build.VERSION.SDK_INT < android.os.Build.VERSION_CODES.HONEYCOMB))) {
        mActivity.requestWindowFeature(Window.FEATURE_INDETERMINATE_PROGRESS);
      }
    }
    updateProgress();
  }
  
  public Menu getMenu() {
    return mMenu;
  }
  public void setMenu(Menu activity) {
    mMenu = activity;
    updateProgress();
  }
  
  @TargetApi(Build.VERSION_CODES.HONEYCOMB)
  private void updateProgress() {
    if (mMenu != null) {
      if (((android.os.Build.VERSION.SDK_INT >= android.os.Build.VERSION_CODES.HONEYCOMB))) {
        MenuItem refreshItem = mMenu.findItem(mMenuItemId);
        if (refreshItem != null) {
          if (mShow) {
            refreshItem.setActionView(mMenuItemViewId);
          } else {
            refreshItem.setActionView(null);
          }
        }
      } else {
        if (mActivity != null) {
          mActivity.setProgressBarIndeterminateVisibility(mShow);
        }
      }
    }
  }
  
  @Override
  public void showProgress(boolean show) {
    mShow = show;
    updateProgress();
  }
}
