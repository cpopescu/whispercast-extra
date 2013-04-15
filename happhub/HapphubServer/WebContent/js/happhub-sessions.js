!(function(happhub) {
var sessions = {
  options: {
  },

  mvc: function(account_id, model, view, controller) {
    return $$(model, view, controller).persist($$.adapter.restfulJson, {collection: '/accounts/'+account_id+'/sessions'});
  },

  previewUrlOutput: function(session) {
    return 'rtmp://streaming2.mediares.ucl.ac.uk/vod/advances/enterprise/2009/awards/studio/prof_rajiv_jalan';
  },
  previewUrlCamera: function(session, camera) {
    return 'rtmp://streaming2.mediares.ucl.ac.uk/vod/advances/enterprise/2009/awards/studio/prof_rajiv_jalan';
  }
};

var previous = happhub.initialize; 
happhub.initialize = function(user, account, session, model, options) {
  $.extend(sessions.options, (options && options.sessions) ? options.sessions : {});
  happhub.sessions = sessions;
  
  previous.call(this, user, account, session, model, options);
};

})(window.happhub);
