package com.happhub.Rest;

import java.util.ArrayList;
import java.util.HashSet;
import java.util.List;
import java.util.Set;

import javax.ejb.Stateless;
import javax.ws.rs.*;
import javax.ws.rs.core.*;

import com.happhub.Resource;
import com.happhub.Api.Sessions.CameraParams;
import com.happhub.Api.Sessions.CreateParams;
import com.happhub.Api.Sessions.UpdateParams;
import com.happhub.Model.Account;
import com.happhub.Model.Camera;
import com.happhub.Model.Server;
import com.happhub.Model.Session;
//import com.happhub.Whispercast.Interface;

@Stateless
@Path("api/accounts/{accountsId}/sessions")
public class SessionsResource extends Resource {
  protected Account getAccount(int accountsId) {
    Account account = (new AccountsService(getUser(), mEm)).retrieve(accountsId);
    if (account == null) {
      throw new WebApplicationException(Response.Status.NOT_FOUND);
    }
    return account;
  }
  
  protected SessionsService getService(int accountsId) {
    return new SessionsService(getUser(), mEm, getAccount(accountsId).getId());
  }
  
  @GET
  @Produces ({MediaType.APPLICATION_XML,MediaType.APPLICATION_JSON})
  public List<Session> getAll(@PathParam("accountsId") int accountsId, @QueryParam("search") String search, @QueryParam("start") int start, @QueryParam("limit") int limit) {
    return getService(accountsId).retrieveAll(search, start, limit);
  }
  
  @POST
  @Consumes ({MediaType.APPLICATION_XML,MediaType.APPLICATION_JSON})
  @Produces ({MediaType.APPLICATION_XML,MediaType.APPLICATION_JSON})
  public Session create(CreateParams params, @PathParam("accountsId") int accountsId) {
    Account account = getAccount(accountsId);
    
    Session session = new Session();
    session.setName(params.name);
    session.setAccount(account);
    
    ArrayList<Camera> cameras = new ArrayList<Camera>();
    if (params.cameras != null && !params.cameras.isEmpty()) {
      for (CameraParams icamera : params.cameras) {
        Camera camera = new Camera();
        camera.setName(icamera.name);
        camera.setSession(session);
        
        cameras.add(camera);
      }
    } else {
      Camera camera = new Camera();
      camera.setName("Default");
      camera.setSession(session);
      
      cameras.add(camera);
    }
    session.setCameras(cameras);
    
    // TODO: Mapping of sessions to servers..
    Server server = mEm.find(Server.class, 1);
    session.setServer(server);

    Session created = getService(accountsId).create(session);
    mEm.flush();
    
    com.happhub.Whispercast.Rpc rpc = new com.happhub.Whispercast.Rpc(created.getServer());
    rpc.createSession(created);
      
    return created;
  }
  
  @GET
  @Path("{sessionsId}")
  @Produces ({MediaType.APPLICATION_XML,MediaType.APPLICATION_JSON})
  public Session retrieve(@PathParam("accountsId") int accountsId, @PathParam("sessionsId") int sessionsId) {
    Session session = getService(accountsId).retrieve(sessionsId); 
    if (session == null) {
      throw new WebApplicationException(Response.Status.NOT_FOUND);
    }
    return session;
  }
  
  @PUT
  @Consumes ({MediaType.APPLICATION_XML,MediaType.APPLICATION_JSON})
  @Produces ({MediaType.APPLICATION_XML,MediaType.APPLICATION_JSON})
  @Path("{sessionsId}")
  public Session update(UpdateParams params, @PathParam("accountsId") int accountsId, @PathParam("sessionsId") int sessionsId) {
    SessionsService service = getService(accountsId);
    
    Session session = service.retrieve(sessionsId); 
    if (session == null) {
      throw new WebApplicationException(Response.Status.NOT_FOUND);
    }
    
    List<Camera> cameras = session.getCameras();
    List<Camera> updated = new ArrayList<Camera>();
    
    Set<Integer> touched = new HashSet<Integer>();
    touched.add(0);
    
    for (CameraParams icamera : params.cameras) {
      Camera camera = null;
      if (icamera.id > 0) {
        for (Camera scamera : cameras) {
          if (scamera.getId() == icamera.id) {
            camera = scamera;
            break;
          }
        }
        
        if (camera == null) {
          throw new WebApplicationException(Response.Status.NOT_FOUND);
        }
        camera.setName(icamera.name);
      } else {
        camera = new Camera();
        camera.setName(icamera.name);
        camera.setSession(session);
      }
      updated.add(camera);
    }
    
    session.setName(params.name);
    session.setCameras(updated);
    
    return service.update(session);
  }
  
  @DELETE
  @Path("{sessionsId}")
  @Produces ({MediaType.APPLICATION_XML,MediaType.APPLICATION_JSON})
  public void delete(@PathParam("accountsId") int accountsId, @PathParam("sessionsId") int sessionsId) {
    SessionsService model = getService(accountsId);
    
    Session session = model.retrieve(sessionsId); 
    if (session == null) {
      throw new WebApplicationException(Response.Status.NOT_FOUND);
    }
    
    com.happhub.Whispercast.Rpc rpc = new com.happhub.Whispercast.Rpc(session.getServer());
    rpc.deleteSession(session);
      
    model.delete(session);
  }
}
