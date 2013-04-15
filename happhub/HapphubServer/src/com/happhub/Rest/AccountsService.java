package com.happhub.Rest;

import java.util.List;

import javax.persistence.EntityManager;

import com.happhub.Model.Account;
import com.happhub.Model.User;

public class AccountsService extends Service<Account> {
  @Override
  public List<Account> retrieveAll(String search, int start, int limit) {
    return mEm.createQuery(
      "SELECT a FROM Account AS a JOIN a.roles AS r WHERE r.user.id = :users_id ORDER BY a.id", Account.class).
      setParameter("users_id", mUser.getId()).
      getResultList();
  }
  
  @Override
  public Account retrieve(int id) {
    List<Account> accounts = mEm.createQuery(
      "SELECT a FROM Account AS a JOIN a.roles AS r WHERE a.id = :accounts_id AND r.user.id = :users_id", Account.class).
      setParameter("accounts_id", id).
      setParameter("users_id", mUser.getId()).
      getResultList();
    
    if (accounts.isEmpty()) {
      return null;
    }
    return accounts.get(0);
  }
  
  public AccountsService(User user, EntityManager em) {
    super(user, em, Account.class);
  }
}
