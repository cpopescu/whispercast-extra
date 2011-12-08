(function() {
/* The Resolver Whispercast namespace */
if (Whispercast.Resolver == undefined) {
  Whispercast.Resolver = {
    _modules: {},
    _resolvedModules: {},

    addModule: function(name, type, source, dependencies) {
      Whispercast.Resolver._modules[name] = {
        type: type,
        source: source,
        dependencies: dependencies
      }
    },
    resolve: function(modules, data, callbacks) {
      return (new Whispercast.Resolver.Base({modules:modules, data:data, callbacks:callbacks})).run();
    }
  };

  /* Base resolver class */
  Whispercast.Resolver.Base = Whispercast.Resolver.Base ? Whispercast.Resolver.Base : function(cfg) {
    this._initialize(cfg ? Whispercast.clone(cfg) : {});
  };
  Whispercast.Resolver.Base.prototype = {
    run: function() {
      if (!this._running) {
        this._modulesResolving = false;
        this._modulesResolved = false;

        this._dataResolving = {};
        this._dataResolved = {};

        for (var i in this._cfg.data) {
          this._dataResolving[this._cfg.data[i]] = false;
          this._dataResolved[this._cfg.data[i]] = false;
        }
        this._running = true;

        this._run();
      }
    },
    isRunning: function() {
      return this._running;
    },

    _initialize: function(cfg) {
      this._cfg = cfg;

      this._cfg.modules = this._cfg.modules ? (YAHOO.lang.isArray(this._cfg.modules) ? this._cfg.modules : [this._cfg.modules]) : [];
      this._cfg.data = this._cfg.data ? (YAHOO.lang.isArray(this._cfg.data) ? this._cfg.data : [this._cfg.data]) : [];

      this._cfg._callbacks = this._cfg.callbacks ? this._cfg.callbacks : {};

      this._running = false;
    },

    _calculateDependencies: function(loader, name) {
      var count = Whispercast.Resolver._resolvedModules[name] ? 0 : 1;
      if (Whispercast.Resolver._modules[name] != undefined) {
        var module =  Whispercast.Resolver._modules[name];
        for (var i =0, len = module.dependencies.length; i < len; ++i) {
          count += this._calculateDependencies(loader, module.dependencies[i]); 
        }
        loader.addModule({
          name: name,
          type: module.type,
          requires: module.dependencies,
          fullpath: Whispercast.Uri.base+'/'+module.type+'/'+module.source 
        });
      }
      return count;
    },

    _run: function() {
      if (!this._modulesResolving) {
        if (this._cfg.modules.length > 0) {
          this._modulesResolving = true;
          var loader = new YAHOO.util.YUILoader({
            base: Whispercast.Uri.base+'/yui/',

            loadOptional: true,

            filter: 'MIN',
            allowRollup: true,

            onSuccess: this._onModuleSuccess,
            onFailure: this._onModuleFailure,
            scope: this
          }
          );
          Whispercast.overload(loader, 'loadNext', function(previous, name) {
            if (name) {
              Whispercast.log('Resolved: ' + name, 'RESOLVER');
              Whispercast.Resolver._resolvedModules[name] = true;
              switch (name) {
                case 'container':
                  var FADE =  YAHOO.widget.ContainerEffect.FADE;
                  YAHOO.widget.ContainerEffect.FADE = function(overlay, dur) {
                    var fade = FADE(overlay, dur);
                    Whispercast.log('Overloaded YAHOO.widget.ContainerEffect.FADE', 'APPLICATION');
                    fade.animIn.onStart.subscribe(function() {
                      if (overlay.mask) {
                        YAHOO.util.Dom.setStyle(overlay.mask, 'opacity', 0.1);
                      }
                    }, fade);
                    fade.animOut.onTween.subscribe(function() {
                      if (overlay.mask) {
                        YAHOO.util.Dom.setStyle(overlay.mask, 'opacity', YAHOO.util.Dom.getStyle(overlay.element, 'opacity')/10);
                      }
                    }, fade);
                    return fade;
                  }
                  break;
              }
            }
            previous.call(this, name);
          });

          var count = 0;
          for (var i = 0, len = this._cfg.modules.length; i < len; ++i) {
            var name = this._cfg.modules[i];

            var dcount = this._calculateDependencies(loader, name);
            if (dcount > 0) {
              Whispercast.log('Resolving: ' + name, 'RESOLVER');
              loader.require(name);
            } else {
              Whispercast.log('Skipping: ' + name, 'RESOLVER');
            }
            count += dcount;
          }
          if (count > 0) {
            loader.insert();
            return false;
          }
        }
        this._modulesResolved = true;
      }
      if (!this._modulesResolved) {
        return false;
      }

      for (var i in this._dataResolving) {
        this._dataResolved[i] = 
        this._cfg.callbacks['resolve'+i]({
          success: this._onDataSuccess,
          failure: this._onDataFailure,
          scope: this,
          argument: i
        });
        delete(this._dataResolving[i]);
      }

      var result = true;
      for (var i in this._dataResolved) {
        if (!this._dataResolved[i]) {
          result = false;
        } else {
          delete(this._dataResolved[i]);
        }
      }
      if (result) {
        this._done(this._cfg.callbacks.done);
      }
      return result;
    },
    _done: function(callback) {
      this._running = false;
      
      delete this._modulesResolving;
      delete this._modulesResolved;

      delete this._dataResolving;
      delete this._dataResolved;

      if (callback) {
        var args = [];
        for (var i = 1; i < arguments.length; i++ ) {
          args.push(arguments[i]);
        }
        callback.apply(null, args);
      }
    },

    _onModuleSuccess: function(o) {
      this._modulesResolved = true;
      if (this._cfg.callbacks.success) {
        this._cfg.callbacks.success(o.argument, o, false);
      }

      setTimeout(Whispercast.closure(this, this._run), 0);
    },
    _onModuleFailure: function(o) {
      this._done(this._cfg.callbacks.failure, o.argument, o, false);
    },

    _onDataSuccess: function(o) {
      this._dataResolved[o.argument] = true;
      if (this._cfg.callbacks.success) {
        this._cfg.callbacks.success(o.argument, o, true);
      }
      if (this._cfg.callbacks.data && this._cfg.callbacks.data[o.argument] && this._cfg.callbacks.data[o.argument].success) {
        this._cfg.callbacks.data[o.argument].success.apply(null, [o.argument, o, o.responseText]);
      }

      setTimeout(Whispercast.closure(this, this._run), 0);
    },
    _onDataFailure: function(o) {
      if (this._cfg.callbacks.data && this._cfg.callbacks.data[o.argument] && this._cfg.callbacks.data[o.argument].failure) {
        this._cfg.callbacks.data[o.argument].failure.apply(null, [o.argument, o]);
      }
      this._done(this._cfg.callbacks.failure, o.argument, o, true);
    }
  };
}

}());
