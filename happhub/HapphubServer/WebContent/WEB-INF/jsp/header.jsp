<%@ taglib uri="http://java.sun.com/jsp/jstl/core" prefix="c" %>
<%
com.happhub.RootContext r = (com.happhub.RootContext)request.getAttribute("it");
javax.xml.bind.Marshaller marshaller = r.getContext().createMarshaller();
marshaller.setProperty("eclipselink.media-type", "application/json");
marshaller.setProperty("eclipselink.json.include-root", false);
%>
<%@ page contentType="text/html" pageEncoding="UTF-8" %>
<!DOCTYPE html>
<!--[if lt IE 7]>      <html class="no-js lt-ie9 lt-ie8 lt-ie7"> <![endif]-->
<!--[if IE 7]>         <html class="no-js lt-ie9 lt-ie8"> <![endif]-->
<!--[if IE 8]>         <html class="no-js lt-ie9"> <![endif]-->
<!--[if gt IE 8]><!--> <html class="no-js"> <!--<![endif]-->
<head> 
    <title>Happhub</title>
    <meta name="viewport" content="width=device-width, initial-scale=1, maximum-scale=1">
    <link href="/css/bootstrap.css" rel="stylesheet">
    <link href="/css/bootstrap-responsive.css" rel="stylesheet">
     
    <link href="/css/font-awesome.css" rel="stylesheet">
    <link href="http://fonts.googleapis.com/css?family=Open+Sans:400italic,600italic,400,600" rel="stylesheet">
    <link href="http://fonts.googleapis.com/css?family=Open+Sans:400italic,600italic,400,600" rel="stylesheet">
    
    <link href="/css/happhub.css" rel="stylesheet">
    
    <script src="/js/jquery-1.9.1.js"></script>
    <script src="/js/happhub.js"></script>
  </head>
  <body>
    <script type="text/javascript">
      $(function() {
        happhub.initialize(
          <c:choose><c:when test="${it.user == null}">null</c:when><c:otherwise><% marshaller.marshal(r.getUser(), out); %></c:otherwise></c:choose>,
          <c:choose><c:when test="${it.account == null}">null</c:when><c:otherwise><% marshaller.marshal(r.getAccount(), out); %></c:otherwise></c:choose>,
          <c:choose><c:when test="${it.session == null}">null</c:when><c:otherwise><% marshaller.marshal(r.getSession(), out); %></c:otherwise></c:choose>,
          <c:choose><c:when test="${it.model == null}">null</c:when><c:otherwise><% marshaller.marshal(r.getModel(), out); %></c:otherwise></c:choose>,
          {} // options
        );
      });
    </script>
    <div class="header">
      <div class="navbar navbar-inverse">
        <div id="header-progress" class="navbar-progress"></div>
        <div class="navbar-inner">
          <a class="btn btn-navbar" data-toggle="collapse" data-target=".nav-collapse">
            <span class="icon-bar"></span>
            <span class="icon-bar"></span>
            <span class="icon-bar"></span>
          </a>
          <span id="header-logo" class="brand">
            <a href="/"><img src="/img/happhub_icon.png" id="header-logo-image"/>&nbsp;Happhub</a>
          </span>
          <div class="nav-collapse collapse">
            <ul class="nav pull-right">
<c:choose>
<c:when test="${(it.user != null) && (it.user.email != null)}">            
              <li class="dropdown">
                <a href="#" class="dropdown-toggle" data-toggle="dropdown"><i class="icon-user"></i><c:out value="${it.user.email}"/><i class="icon-caret-down"></i></a>
                <ul class="dropdown-menu">
                  <li><a href="#" id="header-user-logout">Sign Out</a></li>
                </ul>
              </li>
              <script type="text/javascript">
                $(function() {
                  $('#header-user-logout').bind('click', function(e) {
                    happhub.logout(function() {
                      happhub.busyOn();
                      window.location.reload();  
                    });
                    return false;
                  });
                });
              </script>
</c:when>
<c:otherwise>            
              <li>
                <a href="/signup/" id="header-signup"class="btn-header">Signup</a>
              </li>
              <li>
                <a href="#" id="header-login" class="btn-header">Sign In</a>
              </li>
              <script type="text/javascript">
                $(function() {
                  $('#header-login').bind('click', function(e) {
                    happhub.dialog(
                      $('#header-login-form'), {
                        header: '<h3>Sign In</h3>'
                      } 
                    ).show();
                    return false;
                  });
                });
              </script>
</c:otherwise>
</c:choose>
            </ul>
          </div>
        </div>
      </div>
    </div>
    <form action="#" id="header-login-form" class="span5" style="display:none">
      <div>
        <fieldset class="control-group">
          <legend class="accessible-only">Sign In Form</legend>
          <div class="controls">
            <label for="header-login-form-email" class="control-label accessible-only">E-mail</label>
            <label class="input-fill input-icon icon-user">
              <input id="header-login-form-email" name="email" type="text" placeholder="E-mail"></input>
            </label>
          </div>
          <div class="controls">
            <label for="header-login-form-password" class="control-label accessible-only">Password</label>
            <label class="input-fill input-icon icon-lock">
              <input id="header-login-form-password" name="password "type="password" placeholder="Password"></input>
            </label>
          </div>
        </fieldset>
        <label class="checkbox">
          <input type="checkbox" name="remember"> Keep me signed in
        </label>
      </div>
      <div class="form-actions">
        <input type="submit" class="btn btn-primary btn-block btn-large" value="Sign In"/>
      </div>
    </form>
    <script>
      $(function() {
        happhub.form(
          $('#header-login-form'), {
            validator: {
              fields: {
                '#header-login-form-email': {
                  rules:[{
                    rule: 'isEmail',
                    message: 'The e-mail address is not valid.'
                  }]
                },
                '#header-login-form-password': {
                  rules: [{
                    rule: 'minLength',
                    message: 'Cannot be empty.',
                    args: [1]
                  }]
                }
              }
            }
          }
        ).$.
        on('submit', function() {
          happhub.login(
            $('#header-login-form-email').val(),
            $('#header-login-form-password').val(),
            happhub.validator($('#header-login-form')).ajaxCallback(
              'Failed logging in - please check if the e-mail address and password are correct.',
              function(result) {
                happhub.busyOn();
                window.location.reload();
                return;
              }
            )
          );
          return false;
        });
      });
    </script>    