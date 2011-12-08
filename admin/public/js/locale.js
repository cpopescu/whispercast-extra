(function() {

var _reTime = new RegExp('^((?:[0-1]?[0-9])|(?:2[0-3]))(?::([0-5][0-9])(?::([0-5][0-9]))?)?$', 'i');
var _reDate = new RegExp('^((?:[0-2]?[0-9])|(?:3[0-1]))/((?:0{0,1}[1-9])|(?:1[0-2]))/([0-9]*)$', 'i');

/* The Locale Whispercast namespace */
if (Whispercast.Locale == undefined) {
  Whispercast.Locale = {
    monthNames: Date.CultureInfo.monthNames,
    monthNamesShort: Date.CultureInfo.abbreviatedMonthNames,
    dayNames: Date.CultureInfo.dayNames,
    dayNamesShort: Date.CultureInfo.abbreviatedDayNames,

    parseTime: function(s) {
      var t = Date.parseExact(s, [
        Date.CultureInfo.formatPatterns.longTime,
        Date.CultureInfo.formatPatterns.shortTime,
        'hh:mm:ss tt',
        'hh:mm tt',
        'hh tt',
        'h:mm:ss tt',
        'h:mm tt',
        'h tt',
        'HH:mm:ss',
        'HH:mm',
        'HH',
        'H:mm:ss',
        'H:mm',
        'H'
      ]);
      if (t) {
        return {hour:t.getHours(),minute:t.getMinutes(),second:t.getSeconds()};
      }
      // fallback
      var r = _reTime.exec(s);
      if (r != null) {
        return {hour:parseInt(r[1],10),minute:r[2] ? parseInt(r[2],10) : 0,second:r[3] ? parseInt(r[3],10) :0};
      }
      return null;
    },
    parseDate: function(s) {
      var d = Date.parseExact(s, [Date.CultureInfo.formatPatterns.longDate, Date.CultureInfo.formatPatterns.shortDate]);
      if (d) {
        return {year:d.getFullYear(),month:d.getMonth(),day:d.getDate()};
      }
      // fallback
      var r = _reDate.exec(s);
      if (r != null) {
        return {year:parseInt(r[3],10),month:parseInt(r[2],10),day:parseInt(r[3],10)};
      }
      return null;
    },

    formatTime: function(d, full) {
      return d.toString(full ? Date.CultureInfo.formatPatterns.longTime : Date.CultureInfo.formatPatterns.shortTime);
    },
    formatDate: function(d, full) {
      return d.toString(full ? Date.CultureInfo.formatPatterns.longDate : Date.CultureInfo.formatPatterns.shortDate);
    },

    formatDateTime: function(d) {
      return d.toString(Date.CultureInfo.formatPatterns.fullDateTime);
    }
  }
}

}());
