package com.happhub.android.rest;

public interface DataHandler<T> {
  public void onDataChanged(Result<T> data, boolean reportErrors);
};