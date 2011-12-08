(function() {

/* The Form Whispercast namespace */
if (Whispercast.Form == undefined) {
  Whispercast.Form = {
  };

  var _cfgDefaults = {
    cancelOnHide: true
  };

  /* Base form class */
  Whispercast.Form.Base = function(cfg) {
    Whispercast.Form.Base.superclass.constructor.call(this, cfg, _cfgDefaults);
  };
  YAHOO.lang.extend(Whispercast.Form.Base, Whispercast.Base, {
    isCreated: function() {
      return this._created;
    },
    isRunning: function() {
      return this._running;
    },

    getDialog: function() {
      return this._dialog;
    },

    create: function() {
      if (!this._created) {
        this._create();
        this._onCreate();
      }
    },
    destroy: function() {
      if (this._created) {
        this._destroy();
        this._onDestroy();
      }
    },

    run: function(record) {
      if (!this._running) {
        if (this._cfg.resolver) {
          var resolver = Whispercast.clone(this._cfg.resolver);

          resolver.callbacks.success = Whispercast.closure(this, function(success) {
            if (success) {
              success.call(null);
            }
            this._run(record);
          }, resolver.callbacks.success);
          resolver.callbacks.failure = Whispercast.closure(this, function(failure) {
            if (failure) {
              failure.call(null);
            }
            var message = YAHOO.util.Connect.formatRequestError(o);
            if (Whispercast.Dialog != undefined) {
              (new Whispercast.Dialog.Prompt('Eroare', message, 'E')).show();
            } else {
              alert(message);
            }
          }, resolver.callbacks.failure);

          new (Whispercast.Resolver.Base(resolver)).run();
        } else {
          this._run(record);
        }
      } else {
        if (this._dialog) {
          this._dialog.show();
        }
      }
    },

    submit: function() {
      if (this._request) {
        return false;
      }

      this._clearErrors();
      if (this._validate()) {
        if (this._dialog) {
          Whispercast.Progress.show(this._cfg.id, this._dialog.getDialog().body.parentNode);
        } else {
          Whispercast.Progress.show(this._cfg.id, Whispercast.App.getLayoutElement('center').body);
        }

        return this._onSubmit();
      }
      return false;
    },
    cancel: function() {
      if (this._request) {
        this._request.conn.abort();
        delete this._request;
      }
      this._onCancel();
    },

    _create: function() {
      if (this._cfg.dialog) {
        var cfgd = Whispercast.clone(this._cfg.dialog);

        if (cfgd.id == undefined) {
          cfgd.id = this._cfg.id+'-dialog';
        }
        if (cfgd.buttons == undefined) {
          cfgd.buttons = [{
            text: _('Save'),
            handler: Whispercast.closure(this, this.submit),
            isDefault: true
          }, {
            text: _('Cancel'),
            handler: Whispercast.closure(this, this.cancel),
          }];
        }

        this._dialog = new Whispercast.Dialog.Base(cfgd);
        this._dialogOwned = true;

        if (this._cfg.cancelOnHide) {
          Whispercast.overload(this._dialog, '_onHide', Whispercast.closure(this, function(previous) {
            var dialog = this._dialog;
            this.cancel();

            previous.call(dialog);
          }));
        }
        Whispercast.overload(this._dialog, '_onDestroy', Whispercast.closure(this, function(previous) {
          var dialog = this._dialog;
          this.destroy();

          previous.call(dialog);
        }));

        this._listenerFocus = Whispercast.closure(this, function(e) {
          this._onFocus(e);
        });
        YAHOO.util.Event.addFocusListener(this._cfg.id, this._listenerFocus);
      }

      this._elements = {};
      this._errors = {};

      this._dateFields = {};
      if (this._cfg.dateFields) {
        for (var id in this._cfg.dateFields) {
          var field = document.getElementById(id);
          YAHOO.util.Dom.setAttribute(field, 'readonly', true);

          var name = this._cfg.dateFields[id].name;
          if (!name) {
            name = YAHOO.util.Dom.getAttribute(field, 'name');
            if (!name) {
              name = id;
            }
          }

          var year = document.createElement('input');
          YAHOO.util.Dom.setAttribute(year, 'type', 'hidden');
          YAHOO.util.Dom.setAttribute(year, 'id', id+'[year]');
          YAHOO.util.Dom.setAttribute(year, 'name', name+'[year]');
          field.parentNode.appendChild(year);
          var month = document.createElement('input');
          YAHOO.util.Dom.setAttribute(month, 'type', 'hidden');
          YAHOO.util.Dom.setAttribute(month, 'id', id+'[month]');
          YAHOO.util.Dom.setAttribute(month, 'name', name+'[month]');
          field.parentNode.appendChild(month);
          var day = document.createElement('input');
          YAHOO.util.Dom.setAttribute(day, 'type', 'hidden');
          YAHOO.util.Dom.setAttribute(day, 'id', id+'[day]');
          YAHOO.util.Dom.setAttribute(day, 'name', name+'[day]');
          field.parentNode.appendChild(day);

          var container = document.createElement('div');
          YAHOO.util.Dom.setStyle(container, 'position', 'absolute');
          YAHOO.util.Dom.setStyle(container, 'display', 'none');
          YAHOO.util.Dom.setStyle(container, 'margin-top', '-8em');
          YAHOO.util.Dom.setStyle(container, 'z-index', 9999);
          field.parentNode.appendChild(container);

          var calendar = new YAHOO.widget.Calendar(id+'calendar', container, {title: _('Enter the date'), close: true});
          calendar.render();

          calendar.selectEvent.subscribe(Whispercast.closure(this, function(id, type, args, obj) {
            var c = this._dateFields[id];
            var d = new Date(args[0][0][0], args[0][0][1]-1, args[0][0][2]);
            document.getElementById(id).value = Whispercast.Locale.formatDate(d, true);
            document.getElementById(id+'[year]').value = d.getFullYear();
            document.getElementById(id+'[month]').value = d.getMonth()+1;
            document.getElementById(id+'[day]').value = d.getDate();
            c.calendar.hide();
          }, id));

          YAHOO.util.Event.on(id, 'focus', Whispercast.closure(this, function(id, e) {
            YAHOO.util.Event.preventDefault(e);
            var c = this._dateFields[id];
            c.calendar.show();
          }, id));
          YAHOO.util.Event.on(id, 'blur', Whispercast.closure(this, function(id, e) {
          }, id));

          this._dateFields[id] = {
            name: name,
            container: container,
            calendar: calendar
          };
        }
      }
      this._timeFields = {};
      if (this._cfg.timeFields) {
        for (var id in this._cfg.timeFields) {
          var field = document.getElementById(id);

          var name = this._cfg.timeFields[id].name;
          if (!name) {
            name = YAHOO.util.Dom.getAttribute(field, 'name');
            if (!name) {
              name = id;
            }
          }

          var hour = document.createElement('input');
          YAHOO.util.Dom.setAttribute(hour, 'type', 'hidden');
          YAHOO.util.Dom.setAttribute(hour, 'id', id+'[hour]');
          YAHOO.util.Dom.setAttribute(hour, 'name', name+'[hour]');
          field.parentNode.appendChild(hour);
          var minute = document.createElement('input');
          YAHOO.util.Dom.setAttribute(minute, 'type', 'hidden');
          YAHOO.util.Dom.setAttribute(minute, 'id', id+'[minute]');
          YAHOO.util.Dom.setAttribute(minute, 'name', name+'[minute]');
          field.parentNode.appendChild(minute);
          var second = document.createElement('input');
          YAHOO.util.Dom.setAttribute(second, 'type', 'hidden');
          YAHOO.util.Dom.setAttribute(second, 'id', id+'[second]');
          YAHOO.util.Dom.setAttribute(second, 'name', name+'[second]');
          field.parentNode.appendChild(second);

          this._timeFields[id] = {
            name: name
          };
        }
      }

      this._created = true;
      this._running = false;

      this._result = false;
    },
    _destroy: function() {
      if (this._listenerFocus) {
        YAHOO.util.Event.removeFocusListener(this._cfg.id, this._listenerFocus);
        delete this._listenerFocus;
      }

      if (this._dialogOwned) {
        this._dialog.destroy();
        delete this._dialog;
      }

      delete this._result;

      delete this._running;
      delete this._created;

      delete this._errors;     
      delete this._elements;
    },

    _run: function(record) {
      if (!this._created) {
        this.create();
      }

      this._running = true;
      this._result = false;

      this._form = document.getElementById(this._cfg.id);
      if (this._form.getAttributeNode) {
        var action = this._form.getAttributeNode('action');
        if (action) {
          this._action = action.value;
        } else {
          this._action = null;
        }
        var method = this._form.getAttributeNode('method');
        if (method) {
          this._method = method.value;
        } else {
          this._method = null;
        }
      } else {
        this._action = this._form.getAttribute('action');
        this._method = this._form.getAttribute('method');
      }

      this._elements.errors = document.getElementById(this._cfg.id + '-errors');
      if (this._elements.errors) {
        YAHOO.util.Dom.setStyle(this._elements.errors, 'white-space', 'pre-line');
      }

      this._clearData();
      this._clearErrors();

      this._submitButton = document.createElement('input')

      {
      var s = new YAHOO.util.Element(this._submitButton);
      s.set('type', 'submit', true);
      s.appendTo(this._form);

      s.setStyle('visibility', 'hidden');
      s.setStyle('width', '0px');
      s.setStyle('height', '0px');
      }

      this._listenerSubmit = Whispercast.closure(this, function(e) {
        if (e) {
          YAHOO.util.Event.preventDefault(e);
        }
        this.submit();
      });
      YAHOO.util.Event.addListener(this._cfg.id, 'submit', this._listenerSubmit, null, this);

      if (this._dialog) {
        this._dialog.create();
      }

      this._onRun(record);
    },
    _done: function() {
      this._running = false;

      if (this._form) {
        YAHOO.util.Event.removeListener(this._form, this._listenerSubmit);
        delete this._listenerSubmit;

        this._form.removeChild(this._submitButton);
        delete this._submitButton;

        delete this._form;
      }

      delete this._action;
      delete this._method;

      delete this._elements.errors;

      if (this._dialog) {
        this._dialog.hide();
      }
      Whispercast.Progress.hide(this._cfg.id);

      this._onDone();
    },

    _validate: function() {
      var result = true;
      for (var id in this._timeFields) {
        var c = this._timeFields[id];
        var v = document.getElementById(id).value;
        if (v) {
          var t = Whispercast.Locale.parseTime(document.getElementById(id).value);
          if (t) {
            document.getElementById(id+'[hour]').value = t.hour;
            document.getElementById(id+'[minute]').value = t.minute;
            document.getElementById(id+'[second]').value = t.second;
          } else {
            document.getElementById(id+'[hour]').value = '';
            document.getElementById(id+'[minute]').value = '';
            document.getElementById(id+'[second]').value = '';
          }
        }
      }
      return result;
    },

    _setDataElement: function(id, value) {
      if (YAHOO.lang.isArray(value)) {
        for (var i = 0; i < value.length; i++) {
          this._setDataElement(id+'['+i+']', value[i]);
        }
        return;
      }
      if (YAHOO.lang.isObject(value)) {
        if (value instanceof Date) {
          if (this._dateFields[id]) {
            this._dateFields[id].calendar.select((value.getMonth()+1)+'/'+value.getDate()+'/'+value.getFullYear());
            this._dateFields[id].calendar.cfg.setProperty('pagedate', (value.getMonth()+1)+'/'+value.getFullYear());
            this._dateFields[id].calendar.render();
          }
          if (this._timeFields[id]) {
            document.getElementById(id).value = Whispercast.Locale.formatTime(value, true);
          }
        } else {
          for (var i in value) {
            this._setDataElement(id+'['+i+']', value[i]);
          }
        }
        return;
      }
  
      var field = document.getElementById(id);
      if (field) {
        switch (field.tagName.toLowerCase()){
          case 'input':
            switch (YAHOO.util.Dom.getAttribute('type')) {
              case 'hidden':
              case 'password':
              case 'text':
                field.value = value;
                break;
              case 'checkbox':
                field.checked = value ? true : false;
            }
            break;
          case 'select':
            for (var j = 0; j < field.options.length; j++) {
              if (field.options[j].value == value) {
                field.selectedIndex = j;
                break;
              }
            }
            break;
        }
      }
    },
    _setData: function(record) {
      for (var i in record) {
        this._setDataElement(i, record[i]);
      }
    },
    _clearData: function() {
    },

    _clearError: function(field) {
      if (this._errors[field] !== undefined) {
        var error = this._errors[field];
        delete this._errors[field];

        if (error === null) {
          Whispercast.Errors.clearMessage(field);
        } else
        if (error === 0) {
          Whispercast.Errors.clearMessage(field+'-proxy', field);
        } else
        if (error === 1) {
          Whispercast.Errors.clearMessage(field+'-proxy', field+'-proxy');
        } else {
          error.parentNode.removeChild(error);
        }
        return true;
      }
      return false;
    },
    _clearErrors: function() {
      if (this._elements.errors) {
        while (this._elements.errors.childNodes.length > 0) {
          this._elements.errors.removeChild(this._elements.errors.firstChild);
        }
      }
     
      for (var field in this._errors) {
        var error = this._errors[field];

        if (error === null) {
          Whispercast.Errors.clearMessage(field);
        } else
        if (error === 0) {
          Whispercast.Errors.clearMessage(field+'-proxy', field);
        } else
        if (error === 1) {
          Whispercast.Errors.clearMessage(field+'-proxy', field+'-proxy');
        }
      }
      this._errors = {};
    },

    _onCreate: function() {
      
    },
    _onDestroy: function() {
    },

    _onFocus: function(e) {
      var form = document.getElementById(this._cfg.id);

      var field = e.target;
      while (field && field != form) {
        var id = YAHOO.util.Dom.getAttribute(field, 'id');
        if (id) {
          var proxy = /^(.*)-proxy$/.exec(id);
          if (proxy) {
            if (this._clearError(proxy[1])) {
              break;
            }
          }
          if (this._clearError(id)) {
            break;
          }
        }
        field = field.parentNode;
      }
    },

    _onRun: function(record) {
      if (record) {
        this._setData(record);
      }

      if (this._dialog) {
        this._dialog.show();
      }
    },

    _onSubmit: function() {
      if (this._method) {
        YAHOO.util.Connect.setForm(this._form);

        this._request =
        YAHOO.util.Connect.asyncRequest(
          this._method,
          Whispercast.Uri.build(this._action, {format: 'json'}, true), {
            success: Whispercast.closure(this, function(o) {
              delete this._request;
              var response = Whispercast.eval('return ('+o.responseText+')');
              this._onSubmitSuccess(o, response);
            }),
            failure: Whispercast.closure(this, function(o) {
              delete this._request;
              this._onSubmitFailure(o);
            })
          }
        );
      } else {
         this._onSubmitSuccess(null, {model:{result:1,errors:{}}});
      }
    },
    _onSubmitComplete: function(success, response) {
      return success && (response != null) && (response.model != undefined) && (response.model.result > 0);
    },

    _fitDialog: function() {
      if (this._dialog) {
        var xy = this._dialog.getDialog().cfg.getProperty('xy');
        this._dialog.getDialog().cfg.setProperty('xy', this._dialog.getDialog().getConstrainedXY(xy[0], xy[1]));
      }
    },

    _onSubmitSuccess: function(o, response) {
      this._result = this._onSubmitComplete(true, response);
      if (!this._result) {
        var dangling = false;
        for (var index in response.model.errors) {
          if (this._onError(index, response.model.errors[index])) {
            delete response.model.errors[index];
          } else {
            dangling = true;
          }
        }
        if (dangling) {
          for (var index in response.model.errors) {
            this._onError(null, response.model.errors[index]);
          }
        }

        this._fitDialog();
        Whispercast.Progress.hide(this._cfg.id);
        return;
      }
      this._done();
    },
    _onSubmitFailure: function(o) {
      this._result = this._onSubmitComplete(false, {});
      if (!this._result) {
        this._onError(null, YAHOO.util.Connect.formatRequestError(o));

        this._fitDialog();
        Whispercast.Progress.hide(this._cfg.id);
        return;
      }
      this._done();
    },

    _onCancel: function() {
      this._done();
    },

    _onDone: function() {
    },

    _onError: function(field, error) {
      if (!YAHOO.lang.isArray(error)) {
        error = [error];
      }

      if (field) {
        var f = field+'-proxy';
        var v = 0;

        var element = document.getElementById(f);
        if (!element || (YAHOO.util.Dom.getStyle(element, 'display') == 'none')) {
          f = field;
          v = null;

          element = document.getElementById(f);
        }
        if (element) {
          if (YAHOO.util.Dom.getStyle(element, 'display') != 'none') {
            if (YAHOO.util.Dom.getStyle(field, 'display') == 'none') {
              Whispercast.Errors.setMessage(f, error);
              v = 1;
            } else {
              Whispercast.Errors.setMessage(f, error, field);
            }
            this._errors[field] = v;
            return true;
          }
        }
        return false;
      }

      if (this._elements.errors) {
        if (this._elements.errors.childNodes.length == 0) {
          this._elements.errors.appendChild(document.createElement('hr'));
        }

        var e,d;
        for (var i = 0; i < error.length; i++) {
          e = error[i].replace(new RegExp('[\\s]+$', 'g'), '');
          if (e[e.length-1] != '.') {
            e += '.';
          }
          d = document.createElement('div');
          d.appendChild(document.createTextNode(e));
          this._elements.errors.appendChild(d);

          this._errors[field] = d;
        }
        return true;
      }
      return false;
    }
  });
}

}());
