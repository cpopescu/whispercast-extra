$.extend($.expr[':'], {
  data: function(elem, i, match) {
    return !!$.data(elem, match[3]);
  },

  focusable: function(element) {
    var nodeName = element.nodeName.toLowerCase(),
      tabIndex = $.attr(element, 'tabindex');
    return (/input|select|textarea|button|object/.test(nodeName)
      ? !element.disabled
      : 'a' == nodeName || 'area' == nodeName
        ? element.href || !isNaN(tabIndex)
        : !isNaN(tabIndex))
      && !$(element)['area' == nodeName ? 'parents' : 'closest'](':hidden').length;
  },

  tabbable: function(element) {
    var tabIndex = $.attr(element, 'tabindex');
    return (isNaN(tabIndex) || tabIndex >= 0) && $(element).is(':focusable');
  }
});

window.happhub = (function (happhub) {
  happhub.escapeHtml = function(s) {
    return s.replace(/&/g,'&amp;').replace(/</g,'&lt;').replace(/>/g,'&gt;');
  };

  var busyCount = 0;
  happhub.busyOn = function(callback) {
    if (busyCount++ == 0) {
      $.blockUI();
      $('#header-progress').fadeIn(200, callback);
    }
  };
  happhub.busyOff = function(callback) {
    if (--busyCount == 0) {
      $.unblockUI();
      $('#header-progress').fadeOut(200, function() {
        if (callback) {
          callback.call(this);
        }
      });
    }
  };
  
  happhub.morphEvent = function(element, event, options) {
    var l = options;
    $(element).on(event, function(e) {
      if (l.event) {
        $(l.eventTarget ? l.eventTarget : document).trigger(l.event, l.eventData);
      } else
      if (l.callback) {
        l.callback();
      }
    });
  };
  
  happhub.notify = function(anchor, message, type, placement, container) {
    if (!anchor) {
      anchor = $('body');
    } else {
      anchor = $(anchor);
    }
    
    if (message) {
      anchor.addClass('popoverAnchor');
      anchor.popover({
        content: message,
        placement: placement ? placement : 'bottom',
        trigger: 'manual',
        container: container ? container : 'body'
      });
      anchor.popover('show');
    }
  };
  
  var __checkApiResult = function(result, status, xhr, cb, cbContext) {
    if (xhr.status == 403) {
      happhub.alert(
      'Your session has expired - please sign in again.',
      'Session Expired', function() {
        window.location.reload();
      }).show();
      return;
    }
    if (cb) {
      if (result) {
        cb.call(cbContext, result.code, result);
      } else {
        cb.call(cbContext, null, xhr);
      }
    }
  };

  happhub.ajax = function(path, data, type, cb, cbContext) {
    happhub.busyOn();
    
    $.ajax(path, {
      headers: {
        "Accept" : "application/json",
        "Content-Type": "application/json"
      },
      processData: false,
      data: data ? $.toJSON(data) : undefined,
      type: type
    }).done(function(result, status, xhr) {
      __checkApiResult((xhr.status == 204) ? null : result, status, xhr, cb, cbContext);
      happhub.busyOff();
    }).fail(function(xhr, status, result) {
      __checkApiResult(null, status, xhr, cb, cbContext);
      happhub.busyOff();
    });
  };
  
  happhub.login = function(email, password, cb, cbContext) {
    happhub.ajax('/login', {email:email,password:password}, 'POST', cb, cbContext);
  };
  happhub.logout = function(cb, cbContext) {
    happhub.ajax('/logout', null, 'POST', cb, cbContext);
  };

  happhub.persist = function(entity, operation, cb, cbContext) {
    entity.bind('persist:start.happhub0', function(e) {
      happhub.busyOn();
    });
    entity.bind('persist:stop.happhub0', function(e) {
      happhub.busyOff();
      entity.unbind('.happhub0');
    });
    
    if (cb) {
      entity.bind('persist:'+operation+':success.happhub1', function(e) {
        cb.call(cbContext, true, entity);
        entity.unbind('.happhub1');
      });
      entity.bind('persist:'+operation+':error.happhub1', function(e) {
        cb.call(cbContext, false, entity);
        entity.unbind('.happhub1');
      });
    }
    
    var args = Array.prototype.slice.call(arguments, 3);
    entity[operation].apply(entity, args);
  };
  happhub.persistQuery = function(query) {
    var result = [];
    if (query) {
      $.each(query, function(i, e) {
        result.push(encodeURIComponent(i)+'='+encodeURIComponent(e));
      });
    }
    return result.join('&');
  };
  
  happhub.initialize = function(user, account, session, model, options) {
    $.blockUI.defaults.message = null;
    
    $.blockUI.defaults.baseZ = 2000;
    
    $.blockUI.defaults.fadeIn = 200;
    $.blockUI.defaults.fadeOut = 200;
    
    $.blockUI.defaults.overlayCSS.opacity = 0.2;
    
    $$.adapter.restfulJson = function(_params){
      var params = $.extend({
        headers: {
          "Accept" : "application/json",
          "Content-Type": "application/json"
        },
        processData: false,
        dataType: 'json',
        url: (this._data.persist.baseUrl || '/api') + this._data.persist.collection + (_params.id ? '/'+_params.id : '')
      }, _params);
      
      if (params.type == 'GET' || params.type == 'HEAD') {
        params.data = happhub.persistQuery(params.data);
      } else {
        params.data = $.toJSON(params.data);
      }
      
      var previous = params.error;
      params.error = function(xhr, status, error) {
        __checkApiResult(null, status, xhr, previous, null);
      };
      
      $.ajax(params);
    };
    
    happhub.user = user;
    happhub.account = account;
    happhub.session = session;
    happhub.model = model;
  };
  return happhub;
})({});