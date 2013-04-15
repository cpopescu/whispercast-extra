<%@ taglib uri="http://java.sun.com/jsp/jstl/core" prefix="c" %>
<div class="container page">
<c:choose>
<c:when test="${it.model.step == 0}">
  <div class="span5">
    <p class="lead">
      Please enter your e-email for the e-mail confirmation, bla, bla.
    </p>
  </div>
  <form id="signup-form" class="modal modeless section-right span5">
    <div class="modal-header">
      <h3>Sign Up</h3>
    </div>
    <div class="modal-body">
      <fieldset class="control-group">
        <legend class="accessible-only">Sign Up</legend>
        <div class="controls">
          <label for="signup-form-email" class="control-label accessible-only">E-mail</label>
          <label class="input-fill input-icon icon-lock">
            <input type="text" name="email" id="signup-form-email" placeholder="E-mail"></input>
          </label>
        </div>
      </fieldset>
      <small>By signing up, I accept <b>Happhub</b>'s <a href="#">Terms of Service</a> and <a href="#">Privacy Policy</a>.</small>
    </div>
    <div class="modal-footer">
      <div class="form-actions">
        <input type="submit" value="Sign Up" class="btn btn-primary btn-large btn-block"/>
      </div>
    </div>
  </form>
  <script type="text/javascript">
  $(function() {
    happhub.form(
      '#signup-form', {
        validator: {
          fields: {
            '#signup-form-email': {
              rules:[{
                rule: 'isEmail',
                message: 'The e-mail address is not valid.'
              }]
            }
          }
        }
      }
    ).$.
    on('submit', function() {
      happhub.ajax(
        '/signup', {
          email: $('#signup-form-email').val(),
          accountsId: 0
        },
        'POST',
        function(code, result) {
          if (code == null) {
            if (result.status == 409) {
              happhub.validator($('#signup-form')).error('The e-mail address you entered is already registered with <b>Happhub</b>.');
            } else {
              if (result.status == 404) {
                happhub.validator($('#signup-form')).error('Cannot find the account to be attached to.');
              } else {
                happhub.validator($('#signup-form')).error('Signing up failed, code: '+result.status+' ('+happhub.escapeHtml(result.statusText)+').');
              }
            }
          } else {
            if (code == 0) {
              window.location.assign('/signup/done');
            } else {
              happhub.validator($('#signup-form')).error('Cannot perform your signup now, please try again later.');
            }
          }
        }
      );
      return false;
    });
  });
  </script>
</c:when>
<c:otherwise>
  <p class="lead">
    Please check your e-email for the e-mail confirmation, bla, bla.
  </p>
</c:otherwise>
</c:choose>
</div>