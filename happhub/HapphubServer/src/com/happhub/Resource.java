package com.happhub;

import java.util.List;

import javax.ejb.Stateless;
import javax.persistence.EntityManager;
import javax.persistence.PersistenceContext;
import javax.servlet.http.HttpServletRequest;
import javax.ws.rs.core.Context;

import com.happhub.Model.User;

@Stateless
public class Resource {
  @PersistenceContext(unitName="Happhub")
  protected EntityManager mEm;
  
  @Context
  protected HttpServletRequest mRequest;

  protected User getUser() {
    User user = null;
    if (mRequest.getUserPrincipal() != null) {
      List<User> users = mEm.createQuery(
        "SELECT u FROM User u WHERE u.email = :email", User.class).
        setParameter("email", mRequest.getUserPrincipal().getName()).
        getResultList();
      
      if (users.size() > 0) {
        user = users.get(0);  
      }
    }
    return user;
  }
}
