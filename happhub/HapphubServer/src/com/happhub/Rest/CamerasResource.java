package com.happhub.Rest;

import java.util.List;

import javax.ejb.Stateless;
import javax.ws.rs.*;
import javax.ws.rs.core.*;

import com.happhub.Resource;
import com.happhub.Api.Cameras.CreateParams;
import com.happhub.Api.Cameras.UpdateParams;
import com.happhub.Model.Account;
import com.happhub.Model.Camera;
import com.happhub.Model.Session;

@Stateless
@Path("api/accounts/{accountsId}/sessions/{sessionsId}/cameras")
public class CamerasResource extends Resource {
  protected Account getAccount(int accountsId) {
    Account account = (new AccountsService(getUser(), mEm)).retrieve(accountsId);
    if (account == null) {
      throw new WebApplicationException(Response.Status.NOT_FOUND);
    }
    return account;
  }
  
  protected Session getSession(int accountsId, int sessionsId) {
    Session session = (new SessionsService(getUser(), mEm, accountsId)).retrieve(sessionsId);
    if (session == null) {
      throw new WebApplicationException(Response.Status.NOT_FOUND);
    }
    return session;
  }
  
  protected CamerasService getService(int accountsId, int sessionsId) {
    return new CamerasService(getUser(), mEm, getSession(accountsId, sessionsId).getId());
  }
  
  @GET
  @Produces ({MediaType.APPLICATION_XML,MediaType.APPLICATION_JSON})
  public List<Camera> getAll(@PathParam("accountsId") int accountsId, @PathParam("sessionsId") int sessionsId, @QueryParam("search") String search, @QueryParam("start") int start, @QueryParam("limit") int limit) {
    return getService(accountsId, sessionsId).retrieveAll(search, start, limit); 
  }
  
  @POST
  @Consumes ({MediaType.APPLICATION_XML,MediaType.APPLICATION_JSON})
  @Produces ({MediaType.APPLICATION_XML,MediaType.APPLICATION_JSON})
  public Camera create(CreateParams params, @PathParam("accountsId") int accountsId, @PathParam("sessionsId") int sessionsId) {
    Session session = getSession(accountsId, sessionsId);
    
    Camera camera = new Camera();
    camera.setName(params.name);
    camera.setSession(session);
      
    return getService(accountsId, sessionsId).create(camera);
  }
  
  @GET
  @Path("{camerasId}")
  @Produces ({MediaType.APPLICATION_XML,MediaType.APPLICATION_JSON})
  public Camera retrieve(@PathParam("accountsId") int accountsId, @PathParam("sessionsId") int sessionsId, @PathParam("camerasId") int camerasId) {
    Camera camera = getService(accountsId, sessionsId).retrieve(camerasId); 
    if (camera == null) {
      throw new WebApplicationException(Response.Status.NOT_FOUND);
    }
    return camera;
  }
  
  @PUT
  @Consumes ({MediaType.APPLICATION_XML,MediaType.APPLICATION_JSON})
  @Produces ({MediaType.APPLICATION_XML,MediaType.APPLICATION_JSON})
  @Path("{camerasId}")
  public Camera update(UpdateParams params, @PathParam("accountsId") int accountsId, @PathParam("sessionsId") int sessionsId, @PathParam("camerasId") int camerasId) {
    CamerasService model = getService(accountsId, sessionsId);
    
    Camera camera = model.retrieve(camerasId); 
    if (camera == null) {
      throw new WebApplicationException(Response.Status.NOT_FOUND);
    }
    
    camera.setName(params.name);
    return model.update(camera);
  }
  
  @DELETE
  @Path("{sessionsId}")
  @Produces ({MediaType.APPLICATION_XML,MediaType.APPLICATION_JSON})
  public void delete(@PathParam("accountsId") int accountsId, @PathParam("sessionsId") int sessionsId, @PathParam("camerasId") int camerasId) {
    CamerasService model = getService(accountsId, sessionsId);
    
    Camera camera = model.retrieve(camerasId); 
    if (camera == null) {
      throw new WebApplicationException(Response.Status.NOT_FOUND);
    }
    
    model.delete(camera);
  }
}
