package com.happhub.Model;

import java.io.Serializable;

public class Camera implements Serializable {
  private static final long serialVersionUID = 1L;

  private int id = -1;
  private String name = null;
  private Session session = null;

  public Camera() {
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

  public Session getSession() {
    return this.session;
  }
  public void setSession(Session session) {
    this.session = session;
  }
}