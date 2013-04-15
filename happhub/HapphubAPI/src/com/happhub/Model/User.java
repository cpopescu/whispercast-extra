package com.happhub.Model;

import java.io.Serializable;
import java.util.List;

public class User implements Serializable {
  private static final long serialVersionUID = 1L;

  private int id = -1;
  private String email = null;
  private String password = null;
  private List<Role> roles;

  public User() {
  }

  public int getId() {
    return this.id;
  }
  public void setId(int id) {
    this.id = id;
  }

  public String getEmail() {
    return this.email;
  }
  public void setEmail(String email) {
    this.email = email;
  }

  public String getPassword() {
    return this.password;
  }
  public void setPassword(String password) {
    this.password = password;
  }

  public List<Role> getRoles() {
    return this.roles;
  }
  public void setRoles(List<Role> roles) {
    this.roles = roles;
  }
}