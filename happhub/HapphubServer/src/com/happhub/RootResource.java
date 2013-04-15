package com.happhub;

import java.math.BigInteger;
import java.security.MessageDigest;
import java.security.SecureRandom;
import java.util.Date;
import java.util.List;

import javax.annotation.Resource;
import javax.ejb.Stateless;
import javax.mail.Message;
import javax.mail.Transport;
import javax.mail.internet.InternetAddress;
import javax.mail.internet.MimeMessage;
import javax.ws.rs.*;
import javax.ws.rs.core.*;
import javax.ws.rs.ext.ContextResolver;
import javax.xml.bind.DatatypeConverter;
import javax.xml.bind.JAXBContext;

import com.happhub.Model.Root;
import com.happhub.Model.Account;
import com.happhub.Model.Signup;
import com.happhub.Model.Role;
import com.happhub.Model.Session;
import com.happhub.Model.User;
import com.happhub.Rest.AccountsService;
import com.happhub.Rest.SessionsService;
import com.sun.jersey.api.view.Viewable;

@Stateless
@Path("/")
public class RootResource extends com.happhub.Resource {
  @Resource(name = "mail/Happhub")
  private javax.mail.Session mMailSession;
  
  @Context
  private UriInfo mUriInfo;
  @Context
  ContextResolver<JAXBContext> mContextResolver;
  
  protected Account getAccount(int accountsId) {
    Account account = null;
    
    User user = getUser();
    if (user != null) {
      account = (new AccountsService(user, mEm)).retrieve(accountsId);
      if (account == null) {
        throw new WebApplicationException(Response.Status.NOT_FOUND);
      }
    }
    return account;
  }
  protected Session getSession(int accountsId, int sessionsId) {
    Session session = null;
    
    User user = getUser();
    if (user != null) {
      session = (new SessionsService(user, mEm, accountsId)).retrieve(sessionsId);
      if (session == null) {
        throw new WebApplicationException(Response.Status.NOT_FOUND);
      }
    }
    return session;
  }
  
  /* Root */
  
  @GET
  @Produces ({MediaType.TEXT_HTML})
  public Response siteIndex() throws Exception  {
    User user = getUser();
    if (user != null) {
      List<com.happhub.Model.Role> roles = user.getRoles();
      if (roles.size() == 1) {
        return Response.temporaryRedirect(new java.net.URI("/accounts/"+roles.get(0).getAccount().getId())).build();
      }
    } else {
      user = new User();
    }
    return Response.ok(new Viewable("/index.jsp", new RootContext(mContextResolver, user, null, null, null))).build();
  }
  
  /* Accounts */
  
  @GET
  @Path("accounts/{accountsId}")
  @Produces ({MediaType.TEXT_HTML})
  public Response accountIndex(@PathParam("accountsId") int accountsId) throws Exception {
    User user = getUser();
    if (user == null) {
      return Response.temporaryRedirect(new java.net.URI("/")).build();
    }
    
    Account account = getAccount(accountsId);
    if (account == null) {
      throw new WebApplicationException(Response.Status.NOT_FOUND);
    }
    return Response.ok(new Viewable("/accounts/index.jsp", new RootContext(mContextResolver, user, account, null, null))).build();
  }
  
  /* Sessions */
  
  @GET
  @Path("accounts/{accountsId}/sessions/{sessionsId}")
  @Produces ({MediaType.TEXT_HTML})
  public Response sessionIndex(@PathParam("accountsId") int accountsId, @PathParam("sessionsId") int sessionsId) throws Exception {
    User user = getUser();
    if (user == null) {
      return Response.temporaryRedirect(new java.net.URI("/")).build();
    }
    
    Session session = getSession(accountsId, sessionsId);
    if (session == null) {
      throw new WebApplicationException(Response.Status.NOT_FOUND);
    }
    return Response.ok(new Viewable("/accounts/sessions/index.jsp", new RootContext(mContextResolver, user, session.getAccount(), session, null))).build();
  }

  /* Signup */

  @GET
  @Path("signup")
  @Produces ({MediaType.TEXT_HTML})
  public Response signupIndex() throws Exception  {
    if (getUser() != null) {
      return Response.temporaryRedirect(new java.net.URI("/")).build();
    }
    Root model = new Root();
    model.setStep(0);
    return Response.ok(new Viewable("/signup/index.jsp", new RootContext(mContextResolver, null, null, null, model))).build();
  }
  
  @POST
  @Consumes ({MediaType.APPLICATION_XML, MediaType.APPLICATION_JSON})
  @Produces ({MediaType.APPLICATION_XML, MediaType.APPLICATION_JSON})
  @Path("signup")
  public com.happhub.Api.Signup.Result signupProcess(com.happhub.Api.Signup.CreateParams params) throws Exception {
    if (params.email == null) {
      throw new WebApplicationException(Response.Status.BAD_REQUEST);
    }
    if (getUser() != null) {
      throw new WebApplicationException(Response.Status.CONFLICT);
    }
    
    if (!mEm.createQuery(
      "SELECT u FROM User u WHERE u.email = :email", User.class).
      setParameter("email", params.email).getResultList().isEmpty()) {
      throw new WebApplicationException(Response.Status.CONFLICT);
    }
    if (params.accountsId != 0) {
      if (mEm.createQuery(
          "SELECT a FROM Account a WHERE a.id = :accounts_id", Account.class).
          setParameter("accounts_id", params.accountsId).getResultList().isEmpty()) {
        throw new WebApplicationException(Response.Status.NOT_FOUND);
      }
    }
    
    Signup signup;
    
    List<Signup> signups = mEm.createQuery(
      "SELECT r FROM Signup AS r WHERE r.email = :email", Signup.class).
      setParameter("email", params.email).
      getResultList();
      
    if (signups.isEmpty()) {
      signup = new Signup();
      signup.setEmail(params.email);
      
      SecureRandom random = new SecureRandom();
      signup.setHash(new BigInteger(130, random).toString(32));
    } else {
      signup = signups.get(0);
    }
    
    signup.setAccountsId(params.accountsId);
    
    mEm.persist(signup);
    
    String body = 
    "Welcome to Happhub!\r\n\r\n" +
    "Please click on the following link to complete signup:\r\n"+
    mUriInfo.getAbsolutePath().toString()+"/confirm/"+signup.getHash();
    
    Message message = new MimeMessage(mMailSession);

    message.setRecipients(Message.RecipientType.TO, InternetAddress.parse(params.email, false));
    message.setSubject("Happhub Signup");
    message.setText(body);
    message.setHeader("X-Mailer", "Happhub");

    Date timestamp = new Date();
    message.setSentDate(timestamp);

    Transport.send(message);
    return new com.happhub.Api.Signup.Result(0, "Created", null);
  }
  
  @GET
  @Path("signup/done")
  @Produces ({MediaType.TEXT_HTML})
  public Response signupProcessDone() throws Exception  {
    if (getUser() != null) {
      return Response.temporaryRedirect(new java.net.URI("/")).build();
    }
    Root model = new Root();
    model.setStep(1);
    return Response.ok(new Viewable("/signup/index.jsp", new RootContext(mContextResolver, null, null, null, model))).build();
  }
  
  @GET
  @Path("signup/confirm/{hash}")
  @Produces ({MediaType.TEXT_HTML})
  public Response signupConfirmIndex(@PathParam("hash") String hash) throws Exception  {
    if (getUser() != null) {
      return Response.temporaryRedirect(new java.net.URI("/")).build();
    }
    
    List<Signup> signups = mEm.createQuery(
      "SELECT r FROM Signup AS r WHERE r.hash = :hash", Signup.class).
      setParameter("hash", hash).
      getResultList();
      
    if (signups.isEmpty()) {
      throw new WebApplicationException(Response.Status.NOT_FOUND);
    }
    return Response.ok(new Viewable("/signup/confirm/index.jsp", new RootContext(mContextResolver, null, null, null, signups.get(0)))).build();
  }
  
  @POST
  @Path("signup/confirm/{hash}")
  @Consumes ({MediaType.APPLICATION_XML, MediaType.APPLICATION_JSON})
  @Produces ({MediaType.APPLICATION_XML, MediaType.APPLICATION_JSON})
  public com.happhub.Api.Signup.Result signupConfirmProcess(com.happhub.Api.Signup.ConfirmParams params, @PathParam("hash") String hash) throws Exception {
    if (getUser() != null) {
      throw new WebApplicationException(Response.Status.CONFLICT);
    }
    
    List<Signup> signups = mEm.createQuery(
      "SELECT r FROM Signup AS r WHERE r.hash = :hash", Signup.class).
      setParameter("hash", hash).
      getResultList();
      
    if (signups.isEmpty()) {
      throw new WebApplicationException(Response.Status.NOT_FOUND);
    }
    
    Signup signup = signups.get(0);
    
    MessageDigest sha = MessageDigest.getInstance("SHA-256");
    byte[] hashed = sha.digest((signup.getEmail()+params.password).getBytes("UTF-8"));
    
    Role role = new Role();

    boolean newAccount = false;
    
    Account account;
    if (signup.getAccountsId() != 0) {
      List<Account> accounts = mEm.createQuery(
        "SELECT a FROM Account a WHERE a.id = :accounts_id", Account.class).
        setParameter("accounts_id", signup.getAccountsId()).getResultList();
      
      if (accounts.isEmpty()) {
        throw new WebApplicationException(Response.Status.NOT_FOUND);
      }
      account = accounts.get(0);
      
      role.setRole("attached");
    } else {
      account = new Account();
      account.setName(signup.getEmail());
      
      role.setRole("owner");
      newAccount = true;
    }
    
    mEm.persist(account);
    mEm.flush();
    
    User user = new User();
    user.setEmail(signup.getEmail());
    user.setPassword(DatatypeConverter.printHexBinary(hashed));
   
    mEm.persist(user);
    mEm.flush();
    
    com.happhub.Model.RolePK roleId = new com.happhub.Model.RolePK();
    
    account.getRoles().add(role);
    role.setAccount(account);
    roleId.setAccountsId(account.getId());
    
    user.getRoles().add(role);
    role.setUser(user);
    roleId.setUsersId(user.getId());
    
    role.setId(roleId);
    
    mEm.persist(role);
    
    mEm.remove(signup);
    
    if (newAccount) {
      com.happhub.Whispercast.Rpc rpc = new com.happhub.Whispercast.Rpc(null);
      rpc.createAccount(account);
    }
    return new com.happhub.Api.Signup.Result(0, "Confirmed", hash);
  }
  
  /* Login/Logout */

  @GET
  @Path("login")
  @Produces ({MediaType.APPLICATION_XML, MediaType.APPLICATION_JSON})
  public com.happhub.Api.Login.Result loginCheck() {
    User user = getUser();
    if (user != null) {
      return new com.happhub.Api.Login.Result(0, "Logged in", user);
    }
    return new com.happhub.Api.Login.Result(1, "Not logged in", null);
  }
  
  @POST
  @Path("login")
  @Consumes ({MediaType.APPLICATION_XML, MediaType.APPLICATION_JSON})
  @Produces ({MediaType.APPLICATION_XML, MediaType.APPLICATION_JSON})
  public com.happhub.Api.Login.Result login(com.happhub.Api.Login.LoginParams params) throws Exception {
 
    try {
      User user = getUser();
      if (user != null) {
        MessageDigest sha = MessageDigest.getInstance("SHA-256");
        byte[] hashed = sha.digest((params.email+params.password).getBytes("UTF-8"));
        
        if (user.getEmail() == params.email && user.getPassword() == DatatypeConverter.printHexBinary(hashed)) {
          return new com.happhub.Api.Login.Result(0, "Logged in", user);
        }
        mRequest.logout();
      }
      
      mRequest.login(params.email, params.email+params.password);
    } catch(javax.servlet.ServletException e) {
      return new com.happhub.Api.Login.Result(1, "Not logged in", null);
    }
    return loginCheck();
  }
  
  @POST
  @Path("logout")
  @Produces ({MediaType.APPLICATION_XML, MediaType.APPLICATION_JSON})
  public com.happhub.Api.Login.Result logout() throws Exception {
    mRequest.logout();
    return new com.happhub.Api.Login.Result(0, "Logged out", null);
  }
}