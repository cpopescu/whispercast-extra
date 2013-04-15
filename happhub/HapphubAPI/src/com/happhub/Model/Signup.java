package com.happhub.Model;

import java.io.Serializable;

public class Signup implements Serializable {
  private static final long serialVersionUID = 1L;

  private String hash = null;
  private String email = null;
  private int accountsId = -1;

  public Signup() {
  }

  public String getHash() {
    return this.hash;
  }
  public void setHash(String hash) {
    this.hash = hash;
  }

  public String getEmail() {
    return this.email;
  }
  public void setEmail(String email) {
    this.email = email;
  }

  public int getAccountsId() {
    return this.accountsId;
  }
  public void setAccountsId(int accountsId) {
    this.accountsId = accountsId;
  }
}