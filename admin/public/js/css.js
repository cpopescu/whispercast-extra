(function() {

/* The Css Whispercast namespace */
if (Whispercast.Css == undefined) {
  Whispercast.Css = {
    addSheet: function(source) {
      var sheet;
      if (source) {
        sheet = document.createElement('link');
        sheet.href = source;
        sheet.rel = 'stylesheet';
      } else {
        sheet = document.createElement('style');
      }
      sheet.type = 'text/css';
      document.getElementsByTagName('head')[0].appendChild(sheet);
      return sheet;
    },
    addRule: function(sheet, selector, rule) {
      if (sheet.styleSheet) {
      if (sheet.styleSheet.cssText == '') {
        sheet.styleSheet.cssText = '';
      }
      sheet.styleSheet.cssText += selector + ' { '+rule+' }';
      } else {
        sheet.appendChild(document.createTextNode(selector + ' { '+rule+' }'));
      }
    }
  }
}

}());
