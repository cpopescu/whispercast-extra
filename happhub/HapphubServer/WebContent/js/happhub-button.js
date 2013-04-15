!(function (happhub) {
  var l_defaultOptions = {};
  var Button = function($content, options) {
    this.options = $.extend({}, l_defaultOptions, options);
    
    this.$content = $content;
    this.$content.addClass('has-button');

    var self = this;
    
    this.$ = $(this.options.tag ? '<'+this.options.tag+'/>' : '<button/>').
      addClass('btn').
      addClass(this.options.clss ? this.options.clss : '').
      css(this.options.style ? this.options.style : {}).
      attr(this.options.attributes ? this.options.attributes : {}).
      button().
      click(function() {
        if (self.options.event) {
          $(self.options.eventTarget ? self.options.eventTarget : document).trigger(self.options.event, self.options.eventData);
        } else
        if (self.options.clickCB) {
          self.options.clickCB();
        }
      }).
      append(this.$content);
  };
  Button.prototype.destroy = function() {
    this.$.remove();
    
    this.$content.removeClass('has-button');
    this.$content.removeData('hh_dialog');
    
    delete this.$content;
    delete this.$;
  };
  
  happhub.button = function(that, options) {
    if (!that) {
      that = $('<span></span>');
      if (options.text) {
        that.text(options.text);
      } else
      if (options.html) {
        that.html(options.html);
      }
      if (options.icon) {
        if (options.prepend) {
          that.prepend($('<i></i>').addClass(options.icon));
        } else {
          that.append($('<i></i>').addClass(options.icon));
        }
      }
    }
    var $this = $(that);
    
    var plugin = $this.data('hh_button');
    if (options !== undefined) {
      if (plugin) {
        plugin.destroy();
      }
      $this.data('hh_button', (plugin = new Button($this, options)));
    }
    return plugin; 
  };
  happhub.buttonBar = function(buttons, options) {
    var bar = $('<div></div>').addClass('btn-group').
    addClass(options && options.clss ? options.clss : '').
    css(options && options.style ? options.style : {});
    
    $.each(buttons, function(i, v) {
      var o = v;
      if (v.content) {
        o = v.options;
      }
      bar.append(happhub.button(v.content, o).$);
    });
    return bar;
  };
})(window.happhub);