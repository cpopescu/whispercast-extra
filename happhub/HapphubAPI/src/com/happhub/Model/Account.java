package com.happhub.Model;

import java.io.Serializable;
import java.util.List;

public class Account implements Serializable {
  private static final long serialVersionUID = 1L;

  private int id = -1;
  private String name = null;
  private List<Role> roles =  null;
  private List<Session> sessions = null;

  public Account() {
  }

  public int getId() {
    return this.id;
  }
  public void setId(int id) {
    this.id = id;
  }

  public String getName() {
    return this.name;
  }
  public void setName(String name) {
    this.name = name;
  }

  public List<Role> getRoles() {
    return this.roles;
  }
  public void setRoles(List<Role> roles) {
    this.roles = roles;
  }

  public List<Session> getSessions() {
    return this.sessions;
  }
  public void setSessions(List<Session> sessions) {
    this.sessions = sessions;
  }
}