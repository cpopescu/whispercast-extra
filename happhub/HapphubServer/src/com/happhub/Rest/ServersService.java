package com.happhub.Rest;

import java.util.List;

import javax.persistence.EntityManager;

import com.happhub.Model.Server;
import com.happhub.Model.User;

public class ServersService extends Service<Server> {
  @Override
  public List<Server> retrieveAll(String search, int start, int limit) {
    return mEm.createQuery(
      "SELECT s FROM Server AS s WHERE ORDER BY s.id", Server.class).
      setFirstResult(start).
      setMaxResults(limit).
      getResultList();
  }
  
  @Override
  public Server retrieve(int id) {
    List<Server> servers = mEm.createQuery(
      "SELECT s FROM Server AS s WHERE s.id = :servers_id", Server.class).
      setParameter("servers_id", id).
      getResultList();
    
    if (servers.isEmpty()) {
      return null;
    }
    return servers.get(0);
  }
  
  public ServersService(User user, EntityManager em, int accountsId) {
    super(user, em, Server.class);
  }
}
