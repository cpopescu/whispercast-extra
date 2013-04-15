package com.happhub;

import javax.ws.rs.ext.ContextResolver;
import javax.xml.bind.JAXBContext;

import com.happhub.Model.Account;
import com.happhub.Model.Session;
import com.happhub.Model.User;

public class RootContext {
  final private ContextResolver<JAXBContext> mContextResolver;
  public JAXBContext getContext() {
    return mContextResolver.getContext(null);
  }
  
  final private User mUser;
  public User getUser() {
    return mUser;
  }
  final private Account mAccount;
  public Account getAccount() {
    return mAccount;
  }
  final private Session mSession;
  public Session getSession() {
    return mSession;
  }
  
  final private Object mModel;
  public Object getModel() {
    return mModel;
  }
  
  public RootContext(ContextResolver<JAXBContext> contextResolver, User user, Account account, Session session, Object model) {
    mContextResolver = contextResolver;
    
    mUser = user;
    mAccount = account;
    mSession = session;
    mModel = model;
  }
}