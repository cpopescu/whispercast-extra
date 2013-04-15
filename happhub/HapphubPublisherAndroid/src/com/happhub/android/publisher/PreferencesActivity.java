package com.happhub.android.publisher;

import android.hardware.Camera;
import android.os.Bundle;
import android.preference.ListPreference;
import android.preference.PreferenceActivity;
import android.preference.PreferenceFragment;
import android.preference.PreferenceManager;

public class PreferencesActivity extends PreferenceActivity {
  @Override
  protected void onCreate(Bundle savedInstanceState) {
    super.onCreate(savedInstanceState);
    
    getFragmentManager().beginTransaction().replace(android.R.id.content, new PreferencesFragment()).commit();
  }

  public static class PreferencesFragment extends PreferenceFragment {
    @Override
    public void onCreate(Bundle savedInstanceState) {
      super.onCreate(savedInstanceState);

      PreferenceManager.setDefaultValues(getActivity(), R.xml.preferences_publisher, false);
      addPreferencesFromResource(R.xml.preferences_publisher);
      
      ListPreference cameras = (ListPreference)findPreference("publisher_camera");
      
      int count = Camera.getNumberOfCameras();
      if (count > 0) {
        Camera.CameraInfo info = new Camera.CameraInfo();
        
        String keys[] = new String[count];
        String values[] = new String[count];
        
        for (int i = 0; i < count; ++i) {
          Camera.getCameraInfo(i, info);
          
          keys[i] = Integer.valueOf(i).toString();
          values[i] = (info.facing == Camera.CameraInfo.CAMERA_FACING_BACK) ? getString(R.string.camera_back) : getString(R.string.camera_front); 
        }
        
        cameras.setEntryValues(keys);
        cameras.setEntries(values);
      } else {
        getPreferenceScreen().removePreference(cameras);
      }
    }
  }
}