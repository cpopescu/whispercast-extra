package com.happhub.android.publisher;

import java.net.URI;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

import org.apache.http.HttpStatus;
import org.apache.http.client.CookieStore;
import org.apache.http.client.HttpClient;
import org.apache.http.client.protocol.ClientContext;
import org.apache.http.impl.client.BasicCookieStore;
import org.apache.http.impl.client.DefaultHttpClient;
import org.apache.http.params.BasicHttpParams;
import org.apache.http.params.HttpParams;
import org.apache.http.protocol.BasicHttpContext;
import org.apache.http.protocol.HttpContext;
import org.springframework.http.HttpEntity;
import org.springframework.http.HttpMethod;
import org.springframework.http.client.HttpComponentsClientHttpRequestFactory;
import org.springframework.http.converter.HttpMessageConverter;
import org.springframework.http.converter.StringHttpMessageConverter;
import org.springframework.http.converter.json.MappingJacksonHttpMessageConverter;
import org.springframework.web.client.HttpStatusCodeException;

import com.happhub.Api.Login.LoginParams;
import com.happhub.Model.User;
import com.happhub.android.rest.Result;

import android.util.Log;

public class Happhub extends org.springframework.web.client.RestTemplate {
  private class RequestFactory extends HttpComponentsClientHttpRequestFactory {
    private final HttpContext mHttpContext;
  
    public RequestFactory(HttpClient httpClient, HttpContext httpContext) {
      super(httpClient);
      mHttpContext = httpContext;
    }
  
    @Override
    protected HttpContext createHttpContext(HttpMethod httpMethod, URI uri) {
      return mHttpContext;
    }
  }
  
  private final HttpClient mHttpClient;
  private final CookieStore mCookieStore;
  private final HttpContext mHttpContext;
  private final RequestFactory mRequestFactory;

  private final String mBaseUrl;
  
  private User mUser = null;
  private String mPassword = null;
  
  public Happhub(String baseUrl) {
    HttpParams params = new BasicHttpParams();

    mHttpClient = new DefaultHttpClient(params);
    mCookieStore = new BasicCookieStore();
    mHttpContext = new BasicHttpContext();
    mHttpContext.setAttribute(ClientContext.COOKIE_STORE, getCookieStore());
    
    mRequestFactory = new RequestFactory(mHttpClient, mHttpContext);
    super.setRequestFactory(mRequestFactory);
    
    List<HttpMessageConverter<?>> messageConverters = new ArrayList<HttpMessageConverter<?>>();
    messageConverters.add(new StringHttpMessageConverter());
    messageConverters.add(new MappingJacksonHttpMessageConverter());
    super.setMessageConverters(messageConverters);
    
    mBaseUrl = baseUrl;
  }

  public HttpClient getHttpClient() {
    return mHttpClient;
  }
  public CookieStore getCookieStore() {
    return mCookieStore;
  }
  public HttpContext getHttpContext() {
    return mHttpContext;
  }
  public RequestFactory getRequestFactory() {
    return mRequestFactory;
  }
  
  public User getUser() {
    synchronized(this) {
      return mUser;
    }
  }
  
  public <T> Result<T> createResult(int status, Exception exception, T value) {
    return new Result<T>(status, exception, value);
  }
  
  public Result<User> login(String email, String password) {
    LoginParams params = new LoginParams();
    params.email = email;
    params.password = password;
    
    Result<User> result;
    
    try {
      com.happhub.Api.Login.Result l = this.exchange(mBaseUrl+"/login", HttpMethod.POST, new HttpEntity<LoginParams>(params), com.happhub.Api.Login.Result.class, (Object)null).getBody();
      Log.d("HAPPHUB", "REST login: " + l.getCode() + "/" + l.getResult());
      
      if (l.getCode() == 0) {
        synchronized(this) {
          mUser = l.getUser();
          mPassword = password;

          return createResult(0, null, mUser);
        }
      }
      return createResult(l.getCode(), null, null);
    } catch(HttpStatusCodeException e) {
      Log.e("HAPPHUB", "REST login: " + e.toString());
      result = createResult(e.getStatusCode().value(), e, null);
    } catch(Exception e) {
      Log.e("HAPPHUB", "REST login: " + e.toString());
      result = createResult(-1, e, null);
    }
    
    synchronized(this) {
      mUser = null;
      mPassword = null;
    }
    return result;
  }
  public Result<User> logout() {
    Result<User> result;
    
    try {
      com.happhub.Api.Login.Result l = this.exchange(mBaseUrl+"/logout", HttpMethod.POST, new HttpEntity<Object>(null), com.happhub.Api.Login.Result.class, (Object)null).getBody();
      Log.d("HAPPHUB", "REST logout: " + l.getCode() + "/" + l.getResult());
      
      result = createResult(l.getCode(), null, null);
    } catch(Exception e) {
      Log.e("HAPPHUB", "REST logout: " + e.toString());
      result = createResult(-1, e, null);
    }
    
    synchronized(this) {
      mUser = null;
      mPassword = null;
    }
    return result;
  }
  
  private boolean checkLogin() {
    synchronized(this) {
      if (mUser != null) {
        return login(mUser.getEmail(), mPassword).getStatus() == 0;
      }
    }
    return false;
  }
  
  @SuppressWarnings("unchecked")
  public <T> Result<T> get(String url, Class<?> clss, Map<String, String> params) {
    Result<T> result = null;
    
    boolean loginChecked = false;
    do {
      try {
        result = createResult(0, null, (T)this.getForObject(mBaseUrl+"/api"+url, clss, (params != null) ? params : new HashMap<String, String>()));
        Log.d("HAPPHUB", "REST get("+url+"): " + result);
        break;
      } catch(HttpStatusCodeException e) {
        Log.e("HAPPHUB", "REST get("+url+"): " + e.toString());
        
        if (e.getStatusCode().value() == HttpStatus.SC_FORBIDDEN) {
          if (!loginChecked) {
            loginChecked = true;
            if (checkLogin()) {
              continue;
            }
          }
        }
        result = createResult(e.getStatusCode().value(), e, null);
        break;
      } catch(Exception e) {
        Log.e("HAPPHUB", "REST get("+url+"): " + e.toString());
        
        result = createResult(-1, e, null);
        break;
      }
    } while(true);
    
    return result;
  }
}
