(function() {
/* The Notifier Whispercast subjectspace */
if (Whispercast.Notifier == undefined) {
  /* Private stuff */
  var _subjects = {};
  var _peers = [];
  
  Whispercast.Notifier = {
    registerListener: function(subject, callback) {
      if (_subjects[subject] == undefined) {
        _subjects[subject] = [];
      }
      _subjects[subject].push(callback);
      Whispercast.log('Registering listener for "'+subject+'"...', 'NOTIFIER');
    },
    unregisterListener: function(subject, callback) {
      var callbacks = (_subjects[subject] != undefined) ? _subjects[subject] : null;
      if (callbacks) {
        for (var i = 0; i< callbacks.length; i++) {
          if (callback == callbacks[i]) {
            callbacks.splice(i, 1);
            i--;
          }
        }
        if (callbacks.length == 0) {
          delete _subjects[subject];
        }
      }
    },

    hasListener: function(subject) {
      return _subjects[subject] != undefined;
    },

    registerPeer: function(peer) {
      _peers.push(peer);
    },
    unregisterPeer: function(peer) {
      for (var i = 0; i < _peers.length; i++) {
        if (peer == _peers[i]) {
          _peers.splice(i, 1);
          i--;
        }
      }
    },

    notify: function(subject) {
      Whispercast.log('Window "'+window.name+'" is sending notification "'+subject+'"...', 'NOTIFIER');

      var args = [subject, {}];
      for (var i = 1; i < arguments.length; i++) {
        args.push(arguments[i]);
      }
      Whispercast.Notifier._notify.apply(window, args);
      Whispercast.log('Window "'+window.name+'" has sent notification "'+subject+'"...', 'NOTIFIER');
    },
    _notify: function(subject, visited) {
      Whispercast.log('"'+window.name+'" is being notified of "'+subject+'"...', 'NOTIFIER');
      visited[window.name] = true;
      
      var args = [];
      for (var i = 2; i < arguments.length; i++) {
        args.push(arguments[i]);
      }

      if (_subjects[subject]) {
        var callbacks = _subjects[subject];
        for (var i = 0; i< callbacks.length; i++) {
          callbacks[i].apply(null, args);
        }
      }

      args.unshift(subject, visited);
      for (var i = 0; i < _peers.length; i++) {
        if (_peers[i].closed) {
          _peers.splice(i, 1);
          i--;

          continue;
        }

        if (visited[_peers[i].name]) {
          continue;
        }
        _peers[i].Whispercast.Notifier._notify.apply(_peers[i], args);
      }
    }
  };
}

}());
