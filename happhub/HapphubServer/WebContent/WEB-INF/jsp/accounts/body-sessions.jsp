<style>
.session.modeless {
  margin-bottom: 8px;
}

.session a.block {
  display: block;
  cursor: pointer;
}
.session a.block:hover .img-polaroid {
  border-color: rgba(82, 168, 236, 0.8);
  -webkit-box-shadow: inset 0 1px 1px rgba(0, 0, 0, 0.075), 0 0 8px rgba(82, 168, 236, 0.6);
  -moz-box-shadow: inset 0 1px 1px rgba(0, 0, 0, 0.075), 0 0 8px rgba(82, 168, 236, 0.6);
  box-shadow: inset 0 1px 1px rgba(0, 0, 0, 0.075), 0 0 8px rgba(82, 168, 236, 0.6);
}

</style>
<div id="page-sessions" class="tab-pane fade in active">
  <div id="page-sessions-bar" class="clearfix">
    <div class="pull-left">
      <form action="#" id="page-sessions-form-search">
        <div class="control-group">
          <div class="controls">
            <div class="input-append">
              <input id="page-sessions-form-search-keywords" name="keywords" type="text" placeholder="Search by keyword" class="span3"></input>
              <button type="submit" class="btn">
                <i class="icon-search icon-black"></i>
              </button>
            </div>
          </div>
        </div>
      </form>
    </div>
    <div class="pull-right">
      <button id="page-sessions-add" class="btn btn-primary">New Session</button>
    </div>
  </div>
  <div id="page-sessions-sessions" class="container"></div>
</div>
<form action="#" id="page-sessions-session-form" class="span5" style="display:none">
  <fieldset>
    <div class="control-group">
      <label for="page-sessions-session-form-name" class="control-label">Name</label>
      <span class="input-fill">
        <input type="text" name="name" id="page-sessions-session-form-name" placeholder="Name"/>
      </span>
    </div>
  </fieldset>
  <div class="form-actions">
    <a href="#" class="btn" data-dismiss="modal">Cancel</a>
    <input type="submit" id="page-sessions-session-form-submit" class="btn btn-primary" value="Submit"/>
  </div>
</form>
<script src="/js/happhub-sessions.js"></script>
<script type="text/javascript">
  $(function() {
    var sessionMVC = happhub.sessions.mvc(
      happhub.account.id, {
      }, {
    	  format:
        '<div class="span4">'+
        '<div class="session modal modeless">'+
          '<div class="modal-header">'+
            '<a href="#" class="block -event" data-event="session-manage">'+
      		    '<div class="img-polaroid animated"><img src="/img/no_thumbnail.png" style="width:100%"/></div>'+
      		    '<h4 class="nowrap animated" data-bind="name"></h4>'+
    		    '</a>'+
    		  '</div>'+
          '<div class="modal-footer clearfix" style="text-align:inherit">'+
            '<div class="pull-left">'+
              '<div class="btn-group">'+
                '<a class="btn -event" data-event="session-edit">Edit</a>'+
                '<a class="btn btn-danger -event" data-event="session-delete">Delete</a>'+
              '</div>'+
            '</div>'+
            '<div class="btn-group pull-right dropup">'+
              '<a class="btn dropdown-toggle" data-toggle="dropdown" href="#">Cameras <span class="caret"></span></a>'+
              '<ul class="dropdown-menu" data-bind="cameras" data-member="name" data-markup="&lt;a href=&quot;#&quot; class=&quot;-event&quot; data-event=&quot;session-camera&quot;&gt;&lt;/a&gt;"/>'+
            '</div>'+
          '</div>'+
        '</div>'+
        '</div>'
      }, {
      create: function() {
        var self = this;
        $.each(this.view.$root.find('.-event'), function(i,e) {
          happhub.morphEvent(e, 'click', {event:$(e).data('event'),eventData:self}); 
        });
      }
    });
    
    var sessionListMVC = $$({}, '<div class="row"></div>', {}).persist();
    $$({
      view: {
        $root: $('#page-sessions-sessions')
      }
    }).append(sessionListMVC);
    
    var sessionListFetchState = {
      search: '',
      start: 0,
      limit: 36
    };
    
    var sessionListFetchDone = false;
    var sessionListFetchRunning = false;
    
    var sessionListFetch = function(force) {
      if (!sessionListFetchRunning && (force || !sessionListFetchDone)) {
        sessionListFetchRunning = true;
        
        var previousSize = sessionListMVC.size(); 
        happhub.persist(
          sessionListMVC,
          'gather',
          function(success, entity) {
            sessionListFetchRunning = false;
            if (success) {
              sessionListFetchState.start = sessionListMVC.size();  
              
              if (sessionListMVC.size() < (previousSize + sessionListFetchState.limit)) {
                sessionListFetchDone = true;
              }
            } else {
              happhub.notify('Failed retrieving the sessions list.', 'error');
            }
            
            $('#page-sessions-sessions').fadeIn(100);
          },
          sessionMVC,
          'append',
          sessionListFetchState
        );
      }
    };
    
    var sessionForm = happhub.mvcForm('#page-sessions-session-form', {
      mvc: sessionMVC,
      toForm: function(model) {
        $('#page-sessions-session-form-name').val(model['name']);
      },
      fromForm: function(model) {
        model.set({name: $('#page-sessions-session-form-name').val()});
        return true;
      },
      validator: {
        fields: {
          '#page-sessions-session-form-name': {
            rules: [{
              rule: 'minLength',
              message: 'Cannot be empty.',
              args: [1]
            }, {
              rule: 'maxLength',
              message: 'At most 255 characters are allowed.',
              args: [255]
            }]
          }
        }
      }
    });
    
    $('#page-sessions-add').on('click', function() {
      $('#page-sessions-session-form-submit').attr('value', 'Add');
      
      var dialog = null;
      sessionForm.setup(null,  function(start, success, entity) {
        start ? dialog.$.block({message:null}) : dialog.$.unblock();
        if (!start) {
          if (success) {
            sessionListMVC.append($$(sessionMVC, entity.get()));
            dialog.hide();
          } else {
            happhub.validator($('#page-sessions-session-form')).error('Failed adding a session.');
          }
        }
      });
      
      dialog = happhub.dialog(
        $('#page-sessions-session-form'), {
          header: '<h3>Add Session</h3>'
        }
      ).show();
    });
    $(document).on('session-manage', function(e, session) {
      window.location.href = '/accounts/'+happhub.account.id+'/sessions/'+session.model.get('id');
    });
    $(document).on('session-edit', function(e, session) {
      $('#page-sessions-session-form-submit').attr('value', 'Update');
      
      var dialog = null;
      sessionForm.setup(session, function(start, success, entity) {
        start ? dialog.$.block({message:null}) : dialog.$.unblock();
        if (!start && success) {
          dialog.hide();
        }
      });
      
      dialog = happhub.dialog(
        $('#page-sessions-session-form'), {
          header: '<h3>Edit Session</h3>'
        }
      ).show();
    });
    $(document).on('session-delete', function(e, session) {
      var dialog = happhub.confirm(
        'All the associated data and stored media content will be also deleted.<br/><br/>Are you sure you want to delete this session ?',
        'Delete <em class="nowrap">"'+happhub.escapeHtml(session.model.get('name'))+'</em>"',
        function() {
          happhub.persist(session, 'erase', function(success, entity) {
            if (success) {
              dialog.hide();
            } else {
              happhub.validator(dialog.$,{}).error('Failed deleting the session.');
            }
          });
        }
      ).show();
      return false;
    });
    $(document).on('session-camera', function(e, session) {
      alert('aaa');
    });
    
    $(document).on('scroll', function(e) {
      if (e.srcElement == document) {
        var t0 = $('#page-sessions-sessions').position().top + $('#page-sessions-sessions').height();
        var t1 = $(document).scrollTop() + $(window).height();
        if (t1 >= t0) {
          sessionListFetch(false);
        }
      }
    });
    
    var refresh = function() {
      $('#page-sessions-sessions').fadeOut(100, function() {
        sessionListMVC.empty();
        
        sessionListFetchState.search = $('#page-sessions-form-search-keywords').val();
        sessionListFetchState.start = 0;
        sessionListFetch(true);
      });
    };
    
    $('#page-sessions-form-search').on('submit', function() {
      refresh();
      return false;
    });
    refresh();
  });
</script>
