package com.happhub.Api.Signup;

public class Result {
  private int code;
  private String result;
  private String hash;
    
  public Result() {
  }
  public Result(int code, String result, String hash) {
    this.code = code;
    this.result = result;
    this.hash = hash;
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
  
  public String getHash() {
    return this.hash;
  }
  public void setHash(String hash) {
    this.hash = hash;
  }
}