package com.happhub.android.rest;

public interface ErrorHandler {
  public void onError(int status, Exception exception, boolean reportErrors);
};
