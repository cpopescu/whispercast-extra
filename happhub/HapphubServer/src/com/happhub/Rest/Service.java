package com.happhub.Rest;

import java.util.List;

import javax.persistence.EntityManager;

import com.happhub.Model.User;

abstract public class Service<T> {
  protected Class<T> mType;
  
  protected EntityManager mEm;
  protected User mUser;
  
  public Service(User user, EntityManager em, Class<T> type) {
    mEm = em;
    mUser = user;
    mType = type;
  }
  
  abstract public List<T> retrieveAll(String search, int start, int limit);
  
  public T create(T entity) {
    mEm.persist(entity);
    return entity;
  }
  public T retrieve(int id) {
    return mEm.find(mType, id);
  }
  public T update(T entity) {
    mEm.persist(entity);
    return entity;
  }
  public void delete(T entity) {
    mEm.remove(entity);
  }
}
 