package com.happhub.android;

import java.util.Timer;
import java.util.TimerTask;

import android.app.Activity;
import android.view.View;
import android.view.animation.Animation;
import android.view.animation.AnimationUtils;

public class ViewToggler {
  protected Activity mActivity = null;
  protected View mView = null;
  
  protected Animation mInAnimation = null;
  protected Animation mOutAnimation = null;
  
  protected boolean mIsVisible = false;
  
  protected int mAutohideTimeout = 0;
  protected Timer mAutohideTimer = null;
  
  public ViewToggler(Activity activity, View view, int inAnimationId, int outAnimationId) {
    mActivity = activity;
    
    mView = view;
    mIsVisible = (view.getVisibility() == View.VISIBLE); 
    
    Animation.AnimationListener animationListener = new Animation.AnimationListener() {
      @Override
      public void onAnimationEnd(Animation animation) {
        mView.setVisibility(mIsVisible ? View.VISIBLE : View.GONE);
      }

      @Override
      public void onAnimationRepeat(Animation animation) {
      }
      @Override
      public void onAnimationStart(Animation animation) {
      }
    };
    
    if (inAnimationId != 0) {
      mInAnimation = AnimationUtils.loadAnimation(mActivity, inAnimationId);
      mInAnimation.setAnimationListener(animationListener);
    }
    if (outAnimationId != 0) {
      mOutAnimation = AnimationUtils.loadAnimation(mActivity, outAnimationId);
      mOutAnimation.setAnimationListener(animationListener);
    }
  }
  
  public boolean isVisible() {
    return mIsVisible;
  }
  
  public void show(boolean show) {
    if (show == mIsVisible) {
      return ;
    }
    
    if (show && (mAutohideTimeout > 0)) {
      restartAutoHide();
    }
      
    mIsVisible = show;
    if (mIsVisible) {
      if (mInAnimation != null) {
        if (mView.getAnimation() != mInAnimation) {
          mView.clearAnimation();
          mView.startAnimation(mInAnimation);
        }
      } else {
        mView.setVisibility(View.VISIBLE);
      }
    } else {
      if (mOutAnimation != null) {
        if (mView.getAnimation() != mOutAnimation) {
          mView.clearAnimation();
          mView.startAnimation(mOutAnimation);
        }
      } else {
        mView.setVisibility(View.GONE);
      }
    }
  }
  
  public void autoHide(int timeout) {
    mAutohideTimeout = timeout;
    restartAutoHide();
  }
  protected void restartAutoHide() {
    if (mAutohideTimer != null) {
      mAutohideTimer.cancel();
      mAutohideTimer = null;
    }
    if (mAutohideTimeout > 0) {
      mAutohideTimer = new Timer();
      mAutohideTimer.schedule(new TimerTask() {
        @Override
        public void run() {
          mActivity.runOnUiThread(new Runnable() {
            public void run() {
              ViewToggler.this.show(false);
            }
          });
        }
      }, mAutohideTimeout);
    } else {
      ViewToggler.this.show(true);
    }
  }
}
