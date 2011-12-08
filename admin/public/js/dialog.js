(function() {

/* The Dialog Whispercast namespace */
if (Whispercast.Dialog == undefined) {
  Whispercast.Dialog = {
    _zIndex: 1000
  };

  var _cfgDefaults = {
    title: '',
    width: 'auto',
    modal: true,
    close: true,
    iframe: true,
    fixedcenter: true
  };
  var _manager = null;

  /* Base dialog class */
  Whispercast.Dialog.Base = Whispercast.Dialog.Base ? Whispercast.Dialog.Base : function(cfg) {
    Whispercast.Dialog.Base.superclass.constructor.call(this, cfg, _cfgDefaults);
  };
  YAHOO.lang.extend(Whispercast.Dialog.Base, Whispercast.Base, {
    isCreated: function() {
      return this._dialog != undefined;
    },
    isRunning: function() {
      return (this._dialog != undefined) && this._dialog.visible;
    },

    getDialog: function() {
      return this._dialog;
    },

    create: function() {
      if (_manager == null) {
        _manager = new YAHOO.widget.OverlayManager();
      }

      if (!this._dialog) {
        this._create();

        this._dialog.setHeader(this._cfg.title);
        if (this._cfg.body) {
          this._dialog.setBody(this._cfg.body);
        }
        this._dialog.render(document.body);

        this._dialog.showEvent.subscribe(Whispercast.closure(this, function() {
          this._onShow();
        }));
        this._dialog.hideEvent.subscribe(Whispercast.closure(this, function() {
          this._onHide();
        }));

        this._onCreate();
      }
    }, 
    destroy: function() {
      if (this._dialog) {
        this._destroy();

        this._onDestroy();
      } 
    },

    show: function() {
      this._dialog ? this._dialog.show() : false;
    },
    hide: function() {
      this._dialog ? this._dialog.hide() : false;
    },

    _create: function() {
      this._dialog = new YAHOO.widget.Dialog(this._cfg.id, this._getDialogCfg());
      document.getElementById(this._cfg.id).style.position = 'relative';

      // TODO: Hack to allow us manage z-indices
      this._dialog.bringToTop = function() {}

      //_manager.register(this._dialog);

      Whispercast.log('Dialog z-index: ' + Whispercast.Dialog._zIndex + ' ('+this._cfg.title+')', 'DIALOG');
      Whispercast.Dialog._zIndex += 2;
    },
    _destroy: function() {
      var dialog = this._dialog;
      delete this._dialog;

      Whispercast.Dialog._zIndex -= 2;
      //_manager.remove(dialog);

      setTimeout(Whispercast.closure(this, function(dialog) {
        dialog.destroy();
      }, dialog), 0);
    },

    _getDialogCfg: function() {
      var buttons;
      if (this._cfg.buttons == undefined) {
        buttons =[{
          text: 'Închide',
          handler: Whispercast.closure(this, this.hide),
          isDefault: true
        }];
      } else {
        buttons = (this._cfg.buttons.length == 0) ? null : this._cfg.buttons;
      }
      return {
        modal: this._cfg.modal,
        visible: false,
        draggable: true,
        close: this._cfg.close,
        postmethod: 'manual',
        fixedcenter: this._cfg.fixedcenter,
        constraintoviewport: true,
        hideaftersubmit: false,
        buttons: buttons,
        width: this._cfg.width,
        zIndex: Whispercast.Dialog._zIndex,
        iframe: this._cfg.iframe,
        effect: this._cfg.effect ? this._cfg.effect : {effect:YAHOO.widget.ContainerEffect.FADE,duration:0.1}
      }
    },

    _onCreate: function() {
      Whispercast.log('Creating "'+this._cfg.title+'"...', 'DIALOG');
    },
    _onDestroy: function() {
      Whispercast.log('Destroying "'+this._cfg.title+'"...', 'DIALOG');
    },

    _onShow: function() {
      this._dialog.cfg.setProperty('fixedcenter', false);
      Whispercast.log('Showing "'+this._cfg.title+'"...', 'DIALOG');
    },
    _onHide: function() {
      Whispercast.log('Hiding "'+this._cfg.title+'"...', 'DIALOG');
    }
  });

  /* Prompt dialog class */
  Whispercast.Dialog.Prompt = Whispercast.Dialog.Prompt ? Whispercast.Dialog.Prompt : function(title, body, typeOrButtons, callback) {
    var cfg = {
      modal: true,
      title: title,
      body: body
    };

    var defaultHandler = Whispercast.closure(this, function() {this.hide()});
    switch (typeOrButtons) {
      case 'E':
        cfg.icon = YAHOO.widget.SimpleDialog.ICON_BLOCK;
        break;
      case 'I':
      case undefined:
        cfg.icon = YAHOO.widget.SimpleDialog.ICON_INFO;
        break;
      default:
        if (typeOrButtons) {
          cfg.buttons = [];
          for (var text in typeOrButtons) {
            var definition = typeOrButtons[text];
            cfg.buttons.push({
              text: _(text),
              handler: definition.handler ? definition.handler : defaultHandler,
              isDefault: (definition.isDefault != undefined) ? definition.isDefault : false
            });
          }
        }
    }

    if (!cfg.buttons) {
      cfg.buttons = [{
        text: _('Close'),
        handler: callback ? callback : defaultHandler,
        isDefault: true
      }];
    }

    Whispercast.Dialog.Prompt.superclass.constructor.call(this, cfg);

    this.create();
    this.show();
  };
  YAHOO.lang.extend(Whispercast.Dialog.Prompt, Whispercast.Dialog.Base, {
    _create: function() {
      this._dialog = new YAHOO.widget.SimpleDialog('WhispercastPrompt'+Whispercast.Dialog._zIndex, this._getDialogCfg());
    },
    _onCreate: function() {
      if (this._cfg.icon) {
        this._dialog.cfg.setProperty('icon', YAHOO.widget.SimpleDialog.ICON_BLOCK);
      }
    },
    _onHide: function() {
      setTimeout(Whispercast.closure(this, function() {
        this.destroy();
      }), 0);
    }
  });
}

}());
