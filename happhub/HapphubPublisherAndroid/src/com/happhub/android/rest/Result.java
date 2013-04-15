package com.happhub.android.rest;

public class Result<T>  {
  protected int status;
  protected Exception exception;
  protected T value;
  
  public Result(int status, Exception exception, T value) {
    this.status = status;
    this.exception = exception;
    this.value = value;
  }
  
  public int getStatus() {
    return status;
  }
  public Exception getException() {
    return exception;
  }
  public T getValue() {
    return value;
  }
};
