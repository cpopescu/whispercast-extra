package com.happhub.Model;

import java.io.Serializable;
import java.util.List;

import com.happhub.Model.Camera;
import com.happhub.Model.Server;

public class Session implements Serializable {
  private static final long serialVersionUID = 1L;

  private int id = -1;
  private String name = null;
  private Account account = null;
  private List<Camera> cameras = null;
  private Server server = null;
  
  @Override
  public boolean equals(Object o) {
    return (o instanceof Session) && (((Session)o).getId() == getId());
  }

  public Session() {
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

  public Account getAccount() {
    return this.account;
  }
  public void setAccount(Account account) {
    this.account = account;
  }
  
  public List<Camera> getCameras() {
    return this.cameras;
  }
  public void setCameras(List<Camera> cameras) {
    this.cameras = cameras;
  }

  public Server getServer() {
    return this.server;
  }
  public void setServer(Server server) {
    this.server = server;
  }
}