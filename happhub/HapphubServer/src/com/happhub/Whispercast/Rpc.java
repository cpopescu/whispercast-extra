package com.happhub.Whispercast;

import java.net.URI;
import java.util.ArrayList;

import javax.ws.rs.WebApplicationException;
import javax.ws.rs.core.MediaType;
import javax.ws.rs.core.Response;
import javax.ws.rs.core.UriBuilder;

import com.happhub.Model.Account;
import com.happhub.Model.Camera;
import com.happhub.Model.Server;
import com.happhub.Model.Session;
import com.happhub.Model.User;
import com.sun.jersey.api.client.Client;
import com.sun.jersey.api.client.WebResource;
import com.sun.jersey.api.client.config.ClientConfig;
import com.sun.jersey.api.client.config.DefaultClientConfig;
import com.sun.jersey.api.client.filter.HTTPBasicAuthFilter;
import com.sun.jersey.api.client.filter.LoggingFilter;
import com.sun.jersey.api.json.JSONConfiguration;

public class Rpc {
  public final static int MSG_CALL = 0;
  public final static int MSG_REPLY = 1;
  
  public final static int RESULT_SUCCESS = 0;
  public final static int RESULT_SERVICE_UNAVAILABLE = 1;
  public final static int RESULT_METHOD_UNAVAILABLE = 2;
  public final static int RESULT_GARBAGE_ARGUMENTS = 3;
  public final static int RESULT_RPC_SYSTEM_ERROR = 4;
  public final static int RESULT_RPC_SERVER_BUSY = 5;
  
  final private Server mServer;
  public Rpc(Server server) {
    mServer = server;
  }
  
  private URI getURI() {
    return UriBuilder.fromUri("http://localhost:8180/rpc-config").build();
  }
  
  public void createAccount(Account account) {
    ClientConfig config = new DefaultClientConfig();
    config.getFeatures().put(JSONConfiguration.FEATURE_POJO_MAPPING, Boolean.TRUE);
    config.getClasses().add(RpcRequest.class);
    config.getClasses().add(RpcResponse.class);
    
    Client client = Client.create(config);
    client.addFilter(new HTTPBasicAuthFilter("admin", "edatnsoscdm-=33"));
    client.addFilter(new LoggingFilter(System.out));
    
    WebResource service = client.resource(getURI());
    
    ArrayList<Object> params = new ArrayList<Object>();
    params.add(String.valueOf(account.getId()));
    
    RpcRequest req = new RpcRequest(1, "HappHub", "AddUser", params);
    
    RpcResponse rpcResponse = service.type(MediaType.APPLICATION_JSON).accept(MediaType.APPLICATION_JSON).post(RpcResponse.class, req);
    if ((rpcResponse.rbody.result != null) && !rpcResponse.rbody.result.success_) {
      // TODO: Customize
      throw new WebApplicationException(Response.Status.INTERNAL_SERVER_ERROR);
    }
  }
  public void deleteAccount(Account account) {
    ClientConfig config = new DefaultClientConfig();
    config.getFeatures().put(JSONConfiguration.FEATURE_POJO_MAPPING, Boolean.TRUE);
    config.getClasses().add(RpcRequest.class);
    config.getClasses().add(RpcResponse.class);
    
    Client client = Client.create(config);
    client.addFilter(new HTTPBasicAuthFilter("admin", "edatnsoscdm-=33"));
    client.addFilter(new LoggingFilter(System.out));
    
    WebResource service = client.resource(getURI());
    
    ArrayList<Object> params = new ArrayList<Object>();
    params.add(String.valueOf(account.getId()));
    
    RpcRequest req = new RpcRequest(1, "HappHub", "DelUser", params);
    
    RpcResponse rpcResponse = service.type(MediaType.APPLICATION_JSON).accept(MediaType.APPLICATION_JSON).post(RpcResponse.class, req);
    if ((rpcResponse.rbody.result != null) && !rpcResponse.rbody.result.success_) {
      // TODO: Customize
      throw new WebApplicationException(Response.Status.INTERNAL_SERVER_ERROR);
    }
  }
  
  public void createSession(Session session) {
    ClientConfig config = new DefaultClientConfig();
    config.getFeatures().put(JSONConfiguration.FEATURE_POJO_MAPPING, Boolean.TRUE);
    config.getClasses().add(RpcRequest.class);
    config.getClasses().add(RpcResponse.class);
    
    Client client = Client.create(config);
    client.addFilter(new HTTPBasicAuthFilter("admin", "edatnsoscdm-=33"));
    client.addFilter(new LoggingFilter(System.out));
    
    WebResource service = client.resource(getURI());
    
    ArrayList<Object> params = new ArrayList<Object>();
    params.add(String.valueOf(session.getAccount().getId()));
    params.add(String.valueOf(session.getId()));
    params.add(Boolean.valueOf(true));
    
    RpcRequest req = new RpcRequest(1, "HappHub", "AddEvent", params);
    
    RpcResponse rpcResponse = service.type(MediaType.APPLICATION_JSON).accept(MediaType.APPLICATION_JSON).post(RpcResponse.class, req);
    if ((rpcResponse.rbody.result != null) && !rpcResponse.rbody.result.success_) {
      // TODO: Customize
      throw new WebApplicationException(Response.Status.INTERNAL_SERVER_ERROR);
    }
  }
  public void deleteSession(Session session) {
    ClientConfig config = new DefaultClientConfig();
    config.getFeatures().put(JSONConfiguration.FEATURE_POJO_MAPPING, Boolean.TRUE);
    config.getClasses().add(RpcRequest.class);
    config.getClasses().add(RpcResponse.class);
    
    Client client = Client.create(config);
    client.addFilter(new HTTPBasicAuthFilter("admin", "edatnsoscdm-=33"));
    client.addFilter(new LoggingFilter(System.out));
    
    WebResource service = client.resource(getURI());
    
    ArrayList<Object> params = new ArrayList<Object>();
    params.add(String.valueOf(session.getAccount().getId()));
    params.add(String.valueOf(session.getId()));
    
    RpcRequest req = new RpcRequest(1, "HappHub", "DelEvent", params);
    
    RpcResponse rpcResponse = service.type(MediaType.APPLICATION_JSON).accept(MediaType.APPLICATION_JSON).post(RpcResponse.class, req);
    if ((rpcResponse.rbody.result != null) && !rpcResponse.rbody.result.success_) {
      // TODO: Customize
      throw new WebApplicationException(Response.Status.INTERNAL_SERVER_ERROR);
    }
  }
  
  public void createCamera(Camera camera) {
  }
  public void deleteCamera(Camera camera) {
  }
}
