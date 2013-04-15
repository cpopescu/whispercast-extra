package com.happhub.Model;

import java.io.Serializable;
import java.util.List;

public class Server implements Serializable {
  private static final long serialVersionUID = 1L;

  private int id = -1;
  private String host = null;
  private String name = null;
  private String path = null;
  private int port = -1;
  private String protocol = null;
  private List<Session> sessions = null;

  public Server() {
  }

  public int getId() {
    return this.id;
  }
  public void setId(int id) {
    this.id = id;
  }

  public String getHost() {
    return this.host;
  }
  public void setHost(String host) {
    this.host = host;
  }

  public String getName() {
    return this.name;
  }
  public void setName(String name) {
    this.name = name;
  }

  public String getPath() {
    return this.path;
  }
  public void setPath(String path) {
    this.path = path;
  }

  public int getPort() {
    return this.port;
  }
  public void setPort(int port) {
    this.port = port;
  }

  public String getProtocol() {
    return this.protocol;
  }
  public void setProtocol(String protocol) {
    this.protocol = protocol;
  }
  
  public List<Session> getSessions() {
    return this.sessions;
  }
  public void setSessions(List<Session> sessions) {
    this.sessions = sessions;
  }
}