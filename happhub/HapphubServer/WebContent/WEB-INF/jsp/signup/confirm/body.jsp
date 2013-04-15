<%@ taglib uri="http://java.sun.com/jsp/jstl/core" prefix="c" %>
<div class="container page">
  <div class="span5">
    <p class="lead">Please fill in the form in order to complete signing up with <b>Happhub</b>.</p>
  </div>
  <form id="signup-confirm-form" class="modal modeless section-right span5">
    <div class="modal-header">
      <h3>Sign Up Confirmation</h3>
    </div>
    <div class="modal-body">
      <fieldset class="control-group">
        <legend class="accessible-only">Sign Up Confirmation</legend>
        <div class="controls">
          <label for="signup-confirm-form-password" class="control-label accessible-only">Password</label>
          <label class="input-fill input-icon icon-lock">
            <input type="password" name="password" id="signup-confirm-form-password" class="span4" placeholder="Password"></input>
          </label>
        </div>
        <div class="controls">
          <label for="signup-confirm-form-password-confirmation" class="control-label accessible-only">Confirmation</label>
          <label class="input-fill input-icon icon-lock">
            <input type="password" name="password-confirmation" id="signup-confirm-form-password-confirmation" placeholder="Password confirmation"></input>
          </label>
        </div>
      </fieldset>
    </div>
    <div class="modal-footer">
      <div class="form-actions">
        <input type="submit" value="Sign Up" class="btn btn-primary btn-large btn-block"/>
      </div>
    </div>
  </form>
</div>
<script type="text/javascript">
  $(function() {
    happhub.form(
      '#signup-confirm-form', {
        validator: {
          fields: {
            '#signup-confirm-form-password': {
              rules:[{
                rule: 'minLength',
                message: 'Cannot be empty.',
                args: [1]
              }]
            },
            '#signup-confirm-form-password-confirmation': {
              rules: [{
                rule: 'minLength',
                message: 'Cannot be empty.',
                args: [1]
              }, {
                rule: 'sameAs',
                message: 'Password and password confirmation don\'t match.',
                args: ['#signup-confirm-form-password']
              }]
            }
          }
        }
      }
    ).$.
    on('submit', function() {
      happhub.ajax(
        '/signup/confirm/'+happhub.model.hash, {
          password: $('#signup-confirm-form-password').val()
        },
        'POST',
        function(code, result) {
          if (code == null) {
            if (result.status == 404) {
              happhub.validator($('#signup-confirm-form')).error('This signup was completed or is expired.');
            } else {
              happhub.validator($('#signup-confirm-form')).error('Registration failed, code: '+result.status+' ('+happhub.escapeHtml(result.statusText)+').');
            }
          } else {
            if (code == 0) {
              happhub.busyOn();
              window.location.assign('/');
            } else {
              happhub.validator($('#signup-confirm-form')).error('Cannot perform signups now, please try again later.');
            }
          }
        }
      );
      return false;
    });
  });
</script>