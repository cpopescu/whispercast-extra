package com.happhub.android.publisher;

import java.util.List;

import com.happhub.Model.Session;
import com.happhub.android.rest.ListAdapter;

import android.content.Context;
import android.view.View;
import android.widget.TextView;

public class SessionsAdapter extends ListAdapter<Session> {
  public SessionsAdapter(Context context) {
    super(context, R.layout.sessions_item, new Session[0]);
  }
  public SessionsAdapter(Context context, Session[] values) {
    super(context, R.layout.sessions_item, values);
  }
  public SessionsAdapter(Context context, List<Session> values) {
    super(context, R.layout.sessions_item, values);
  }

  @Override
  protected void toView(int position, Session t, View view) {
    TextView title = (TextView)view.findViewById(R.id.title);
    if (title != null) {
      title.setText(t.getName());
    }
  }
}
