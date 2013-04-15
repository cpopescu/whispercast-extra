package com.happhub.android;

import com.happhub.android.publisher.R;

import android.content.Context;
import android.content.res.TypedArray;
import android.graphics.drawable.Drawable;
import android.util.AttributeSet;
import android.view.View;

public class DrawableView extends View {
  public static final int HORIZONTAL = 0; 
  public static final int VERTICAL = 1; 
  
  private Drawable mDrawable = null;
  
  private int mType = HORIZONTAL;

  public DrawableView(Context context) {
    super(context);
  }

  public DrawableView(Context context, AttributeSet attrs) {
    super(context, attrs);
    initialize(attrs);
  }

  public DrawableView(Context context, AttributeSet attrs, int defStyle) {
    super(context, attrs, defStyle);
    initialize(attrs);
  }
  
  public Drawable getDrawable() {
    return mDrawable;
  }
  public void setDrawable(Drawable drawable) {
    mDrawable = drawable;
    update();
    
    requestLayout();
    invalidate();
  }
  
  private void initialize(AttributeSet attrs) {
    TypedArray resolved = getContext().obtainStyledAttributes(attrs, R.styleable.DrawableView);
    
    mType = resolved.getInt(R.styleable.DrawableView_type, HORIZONTAL);
    
    int srcId = resolved.getResourceId(R.styleable.DrawableView_src, -1);
    if (srcId > 0) {
      mDrawable = getResources().getDrawable(srcId);
    }

    update();
  }
  
  @SuppressWarnings("deprecation")
  private void update() {
    if (mDrawable != null) {
      if(android.os.Build.VERSION.SDK_INT < android.os.Build.VERSION_CODES.JELLY_BEAN) {
        setBackgroundDrawable(mDrawable);
      } else {
        setBackground(mDrawable);
      }
    }
  }
  
  @Override
  protected void onMeasure(int widthMeasureSpec, int heightMeasureSpec) {
    if (mDrawable != null) {
      if (mType == VERTICAL) {
        int height = MeasureSpec.getSize(heightMeasureSpec);
        int width = height * mDrawable.getIntrinsicWidth() / mDrawable.getIntrinsicHeight();
        setMeasuredDimension(width, height);
      } else {
        int width = MeasureSpec.getSize(widthMeasureSpec);
        int height = width * mDrawable.getIntrinsicHeight() / mDrawable.getIntrinsicWidth();
        setMeasuredDimension(width, height);
      }
    } else {
      super.onMeasure(widthMeasureSpec, heightMeasureSpec);
    }
  }
}