package com.happhub.android.publisher;

import com.happhub.Model.User;
import com.happhub.android.ProgressHandler;
import com.happhub.android.Utils;
import com.happhub.android.rest.Result;
import com.happhub.android.rest.Fragment;

import android.app.Activity;
import android.content.Context;
import android.content.SharedPreferences;
import android.os.Bundle;
import android.preference.PreferenceManager;
import android.text.TextUtils;
import android.text.method.LinkMovementMethod;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.KeyEvent;
import android.view.View;
import android.view.ViewGroup;
import android.view.inputmethod.EditorInfo;
import android.view.inputmethod.InputMethodManager;
import android.widget.EditText;
import android.widget.TextView;

public class LoginFragment extends Fragment<User> implements ProgressHandler {
  public interface LoginListener {
    public void onLogin(User user);
  };

  static
  private class Processor extends com.happhub.android.rest.Processor<User> {
    public String mEmail = "";
    public String mPassword = "";
    
    public void setup(String email, String password) {
      if ((Utils.compareStrings(mEmail, email) != 0) || (Utils.compareStrings(mPassword, password) != 0)) {
        setData(null, false);
        mIsDirty = true;
      } else
      if (mData == null) {
        mIsDirty = true;
      }
      
      mIsDirty &= (email != "") && (password != "");
      
      mEmail = email;
      mPassword = password;
    }
    
    @Override
    protected boolean onExecute() {
      Log.v("HAPPHUB", "Executing " + this);
      
      if (mEmail != "" && mPassword != "") {
        if (isBound()) {
          if (mTaskId == null) {
            mTaskId = ((Service)mServiceConnection.getService()).login(this, mEmail, mPassword);
            return false;
          }
        }
        return true;
      }
      return false;
    }
  }
  
  private EditText mEmailView = null;
  private EditText mPasswordView = null;
  
  private View mLoginFormView = null;
  private View mLoginProgressView = null;
  
  private User mUser = null;
  private LoginListener mListener = null;
  
  public LoginFragment() {
    create(Service.class, 0, 0);
  }

  private void submit() {
    ((InputMethodManager)getActivity().getSystemService(Context.INPUT_METHOD_SERVICE)).hideSoftInputFromWindow(mEmailView.getWindowToken(), 0);
    
    String email = mEmailView.getText().toString();
    String password = mPasswordView.getText().toString();

    boolean cancel = false;
    View focusView = null;

    int messageId = -1;
    
    if (TextUtils.isEmpty(password)) {
      messageId = R.string.login_error_password_required;
      focusView = mPasswordView;
      cancel = true;
    }

    if (TextUtils.isEmpty(email)) {
      messageId = R.string.login_error_email_required;
      focusView = mEmailView;
      cancel = true;
    } 
    if (!email.contains("@")) {
      messageId = R.string.login_error_invalid_email;
      focusView = mEmailView;
      cancel = true;
    }

    if (cancel) {
      Application.toast(getActivity(), messageId);
      focusView.requestFocus();
    } else {
      SharedPreferences.Editor editor = PreferenceManager.getDefaultSharedPreferences(getActivity()).edit();
      editor.putString("login_email", email);
      editor.putString("login_password", password);
      editor.commit();
      
      ((Processor)mRestProcessor).setup(email, password);
      mRestProcessor.execute();
    }
  }
  
  @Override
  public void onCreate(Bundle savedInstanceState) {
    if (mRestProcessor == null) {
      String email = null;
      String password = null;
      
      if (savedInstanceState != null) {
        email = savedInstanceState.getString("login_email");
        password = savedInstanceState.getString("login_password");
      } else {
        SharedPreferences preferences = PreferenceManager.getDefaultSharedPreferences(getActivity());
        email = preferences.getString("login_email", "");
        password = preferences.getString("login_password", "");
      }
      
      mRestProcessor = new Processor();
      ((Processor)mRestProcessor).setup(email, password);
    }
    super.onCreate(savedInstanceState);
  }
  
  @Override
  public void onStart() {
    super.onStart();
    
    ((TextView)getView().findViewById(R.id.register)).setMovementMethod(LinkMovementMethod.getInstance());
  }
  
  @Override
  public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
    View view = inflater.inflate(R.layout.login_fragment, container, false);
    mEmailView = (EditText)view.findViewById(R.id.email);

    mPasswordView = (EditText)view.findViewById(R.id.password);
    mPasswordView.setOnEditorActionListener(new TextView.OnEditorActionListener() {
      @Override
      public boolean onEditorAction(TextView textView, int id, KeyEvent keyEvent) {
        if (id == R.id.login || id == EditorInfo.IME_NULL) {
          submit();
          return true;
        }
        return false;
      }
    });

    mLoginFormView = view.findViewById(R.id.login_form);
    mLoginProgressView = view.findViewById(R.id.progress);
    
    TextView progress = (TextView)mLoginProgressView.findViewById(R.id.progress_text);
    if (progress != null) {
      progress.setText(R.string.login_progress_signing_in);
    }

    view.findViewById(R.id.sign_in_button).setOnClickListener(
      new View.OnClickListener() {
        @Override
        public void onClick(View view) {
          submit();
        }
      }
    );

    mEmailView.setText(((Processor)mRestProcessor).mEmail);
    mPasswordView.setText(((Processor)mRestProcessor).mPassword);
    
    return view;
  }
  
  @Override
  public void onAttach(Activity activity) {
    mProgressHandler.setProgressHandler(this);
    super.onAttach(activity);
    
    if (activity instanceof LoginListener) {
      mListener = (LoginListener)activity;
    }
  }
  
  @Override
  public void onDetach() {
    mListener = null;
    
    super.onDetach();
    mProgressHandler.setProgressHandler(null);
  }
  
  @Override
  public void onSaveInstanceState(Bundle outState) {
    super.onSaveInstanceState(outState);
  }
  
  @Override
  public void onDataChanged(Result<User> data, boolean reportErrors) {
    User user = mUser;
    if (data != null) {
      if (data.getStatus() != 0) {
        user = null;
        if (reportErrors) {
          Application.restError(getActivity(), data);
        }
      } else {
        user = data.getValue();
      }
    } else {
      user = null;
    }
    
    if (user != mUser) {
      mUser = user;
      if (mListener != null) {
        mListener.onLogin(mUser);
      }
    }
    super.onDataChanged(data, false);
  }

  @Override
  public void showProgress(boolean shown) {
    if (mLoginProgressView != null)  {
      mLoginProgressView.setVisibility(shown ? View.VISIBLE : View.GONE);
      mLoginFormView.setVisibility(shown ? View.GONE : View.VISIBLE);
    }
  }
}
