package com.happhub.android.rest;

import android.view.View;
import android.widget.AbsListView;
import android.widget.AdapterView;

abstract
public class ListAdapterFragment<T, TAdapter extends ListAdapter<T>, TView extends AbsListView> extends Fragment<T[]> {
  protected TView mView = null;
  protected TAdapter mAdapter = null;
  
  protected int mListViewId = -1;
  protected int mEmptyViewId = -1;
  
  @SuppressWarnings("unchecked")
  protected void updateAdapter(T[] values) {
    if (values == null) {
      mAdapter = null;
    } else {
      mAdapter = createAdapter(values);
    }
    
    if (mView != null) {
      ((AdapterView<TAdapter>)mView).setAdapter(mAdapter);
    }
  }
  
  abstract
  protected TAdapter createAdapter(T[] values);
  
  public void create(Class<?> service, int menuId, int refreshMenuItemId, int listViewId, int emptyViewId) {
    super.create(service, menuId, refreshMenuItemId);
    
    mListViewId = listViewId;
    mEmptyViewId = emptyViewId;
  }
  
  @SuppressWarnings("unchecked")
  @Override
  public void onStart() {
    super.onStart();
    
    if (mListViewId > 0) {
      mView = (TView)getView().findViewById(mListViewId);
    }
    
    T[] values = null;
    
    Processor<T[]> restProcessor = getRestProcessor();
    if (restProcessor != null) {
      Result<T[]> data = restProcessor.getData();
      if (data != null) {
        values = data.getValue();
      }
    }
    
    updateAdapter(values);
  }
  
  @Override
  public void onDataChanged(Result<T[]> data, boolean reportErrors) {
    updateAdapter((data != null) ? data.getValue() : null);
    super.onDataChanged(data, reportErrors);
  }

  @Override
  public void showProgress(boolean show) {
    View root = getView();
    if (root != null) {
      View emptyView = null;
      if (!show && (mEmptyViewId > 0)) {
        emptyView = getView().findViewById(mEmptyViewId);
      }
      if (mView != null) {
        mView.setEmptyView(emptyView);
      }
    }
    super.showProgress(show);
  }
}