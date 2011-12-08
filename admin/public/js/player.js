(function() {

/* The Player Whispercast namespace */
if (Whispercast.Player == undefined) {
  Whispercast.Player = {
  };

  var _cfgDefaults = {
    swfPlayer: '/player.swf',
    swfExpressInstall: '/expressinstall.swf',
    width: 320,
    height: 240,
    backgroundColor: '#000000',
  };

  /* Base form class */
  Whispercast.Player.Base = function(cfg) {
    Whispercast.Player.Base.superclass.constructor.call(this, cfg, _cfgDefaults);
  };
  YAHOO.lang.extend(Whispercast.Player.Base, Whispercast.Base, {
    isCreated: function() {
      return this._created;
    },

    create: function() {
      if (!this._created) {
        var vars = {
        };
        if (this._cfg.vars) {
          Whispercast.log(this._cfg.vars, 'PLAYER');
          for (var i in this._cfg.vars) {
            vars[i] = escape(this._cfg.vars[i]);
          }
        }

        YAHOO.util.Dom.setStyle(this._cfg.id, 'width', this._cfg.width+'px');
        YAHOO.util.Dom.setStyle(this._cfg.id, 'height', this._cfg.height+'px');

        swfobject.embedSWF(
          Whispercast.Uri.base+'/swf'+this._cfg.swfPlayer,
          this._cfg.id,
          this._cfg.width,
          this._cfg.height,
          '10',
          Whispercast.Uri.base+'/swf'+this._cfg.swfExpressInstall,
          vars,
          {
            quality: 'high',
            play: 'true',
            loop: 'true',
            scale: 'noscale',
            wmode: 'window',
            devicefont: 'false',
            menu: 'false',
            allowfullscreen: 'true',
            allowscriptaccess: 'always',
            salign: 'lt',
            bgcolor: this._cfg.backgroundColor
          },
          {
            id: this._cfg.id+'_player'
          }
        );
        this._created = true;
      }
    },
    destroy: function() {
      var player = document.getElementById(this._cfg.id+'_player');
      if (player) {
        try {
          player.setURL_(0, null, null);
        } catch(e) {
        }

        try
        {
        // Break circular references.
        (function (o) {
        var a = o.attributes, i, l, n, c;
        if (a) {
        l = a.length;
        for (i = 0; i <l; i += 1) {
        n = a[i].name;
        if (typeof o[n] === 'function') {
        o[n] = null;
        }
        }
        }
        a = o.childNodes;
        if (a) {
        l = a.length;
        for (i = 0; i <l; i += 1) {
        c = o.childNodes[i];
        // Purge child nodes.
        arguments.callee(c);
        }
        }
        }
        )(player);
        }
        catch(e)
        {
        }
        player.parentNode.removeChild(player);
      }
      delete this._created;
    }
  });
}

})();
