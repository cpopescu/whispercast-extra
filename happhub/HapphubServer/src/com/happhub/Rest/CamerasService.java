package com.happhub.Rest;

import java.util.List;

import javax.persistence.EntityManager;

import com.happhub.Model.Camera;
import com.happhub.Model.User;

public class CamerasService extends Service<Camera> {
  protected int mSessionsId;
  
  @Override
  public List<Camera> retrieveAll(String search, int start, int limit) {
    return mEm.createQuery(
      "SELECT c FROM Camera AS c WHERE c.session.id = :sessions_id ORDER BY c.id", Camera.class).
      setParameter("sessions_id", mSessionsId).
      getResultList();
  }
  
  @Override
  public Camera retrieve(int id) {
    List<Camera> cameras = mEm.createQuery(
      "SELECT c FROM Camera AS c WHERE c.session.id = :sessions_id AND c.id = :cameras_id", Camera.class).
      setParameter("sessions_id", mSessionsId).
      setParameter("cameras_id", id).
      getResultList();
    
    if (cameras.isEmpty()) {
      return null;
    }
    return cameras.get(0);
  }
  
  public CamerasService(User user, EntityManager em, int sessionsId) {
    super(user, em, Camera.class);
    mSessionsId = sessionsId;
  }
}
