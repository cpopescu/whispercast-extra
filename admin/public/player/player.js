(function() {

/* The Player Whispercast namespace */
if (Whispercast.Player == undefined) {
  Whispercast.Player = {
    implicit: {
      url: {
        play: null,
        standby: null
      },

      show_ui: true,
      simple_ui: false,

      play_on_load: true,

      text_color: '808080',
      background_color: '000000',

      link_color: '808080',
      hover_color: 'c0c0c0',

      link_underline: false,
      hover_underline: true,

      heading: null,

      popup_width: 640,
      popup_height: 520,

      p_cookie_id: 'wsp',
      s_cookie_id: 'wss',

      popup_opened_content:
      '<div><a href="#" onclick="whispercast.local.openInPopup();return false">Playerul este deschis în altă fereastră.</a></div>'
    },

    setupURL: function(play, standby) {
      var L = Whispercast.Player;

      var u ={}
      u.play = play ? play + (L.params.url.query ? ((play.indexOf('?') < 0)?'?':'&')+L.params.url.query : '') : null;
      u.standby = standby ? standby + (L.params.url.query ? ((standby.indexOf('?') < 0)?'?':'&')+L.params.url.query : '') : null;
      return u;
    },

    setupPage: function() {
      var L = Whispercast.Player;
      var sheet = Whispercast.Css.addSheet();
      Whispercast.Css.addRule(sheet, 'body', 'color:#'+L.params.text_color+';background-color:#'+L.params.background_color);
      Whispercast.Css.addRule(sheet, 'a', 'color:#'+L.params.link_color+';'+(L.params.link_underline?'text-decoration:underline':'text-decoration:none'));
      Whispercast.Css.addRule(sheet, 'a:hover', 'color:#'+L.params.hover_color+';'+(L.params.hover_underline?'text-decoration:underline':'text-decoration:none'));

      Whispercast.Css.addRule(sheet, '#player', 'width:'+L.params.width+'px;height:'+L.params.height+'px');

      var links = [];
      if (L.params.url.alternates) {
        for (var i = 0; i < L.params.url.alternates.length; i++) {
          var a = L.params.a.alternates[i];
          links.push('<a href="#" title="Comută pe streamul \''+a.name+'\'" onclick="javascript:Whispercast.Player.setURL(\''+a.url+'\');return false;">'+a.name+'</a>');
        }
      }
      document.getElementById('links').innerHTML = links.join('&nbsp;&middot;&nbsp;');

      if (L.params.affiliate_id) {
        var path = L.params.affiliate_id.split('/');

        var source = path[0];
        Whispercast.Css.addSheet('affiliates/'+source+'.css');
        for (var index = 1; index < path.length; index++) {
          source = source+'/'+path[index];
        }
        Whispercast.Css.addSheet('affiliates/'+source+'.css');
      }
    },

    createPlayer: function() {
      var L = Whispercast.Player;

      var url = L.setupURL(L.params.url.play, L.params.url.standby);
      Whispercast.log('PLAY: '+url.play);
      Whispercast.log('STANDBY: '+url.standby);

      var vars = {
        url0: (url.play != null) ? url.play : '',
        url_standby0: (url.standby != null) ? url.standby : '',
        font_size: (L.params.font_size != undefined) ? L.params.font_size : 10,
        simple_ui: (L.params.simple_ui != undefined) ? (L.params.simple_ui ? 1 : 0) : 0,
        static_ui: (L.params.static_ui != undefined) ? (L.params.static_ui ? 1 : 0) : 1,
        show_ui: (L.params.show_ui != undefined) ? (L.params.show_ui ? 1 : 0) : 1,
        show_osd: (L.params.show_osd != undefined) ? (L.params.show_osd ? 1 : 0) : 1,
        hardware_accelerated_fullscreen: (L.params.width > 639) ? 1 : 0,
        play_on_load: (L.params.play_on_load != undefined) ? (L.params.play_on_load ? 1 : 0) : 1,
        muted: (L.params.muted != undefined) ? (L.params.muted ? 1 : 0) : 0,
        volume: (L.params.volume != undefined) ? (L.params.volume/100) : 0.5,
        video_align_x: L.params.video_align_x ? L.params.video_align_x : 'center',
        video_align_y: L.params.video_align_y ? L.params.video_align_y : 'center',
        url_mapper: (L.params.url_mapper != undefined) ? L.params.url_mapper : '',
        logger: (L.params.logger != undefined) ? L.params.logger : ''
      };
      for (var i in vars) {
        vars[i] = String(vars[i]);
      }

      Whispercast.log('SHOW UI: '+L.params.show_ui);
      Whispercast.log('SIMPLE UI: '+L.params.simple_ui);

      L.playerContent = document.getElementById('player').innerHTML;
      L.player = new YAHOO.widget.SWF('player', '../swf/player.swf', {
        version: 9.115,
        useExpressInstall: true,
        fixedAttributes: {
          menu: 'false',
          quality: 'autohigh',
          scale: 'exactfit',
          wmode: 'window',
          allowScriptAccess: 'always',
          allowNetworking: 'all',
          allowFullScreen: 'true',
          bgcolor: (L.params.background_color ? L.params.background_color : '')
        },
        flashVars: vars
      });
    },
    destroyPlayer: function() {
      var L = Whispercast.Player;

      var player = L.player;
      delete L.player;

      if (player) {
        try {
          player.callSWF('setURL_', [0, null, null]);
        } catch(e) {
        }

        var element = player.get('element');
        try
        {
        // Break circular references.
        (function (o) {
        var a = o.attributes, i, l, n, c;
        if (a) {
        l = a.length;
        for (i = 0; i <l; i += 1) {
        n = a[i].name;
        if (typeof o[n] === 'function') {
        o[n] = null;
        }
        }
        }
        a = o.childNodes;
        if (a) {
        l = a.length;
        for (i = 0; i <l; i += 1) {
        c = o.childNodes[i];
        // Purge child nodes.
        arguments.callee(c);
        }
        }
        }
        )(element);
        }
        catch(e)
        {
        }
       
        var parent = element.parentNode; 
        parent.removeChild(element);
        parent.innerHTML = L.playerContent;
        delete L.playerContent;
      }
    },

    setURL: function(index, url) {
      if (Whispercast.Player.player) {
        Whispercast.Player.player.callSWF('setURL_', [index, url, null]);
      }
    }
  };
}

}());
