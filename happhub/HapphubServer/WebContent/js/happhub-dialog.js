!(function (happhub) {
  var f_position = function() {
    var width = this.$.outerWidth();
    if (!this.width || width != this.width) {
      this.width = width;
      
      var self = this;
      setTimeout(function() {
        if (self.width != self.$.width()) {
          f_position.call(self);
        }
      },10);
    }
      
    this.$.css({
      'left': '50%',
      'width': this.width+'px',
      'margin-left': -this.width/2+'px'
    });
  };
  var f_reposition = function() {
    this.$.css({
      'width': ''
    });
    
    f_position.call(this);
  };
  
  var f_clear = function() {
    $.each(this.$.find('.has-validator'), function(i, v) {
      happhub.validator(v).clear();
    });
    if (this.$.hasClass('.has-validator')) {
      happhub.validator(this.$).clear();
    }
  };
  
  var l_index = 0;
  
  var l_defaultOptions = {};
  var Dialog = function($content, options) {
    this.options = $.extend({}, l_defaultOptions, options);
    
    this.$content = $content;
    this.$content.addClass('has-dialog');

    this.contentStyle = this.$content.attr('style');
    this.contentStyle = (this.contentStyle == undefined) ? '' : this.contentStyle;
    
    if (!this.options.wrap) {
      this.$content.find('> :not(.invisible,.accessible-only,.form-actions)').wrap('<div class="modal-body"></div>');
      this.$content.find('> .form-actions').wrap('<div class="modal-footer"></div>');
      
      this.contentTabIndex = this.$content.attr('tabindex');
      this.$ = this.$content.attr('tabindex', -1);
    } else {
      this.$ = $('<div tabindex="-1"></div>').append(
        this.$body = $('<div></div>').addClass('modal-body')
      );
    }
    this.$.addClass('modal fade');
    
    if (this.options.header) {
      var header = $('<div></div>').addClass('modal-header -transient');
      if (!this.options.noCloseButton) {
        header.append('<button type="button" class="close" data-dismiss="modal" aria-hidden="true" tabindex="-1">&times;</button>');
      }
      this.$.prepend(header.append(options.header));
    }
    
    var self = this;

    if (options.buttons) {
      var container;
      
      var footer = this.$.find('modal-footer');
      if (footer.length == 0) {
        this.$.append(container = $('<div></div>').addClass('modal-footer -transient'));
      } else {
        footer.append(container = $('<span></span>').addClass('-transient'));
      }
      
      var closeCB = function() {
        self.$.modal('hide');
      };
      $.each(options.buttons, function(i, v) {
        var o = v;
        if (v.content) {
          o = v.options;
        }
        
        if (!o.clickCB && !o.event) {
          o.clickCB = closeCB;
        }
        container.append(happhub.button(v.content, o).$);
      });
    }
    
    this.$.
    css({
      'width':'',
      'max-width': '80%'
    }).
    on('keydown.hh_dialog', function(e) {
      if (e.keyCode !== 9) {
        return;
      }

      var tabbables = $(':tabbable', this);
      
      var first = tabbables.filter(':first');
      var last = tabbables.filter(':last');

      if (e.target === last[0] && !e.shiftKey) {
        first.focus();
        return false;
      } else
      if (e.target === first[0] && e.shiftKey) {
        last.focus();
        return false;
      }
    }).
    on('show.hh_dialog', function(e) {
      if (e.target == e.currentTarget) {
        l_index++;
        
        var modal = self.$.data('modal'); 
        modal.$element.css('z-index',1050+2*l_index);
        
        $(window).on('resize.hh_dialog', function() {
          f_reposition.call(self);
        });
      
        if (self.$body) {
          self.$contentParent = self.$content.parent();
          self.$body.append(self.$content.show());
        } else {
          self.$content.show();
        }
        f_clear.call(self);
      }
    }).
    on('shown.hh_dialog', function(e) {
      if (e.target == e.currentTarget) {
        $(":tabbable", this).filter(':first').focus().select();
      }
    }).
    on('hidden.hh_dialog', function(e) {
      if (e.target == e.currentTarget) {
        l_index--;
        self.destroy();
      }
    }).
    on('contentChanged.hh_dialog', function(e) {
      f_reposition.call(self);
    }).
    modal({show:false,backdrop:'static'});

    var modal = this.$.data('modal');
    modal.__index = l_index;
    
    modal.enforceFocus = function() {
      $(document).on('focusin.modal', function (e) {
        if (self.__index == l_index) {
          if (self.$element[0] !== e.target && !self.$element.has(e.target).length) {
            self.$element.focus();
          }
        }
      });
    };
    
    this.previousBackdrop = modal.backdrop;
    modal.backdrop = function(callback) {
      self.previousBackdrop.apply(modal, arguments);
      modal.$backdrop.css('z-index',1050+(2*l_index-1));
    };
    
    this.previousShow = modal.$element.show;
    modal.$element.show = function() {
      self.previousShow.apply(self, arguments);
      f_position.call(self);
    };
  };
  Dialog.prototype.show = function() {
    this.$.modal('show');
    return this;
  };
  Dialog.prototype.hide = function() {
    this.$.modal('hide');
    return this;
  };
  Dialog.prototype.destroy = function() {
    this.$.data('modal').$element.show = this.previousShow;
    delete this.previousShow;
    
    f_clear.call(this);
    
    $(window).off('.hh_dialog');
    this.$.off('.hh_dialog');
    
    this.$content.attr('style', this.contentStyle);
    delete this.contentStyle;
    
    this.$.find('.-transient').remove();
    
    if (this.$body) {
      if (this.$contentParent) {
        if (this.$contentParent.length > 0) {
          this.$contentParent.append(this.$content);
        }
        delete this.$contentParent;
      }
      delete this.$body;
    } else {
      this.$content.removeClass('modal hide fade');
    }
    
    if (!this.options.wrap) {
      this.$content.find('.modal-body > :not(.form-actions)').unwrap();
      this.$content.find('.modal-footer > .form-actions').unwrap();
        
      this.$content.attr('tabindex', this.contentTabIndex);
      delete this.contentTabIndex;
    } else {
      this.$.remove();
    }
    
    this.$content.removeClass('has-dialog');
    this.$content.removeData('hh_dialog');
    
    delete this.width;
    
    delete this.$content;
    delete this.$;
  };
  
  happhub.dialog = function(that, options) {
    if (!(that instanceof jQuery)) {
      options.wrap = true;
      that = $('<span></span>').html(that);
    }
    var $this = $(that);
    
    var plugin = $this.data('hh_dialog');
    if (options !== undefined) {
      if (plugin) {
        plugin.destroy();
      }
      $this.data('hh_dialog', (plugin = new Dialog($this, options)));
    }
    return plugin; 
  };
  
  happhub.alert = function(message, title, proceedCB) {
    return happhub.dialog(message, {noCloseButton:true,header:'<h3>'+title+'</h3>',buttons:[{text:'Ok',clss:'btn-primary',clickCB:proceedCB}]});
  };
  happhub.confirm = function(message, title, proceedCB, cancelCB) {
    return happhub.dialog(message, {noCloseButton:true,header:'<h3>'+title+'</h3>',buttons:[{text:'Cancel',clickCB:cancelCB},{text:'Proceed',clickCB:proceedCB,clss:'btn-danger'}]});
  };
})(window.happhub);