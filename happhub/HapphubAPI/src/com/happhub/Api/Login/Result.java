package com.happhub.Api.Login;

import com.happhub.Model.User;

public class Result {
  private int code;
  private String result;
  private User user;
    
  public Result() {
  }
  public Result(int code, String result, User user) {
    this.code = code;
    this.result = result;
    this.user = user;
  }
  
  public int getCode() {
    return this.code;
  }
  public void setCode(int code) {
    this.code = code;
  }
  
  public String getResult() {
    return this.result;
  }
  public void setResult(String result) {
    this.result = result;
  }
  
  public User getUser() {
    return this.user;
  }
  public void setUser(User user) {
    this.user = user;
  }
}