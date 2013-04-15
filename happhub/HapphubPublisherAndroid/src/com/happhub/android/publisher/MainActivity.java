package com.happhub.android.publisher;

import java.util.HashMap;

import org.apache.http.HttpStatus;

import com.happhub.Model.Camera;
import com.happhub.Model.Session;
import com.happhub.Model.User;
import com.happhub.android.Activity;
import com.happhub.android.FragmentStateKeeper;
import com.happhub.android.FragmentStateProvider;
import com.happhub.android.ProgressHandlerOnActionBar;
import com.happhub.android.ProgressHandler;
import com.happhub.android.rest.ErrorHandler;

import android.content.Intent;
import android.os.Bundle;
import android.app.Fragment;
import android.app.FragmentManager;
import android.app.FragmentTransaction;
import android.util.Log;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.widget.AdapterView;

public class MainActivity extends Activity implements FragmentManager.OnBackStackChangedListener, FragmentStateKeeper, LoginFragment.LoginListener, AdapterView.OnItemClickListener, ErrorHandler, ProgressHandler {
  private enum State {
    LOGIN,
    SESSIONS,
    SESSION
  };
  private State mState = State.LOGIN;
  private boolean mStopped = true;
  
  private HashMap<String, Fragment> mFragments = new HashMap<String, Fragment>();
  private HashMap<String, Object> mFragmentsState = null;
  
  private ProgressHandlerOnActionBar mProgressHandler = new ProgressHandlerOnActionBar();
  
  @SuppressWarnings("unchecked")
  private <T> T getFragment(State state) {
    Fragment f = mFragments.get(state.name());
    if (f == null) {
      f = getFragmentManager().findFragmentByTag(state.name());
      mFragments.put(state.name(), f);
    }
    return (T)f;
  }
  @SuppressWarnings("unchecked")
  private <T> T getFragmentCreateIfNeeded(State state) {
    Fragment f = mFragments.get(state.name());
    if (f == null) {
      f = getFragmentManager().findFragmentByTag(state.name());
      if (f == null) {
        Log.v("HAPPHUB", "Creating " + state + " fragment");
        
        switch (state) {
          case LOGIN:
            f = new LoginFragment();
            break;
          case SESSIONS:
            f = new SessionsFragment();
            break;
          case SESSION:
            f = new SessionFragment();
            break;
        }
      }
      
      mFragments.put(state.name(), f);
    }
    return (T)f;
  }
  
  private void switchTo(State state, boolean force) {
    if (mStopped) {
      Log.v("HAPPHUB", "Not switching " +mState+"->"+state + " (force: " + force+"), as the activity is stopped");
      return;
    }
    if (force || (state.compareTo(mState) != 0)) {
      Log.v("HAPPHUB", "Switching " +mState+"->"+state + " (force: " + force+")");
      
      FragmentManager manager = getFragmentManager();
      FragmentTransaction transaction = manager.beginTransaction();
      
      Fragment f = getFragmentCreateIfNeeded(state);
      transaction.replace(R.id.main_activity, f, state.name());
      
      if (state.compareTo(mState) != 0) {
        if (mState.compareTo(State.LOGIN) != 0) {
          Log.v("HAPPHUB", "Adding " + mState + " to the back stack while switching to " + state);
          transaction.addToBackStack(null);
        }
        mState = state;
      }
      
      transaction.commit();
      manager.executePendingTransactions();
    } else {
      Log.v("HAPPHUB", "Not switching " +mState+"->"+state + " (force: " + force+"), as we are already there");
    }
  }
  
  @SuppressWarnings({ "unchecked", "deprecation" })
  @Override
  protected void onCreate(Bundle savedInstanceState) {
    mFragmentsState = (HashMap<String, Object>)getLastNonConfigurationInstance();
    Log.v("HAPPHUB", "Retrieved non-configuration state " + mFragmentsState);
    
    String savedState = (savedInstanceState != null) ? savedInstanceState.getString("state") : null;
    if (savedState == null) {
      mState = State.LOGIN;
    } else {
      mState = State.valueOf(savedState);
    }
    
    super.onCreate(savedInstanceState);
    Log.v("HAPPHUB", "onCreate");
    
    setContentView(R.layout.main_activity);
    
    getFragmentManager().addOnBackStackChangedListener(this);

  }
  @Override
  protected void onDestroy() {
    Log.v("HAPPHUB", "onDestroy");
    
    mProgressHandler.setMenu(null);
    mProgressHandler.setActivity(null, R.id.menu_refresh, R.layout.progress_actionbar);
    
    getFragmentManager().removeOnBackStackChangedListener(this);
    super.onDestroy();
  }
  
  @Override
  protected void onStart() {
    super.onStart();
    Log.v("HAPPHUB", "onStart");
    
    mStopped = false;
    
    switchTo(mState, true);
  }
  
  @Override
  protected void onStop() {
    Log.v("HAPPHUB", "onStop");
    
    mStopped = true;
    super.onStop();
  }
  
  @Override
  public boolean onCreateOptionsMenu(Menu menu) {
    super.onCreateOptionsMenu(menu);
    getMenuInflater().inflate(R.menu.common, menu);

    mProgressHandler.setActivity(this, R.id.menu_refresh, R.layout.progress_actionbar);
    mProgressHandler.setMenu(menu);
    return true;
  }
  
  @Override
  public boolean onOptionsItemSelected(MenuItem item) {
    super.onOptionsItemSelected(item);
    
    Fragment f = getFragment(mState);
    if (f != null) {
      if (f.onOptionsItemSelected(item)) {
        return true;
      }
    }
    
    switch (item.getItemId()) {
      case R.id.menu_preferences:
        startActivity(new Intent(this, PreferencesActivity.class));
        return true;
    }
    return false;
  }

  @Override
  public Object onRetainNonConfigurationInstance(){
    HashMap<String, Object> config = new HashMap<String, Object>();
    
    for (State state : State.values()) {
      Fragment f = getFragment(state);
      if (f != null) {
        if (f instanceof FragmentStateProvider) {
          config.put(state.name(), ((FragmentStateProvider)f).getFragmentState());
        }
      }
    }
    Log.v("HAPPHUB", "Saving non-configuration state " + config);
    return config;
  }
  
  @Override
  protected void onSaveInstanceState(Bundle outState) {
    super.onSaveInstanceState(outState);
    outState.putString("state", mState.name());
  }
  
  @Override
  public void onBackStackChanged() {
    Fragment f = getFragmentManager().findFragmentById(R.id.main_activity);
    if (f != null) {
      mState = State.valueOf(f.getTag());
    }
  }
  
  @Override
  public Object getFragmentState(String tag) {
    return (mFragmentsState != null) ? mFragmentsState.remove(tag) : null; 
  }
  
  @Override
  public void onItemClick(AdapterView<?> arg0, View arg1, int arg2, long arg3) {
    Object item = arg0.getItemAtPosition(arg2);
    
    if (item instanceof Session) {
      Session session = (Session)item;
      SessionsFragment sessions = getFragment(State.SESSIONS);
      
      switchTo(State.SESSION, false);
      
      SessionFragment cameras = getFragmentCreateIfNeeded(State.SESSION);
      cameras.setParameters(sessions.getAccountId(), session.getId(), false);
    } else
    if (item instanceof Camera) {
      Intent intent = new Intent(this, PublishActivity.class);
      startActivity(intent);
    }
  }
  
  @Override
  public void onLogin(User user) {
    Log.v("HAPPHUB", "onLogin " + user);
    
    if (user != null) {
      int backStackCount = getFragmentManager().getBackStackEntryCount();
      if (backStackCount > 0) {
        Log.v("HAPPHUB", "onLogin:  Popping back stack with "+backStackCount);
        getFragmentManager().popBackStackImmediate();
        if (getFragmentManager().getBackStackEntryCount() < backStackCount) {
          return;
        }
        Log.v("HAPPHUB", "onLogin:  Popping back stack failed");
      }
      Log.v("HAPPHUB", "onLogin:  switchTo(State.SESSIONS, false)");
      switchTo(State.SESSIONS, false);
      
      SessionsFragment sessions = getFragmentCreateIfNeeded(State.SESSIONS); 
      sessions.setParameters(user.getRoles().get(0).getAccount().getId(), true);
      return;
    }
    Log.v("HAPPHUB", "onLogin: switchTo(State.LOGIN, false)");
    switchTo(State.LOGIN, false);
  }
  
  @Override
  public void onError(int status, Exception e, boolean reportErrors) {
    Log.v("HAPPHUB", "onError, state " + mState);
    
    if (status == HttpStatus.SC_FORBIDDEN || status == 1) {
      if (mState != State.LOGIN) {
        switchTo(State.LOGIN, true);
        return;
      }
    }
    if (reportErrors) {
      Application.restError(this, status, e);
    }
  }
  
  @Override
  public void showProgress(boolean shown) {
    mProgressHandler.showProgress(shown);
  }
}
