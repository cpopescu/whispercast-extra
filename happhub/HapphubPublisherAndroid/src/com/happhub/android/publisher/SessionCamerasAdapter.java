package com.happhub.android.publisher;

import java.util.List;

import com.happhub.Model.Camera;
import com.happhub.android.rest.ListAdapter;

import android.content.Context;
import android.view.View;
import android.widget.TextView;

public class SessionCamerasAdapter extends ListAdapter<Camera> {
  public SessionCamerasAdapter(Context context) {
    super(context, R.layout.session_fragment_cameras_item, new Camera[0]);
  }
  public SessionCamerasAdapter(Context context, Camera[] list) {
    super(context, R.layout.session_fragment_cameras_item, list);
  }
  public SessionCamerasAdapter(Context context, List<Camera> list) {
    super(context, R.layout.session_fragment_cameras_item, list);
  }

  @Override
  protected void toView(int position, Camera t, View view) {
    TextView title = (TextView)view.findViewById(R.id.title);
    if (title != null) {
      title.setText(t.getName());
    }
  }
}
