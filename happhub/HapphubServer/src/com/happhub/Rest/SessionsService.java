package com.happhub.Rest;

import java.util.List;

import javax.persistence.EntityManager;

import com.happhub.Model.Session;
import com.happhub.Model.User;

public class SessionsService extends Service<Session> {
  protected int mAccountsId;
  
  @Override
  public List<Session> retrieveAll(String search, int start, int limit) {
    return mEm.createQuery(
      "SELECT s FROM Session AS s WHERE s.account.id = :accounts_id ORDER BY s.id", Session.class).
      setParameter("accounts_id", mAccountsId).
      setFirstResult(start).
      setMaxResults(limit).
      getResultList();
  }
  
  @Override
  public Session retrieve(int id) {
    List<Session> sessions = mEm.createQuery(
      "SELECT s FROM Session AS s WHERE s.account.id = :accounts_id AND s.id = :sessions_id", Session.class).
      setParameter("accounts_id", mAccountsId).
      setParameter("sessions_id", id).
      getResultList();
    
    if (sessions.isEmpty()) {
      return null;
    }
    return sessions.get(0);
  }
  
  public SessionsService(User user, EntityManager em, int accountsId) {
    super(user, em, Session.class);
    mAccountsId = accountsId;
  }
}
