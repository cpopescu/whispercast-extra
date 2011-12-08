(function() {
/* The Sandbox Whispercast namespace */
if (Whispercast.Sandbox == undefined) {
  var _index = 0;

  Whispercast.Sandbox = {
  };

 /* Base sandbox class */
  Whispercast.Sandbox.Base = function(initialized, destroyed, initialize) {
    Whispercast.log('Sandbox create...', 'SANDBOX');
    this.initialized = function() {
      if (initialized) {
        initialized.call(null);
      }
      delete this.initialized;
      Whispercast.log('Sandbox initialized...', 'SANDBOX');
    };
    this.destroyed = function() {
      this.initialized ? this.initialized() : false;

      if (destroyed) {
        destroyed.call(null);
      }
      delete this.destroyed;
      Whispercast.log('Sandbox destroyed...', 'SANDBOX');
    };
  };
  Whispercast.Sandbox.Base.prototype = {
    destroy: function() {
      this.destroyed ? this.destroyed() : false;
    },

    run: function() {
      this.initialized ? this.initialized() : false;
    }
  };
}

}());
