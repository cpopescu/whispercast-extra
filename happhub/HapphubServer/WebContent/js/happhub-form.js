!(function (happhub) {
  var l_defaultOptions = {};
  var Form = function($form, options) {
    this.options = $.extend({}, l_defaultOptions, options);
    
    this.$ = $form;
    this.$.addClass('has-form');
    
    if (this.options.validator) {
      happhub.validator(this.$, this.options.validator);
    }
    
    var self = this;
    
    this.$.
    on('submit.hh_form', function(e) {
      var validator = happhub.validator(self.$);
      if (validator) {
        if (!validator.validate()) {
          e.stopImmediatePropagation();
          return false;
        }
      }
      return true;
    });
  };
  Form.prototype.setup = function(data) {
    var validator = happhub.validator(this.$);
    if (validator) {
      //validator.clear();
    }
    if (this.options.toForm) {
      this.options.toForm.call(this, data);
    }
  };
  Form.prototype.destroy = function() {
    this.$.off('.hh_form');
    
    this.$.removeClass('has-form');
    this.$.removeData('hh_form');
    
    delete this.$;
  };
  
  happhub.form = function(that, options) {
    var $this = $(that);
    
    var plugin = $this.data('hh_form');
    if (options !== undefined) {
      if (plugin) {
        plugin.destroy();
      }
      $this.data('hh_form', (plugin = new Form($this, options)));
    }
    return plugin; 
  };
})(window.happhub);