package com.happhub.android.rest;

import com.happhub.android.FragmentStateKeeper;
import com.happhub.android.FragmentStateProvider;
import com.happhub.android.ProgressHandler;
import com.happhub.android.ProgressHandlerProxy;
import com.happhub.android.RefreshHandler;
import com.happhub.android.RefreshHandlerProxy;
import com.happhub.android.ServiceConnection;
import com.happhub.android.publisher.R;

import android.app.Activity;
import android.os.Bundle;
import android.util.Log;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.view.View;

public class Fragment<T> extends android.app.Fragment implements FragmentStateProvider, DataHandler<T>, ProgressHandler {
  protected Class<?> mServiceClass = null;
  
  protected int mMenuId = 0;
  protected int mRefreshMenuItemId = 0;
  
  protected ServiceConnection mRestConnection = null;
  protected Processor<T> mRestProcessor = null;
  
  protected boolean mOwningErrorHandler = false;
  protected ErrorHandlerProxy mErrorHandler = new ErrorHandlerProxy();
  protected boolean mOwningProgressHandler = false;
  protected ProgressHandlerProxy mProgressHandler = new ProgressHandlerProxy();
  protected boolean mOwningRefreshHandler = false;
  protected RefreshHandlerProxy mRefreshHandler = new RefreshHandlerProxy();
  
  public void create(Class<?> service, int menuId, int refreshMenuItemId) {
    mServiceClass = service;
    
    mMenuId = menuId;
    mRefreshMenuItemId = refreshMenuItemId;
  }
  
  public void setErrorHandler(ErrorHandler errorHandler) {
    mErrorHandler.setErrorHandler(errorHandler);
  }
  public void setProgressHandler(ProgressHandler progressHandler) {
    mProgressHandler.setProgressHandler(progressHandler);
  }
  public void setRefreshHandler(RefreshHandler refreshHandler) {
    mRefreshHandler.setRefreshHandler(refreshHandler);
  }
  
  public Processor<T> getRestProcessor() {
    return mRestProcessor;
  }
  
  @SuppressWarnings("unchecked")
  @Override
  public void onAttach(Activity activity) {
    Log.v("HAPPHUB", "Fragment onAttach: " + this);
    super.onAttach(activity);
    
    if (activity instanceof FragmentStateKeeper) {
      mRestProcessor = (Processor<T>)((FragmentStateKeeper)activity).getFragmentState(getTag());
    }
  }
  
  @Override
  public void onCreate(Bundle savedInstanceState) {
    Log.v("HAPPHUB", "Fragment onCreate: " + this);
    super.onCreate(savedInstanceState);
    
    setHasOptionsMenu(mMenuId != 0);
    
    Activity activity = getActivity();
    if (mErrorHandler.getErrorHandler() == null) {
      if (this instanceof ErrorHandler) {
        setErrorHandler((ErrorHandler)this);
        mOwningErrorHandler = true;
      } else
      if (activity instanceof ErrorHandler) {
        setErrorHandler((ErrorHandler)activity);
        mOwningErrorHandler = true;
      }
    }
    if (mProgressHandler.getProgressHandler() == null) {
      if (this instanceof ProgressHandler) {
        setProgressHandler((ProgressHandler)this);
        mOwningErrorHandler = true;
      } else
      if (activity instanceof ProgressHandler) {
        setProgressHandler((ProgressHandler)activity);
        mOwningProgressHandler = true;
      }
    }
    if (mRefreshHandler.getRefreshHandler() == null) {
      if (this instanceof RefreshHandler) {
        setRefreshHandler((RefreshHandler)this);
        mOwningErrorHandler = true;
      } else
      if (activity instanceof RefreshHandler) {
        setRefreshHandler((RefreshHandler)activity);
        mOwningRefreshHandler = true;
      }
    }
    
    mRestProcessor.setProgressHandler(mProgressHandler);
    mRestProcessor.setDataHandler(this);
  }
  
  @Override
  public void onDestroy() {
    mRestProcessor = null;
    
    if (mOwningErrorHandler) {
      mErrorHandler.setErrorHandler(null);
      mOwningErrorHandler = false;
    }
    if (mOwningProgressHandler) {
      mProgressHandler.setProgressHandler(null);
      mOwningProgressHandler = false;
    }
    if (mOwningRefreshHandler) {
      mRefreshHandler.setRefreshHandler(null);
      mOwningRefreshHandler = false;
    }
    
    super.onDestroy();
    Log.v("HAPPHUB", "Fragment onDestroy: " + this);
  }
  
  @Override
  public void onStart() {
    Log.v("HAPPHUB", "Fragment onStart: " + this);
    super.onStart();
    
    mRestConnection = new ServiceConnection(mRestProcessor);
    mRestProcessor.setServiceConnection(mRestConnection);
    
    mRestConnection.bind(getActivity(), mServiceClass);
    mRestProcessor.updateProgress();
  }
  @Override
  public void onStop() {
    mRestConnection.unbind(getActivity());
    mRestConnection = null;
    
    mRestProcessor.setServiceConnection(null);
    
    super.onStop();
    Log.v("HAPPHUB", "Fragment onStop: " + this);
  }
  
  @Override
  public void onCreateOptionsMenu(Menu menu, MenuInflater inflater) {
    super.onCreateOptionsMenu(menu, inflater);
    inflater.inflate(mMenuId, menu);
  }
  
  @Override
  public boolean onOptionsItemSelected(MenuItem item) {
    if (item.getItemId() == mRefreshMenuItemId) {
      mRefreshHandler.refresh();
      return true;
    }
    return false;
  }

  @Override
  public Object getFragmentState() {
    Object state = null;
    if (mRestProcessor != null) {
      mRestProcessor.cleanup();
      
      state = mRestProcessor;
      mRestProcessor = null;
    }
    return state;
  }
  
  @Override
  public void onDataChanged(Result<T> data, boolean reportErrors) {
    if (data != null) {
      if (data.getStatus() != 0) {
        mErrorHandler.onError(data.getStatus(), data.getException(), reportErrors);
      }
    }
  }
  
  @Override
  public void showProgress(boolean show) {
    View root = getView();
    if (root != null) {
      View progress = root.findViewById(R.id.progress);
      if (progress != null) {
        progress.setVisibility((show && ((mRestProcessor.getData() == null) || (mRestProcessor.getData().getStatus() != 0))) ? View.VISIBLE : View.GONE);
      }
    }
    
    Activity activity = getActivity();
    if (activity != null) {
      if (activity instanceof ProgressHandler) {
        ((ProgressHandler)activity).showProgress(show);
      }
    }
  }
}
