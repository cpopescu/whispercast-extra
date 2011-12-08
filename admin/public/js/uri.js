(function() {
/* Private stuff */
var _safe =
  '0123456789' +
  'ABCDEFGHIJKLMNOPQRSTUVWXYZ' +
  'abcdefghijklmnopqrstuvwxyz' +
        '-_.!~*\'()';
var _hexa =
  '0123456789ABCDEF';

var _parser = {
  strict: /^(?:([^:\/?#]+):)?(?:\/\/((?:(([^:@]*):?([^:@]*))?@)?([^:\/?#]*)(?::(\d*))?))?((((?:[^?#\/]*\/)*)([^?#]*))(?:\?([^#]*))?(?:#(.*))?)/,
  loose:  /^(?:(?![^:@]+:[^:@\/]*@)([^:\/?#.]+):)?(?:\/\/)?((?:(([^:@]*):?([^:@]*))?@)?([^:\/?#]*)(?::(\d*))?)(((\/(?:[^?#](?![^?#\/]*\.[^?#\/.]+(?:[?#]|$)))*\/?)?([^?#\/]*))(?:\?([^#]*))?(?:#(.*))?)/,

  query: /(?:^|&)([^&=]*)=?([^&]*)/g
};
var _keys = [
  'source','protocol','authority','userInfo','user','password','host','port','relative','path','directory','file','query','anchor'
];

/* The Uri Whispercast namespace */
if (Whispercast.Uri == undefined) {
  var _encodeQuery = function(k, v, q) {
    if (YAHOO.lang.isArray(v)) {
      for (var j = 0; j < v.length; j++) {
        _encodeQuery(k+'['+j+']', v[j], q);
      }
    } else
    if (YAHOO.lang.isObject(v)) {
      for (var j in v) {
        _encodeQuery(k+'['+j+']', v[j], q);
      }
    } else {
      q.push(k+'='+Whispercast.Uri.urlencode(v));
    }
  };

  Whispercast.Uri = {
    base: '',

    urlencode: function(plain) {
      if (typeof(plain) == 'boolean')
        plain = plain ? 1 : 0;
      plain = '' + plain;

      var encoded = '';
      for (var i = 0; i < plain.length; i++ ) {
        var ch = plain.charAt(i);
          if (ch == ' ') {
            encoded += '+';       // x-www-urlencoded, rather than %20
        } else if (_safe.indexOf(ch) != -1) {
            encoded += ch;
        } else {
            var charCode = ch.charCodeAt(0);
          if (charCode > 255) {
            encoded += '+';
          } else {
            encoded += '%';
            encoded += _hexa.charAt((charCode >> 4) & 0xF);
            encoded += _hexa.charAt(charCode & 0xF);
          }
        }
      }
      return encoded;
    },
    urldecode: function(encoded) {
      var _hexaCHARS = '0123456789ABCDEFabcdef'; 

      var plain = '';
      var i = 0;
      while (i < encoded.length) {
          var ch = encoded.charAt(i);
        if (ch == '+') {
            plain += ' ';
          i++;
        } else if (ch == '%') {
          if (i < (encoded.length-2) 
              && _hexaCHARS.indexOf(encoded.charAt(i+1)) != -1 
              && _hexaCHARS.indexOf(encoded.charAt(i+2)) != -1 ) {
            plain += unescape( encoded.substr(i,3) );
            i += 3;
          } else {
            plain += '%[ERROR]';
            i++;
          }
        } else {
          plain += ch;
          i++;
        }
      } // while
      return plain;
    },

    parse: function(uri, parse_query, strict) {
      parse_query = (parse_query == undefined) ? true : parse_query;
      strict = (strict == undefined) ? true : strict;

      var raw  = _parser[strict ? 'strict' : 'loose'].exec(uri);
      var result = {};

      var i = 14;
      while (i--) result[_keys[i]] = raw[i] || '';

      if (parse_query) {
        var query = {};
        result[_keys[12]].replace(_parser['query'], function ($0, $1, $2) {
          if ($1) query[$1] = Whispercast.Uri.urldecode($2);
        });
        result[_keys[12]] = query;
      }
      return result;
    },
    build: function(uri, query, ignore_base) {
      if (!YAHOO.lang.isObject(uri)) {
        uri = Whispercast.Uri.parse(uri);
      }

      if (query) {
        for (var i in query) {
          uri.query[i] = query[i];
        }
      }

      var q = [];
      for (var i in uri.query) {
        _encodeQuery(i, uri.query[i], q);
      }
      q = q.join('&');

      if (uri.relative == uri.source) {
        return (ignore_base ? '' : Whispercast.Uri.base)+(uri.path.substr(0,1) == '/' ? '' : '/')+uri.path+(q ? '?'+q : '')+(uri.anchor ? '#'+uri.anchor : '');
      }
      return (uri.protocol ? uri.protocol+'://' : '')+(uri.userInfo ? uri.userInfo+'@' : '')+uri.host+(uri.port ? ':'+uri.port : '')+(uri.path.substr(0,1) == '/' ? '' : '/')+uri.path+(q ? '?'+q : '')+(uri.anchor ? '#'+uri.anchor : '');
    }
  };
}

}());
