(function() {
/* YUI shortcuts */
var Dom = YAHOO.util.Dom;

/* The Errors Whispercast namespace */
if (Whispercast.Errors == undefined) {
  Whispercast.Errors = {
    setMessage: function(id, message, field) {
      Whispercast.Errors.clearMessage(id, field);

      field = field ? field : id; 

      var outer = document.createElement('div');
      Dom.setAttribute(outer, 'id', id+'-error-message');
      Dom.setAttribute(outer, 'class', 'error');

      var error = document.createElement('div');
      Dom.setAttribute(error, 'class', 'list');
      Dom.setStyle(error, 'position', 'absolute');
      Dom.setStyle(error, 'white-space', 'nowrap');
      outer.appendChild(error);

      var filler = document.createElement('div');
      Dom.setAttribute(filler, 'class', 'filler');
      outer.appendChild(filler);

      if (!YAHOO.lang.isObject(message)) {
        message = {0:message};
      }
      for (var i in message) {
        var m = message[i].replace(new RegExp('[\\s]+$', 'g'), '');
        if (m[m.length-1] != '.') {
          m += '.';
        }

        var item = document.createElement('div');
        Dom.setStyle(item, 'height', '1.25em');
        item.appendChild(document.createTextNode(m));
        error.appendChild(item);

        var fill = document.createElement('div');
        Dom.setStyle(fill, 'height', '1.25em');
        filler.appendChild(fill);
      }

      document.getElementById(id).parentNode.appendChild(outer);
      Dom.addClass(field, 'invalid');
    },
    clearMessage: function(id, field) {
      field = field ? field : id; 

      var error = document.getElementById(id+'-error-message');
      if (error) {
          error.parentNode.removeChild(error);
      }
      Dom.removeClass(field, 'invalid');
    }
  };
}

}());
