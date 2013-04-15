package com.happhub.android.publisher;

import com.happhub.Model.Session;
import com.happhub.android.RefreshHandler;
import com.happhub.android.rest.Fragment;
import com.happhub.android.rest.Result;

import android.os.Bundle;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.AdapterView;
import android.widget.ListView;
import android.widget.TextView;

public class SessionFragment extends Fragment<Session> implements RefreshHandler {
  static
  private class Processor extends ServiceGetProcessor<Session> {
    private int mAccountId = -1;
    private int mSessionId = -1;
    
    public void setup(int accountId, int sessionId) {
      if ((accountId != mAccountId) || (sessionId != mSessionId)) {
        setData(null, false);
      }
      if (mData == null) {
        mIsDirty = true;
      }
      
      mAccountId = accountId;
      mSessionId = sessionId;
      
      if ((mAccountId != -1) && (mSessionId != -1)) {
        super.setup("/accounts/"+mAccountId+"/sessions/"+mSessionId, Session.class, null);
      }
    }
  };
  
  protected View mHeaderView = null;
  
  public SessionFragment() {
    super.create(Service.class, R.menu.session_fragment, R.id.menu_refresh);
  }
  
  public int getAccountId() {
    return (mRestProcessor != null) ? ((Processor)mRestProcessor).mAccountId : -1;
  }
  public int getSessionId() {
    return (mRestProcessor != null) ? ((Processor)mRestProcessor).mSessionId : -1; 
  }
  
  public Session getSession() {
    return (mRestProcessor != null) ? ((mRestProcessor.getData() != null) ? mRestProcessor.getData().getValue() : null) : null;  
  }
  
  public void setParameters(int accountId, int sessionId, boolean reload) {
    if (mRestProcessor == null) {
      mRestProcessor = new Processor();
    }
    
    ((Processor)mRestProcessor).setup(accountId, sessionId);
    
    reload = reload || (mRestProcessor.getData() == null);
    if (reload) {
      mRestProcessor.execute();
    }
    
    updateView(getView());
  }
  
  protected boolean updateView(View root) {
    if (root != null) {
      Session session = getSession(); 
      if (session != null) {
        TextView title = null;
        
        ViewGroup group = (ViewGroup)root.findViewById(R.id.session_fragment);
        ListView cameras = (ListView)root.findViewById(R.id.session_fragment_cameras);
        
        if (mHeaderView != null) {
          cameras.removeHeaderView(mHeaderView);
          group.removeView(mHeaderView);
          
          if (session.getCameras().size() > 0) {
            cameras.addHeaderView(mHeaderView);
          } else {
            group.addView(mHeaderView, 0);
          }
          title = (TextView)mHeaderView.findViewById(R.id.title);
        } else {
          title = (TextView)root.findViewById(R.id.title);
        }

        if (title != null) {
          title.setText(session.getName());
        }
        
        cameras.setAdapter(new SessionCamerasAdapter(getActivity(), session.getCameras()));
        return true;
      }
    }
    return false;
  }
  
  @Override
  public void onCreate(Bundle savedInstanceState) {
    if (mRestProcessor == null) {
      int accountId = -1;
      int sessionId = -1;
      
      if (savedInstanceState != null) {
        accountId = savedInstanceState.getInt("account_id");
        accountId = (accountId == 0) ? -1 : accountId; 
        sessionId = savedInstanceState.getInt("session_id");
        sessionId = (sessionId == 0) ? -1 : sessionId; 
      }
      Log.v("HAPPHUB", "Camera fragment created with " + accountId+","+sessionId);
      
      mRestProcessor = new Processor();
      ((Processor)mRestProcessor).setup(accountId, sessionId);
    }
    super.onCreate(savedInstanceState);
  }
  
  @Override
  public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
    View root = inflater.inflate(R.layout.session_fragment, container, false);
    if (root.findViewById(R.id.header) == null) {
      mHeaderView = inflater.inflate(R.layout.session_fragment_cameras_header, null, false); 
    }
    
    if (getActivity() instanceof AdapterView.OnItemClickListener) {
      ((ListView)root.findViewById(R.id.session_fragment_cameras)).setOnItemClickListener((AdapterView.OnItemClickListener)getActivity()); 
    }
    
    updateView(root);
    return root;
  }
  
  @Override
  public void onSaveInstanceState(Bundle outState) {
    super.onSaveInstanceState(outState);
    
    outState.putInt("account_id", getAccountId());
    outState.putInt("session_id", getSessionId());
  }
  
  @Override
  public void onDataChanged(Result<Session> data, boolean reportErrors) {
    super.onDataChanged(data, reportErrors);
    updateView(getView());
  }
  
  @Override
  public void showProgress(boolean show) {
    View root = getView();
    if (root != null) {
      View emptyView = null;
      if (!show) {
        emptyView = getView().findViewById(android.R.id.empty);
      }
      
      ListView cameras = (ListView)root.findViewById(R.id.session_fragment_cameras);
      if (cameras != null) {
        cameras.setEmptyView(emptyView);
      }
    }
    super.showProgress(show);
  }
  
  @Override
  public void refresh() {
    if (mRestProcessor != null) {
      mRestProcessor.execute();
    }
  }
}