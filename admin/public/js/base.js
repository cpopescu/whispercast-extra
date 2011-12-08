function _(s) {
  return s;
}
(function() {

var _reHtmlEscape = new RegExp('[&<>"]', 'img');

/* The base Whispercast namespace */
Whispercast = (typeof(Whispercast) != 'undefined') ? Whispercast : {
  htmlEscape: function(s) {
    if (s) {
      return (''+s).replace(
        _reHtmlEscape,
        function(c) {
          switch (c) {
            case '&':
              return '&amp;';
            case '<':
              return '&lt;';
            case '>':
              return '&gt;';
            case '"':
              return '&quot;';
          }
          return c;
        }
      );
    }
    return s;
  },
  htmlUnescape: function(s) {
  },

  closure: function(scope, callback) {
    var bound = [];
    for (var i = 2; i < arguments.length; i++) {
      bound.push(arguments[i]);
    }
    var f = function() {
      var args = bound.slice(0);
      for (var i = 0; i < arguments.length; i++) {
        args.push(arguments[i]);
      }
      return callback.apply(scope, args);
    }

    f.toString = function() {
      return ''+callback;
    }
    return f;
  },

  overload: function(o, method, overload) {
    var previous;
    if (o) {
      previous = o[method];
    } else {
      previous = method;
    }
    var overloaded = function() {
      var args = [previous];
      for (var i = 0; i < arguments.length; i++) {
        args.push(arguments[i]);
      }
      return overload.apply(o, args);
    }

    if (o) {
      o[method] = overloaded;
    }
    return overloaded;
  },

  merge: function(d, s) {
    if (s.constructor == Object) {
      var r = d ? d : {};
      for (var i in s) {
        r[i] = Whispercast.merge(r[i], s[i]);
      }
      d = r;
    } else {
      d = s;
    }
    return d;
  },
  clone: function(o) {
    if (!o || (typeof(o) != 'object')) {
      return o;
    }

    if (typeof(o.clone) == 'function') {
      return o.clone();
    }

    if (o.constructor == Array) {
      return [].concat(o);
    }
    if (o.constructor == Object) {
      var r = {};
      for (var i in o) {
        r[i] = Whispercast.clone(o[i]);
      }
      return r;
    }
    return o;
  },

  strformat: function(s) {
    var b = s.split('%');
    var o = b[0];
    var re = /^([0-9]+)(.*)$/;
    for (var i=1; i<b.length; i++) {
      p = re.exec(b[i]);
      if (!p) {
        o += '%'+b[i];
        continue;
      }
      var j = parseInt(p[1],10);
      if (j < arguments.length) {
        o += arguments[j];
      }
      o += p[2];
    }
    return o;
  },

  eval: function(s, scope) {
    return new Function(s).call((scope != undefined) ? scope : null);
  }
};

Whispercast.Base = function(cfg, defaults) {
  var _cfg = defaults ? Whispercast.clone(defaults) : {};
  
  for (var i in cfg) {
    _cfg[i] = cfg[i];
  }
  delete _cfg.overloads;

  if (cfg.overloads) {
    for (var i in cfg.overloads) {
      Whispercast.overload(this, i, cfg.overloads[i]);
    }
  }

  this._private = {};

  this._initialize(_cfg);
};
Whispercast.Base.prototype = {
  _initialize: function(cfg) {
    this._cfg = cfg;
  }
};

}());
