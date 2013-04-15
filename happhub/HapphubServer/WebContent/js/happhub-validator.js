!(function(happhub) {
  var l_rules = {
    minLength : function(l) {
      return ('' + this).length >= l;
    },
    maxLength : function(l) {
      return ('' + this).length <= l;
    },
    isEmail : function() {
      return /^((([a-z]|\d|[!#\$%&'\*\+\-\/=\?\^_`{\|}~]|[\u00A0-\uD7FF\uF900-\uFDCF\uFDF0-\uFFEF])+(\.([a-z]|\d|[!#\$%&'\*\+\-\/=\?\^_`{\|}~]|[\u00A0-\uD7FF\uF900-\uFDCF\uFDF0-\uFFEF])+)*)|((\x22)((((\x20|\x09)*(\x0d\x0a))?(\x20|\x09)+)?(([\x01-\x08\x0b\x0c\x0e-\x1f\x7f]|\x21|[\x23-\x5b]|[\x5d-\x7e]|[\u00A0-\uD7FF\uF900-\uFDCF\uFDF0-\uFFEF])|(\\([\x01-\x09\x0b\x0c\x0d-\x7f]|[\u00A0-\uD7FF\uF900-\uFDCF\uFDF0-\uFFEF]))))*(((\x20|\x09)*(\x0d\x0a))?(\x20|\x09)+)?(\x22)))@((([a-z]|\d|[\u00A0-\uD7FF\uF900-\uFDCF\uFDF0-\uFFEF])|(([a-z]|\d|[\u00A0-\uD7FF\uF900-\uFDCF\uFDF0-\uFFEF])([a-z]|\d|-|\.|_|~|[\u00A0-\uD7FF\uF900-\uFDCF\uFDF0-\uFFEF])*([a-z]|\d|[\u00A0-\uD7FF\uF900-\uFDCF\uFDF0-\uFFEF])))\.)+(([a-z]|[\u00A0-\uD7FF\uF900-\uFDCF\uFDF0-\uFFEF])|(([a-z]|[\u00A0-\uD7FF\uF900-\uFDCF\uFDF0-\uFFEF])([a-z]|\d|-|\.|_|~|[\u00A0-\uD7FF\uF900-\uFDCF\uFDF0-\uFFEF])*([a-z]|[\u00A0-\uD7FF\uF900-\uFDCF\uFDF0-\uFFEF])))$/i
          .test(this);
    },
    sameAs: function(selector) {
      return this == $(selector).val();
    }
  };
  var l_validators = [];

  var l_defaultFieldOptions = {};
  var FieldValidator = function(validator, field, rules, options) {
    this.options = $.extend({}, l_defaultFieldOptions, options);
    
    this.$field = $(field);
    
    this.rules = rules ? rules : {};

    var self = this;
    this.$field.on('keydown.hh_validator', function(e) {
      if (e.target == e.currentTarget) {
        self.clear();
      }
    });
  };
  FieldValidator.prototype.validate = function() {
    var messages = [];

    var v = this.$field.val();
    for ( var i = 0; i < this.rules.length; ++i) {
      var result = true;
      if ($.isFunction(this.rules[i].rule)) {
        result = this.rules[i].rule.apply(v, this.rules[i].args ? this.rules[i].args : []);
      } else {
        result = l_rules[this.rules[i].rule].apply(v, this.rules[i].args ? this.rules[i].args : []); 
      }
      if (!result) {
        messages.push(this.rules[i].message);
      }
    }
    
    this.error(messages);
    return messages.length == 0;
  };
  FieldValidator.prototype.error = function(messages) {
    if (messages.length > 0) {
      this.$group = this.$field.closest('.controls');
      if (this.$group.length == 0) {
        this.$group = this.$field.closest('.control-group');
      }
      
      this.$container = this.options.container ? $(this.options.container) : this.$group;
      if (this.$container.length == 0) {
        this.$container = (this.$group.length > 0) ? this.$group : this.$field.parent();
      }
      
      var popoverOptions = null;
      var popoverContainer = null;
      
      if (this.$field.closest('.modal').length > 0) {
        popoverOptions = null;
      } else {
        popoverOptions = this.options.popover;
      }
      
      if (popoverOptions) {
        if (this.options.popoverContainer) {
          popoverContainer = this.$field.closest(this.options.popoverContainer);
        } else {
          popoverContainer = this.$container;
        }
        if (popoverContainer.length == 0) {
          popoverContainer = $('body');
        }
      }
      if (popoverOptions) {
        this.hasPopover = true;
        
        var options = $.extend({
            content: messages.join('<br/>'),
            trigger: 'manual',
            container: popoverContainer
          },
          popoverOptions
        );
        this.$field.popover(options).addClass('error').popover('show');
      } else {
        if (this.hasPopover) {
          delete this.hasPopover;
          this.$field.popover('destroy');
        }
        
        if (!this.$error) {
          this.$error = $('<div class="validator-error"></div>').addClass(this.options.clss ? this.options.clss : '');
        }
        this.$error.html(messages.join('<br/>'));
        
        if (this.options.prepend) {
          this.$container.prepend(this.$error);
        } else {
          this.$container.append(this.$error);
        }
        this.$container.trigger('contentChanged');
      }

      this.$group.addClass('error');
      
      var i = l_validators.indexOf(this);
      if (i < 0) {
        l_validators.push(this);
      }
    } else {
      this.clear();
    }
  };
  FieldValidator.prototype.clear = function() {
    if (this.$error) {
      this.$error.remove();
      delete this.$error;
      
      this.$container.trigger('contentChanged');
    }
    if (this.hasPopover) {
      delete this.hasPopover;
      this.$field.popover('destroy');
    }
    
    var i = l_validators.indexOf(this);
    if (i >= 0) {
      l_validators.splice(i, 1);
    }
    
    if (this.$group) {
      this.$group.removeClass('error');
      delete this.$group;
    }
    delete this.$container;
  };
  FieldValidator.prototype.update = function() {
    if (this.hasPopover) {
      this.$field.popover('show');
    }
  };
  FieldValidator.prototype.destroy = function() {
    this.clear();
    this.$field.off('.hh_validator');
  };

  var l_defaultOptions = {fields:{}};
  var Validator = function($form, options) {
    this.options = $.extend({}, l_defaultOptions, options);
    
    this.$ = $form;
    this.$.addClass('has-validator');
    
    this.formError = new FieldValidator(
      self,
      this.$,
      [],
      $.extend({
          container: this.$,
          prepend: true,
          clss: 'error'
        },
        this.options.defaults,
        this.options.form
      )
    );

    var self = this;
    
    this.fields = [this.formError];
    $.each(this.options.fields, function(i, v) {
      self.fields.push(new FieldValidator(self, i, v.rules, $.extend({}, self.options.defaults, v.options)));
    });
  };
  Validator.prototype.validate = function() {
    var validated = true;
    for ( var i = 0; i < this.fields.length; ++i) {
      if (this.fields[i] != this.formError) {
        validated &= this.fields[i].validate();
      }
    }
    return validated;
  };
  Validator.prototype.clear = function() {
    for ( var i = 0; i < this.fields.length; ++i) {
      this.fields[i].clear();
    }
  };
  Validator.prototype.error = function(text) {
    this.formError.error([text]);
  };
  Validator.prototype.destroy = function() {
    for ( var i = 0; i < this.fields.length; ++i) {
      this.fields[i].destroy();
    }
    this.$.off('.hh_validator');
    
    this.$.removeClass('has-validator');
    this.$.removeData('hh_validator');
    
    delete this.fields;
    delete this.formError;
    
    delete this.$;
  };
  
  Validator.prototype.ajaxCallback = function(message, cb, cbContext) {
    var self = this;
    
    return function(code, result) {
      if (code == null) {
        self.error('Request failed, code: '+result.status+' ('+happhub.escapeHtml(result.statusText)+').');
      } else {
        if (code == 0) {
          cb.call(cbContext, result);
        } else {
          self.error(message);
        }
      }
    };
  };

  happhub.validator = function(that, options) {
    var $this = $(that);

    var plugin = $this.data('hh_validator');
    if (options !== undefined) {
      if (plugin) {
        plugin.destroy();
      }
      $this.data('hh_validator', (plugin = new Validator($this, options)));
    }
    return plugin;
  };
  
  var previous = happhub.initialize; 
  happhub.initialize = function(user, account, session, model, options) {
    $(window).on('resize', function() {
      $.each(l_validators, function(i, v) {
        v.update();
      });
    });
    
    previous.call(this, user, account, session, model, options);
  };
})(window.happhub);