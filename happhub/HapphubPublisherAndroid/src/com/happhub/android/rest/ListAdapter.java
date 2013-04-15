package com.happhub.android.rest;

import java.util.List;

import android.content.Context;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ArrayAdapter;

abstract
public class ListAdapter<T> extends ArrayAdapter<T> {
  protected int mResourceId = 0;
  
  public ListAdapter(Context context, int resourceId, T[] values) {
    super(context, resourceId, values);
    mResourceId = resourceId;
  }
  public ListAdapter(Context context, int resourceId, List<T> values) {
    super(context, resourceId, values);
    mResourceId = resourceId;
  }
  
  abstract
  protected void toView(int position, T t, View view);

  @Override
  public View getView(int position, View convertView, ViewGroup parent) {
    View view = convertView;
    if (view == null) {
      LayoutInflater viewInflater = (LayoutInflater)getContext().getSystemService(Context.LAYOUT_INFLATER_SERVICE);
      view = viewInflater.inflate(mResourceId, parent, false);
    }
    
    toView(position, this.getItem(position), view);
    return view;
  }
}
