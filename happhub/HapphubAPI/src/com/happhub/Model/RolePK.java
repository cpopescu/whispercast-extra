package com.happhub.Model;

import java.io.Serializable;

public class RolePK implements Serializable {
  private static final long serialVersionUID = 1L;

  private int usersId = -1;
  private int accountsId = -1;

  public RolePK() {
  }
  
  public int getUsersId() {
    return this.usersId;
  }
  public void setUsersId(int usersId) {
    this.usersId = usersId;
  }
  
  public int getAccountsId() {
    return this.accountsId;
  }
  public void setAccountsId(int accountsId) {
    this.accountsId = accountsId;
  }

  public boolean equals(Object other) {
    if (this == other) {
      return true;
    }
    if (!(other instanceof RolePK)) {
      return false;
    }
    RolePK castOther = (RolePK)other;
    return 
      (this.usersId == castOther.usersId)
      && (this.accountsId == castOther.accountsId);
  }

  public int hashCode() {
    final int prime = 31;
    int hash = 17;
    hash = hash * prime + this.usersId;
    hash = hash * prime + this.accountsId;
    
    return hash;
  }
}