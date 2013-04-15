package com.happhub.android.publisher;

import com.happhub.Model.Session;
import com.happhub.android.RefreshHandler;
import com.happhub.android.rest.ListAdapterFragment;

import android.app.Activity;
import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.AdapterView;
import android.widget.GridView;

public class SessionsFragment extends ListAdapterFragment<Session, SessionsAdapter, GridView> implements RefreshHandler  {
  static
  private class Processor extends ServiceGetProcessor<Session[]> {
    public int mAccountId = -1;
    
    public void setup(int accountId) {
      if (accountId != mAccountId) {
        setData(null, false);
        mIsDirty = true;
      } else
      if (mData == null) {
        mIsDirty = true;
      }
      
      mAccountId = accountId;
      
      if (mAccountId != -1) {
        super.setup("/accounts/"+mAccountId+"/sessions", Session[].class, null);
      }
    }
  };
  
  public SessionsFragment() {
    create(Service.class, R.menu.sessions_fragment, R.id.menu_refresh, R.id.sessions_fragment_sessions, R.id.sessions_fragment_sessions_empty);
  }
  
  public int getAccountId() {
    return (mRestProcessor != null) ? ((Processor)mRestProcessor).mAccountId : -1;
  }
  
  public void setParameters(int accountId, boolean reload) {
    if (mRestProcessor == null) {
      mRestProcessor = new Processor();
    }
    
    ((Processor)mRestProcessor).setup(accountId);
    
    reload = reload || (mRestProcessor.getData() == null);
    if (reload) {
      mRestProcessor.execute();
    }
  }
  
  @Override
  public void onCreate(Bundle savedInstanceState) {
    if (mRestProcessor == null) {
      int accountId = 0;
      if (savedInstanceState != null) {
        accountId = savedInstanceState.getInt("account_id");
        accountId = (accountId == 0) ? -1 : accountId;
      }
      
      mRestProcessor = new Processor();
      ((Processor)mRestProcessor).setup(accountId);
    }
    super.onCreate(savedInstanceState);
    
  }
  
  @Override
  public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
    View root = inflater.inflate(R.layout.sessions_fragment, container, false);
    if (getActivity() instanceof AdapterView.OnItemClickListener) {
      ((GridView)root.findViewById(R.id.sessions_fragment_sessions)).setOnItemClickListener((AdapterView.OnItemClickListener)getActivity()); 
    }
    return root;
  }
  
  @Override
  public void onSaveInstanceState(Bundle outState) {
    super.onSaveInstanceState(outState);

    if (mRestProcessor != null) {
      outState.putInt("account_id", ((Processor)mRestProcessor).mAccountId);
    }
  }
  
  @Override
  protected SessionsAdapter createAdapter(Session[] values) {
    Activity activity = getActivity();
    if (activity != null) {
      return new SessionsAdapter(activity, values);
    }
    return null;
  }
  
  @Override
  public void refresh() {
    if (mRestProcessor != null) {
      mRestProcessor.execute();
    }
  }
}