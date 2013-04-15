!(function (happhub) {
  var l_defaultOptions = {};
  var MvcForm = function($form, options) {
    this.options = $.extend({}, l_defaultOptions, options);
    
    this.$ = happhub.form($form, options).$;
    this.$.addClass('has-mvc');
    
    var self = this;
    
    this.$.
    on('submit.hh_mvc', function(e) {
      var local = $$(self.mvc, self.mvc.model.get());
      if (self.options.fromForm.call(self, local.model)) {
        if (self.processCB) {
          self.processCB.call(self, true);
        }
    
        happhub.persist(local, 'save', function(success, entity) {
          if (success) {
            self.mvc.model.set(local.model.get());
          }
          
          if (self.processCB) {
            self.processCB.call(self, false, success, local.model);
          }
        });
      }
      e.stopImmediatePropagation();
      return false;
    }).
    on('reset.hh_mvc', function(e) {
      if (self.mvcIsOurs) {
        self.mvc.destroy();
      }
      delete self.mvc;
    });
  };
  MvcForm.prototype.setup = function(mvc, processCB) {
    if (mvc) {
      this.mvcIsOurs = false;
      this.mvc = mvc;
    } else {
      this.mvcIsOurs = true;
      this.mvc = $$(this.options.mvc, this.options.mvcDefaults ? this.options.mvcDefaults : {});
    }
    this.processCB = processCB;
    
    happhub.form(this.$).setup(this.mvc.model.get());
  };
  MvcForm.prototype.destroy = function() {
    this.$.off('.hh_mvc');
    
    this.$.removeClass('has-mvc');
    this.$.removeData('hh_mvc_form');
    
    happhub.form($this.$).destroy();
  };
  
  happhub.mvcForm = function(that, options) {
    var $this = $(that);
    
    var plugin = $this.data('hh_mvc_form');
    if (options !== undefined) {
      if (plugin) {
        plugin.destroy();
      }
      $this.data('hh_mvc_form', (plugin = new MvcForm($this, options)));
    }
    return plugin; 
  };
})(window.happhub);