package com.happhub.Rest;

import java.util.List;
import java.util.Vector;

import javax.ejb.Stateless;
import javax.ws.rs.*;
import javax.ws.rs.core.*;

import com.happhub.Resource;
import com.happhub.Api.Accounts.CreateParams;
import com.happhub.Api.Accounts.UpdateParams;
import com.happhub.Model.Account;
import com.happhub.Model.Role;

@Stateless
@Path("api/accounts")
public class AccountsResource extends Resource {
  protected AccountsService getService() {
    return new AccountsService(getUser(), mEm);
  }
  
  @GET
  @Produces ({MediaType.APPLICATION_XML,MediaType.APPLICATION_JSON})
  public List<Account> retrieveAll(@QueryParam("search") String search, @QueryParam("start") int start, @QueryParam("limit") int limit) {
    return getService().retrieveAll(search, start, limit);
  }
  
  @POST
  @Consumes ({MediaType.APPLICATION_XML,MediaType.APPLICATION_JSON})
  @Produces ({MediaType.APPLICATION_XML,MediaType.APPLICATION_JSON})
  public Account create(CreateParams params) {
    Account account = new Account();
    
    Role role = new Role();
    role.setAccount(account);
    role.setUser(getUser());
    role.setRole("owner");
    
    Vector<Role> roles = new Vector<Role>();
    roles.add(role);
    
    account.setRoles(roles);
    
    Account created = getService().create(account);
    mEm.flush();
    
    com.happhub.Whispercast.Rpc rpc = new com.happhub.Whispercast.Rpc(null);
    rpc.createAccount(created);
      
    return created;
  }
  
  @PUT
  @Path("{accountsId}")
  @Produces ({MediaType.APPLICATION_XML,MediaType.APPLICATION_JSON})
  public Account update(UpdateParams params, @PathParam("id") int accountsId) {
    Account account = getService().retrieve(accountsId);
    if (account == null) {
      throw new WebApplicationException(Response.Status.NOT_FOUND);
    }
    return getService().update(account);
  }
  
  @GET
  @Path("{accountsId}")
  @Produces ({MediaType.APPLICATION_XML,MediaType.APPLICATION_JSON})
  public Account retrieve(@PathParam("accountsId") int accountsId) {
    Account account = getService().retrieve(accountsId);
    if (account == null) {
      throw new WebApplicationException(Response.Status.NOT_FOUND);
    }
    return account;
  }
  
  @DELETE
  @Path("{accountsId}")
  @Produces ({MediaType.APPLICATION_XML,MediaType.APPLICATION_JSON})
  public void delete(@PathParam("id") int accountsId) {
    AccountsService model = getService();
    Account account = model.retrieve(accountsId);
    if (account == null) {
      throw new WebApplicationException(Response.Status.NOT_FOUND);
    }
    
    com.happhub.Whispercast.Rpc rpc = new com.happhub.Whispercast.Rpc(null);
    rpc.deleteAccount(account);
      
    model.delete(account);
  }
}