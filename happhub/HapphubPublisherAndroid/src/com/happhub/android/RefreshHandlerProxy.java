package com.happhub.android;

public class RefreshHandlerProxy implements RefreshHandler {
  private boolean mSet = false;
  
  private RefreshHandler mRefreshHandler = null;
  
  public RefreshHandler getRefreshHandler() {
    return mRefreshHandler;
  }
  public void setRefreshHandler(RefreshHandler refreshHandler) {
    mRefreshHandler = refreshHandler;
    if (mSet) {
      if (mRefreshHandler != null) {
        mSet = false;
        mRefreshHandler.refresh();
      }
    }
  }
  
  @Override
  public void refresh() {
    if (mRefreshHandler != null) {
      mRefreshHandler.refresh();
    } else {
      mSet = true;
    }
  }
}
