(function() {
/* YUI shortcuts */
var Dom = YAHOO.util.Dom;

/* The Uploader Whispercast namespace */
if (Whispercast.Uploader == undefined) {
  Whispercast.Uploader = {
  };

  /* Base uploader class */
  Whispercast.Uploader.Base = Whispercast.Uploader.Base ? Whispercast.Uploader.Base : function(cfg) {
    this._initialize(cfg ? cfg : {});
  };
  Whispercast.Uploader.Base.prototype = {
    _initialize: function(cfg) {
      this._cfg = cfg;

      this._files = {};
      
      var region = Dom.getRegion(this._cfg.id+'-select-select');
      Dom.setStyle(this._cfg.id+'-select-overlay', 'width', region.right-region.left + 'px');
      Dom.setStyle(this._cfg.id+'-select-overlay', 'height', region.bottom-region.top + 'px');

      YAHOO.widget.Uploader.SWFURL = Whispercast.Uri.base + '/yui/uploader/assets/uploader.swf';

      this._uploader = new YAHOO.widget.Uploader(this._cfg.id+'-select-overlay');
      this._uploader.addListener('contentReady', this._onLoaded, this, true);

      this._uploader.addListener('fileSelect', this._onFileSelect, this, true);
      this._uploader.addListener('uploadStart', this._onFileStart, this, true);
      this._uploader.addListener('uploadCompleteData', this._onFileComplete, this, true);
      this._uploader.addListener('uploadError', this._onFileError, this, true);
      this._uploader.addListener('uploadCancel', this._onFileCancel, this, true);
      this._uploader.addListener('uploadProgress', this._onFileProgress, this, true);
    },

    upload: function(id, url, method, data, name) {
      this._uploader.upload(id, url, method, data, name);
    },
    remove: function(id) {
      if (this._files[id]) {
        this._uploader.cancel(id);
        this._uploader.removeFile(id);

        this._cfg.callbacks.remove ? this._cfg.callbacks.remove.call(null, id, this._files[id]) : false;
        delete this._files[id];
      }
      this._cfg.callbacks.update ? this._cfg.callbacks.update.call(null, this._files) : false;
    },

    _onFileSelect: function(e) {
      for (var file in e.fileList) {
        if (YAHOO.lang.hasOwnProperty(e.fileList, file)) {
          var id = e.fileList[file].id;

          if (this._files[id] == undefined) {
            this._files[id] = {id:id,name:e.fileList[file].name,status:'idle',progress:0};
            this._cfg.callbacks.add ? this._cfg.callbacks.add.call(null, id, this._files[id]) : false;
          }

          this._cfg.callbacks.update ? this._cfg.callbacks.update.call(null, this._files) : false;
        }
      }
    },
    _onFileStart: function(e) {
      var file = this._files[e.id];
      if (file) {
        file.status = 'uploading';
        file.progress = 0;
        this._cfg.callbacks.start ? this._cfg.callbacks.start.call(null, e.id, file): false;
        this._cfg.callbacks.progress ? this._cfg.callbacks.progress.call(null, e.id, file) : false;
      }
    },
    _onFileComplete: function(e) {
      var file = this._files[e.id];
      if (file) {
        if (file.status != 'complete') {
          file.status = 'complete';
          file.progress = 1;
          this._cfg.callbacks.progress ? this._cfg.callbacks.progress.call(null, e.id, file) : false;
          this._cfg.callbacks.complete ? this._cfg.callbacks.complete.call(null, e.id, file, e.data ? e.data : null) : false;
        }
      }
    },
    _onFileError: function(e) {
      var file = this._files[e.id];
      if (file) {
        if (file.status != 'error') {
          file.status = 'error';
          this._cfg.callbacks.error ? this._cfg.callbacks.error.call(null, e.id, file) : false;
        }
      }
    },
    _onFileCancel: function(e) {
      var file = this._files[e.id];
      if (file) {
        file.status = 'canceled';
        this._cfg.callbacks.cancel ? this._cfg.callbacks.cancel.call(null, e.id, file) : false;
      }
    },
    _onFileProgress: function(e) {
      var file = this._files[e.id];
      if (file) {
        file.progress = e.bytesLoaded/e.bytesTotal;
        this._cfg.callbacks.progress ? this._cfg.callbacks.progress.call(null, e.id, file) : false;
      }
    },

    _onLoaded: function() {
      this._uploader.setAllowMultipleFiles(true);
      this._uploader.setSimUploadLimit(1);
    }
  }
}
YAHOO.widget.Uploader.SWFURL = Whispercast.Uri.base+'/swf/uploader.swf?rnd='+Math.random();

}());
