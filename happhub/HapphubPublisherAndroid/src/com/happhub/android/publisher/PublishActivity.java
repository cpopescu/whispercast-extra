package com.happhub.android.publisher;

import java.util.Timer;
import java.util.TimerTask;

import com.happhub.android.Activity;
import com.happhub.android.Application;
import com.happhub.android.ServiceConnection;
import com.happhub.android.ViewToggler;
import com.happhub.android.media.Publisher;

import android.app.AlertDialog;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.os.Bundle;
import android.preference.PreferenceManager;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.view.ViewGroup;
import android.view.WindowManager;
import android.widget.ImageButton;
import android.widget.TextView;
import android.widget.Toast;

public class PublishActivity extends Activity implements ServiceConnection.Client, Publisher.NotificationListener {
  private ServiceConnection mServiceConnection = null;
  
  private Publisher mPublisher = null;
  
  private ImageButton mPublishButton = null;
  private ImageButton mFlashlightButton = null;
  private ImageButton mFocusModeButton = null;
  
  private View mPublishingMarker = null; 
  private TextView mPublishingInfo = null; 
  
  private ViewToggler mBarToggler = null;
  private ViewToggler mButtonsToggler = null;

  private boolean mCanGoBack = false;
  private Toast mCanGoBackToast = null;
  
  private Timer mUpdateTimer = null;

  private Publisher.Options mPublisherOptions = new Publisher.Options();
  private boolean mPublisherOptionsChanged = false;
  
  private void updateInfo() {
    if (mPublishingInfo != null) {
      String info;
      
      if (mPublisher != null) {
        info = ""+mPublisher.getOutWidth()+"x"+mPublisher.getOutHeight();
        info += (mPublisher.getOptions().captureMode == Publisher.CaptureMode.HARDWARE) ? "/HW" : "/SW";
      } else {
        info = "";
      }
      
      mPublishingInfo.setText(info);
    }
  }
  private void updateUI() {
    boolean show = (mPublisher != null) && (mPublisher.getState() == Publisher.State.PUBLISHING);
    if (show) {
      mBarToggler.autoHide(5000);
      mButtonsToggler.autoHide(5000);
      
      if (mPublishButton != null) {
        mPublishButton.setSelected(true);
      }
      if (mPublishingMarker != null) {
        mPublishingMarker.setVisibility(View.GONE);
      }
      
      if (mUpdateTimer == null) {
        mUpdateTimer = new Timer();
        mUpdateTimer.schedule(new TimerTask() {
          @Override
          public void run() {
            runOnUiThread(new Runnable() {
              public void run() {
                if (mPublisher != null) {
                  if (mPublishingMarker != null) {
                    if (mPublisher.getState() == Publisher.State.PUBLISHING) {
                      mPublishingMarker.setVisibility((mPublishingMarker.getVisibility() == View.VISIBLE) ? View.GONE : View.VISIBLE);
                    } else {
                      mPublishingMarker.setVisibility(View.GONE);
                    }
                  }
                  updateInfo();
                }
              }}
            );
          }
        },
        0, 1000);
      }
    } else {
      mBarToggler.autoHide(0);
      mButtonsToggler.autoHide(0);
      
      if (mPublishButton != null) {
        mPublishButton.setSelected(false);
      }
      
      if (mUpdateTimer != null) {
        mUpdateTimer.cancel();
        mUpdateTimer = null;
      }
      
      if (mPublishingMarker != null) {
        mPublishingMarker.setVisibility(View.GONE);
      }
    }
    
    if (mFlashlightButton != null) {
      mFlashlightButton.setEnabled((mPublisher != null) ? mPublisher.hasFlashlight() : false);
      mFlashlightButton.setSelected((mPublisher != null) ? mPublisher.getFlashlight() : false);
    }
    if (mFocusModeButton != null) {
      Publisher.FocusMode focusMode = Publisher.FocusMode.CONTINUOUS;
      if (mPublisher != null) {
        focusMode = mPublisher.getFocusMode(); 
      }
      mFocusModeButton.getDrawable().setLevel(focusMode.ordinal());
    }
    
    updateInfo();
  }
  
  private void createPublisher() {
    Publisher.Setup setup = new Publisher.Setup();
    
    setup.videoWidth = 640;
    setup.videoHeight = 480;
    setup.videoFramerate = 30;
    setup.videoBitRate = 1000000; // 1 Mbps
    setup.videoMaxLag = 1000000L; // 1 second
    
    setup.audioChannels = 2;
    setup.audioSampleRate = 44100;
    setup.audioBitRate = 64000;
    setup.audioMaxLag = 2000000; // 2 seconds
    
    Publisher.Options options = mPublisherOptions.clone();
    mPublisherOptionsChanged = false;
    
    //mPublisher.create(this, (ViewGroup)findViewById(R.id.publish_activity), "http://95.64.125.43:8080/test_http_server/live1", ap, vp);
    mPublisher.create(
      this,
      (ViewGroup)findViewById(R.id.publish_activity),
      "file://sdcard/output.flv",
      setup,
      options
    );
  }
  
  @Override
  protected void onCreate(Bundle savedInstanceState) {
    super.onCreate(savedInstanceState);
    setFullscreen(true);
    
    mPublisherOptions.load(savedInstanceState);
    
    setContentView(R.layout.publish_activity);
    getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
    
    mPublishButton = (ImageButton)findViewById(R.id.record_button);
    mPublishButton.setOnClickListener(new View.OnClickListener() {
      public void onClick(View v) {
        if (mPublisher != null) {
          if (mPublisher.getState() == Publisher.State.PUBLISHING) {
            mPublisher.stop();
          } else
          if (mPublisher.getState() == Publisher.State.PREVIEWING) {
            mPublisher.start();
          }
        }
      }
    });
    mFlashlightButton = (ImageButton)findViewById(R.id.flash_button);
    mFlashlightButton.setOnClickListener(new View.OnClickListener() {
      public void onClick(View v) {
        startActivity(new Intent(PublishActivity.this, PreferencesActivity.class));
        /*
        if (mPublisher != null) {
          mPublisher.setFlashlight(!mPublisher.getFlashlight());
          mFlashlightButton.setSelected(mPublisher.getFlashlight());
        }
        */
      }
    });
    mFlashlightButton.setVisibility(getPackageManager().hasSystemFeature(PackageManager.FEATURE_CAMERA_FLASH) ? View.VISIBLE : View.GONE);
    
    mFocusModeButton = (ImageButton)findViewById(R.id.focus_mode_button);
    mFocusModeButton.setOnClickListener(new View.OnClickListener() {
      public void onClick(View v) {
        Publisher.FocusMode focusMode = Publisher.FocusMode.CONTINUOUS;
        if (mPublisher != null) {
          focusMode = mPublisher.getFocusMode(); 
        }
        
        final AlertDialog dialog = new AlertDialog.Builder(PublishActivity.this).
        setTitle(R.string.focus_mode).
        setSingleChoiceItems(R.array.publisher_focus_mode_values, 0, new DialogInterface.OnClickListener() {
          @Override
          public void onClick(DialogInterface arg0, int arg1) {
            Publisher.FocusMode focusMode = Publisher.FocusMode.CONTINUOUS;
            switch (arg1) {
              case 0:
                focusMode = Publisher.FocusMode.FIXED;
                break;
              case 1:
                focusMode = Publisher.FocusMode.CONTINUOUS;
                break;
              case 2:
                focusMode = Publisher.FocusMode.MACRO;
                break;
              default:
                return;
            }
            if (mPublisher != null) {
              mPublisher.setFocusMode(focusMode);
            }
            arg0.dismiss();
          }
        }).create();
        
        dialog.show();
        switch (focusMode) {
          case FIXED:
            dialog.getListView().setItemChecked(0, true);
            break;
          case CONTINUOUS:
            dialog.getListView().setItemChecked(1, true);
            break;
          case MACRO:
            dialog.getListView().setItemChecked(2, true);
            break;
        }
      }
    });
    mFocusModeButton.setVisibility(getPackageManager().hasSystemFeature(PackageManager.FEATURE_CAMERA_AUTOFOCUS) ? View.VISIBLE : View.GONE);
    
    mPublishingMarker = findViewById(R.id.publishing_marker);
    mPublishingInfo = (TextView)findViewById(R.id.publishing_info);
    
    mBarToggler = new ViewToggler(
        this,
        findViewById(R.id.publishing_bar),
        R.anim.slide_in_top,
        R.anim.slide_out_top
      );
    mButtonsToggler = new ViewToggler(
      this,
      findViewById(R.id.publishing_buttons),
      R.anim.slide_in_bottom,
      R.anim.slide_out_bottom
    );
    
    getWindow().getDecorView().findViewById(android.R.id.content).setOnClickListener(new View.OnClickListener() {
      public void onClick(View v) {
        PublishActivity p = PublishActivity.this;
        if ((p.mPublisher != null) && (p.mPublisher.getState() == Publisher.State.PUBLISHING)) { 
          p.mBarToggler.show(!p.mBarToggler.isVisible());
          p.mButtonsToggler.show(!p.mButtonsToggler.isVisible());
        }
      }
    });
    
    mServiceConnection = new ServiceConnection(this);
  }
  @Override
  protected void onDestroy() {
    super.onDestroy();
  }
  
  @Override
  public void onResume() {
    super.onResume();
    
    mPublisherOptionsChanged =
    mPublisherOptions.load(PreferenceManager.getDefaultSharedPreferences(this)); 
    
    mServiceConnection.bind(this, Service.class);
  }
  @Override
  public void onPause() {
    mServiceConnection.unbind(this);
    super.onPause();
  }
  
  @Override
  public boolean onCreateOptionsMenu(Menu menu) {
    super.onCreateOptionsMenu(menu);
    getMenuInflater().inflate(R.menu.common, menu);
    
    return true;
  }
  
  @Override
  public boolean onOptionsItemSelected(MenuItem item) {
    super.onOptionsItemSelected(item);
    
    switch (item.getItemId()) {
      case R.id.menu_preferences:
        startActivity(new Intent(this, PreferencesActivity.class));
        return true;
    }
    return false;
  }

  @Override
  protected void onSaveInstanceState(Bundle outState) {
    mPublisherOptions.save(outState);
    super.onSaveInstanceState(outState);
  }
  
  @Override
  public void onBackPressed() {
    if (mCanGoBack || (mPublisher == null) || (mPublisher.getState() != Publisher.State.PUBLISHING)) {
      if (mCanGoBackToast != null) {
        mCanGoBackToast.cancel();
      }
      
      if (mPublisher != null) {
        if (mPublisher.getState() == Publisher.State.PUBLISHING) {
          mPublisher.stop();
        }
      }
      
      super.onBackPressed();
      return;
    }
    
    mCanGoBack = true;
    mCanGoBackToast = Application.toast(this, R.string.publisher_go_back);
    
    mBarToggler.show(true);
    mButtonsToggler.show(true);
    
    Timer timer = new Timer();
    timer.schedule(new TimerTask() {
      @Override
      public void run() {
        runOnUiThread(new Runnable() {
          public void run() {
            mCanGoBack = false;
          }
        });
      }
    }, 2000);
  }

  @Override
  public void onPublisherNotification(Publisher publisher, int what, int arg) {
    if (mPublisher != publisher) {
      return;
    }
    
    if (what == Publisher.NOTIFY_STATE_CHANGED) {
      if (publisher.getState() == Publisher.State.PUBLISHING) {
        mBarToggler.show(false);
        mButtonsToggler.show(false);
      } else
      if (what == Publisher.State.PUBLISHING.ordinal()) {
        if (mPublisherOptionsChanged) {
          mPublisher.destroy();
          createPublisher();
       }
      }
    }
    updateUI();
  }
  
  @Override
  public void onServiceChanged() {
    Service service = (Service)mServiceConnection.getService();
    if (service != null) {
      mPublisher = service.getPublisher();
      mPublisher.addStateListener(this);
      
      if (mPublisherOptionsChanged) {
        if (mPublisher.getState() != Publisher.State.PUBLISHING) {
          mPublisher.destroy();
        } else {
          Application.toast(this, R.string.publisher_settings_not_applied);
        }
      }
      
      if (mPublisher.getState() != Publisher.State.NONE) {
        mPublisher.attach(this, (ViewGroup)findViewById(R.id.publish_activity));
      } else {
        createPublisher();
      }
    } else {
      if (mPublisher != null) {
        if (mPublisher.getState() != Publisher.State.PUBLISHING) {
          mPublisher.destroy();
        } else {
          mPublisher.detach(this);
        }
        
        mPublisher.removeStateListener(this);
        mPublisher = null;
      }
    }
    
    updateUI();
  }
}
