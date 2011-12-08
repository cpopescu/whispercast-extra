(function() {

/* The Uuid Whispercast namespace */
if (Whispercast.Uuid == undefined) {
  /* Private stuff */
  var _chars  = '0123456789ABCDEF'.split('');

  Whispercast.Uuid = {
    generate: function() {
      var uuid = [];
      for (var i = 0; i < 16; i++) {
        uuid[i] = _chars[0 | Math.random()*16];
      }
      return uuid.join('');
    }
  }
}

}());
