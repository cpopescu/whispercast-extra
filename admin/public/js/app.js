Whispercast.App = Whispercast.App ? Whispercast.App : {
  _requests: {
    playlists: {},
    files: {},
    osd: {
      overlays: {
      },
      crawlers: {
      }
    }
  },

  _cookieServer: 0,

  _scriptRegexpAll: new RegExp('<script[^>]*>([\\S\\s]*?)<\/script>', 'img'),
  _scriptRegexpOne: new RegExp('<script[^>]*>([\\S\\s]*?)<\/script>', 'im'),
  _scriptRegexpClean: new RegExp('^<!--(.*)$', 'im'),

  _sandboxes: [],

  _popups: [],
  _popupId: 0,

  _currentPath: '',

  initialize: function(isMain) {
    var handleTransactionResponse = YAHOO.util.Connect.handleTransactionResponse;
    YAHOO.util.Connect.handleTransactionResponse = function(o, callback, isAbort) {
      var httpStatus;
      try
      {
        if(o.conn.status !== undefined && o.conn.status !== 0){
          httpStatus = o.conn.status;
        }
        else{
          httpStatus = 13030;
        }
      }
      catch(e){
        // 13030 is a custom code to indicate the condition -- in Mozilla/FF --
        // when the XHR object's status and statusText properties are
        // unavailable, and a query attempt throws an exception.
        httpStatus = 13030;
      }
      if (httpStatus == 401) {
        if (window.opener && window.opener.Whispercast) {
          window.opener.location.reload(true);
          window.close();
        } else {
          window.location.reload(true);
        }
        return;
      }
      handleTransactionResponse.call(YAHOO.util.Connect, o, callback, isAbort);
    };
    YAHOO.util.Connect.formatRequestError = function(o) {
      if (o.status != 200) {
        return Whispercast.strformat(_('Could not connect to server (HTTP: %1 [%2]).'), o.status, o.statusText);
      }
      return _('Could not connect to server.');
    };

    Whispercast.Notifier.registerListener('windowLoaded', Whispercast.closure(Whispercast.App, Whispercast.App._onPopupLoaded));
    Whispercast.Notifier.registerListener('windowUnloaded', Whispercast.closure(Whispercast.App, Whispercast.App._onPopupUnloaded));

    if (isMain) {
      Whispercast.App._layout = new YAHOO.widget.Layout({
        units: [
          { position: 'top', height: 52, body: 'header', gutter: '3px'},
          { position: 'left', width: 300, body: 'sidebar', gutter: '0 3px 0 3px', scroll: true, resize: true, proxy: true, useShim: true},
          { position: 'center', body: 'body', gutter: '0 3px 0 0', scroll: true},
          { position: 'bottom', height: 32, body: 'footer', gutter: '3px'}
        ]
      });
      Whispercast.App._layout.on('resize', function() {
        Whispercast.Progress.layout();
      });
      Whispercast.App._layout.on('render', function() {
        var sidebar = {};

        sidebar.tree = new YAHOO.widget.TreeView('sidebar');
        sidebar.root = sidebar.tree.getRoot();
        
        sidebar.nodeServers = new YAHOO.widget.TextNode({nowrap:true,label:_('Servers'),key:'servers'}, sidebar.root);
        sidebar.nodeServers.labelStyle += ' icon icon-servers';
        sidebar.nodeServers.setDynamicLoad(function(node, loadComplete) {Whispercast.App.retrieveServers(loadComplete)}, 0);

        sidebar.tree.singleNodeHighlight = true;

        sidebar.tree.subscribe('expandComplete', function(node) {
          if (Whispercast.App._pendingNodeCurrent && (node == Whispercast.App._pendingNodeCurrent)) {
            Whispercast.App._selectNodeProcess();
          }
        });

        sidebar.tree.subscribe('clickEvent', function(e) {
          e.node.focus();
          if (e.node.data.selectable) {
            Whispercast.App.selectNode(Whispercast.App.getNodePath(e.node));
          } else {
            Whispercast.App._loadNode(e.node);
          }
          return false;
        });
        sidebar.tree.subscribe('enterKeyPressed', function(node) {
          Whispercast.App.selectNode(Whispercast.App.getNodePath(node));
        });

        sidebar.tree.subscribe('highlightEvent', function(node) {
          if (node.highlightState) {
            if (!Whispercast.App._pendingNodePath) {
              if (!Whispercast.App._pendingNodeNoHistory) {
                YAHOO.util.History.navigate('node', Whispercast.App.getNodePath(node));
              }
              Whispercast.App._loadNode(node);
            }
          }
        });

        sidebar.tree.render();
        Whispercast.App._sidebar = sidebar;

        Whispercast.Notifier.registerListener('default.servers', function(op, record) {
          Whispercast.App.retrieveServers(function() {
            Whispercast.App._sidebar.tree.render();
          });
        });

        if (Whispercast.History.initial['node']) {
          Whispercast.App.selectNode(Whispercast.History.initial['node'], true);
        }
      });
      Whispercast.App._layout.render();
    }
  },

  getLayoutElement: function(position) {
    return Whispercast.App._layout.getUnitByPosition(position);
  },

  getNodePath: function(node) {
    var path = [];
    while (node != Whispercast.App._sidebar.tree.getRoot()) {
      path.unshift(node.data.key);
      node = node.parent;
    }
    return path.join('/');
  },
  getNodeByPath: function(path, parent) {
    if (!parent) {
      parent = Whispercast.App._sidebar.tree.getRoot();
    }

    var keys = path.split('/');
    while (keys.length > 0) {
      var key = keys.shift();

      var next;
      for (var i = 0; i < parent.children.length; i++) {
        if (parent.children[i].data.key == key) {
          next = parent.children[i];
          break;
        }
      }

      if (!next) {
        return null;
      }
      parent = next;
    }
    return parent;
  },

  _loadNode: function(node) {
    if (node.data.selectable) {
      var nodePath = Whispercast.App.getNodePath(node);
      if (nodePath == Whispercast.App._currentPath) {
        return;
      }
      Whispercast.App._currentPath = nodePath;
    }

    if (node.data.load) {
      var load = node.data.load;
      if (load.unit == 'popup') {
        if (load.args) {
          var args = [load.name, load.url, load.query, load.features];
          Whispercast.App.popup.apply(null, args.concat(load.args));
        } else {
          Whispercast.App.popup(load.name, load.url, load.query, load.features);
        }
      } else {
        if (load.args) {
          var args = [load.unit, load.url, load.query];
          Whispercast.App.retrieveContent.apply(null, args.concat(load.args));
        } else {
          Whispercast.App.retrieveContent(load.unit, load.url, load.query);
        }
      }
    }
  },

  _selectNodeClear: function(node) {
    delete Whispercast.App._pendingNodeCurrent;
    delete Whispercast.App._pendingNodePath; 

    if (node) {
      if (!node.data.selectable) {
        return;
      }
      node.focus();
      node.highlight();
    }

    delete Whispercast.App._pendingNodeNoHistory;
  },
  _selectNodeProcess: function() {
    var next;
    if (Whispercast.App._pendingNodePath.length > 0) {
      next = Whispercast.App.getNodeByPath(Whispercast.App._pendingNodePath.shift(), Whispercast.App._pendingNodeCurrent);
    }
    if (!next) {
      if (Whispercast.App._pendingNodeCurrent) {
        Whispercast.App._selectNodeClear(Whispercast.App._pendingNodeCurrent);
      }
      return;
    }

    Whispercast.App._pendingNodeCurrent = next;
    if (!Whispercast.App._pendingNodeCurrent.isLeaf && !Whispercast.App._pendingNodeCurrent.expanded) {
      Whispercast.App._pendingNodeCurrent.expand();
    } else {
      Whispercast.App._selectNodeProcess();
    }
  },
  selectNode: function(path, noHistory) {
    Whispercast.App._selectNodeClear();
    Whispercast.App._pendingNodePath = path.split('/');
    Whispercast.App._pendingNodeCurrent = Whispercast.App._sidebar.tree.getRoot();
    Whispercast.App._pendingNodeNoHistory = noHistory ? true : false;
    Whispercast.App._selectNodeProcess();
  },

  _onPopupLoaded: function(name, w) {
    if (w != window) {
      if (w.opener == window) {
        Whispercast.App._popups[name] = {
          w: w 
        }

        if (w.sandbox) {
          w.sandbox.run.call(w.sandbox);
        }
      }
    }
  },
  _onPopupUnloaded: function(name, w) {
    if (w != window) {
      if (w.opener == window) {
        delete Whispercast.App._popups[name];
        if (w.sandbox) {
          w.sandbox.destroy.call(w.sandbox);
        }
        window.focus();
      }
    }
  },

  popup: function(name, url, query, features) {
    name = name ? name : 'popup'+Whispercast.App._popupId++;
    query = query ? query : {};

    if (Whispercast.App._popups[name]) {
      if (Whispercast.App._popups[name].w.closed) {
        delete Whispercast.App._popups[name];
      }
    }

    if (Whispercast.App._popups[name]) {
      var w = Whispercast.App._popups[name].w;
      if (w.sandbox && !w.sandbox.initialized) {
        w.focus();
        w.sandbox.run.call(w.sandbox);
      } else {
        w.focus();
      }
    } else {
      var w = window.open('', name, features);
      if (w.location.href == 'about:blank') {
        w = window.open(Whispercast.Uri.build(url, query, true), name, features);
      }
      Whispercast.App._popups[name] = {
        w:w
      }
    }
  },

  setContent: function(unit, content) {
    var container = document.createElement('div');
    YAHOO.util.Dom.setStyle(container, 'display', 'none');

    container.innerHTML = content.replace(Whispercast.App._scriptRegexpAll, '');

    var body = (unit != '') ? Whispercast.App.getLayoutElement(unit).body : document.getElementById('transient');
    while (body.childNodes.length > 0) {
      body.removeChild(body.firstChild);
    }
    body.appendChild(container);

    var sandbox = new Whispercast.Sandbox.Base(
      function() {
        if (unit != '') {
          YAHOO.util.Dom.setStyle(container, 'opacity', 0);
          YAHOO.util.Dom.setStyle(container, 'display', 'block');
          var a = new YAHOO.util.Anim(container, {
            opacity: {to:1}
          }, 0.1);
          a.animate();
          Whispercast.Progress.hide(unit);
        } else {
          Whispercast.Progress.hide('');
        }
        Whispercast.log('Content of "'+(unit ? unit : 'transient')+'" is running', 'APPLICATION');
      },
      function() {
        if (this.initialized) {
          switch (unit) {
            case 'top':
            case 'right':
            case 'bottom':
            case 'left':
            case 'center':
              Whispercast.Progress.hide(unit);
              break;
            default:
              Whispercast.Progress.hide('');
          }
        }
      }
    );
    sandbox._animate = function(callback) {
      var a = new YAHOO.util.Anim(container, {
        opacity: {to:0}
      }, 0.05);
      a.onComplete.subscribe(function() {
        callback.call(this);
      });
      a.animate();
    };
   
    var scripts = content.match(Whispercast.App._scriptRegexpAll);
    if (scripts) {
      for (var i=0; i < scripts.length; i++) {
        var script = scripts[i].match(Whispercast.App._scriptRegexpOne)[1];
        script = script.replace(Whispercast.App._scriptRegexpClean, '');
        Whispercast.eval('var sandbox = this;'+script, sandbox);
      }
    }

    var args = [];
    for (var i = 2; i < arguments.length; i++) {
      args.push(arguments[i]);
    }
   
    Whispercast.App._sandboxes[unit] = sandbox;
    sandbox.run.apply(sandbox, args);
  },
  retrieveContent: function(unit, url, query) {
    query = query ? query : {};
    query.format = 'html';

    var uri =  Whispercast.Uri.build(url, query, true);
    Whispercast.log('Retrieving content for "'+(unit ? unit : 'transient')+'" from "'+uri+'"...', 'APPLICATION');

    var args = [];
    for (var i = 3; i < arguments.length; i++) {
      args.push(arguments[i]);
    }

    switch (unit) {
      case 'top':
      case 'right':
      case 'bottom':
      case 'left':
      case 'center':
        Whispercast.Progress.show(unit, Whispercast.App.getLayoutElement(unit).get('element'), 0);
        break;
      case '':
      case 'hidden':
        Whispercast.Progress.show('');
        break;
      default:
        Whispercast.Progress.show('', unit);
        unit = '';
    }

    var request_key = 'unit'+unit;
    if (Whispercast.App._requests[request_key]) {
      Whispercast.Progress.hide(unit);
      Whispercast.App._requests[request_key].conn.abort();
    }

    var callback = function(o) {
      Whispercast.log('Content for "'+uri+'" was retrieved..', 'APPLICATION');
      delete Whispercast.App._requests[request_key];

      args.unshift((o.status == 200) ? (o.responseText ? o.responseText : '') : '');
      args.unshift(unit);

      var set = function() {
        Whispercast.App.setContent.apply(Whispercast.App, args);

        if (o.status != 200) {
          (new Whispercast.Dialog.Prompt(_('Error'), YAHOO.util.Connect.formatRequestError(o), 'E')).show();
        }
      }

      if (Whispercast.App._sandboxes[unit]) {
        var sandbox = Whispercast.App._sandboxes[unit];
        delete Whispercast.App._sandboxes[unit];

        if (sandbox._animate) {
          sandbox._animate(function() {
            sandbox.destroy.call(sandbox);
            set.call();
          });
          return;
        }
         sandbox.destroy.call(sandbox);
      }
      set.call();
    }

    Whispercast.App._requests[request_key] =
    YAHOO.util.Connect.asyncRequest(
      'get',
      uri,
      {
        success: callback,
        failure: callback
      }
    );
  },

  _removeExpiredNodes: function(parent, cookieName, cookie) {
    var removed = [];
    for (var i in parent.children) {
      if (parent.children[i].data[cookieName] < cookie) {
        removed.push(parent.children[i]);
      }
    }
    if (removed.length > 0) {
      for (var i in removed) {
        if (Whispercast.App._sidebar.tree._currentlyHighlighted == removed[i]) {
          Whispercast.App._sidebar.tree.getRoot().highlight(true);
        }
        Whispercast.log('Removing expired node \''+ removed[i].label+'\'', 'APPLICATION');
        Whispercast.App._sidebar.tree.removeNode(removed[i]);
      }
      parent.refresh();
    }
  },

  _updateServers: function(response) {
    var sidebar = Whispercast.App._sidebar;
    var parent = sidebar.nodeServers;

    if (response) {
      var t = {propagateHighlightUp:true, nowrap:true};

      var previous = null;
      for (var i = 0; i < response.model.records.length; i++) {
        var record = response.model.records[i];
        var sid = record.id;

        Whispercast.log('Creating server node for \''+ record.name+'\'', 'APPLICATION');

        t.key = sid;
        t.label = t.title = record.name;
        t.sc = Whispercast.App._cookieServer;

        t.load = {
          unit: 'popup',
          url: Whispercast.Uri.buildZend('default', 'servers', 'console', sid),
          name: 'whispercast.default.servers.console.'+sid,
          features: 'status=0,toolbar=0,location=0,menubar=0,width=1024,height=360,scrollbars=1,resizable=1'
        };
        var server = new YAHOO.widget.TextNode(t, parent);
        server.labelStyle += ' icon icon-server';

        if (previous) {
          server.insertAfter(previous);
        }
        previous = server;

        delete t.key;
        delete t.sc;

        Whispercast.log('Creating server sub-nodes for \''+ record.name+'\'', 'APPLICATION');
        t.selectable = true;

        if (response.model.acl['sources'] && response.model.acl['sources'][sid] && response.model.acl['sources'][sid]['get']) {
          t.key = 'sources';
          t.load = {
            unit: 'center',
            url: Whispercast.Uri.buildZend('default', 'sources', 'display', sid)
          };
          t.label = t.title = _('Sources');
          server.data.nodeSources = new YAHOO.widget.TextNode(t, server);
          server.data.nodeSources.labelStyle += ' icon icon-sources';
        }

        if (response.model.acl['files'] && response.model.acl['files'][sid] && response.model.acl['files'][sid]['get']) {
          t.key = 'files';
          t.load = {
            unit: 'center',
            url: Whispercast.Uri.buildZend('default', 'files', 'display', sid)
          };
          t.label = t.title = _('Files');
          server.data.nodeFiles = new YAHOO.widget.TextNode(t, server);
          server.data.nodeFiles.labelStyle += ' icon icon-files';
        }

        if (response.model.acl['ranges'] && response.model.acl['ranges'][sid] && response.model.acl['ranges'][sid]['get']) {
          t.key = 'ranges';
          t.load = {
            unit: 'center',
            url: Whispercast.Uri.buildZend('default', 'ranges', 'display', sid)
          };
          t.label = t.title = _('Ranges');
          server.data.nodeRanges = new YAHOO.widget.TextNode(t, server);
          server.data.nodeRanges.labelStyle += ' icon icon-ranges';
        }

        if (response.model.acl['remotes'] && response.model.acl['remotes'][sid] && response.model.acl['remotes'][sid]['get']) {
          t.key = 'remotes';
          t.load = {
            unit: 'center',
            url: Whispercast.Uri.buildZend('default', 'remotes', 'display', sid)
          };
          t.label = t.title = _('Remote Streams');
          server.data.nodeRemotes = new YAHOO.widget.TextNode(t, server);
          server.data.nodeRemotes.labelStyle += ' icon icon-remotes';
        }

        if (response.model.acl['switches'] && response.model.acl['switches'][sid] && response.model.acl['switches'][sid]['get']) {
          t.key = 'switches';
          t.load = {
            unit: 'center',
            url: Whispercast.Uri.buildZend('default', 'switches', 'display', sid)
          };
          t.label = t.title = _('Switches');
          server.data.nodeSwitches = new YAHOO.widget.TextNode(t, server);
          server.data.nodeSwitches.labelStyle += ' icon icon-switches';
        }

        if (response.model.acl['aliases'] && response.model.acl['aliases'][sid] && response.model.acl['aliases'][sid]['get']) {
          t.key = 'aliases';
          t.load = {
            unit: 'center',
            url: Whispercast.Uri.buildZend('default', 'aliases', 'display', sid)
          };
          t.label = t.title = _('Aliases');
          server.data.nodeAliases = new YAHOO.widget.TextNode(t, server);
          server.data.nodeAliases.labelStyle += ' icon icon-aliases';
        }

        if (response.model.acl['programs'] && response.model.acl['programs'][sid] && response.model.acl['programs'][sid]['get']) {
          t.key = 'programs';
          t.load = {
            unit: 'center',
            url: Whispercast.Uri.buildZend('default', 'programs', 'display', sid)
          };
          t.label = t.title = _('Programs');
          server.data.nodePrograms = new YAHOO.widget.TextNode(t, server);
          server.data.nodePrograms.labelStyle += ' icon icon-programs';
        }

        if (response.model.acl['playlists'] && response.model.acl['playlists'][sid] && response.model.acl['playlists'][sid]['get']) {
          t.key = 'playlists';
          t.load = {
            unit: 'center',
            url: Whispercast.Uri.buildZend('default', 'playlists', 'display', sid)
          };
          t.label = t.title = _('Playlists');
          server.data.nodePlaylists = new YAHOO.widget.TextNode(t, server);
          server.data.nodePlaylists.labelStyle += ' icon icon-playlists';
        }

        if (response.model.acl['aggregators'] && response.model.acl['aggregators'][sid] && response.model.acl['aggregators'][sid]['get']) {
          t.key = 'aggregators';
          t.load = {
            unit: 'center',
            url: Whispercast.Uri.buildZend('default', 'aggregators', 'display', sid)
          };
          t.label = t.title = _('Aggregators');
          server.data.nodeAggregators = new YAHOO.widget.TextNode(t, server);
          server.data.nodeAggregators.labelStyle += ' icon icon-aggregators';
        }

        if (response.model.acl['users'] && response.model.acl['users'][sid] && response.model.acl['users'][sid]['get']) {
          t.key = 'users';
          t.load = {
            unit: 'center',
            url: Whispercast.Uri.buildZend('default', 'users', 'display', sid)
          };
          t.label = t.title = _('Users');
          server.data.nodeUsers = new YAHOO.widget.TextNode(t, server);
          server.data.nodeUsers.labelStyle += ' icon icon-users';
        }

        delete t.selectable;
        if (server.data.nodeSources) {
          if (response.model.acl['sources'][sid]['add']) {
            t.key = 'add';
            t.load = {
              unit: '',
              url: Whispercast.Uri.buildZend('default', 'sources', 'add', sid)
            };
            t.label = t.title = _('Add a new source...');
            (new YAHOO.widget.TextNode(t, server.data.nodeSources)).labelStyle += ' icon icon-sources-add';
          }
        }

        if (server.data.nodeRanges) {
          if (response.model.acl['ranges'][sid]['add']) {
            t.key = 'add';
            t.load = {
              unit: '',
              url: Whispercast.Uri.buildZend('default', 'ranges', 'add', sid)
            };
            t.label = t.title = _('Add a new range...');
            (new YAHOO.widget.TextNode(t, server.data.nodeRanges)).labelStyle += ' icon icon-ranges-add';
          }
        }

        if (server.data.nodeRemotes) {
          if (response.model.acl['remotes'][sid]['add']) {
            t.key = 'add';
            t.load = {
              unit: '',
              url: Whispercast.Uri.buildZend('default', 'remotes', 'add', sid)
            };
            t.label = t.title = _('Add a new remote stream...');
            (new YAHOO.widget.TextNode(t, server.data.nodeRemotes)).labelStyle += ' icon icon-remotes-add';
          }
        }
        
        if (server.data.nodeSwitches) {
          if (response.model.acl['switches'][sid]['add']) {
            t.key = 'add';
            t.load = {
              unit: '',
              url: Whispercast.Uri.buildZend('default', 'switches', 'add', sid)
            };
            t.label = t.title = _('Add a new switch...');
            (new YAHOO.widget.TextNode(t, server.data.nodeSwitches)).labelStyle += ' icon icon-switches-add';
          }
        }

        if (server.data.nodeAliases) {
          if (response.model.acl['aliases'][sid]['add']) {
            t.key = 'add';
            t.load = {
              unit: '',
              url: Whispercast.Uri.buildZend('default', 'aliases', 'add', sid)
            };
            t.label = t.title = _('Add a new alias...');
            (new YAHOO.widget.TextNode(t, server.data.nodeAliases)).labelStyle += ' icon icon-aliases-add';
          }
        }

        if (server.data.nodePrograms) {
          if (response.model.acl['programs'][sid]['add']) {
            t.key = 'add';
            t.load = {
              unit: '',
              url: Whispercast.Uri.buildZend('default', 'programs', 'add', sid)
            };
            t.label = t.title = _('Add a new program...');
            (new YAHOO.widget.TextNode(t, server.data.nodePrograms)).labelStyle += ' icon icon-programs-add';
          }
        }

        if (server.data.nodePlaylists) {
          if (response.model.acl['playlists'][sid]['add']) {
            t.key = 'add';
            t.load = {
              unit: '',
              url: Whispercast.Uri.buildZend('default', 'playlists', 'add', sid)
            };
            t.label = t.title = _('Add a new playlist...');
            (new YAHOO.widget.TextNode(t, server.data.nodePlaylists)).labelStyle += ' icon icon-playlists-add';
          }
        }

        if (server.data.nodeAggregators) {
          if (response.model.acl['aggregators'][sid]['add']) {
            t.key = 'add';
            t.load = {
              unit: '',
              url: Whispercast.Uri.buildZend('default', 'aggregators', 'add', sid)
            };
            t.label = t.title = _('Add a new aggregator...');
            (new YAHOO.widget.TextNode(t, server.data.nodeAggregators)).labelStyle += ' icon icon-aggregators-add';
          }
        }

        if (server.data.nodeFiles) {
          if (response.model.acl['files'][sid]['add']) {
            t.key = 'add';
            t.load = {
              unit: 'popup',
              url: Whispercast.Uri.buildZend('default', 'files', 'upload', sid),
              name: 'whispercast.'+sid+'.default.files.upload',
              features: 'status=0,toolbar=0,location=0,menubar=0,width=900,height=750,scrollbars=1,resizable=1'
            };
            t.label = t.title = _('Add a new file...');
            (new YAHOO.widget.TextNode(t, server.data.nodeFiles)).labelStyle += ' icon icon-files-add';
          }
        }

        if (server.data.nodeUsers) {
          if (response.model.acl['users'][sid]['add']) {
            t.key = 'add';
            t.load = {
              unit: '',
              url: Whispercast.Uri.buildZend('default', 'users', 'add', sid)
            };
            t.label = t.title = _('Add a new user...');
            (new YAHOO.widget.TextNode(t, server.data.nodeUsers)).labelStyle += ' icon icon-users-add';
          }
        }

        if (response.model.acl['servers'] && response.model.acl['servers'][sid]) {
          if (response.model.acl['servers'][sid] && response.model.acl['servers'][sid]['set']) {
            t.key = 'edit';
            t.load = {
              unit: '',
              url: Whispercast.Uri.buildZend(null, 'servers', 'set', null),
              query: {id:sid}
            };
            t.label = t.title = _('Edit server setup...');
            (new YAHOO.widget.TextNode(t, server)).labelStyle += ' icon icon-servers-add';
          }
          if (response.model.acl['servers'][sid] && response.model.acl['servers'][sid]['delete']) {
            t.key = 'delete';
            t.load = {
              unit: '',
              url: Whispercast.Uri.buildZend(null, 'servers', 'delete', null),
              query: {id:sid}
            };
            t.label = t.title = _('Delete server...');
            (new YAHOO.widget.TextNode(t, server)).labelStyle += ' icon icon-servers-delete';
          }
        }
      }

      delete t.selectable;

      if (response.model.acl['servers'] && response.model.acl['servers'][0] && response.model.acl['servers'][0]['add']) {
        t.key = 'add';
        t.load = {
          unit: '',
          url: Whispercast.Uri.buildZend(null, 'servers', 'add')
        };
        t.label = t.title = _('Add a new server...');
        t.sc = Whispercast.App._cookieServer;

        (new YAHOO.widget.TextNode(t, parent)).labelStyle += ' no-icon';
      }
    }

    Whispercast.App._removeExpiredNodes(parent, 'sc', Whispercast.App._cookieServer);
    Whispercast.App._cookieServer++;

    if (Whispercast.App._currentPath) {
      Whispercast.App.selectNode(Whispercast.App._currentPath, true);
    }
  },
  retrieveServers: function(callback) {
    if (Whispercast.App._requests['servers']) {
      Whispercast.App._requests['servers'].conn.abort();
    }

    Whispercast.App._requests['servers'] =
    YAHOO.util.Connect.asyncRequest(
      'get',
       Whispercast.Uri.buildZend(null, 'servers', 'index', null, {format: 'json'}), {
        success: function(o) {
          delete(Whispercast.App._requests['servers']);
          var response = Whispercast.eval('return ('+o.responseText+')');

          Whispercast.App._updateServers(response);
          if (callback) {
            callback.call(null);
          }
        },
        failure: function(o) {
          delete(Whispercast.App._requests['servers']);

          Whispercast.App._updateServers(null);
          if (callback) {
            callback.call(null);
          }
        }
      }
    );
  }
};

