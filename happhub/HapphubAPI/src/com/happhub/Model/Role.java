package com.happhub.Model;

import java.io.Serializable;

public class Role implements Serializable {
  private static final long serialVersionUID = 1L;

  private RolePK id = null;
  private String role = null;
  private Account account = null;
  private User user = null;

  public Role() {
  }

  public String getRole() {
    return this.role;
  }
  public void setRole(String role) {
    this.role = role;
  }

  public RolePK getId() {
    return this.id;
  }
  public void setId(RolePK id) {
    this.id = id;
  }
  
  public Account getAccount() {
    return this.account;
  }
  public void setAccount(Account account) {
    this.account = account;
  }
  
  public User getUser() {
    return this.user;
  }
  public void setUser(User user) {
    this.user = user;
  }
}