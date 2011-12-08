(function() {
/* YUI shortcuts */
var Dom = YAHOO.util.Dom,
    Paginator = YAHOO.widget.Paginator;

/* The Table Whispercast namespace */
if (Whispercast.Table == undefined) {
  Whispercast.Table = Whispercast.Table ? Whispercast.Table : {
  };

  /* Total pages paginator UI. */
  Paginator.ui.PagesTotal = function(p) {
    this.paginator = p;

    p.subscribe('rowsPerPageChange',this.update,this,true);
    p.subscribe('totalRecordsChange',this.update,this,true);
    p.subscribe('destroy',this.destroy,this,true);

    //TODO: Make this work
    p.subscribe('pagesTotalContainerClassChange',this.rebuild,this,true);
  };
  Paginator.ui.PagesTotal.prototype = {
    destroy: function() {
      YAHOO.util.Event.purgeElement(this.container, true);
      this.container.parentNode.removeChild(this.container);
      this.container = null;
    },
    render: function(id_base) {
      this.container = document.createElement('span');
      this.container.id = id_base + '-total-pages';
      this.container.className = this.paginator.get('pagesTotalContainerClass');
      this.update({newValue : null, rebuild : true});
      return this.container;
    },
    update: function(e) {
      if (!e.rebuild) {
        if (this.current === this.paginator.getTotalPages()) {
          return;
        }
      }
      this.container.innerHTML = this.paginator.getTotalPages();
      this.current = this.paginator.getTotalPages();
    },
    rebuild: function(e) {
      e.rebuild = true;
      this.update(e);
    }
  };

  /* Pages dropdown paginator UI. */
  Paginator.ui.PagesDropdown = function(p) {
    this.paginator = p;

    p.subscribe('recordOffsetChange',this.update,this,true);
    p.subscribe('rowsPerPageChange',this.update,this,true);
    p.subscribe('totalRecordsChange',this.update,this,true);
    p.subscribe('destroy',this.destroy,this,true);

    //TODO: Make this work
    p.subscribe('pagesDropdownContainerClassChange', this.rebuild,this,true);
  };
  Paginator.ui.PagesDropdown.prototype = {
    destroy: function() {
      YAHOO.util.Event.purgeElement(this.container, true);
      this.container.parentNode.removeChild(this.container);
      this.container = null;
    },
    render: function(id_base) {
      this.container = document.createElement('select');
      this.container.id = id_base + '-pages';
      this.container.className = this.paginator.get('pagesDropdownContainerClass');
      this.update({newValue : null, rebuild : true});

      YAHOO.util.Event.on(this.container,'change', this.onChange, this, true);
      return this.container;
    },
    update: function(e) {
      for (var i = 1; i <= this.paginator.getTotalPages(); i++) {
        var option = document.createElement('option');
        option.value = i;
        option.innerHTML = i;
        if (i == this.paginator.getCurrentPage()) {
          option.selected = true;
        }
        this.container.appendChild(option);
      }
      while (this.container.options.length > this.paginator.getTotalPages()) {
        this.container.removeChild(this.container.firstChild);
      }
      this.current = this.paginator.getCurrentPage();
    },
    rebuild: function(e) {
      e.rebuild = true;
      this.update(e);
    },
    onChange: function (e) {
      this.paginator.setPage(parseInt(this.container.options[this.container.selectedIndex].value,10));
    }
  };

  /* Customize the YUI table class */
  Whispercast.Table._YUITable = function(container, columns, dataSource, config) {
    config.selectionMode = 'single';
    Whispercast.Table._YUITable.superclass.constructor.call(this, container, columns, dataSource, config);

    this.subscribe('rowMouseoverEvent', this.onEventHighlightRow);
    this.subscribe('rowMouseoutEvent', this.onEventUnhighlightRow);
    this.subscribe('rowClickEvent', this.onEventSelectRow);
  };
  YAHOO.lang.extend(Whispercast.Table._YUITable, YAHOO.widget.DataTable, {
    doBeforeLoadData: function(request, response, payload) {
      if (response.error) {
        this.render();
      } else {
        if (payload) {
          this._response = response;

          payload.totalRecords = response.meta.totalRecords;
          // BUGFIX: See http://yuilibrary.com/projects/yui2/ticket/2527763
          if (payload.pagination) {
            payload.pagination.recordOffset = (response.meta.page-1)*response.meta.rowsPerPage;
          }
        }
      } 
      return payload;
    },

    showTableMessage: function(sHTML, sClassName) {
      if (sClassName == YAHOO.widget.DataTable.CLASS_LOADING) {
        if (Dom.getStyle(this._elContainer, 'display') != 'none') {
          var id = Dom.getAttribute(this._elContainer, 'id');
          Whispercast.Progress.show(id, this.getTbodyEl());
        }
      }
      this.fireEvent('tableMsgShowEvent', {html:sHTML, className:sClassName});
    },
    hideTableMessage: function() {
      Whispercast.Progress.hide(Dom.getAttribute(this._elContainer, 'id'));
      this.fireEvent('tableMsgHideEvent');
    },

    _defaultPaginatorContainers: function (create) {
      var below_id = this._sId + '-paginator1',
      below = Dom.get(below_id);

      if (create && !below) {
        if (!below) {
          below = document.createElement('div');
          below.id = below_id;
          Dom.addClass(below, YAHOO.widget.DataTable.CLASS_PAGINATOR);

          this._elContainer.appendChild(below);
        }
      }

      return [below]; 
    }
  });

  /* Base table class */
  Whispercast.Table.Base = function(cfg) {
    this._initialize(cfg ? Whispercast.clone(cfg) : {});
  }
  Whispercast.Table.Base.prototype = {
    destroy: function() {
      if (this._listener) {
        Whispercast.log('Unregistering as a listener of "'+this._cfg.record.module+'.'+this._cfg.record.resource+'"...', 'TABLE');
        Whispercast.Notifier.unregisterListener(this._cfg.record.module+'.'+this._cfg.record.resource, this._listener);
        delete this._listener;
      }
      if (this._reloadTimeout) {
        clearTimeout(this._reloadTimeout);
        delete this._reloadTimeout;
      }
      if (this._table) {
        this._table.hideTableMessage();
        this._table.destroy();

        delete this._table;
      }
    },

    run: function(callback) {
      if (this._cfg.record) {
        if (this._listener == undefined) {
          this._listener = Whispercast.closure(this, function(op, record) {
            this.updateRecord(op, record);
          });
          Whispercast.log('Registering as a listener of "'+this._cfg.record.module+'.'+this._cfg.record.resource+'"...', 'TABLE');
          Whispercast.Notifier.registerListener(this._cfg.record.module+'.'+this._cfg.record.resource, this._listener);
        }
      }

      if (this._table == undefined) {
        this._table = new Whispercast.Table._YUITable(this._cfg.container, this._cfg.columns, this._cfg.dataSource, this._cfg.config);
      }
      // TODO: Quite hacky - we need to monitor the recordset so we update the record id to index mapping,
      // but the current YAHOO API at this moment is inconsistent - we cannot get the index at which a record
      // was inserted into the recordset by watching events, so we overload some methods...

      var recordset = this._table.getRecordSet();
      if (recordset) {
        Whispercast.overload(recordset, 'addRecord', Whispercast.closure(this, function(recordset, original, oData, index) {
          if (index == undefined) {
            this._idToIndex[oData.id] = recordset.getLength();
          } else {
            this._idToIndex[oData.id] = index;
          }
          return original.call(recordset, oData, index);
        }, recordset));
        Whispercast.overload(recordset, 'addRecords', Whispercast.closure(this, function(recordset, original, oData, index) {
          var offset;
          if (index == undefined) {
            offset = recordset.getLength();
          } else {
            offset = index;
          }
          for (var i = 0; i < oData.length; i++) {
            this._idToIndex[oData[i].id] = offset+i;
          }
          return original.call(recordset, oData, index);
        }, recordset));

        // TODO: Overload the deleteXxx() methods too..

        var rebuildIdToIndex = function() {
          var records = this._table.getRecordSet().getRecords();

          var state = this._table.getState();
          var limits = (state.pagination && state.pagination.records) ? state.pagination.records : [0, records.length-1];

          Whispercast.log('Rebuilding '+limits[0]+':'+limits[1], 'TABLE');

          this._idToIndex = {};
          for (var i = limits[0]; i <= limits[1]; i++) {
            Whispercast.log('Rebuild '+records[i].getData('id')+'->'+i, 'TABLE');
            this._idToIndex[records[i].getData('id')] = i;
          }
        }

        recordset.subscribe('recordsSetEvent', Whispercast.closure(this, function(e) {
          rebuildIdToIndex.call(this);
        }));
      }

      // TODO: We also watch the 'columnSortEvent' for updating the record id to index mapping
      // when a not-dynamic table is sorted...

      if (!this._table.get('dynamicData')) {
        this._table.subscribe('columnSortEvent', Whispercast.closure(this, function(e) {
          rebuildIdToIndex.call(this);
        }));
      }

      this._table.subscribe('cellClickEvent', Whispercast.closure(this, function(e) {
        var target = YAHOO.util.Event.getTarget(e);

        var column = this._table.getColumn(target);
        if (column.action) {
          var record = this._table.getRecord(target);
          if (typeof(column.action) == 'function') {
            column.action.call(null, this, record, column, e.event.target);
          } else {
            var serverId = record.getData('server_id');
            if (!serverId) {
              serverId = (this._table._response && this._table._response.meta && this._table._response.meta.server) ? this._table._response.meta.server.id : 0;
            }
            var params = {id:record.getData('id')};
            if (column.params) {
              for (var i in column.params) {
                params[i] = column.params[i];
              }
            }
            Whispercast.App.retrieveContent('', Whispercast.Uri.buildZend(column.module, column.controller, column.action, serverId), params);
          }
          return false;
        }
        this._table.onEventShowCellEditor(e);
      }));

      // We overload the renderPaginator method, as we render here the additional markup (links, etc..) also
      Whispercast.overload(this._table, 'renderPaginator', Whispercast.closure(this, function(original) {
        var tbody = document.createElement('tbody');
        var tr = document.createElement('tr');
        var td = document.createElement('td');
        Dom.setAttribute(td, 'colspan', this._table.getColumnSet().keys.length);
        Dom.setStyle(td, 'border', 'none');
        var div  = document.createElement('div');
        td.appendChild(div);
        tr.appendChild(td);
        tbody.appendChild(tr);
        this._table.getTableEl().appendChild(tbody);

        var paginator = this._table.get('paginator');
        if (paginator) {
          paginator.set('containers', [div]);
          original.call(this._table);
        } else {
          div.innerHTML = '<div class="yui-pg-container"><table border="0" cellpadding="0" cellspacing="0" style="border:none"><tr class="form"><td></td>'+this._filters+this._links+'</tr></table></div>';
        }
      }));

      // Schedule the callback after the first rendering
      var f = Whispercast.closure(this, function(e) {
        for (var i = 0; i < this._cfg.columns.length; i++) {
          if (this._cfg.columns[i].fill) {
            Dom.setStyle(this._table.getThEl(this._table.getColumn(i)), 'width', '100%');
          }
        }

        if (callback) {
          callback.call(null);
        }
        this._table.unsubscribe('postRenderEvent', f);
      });
      this._table.subscribe('postRenderEvent', f);

      // TODO: We overload this so we can fire up the state update mechanism for any new rows...

      Whispercast.overload(this._table, 'onDataReturnSetRows', Whispercast.closure(this, function(original, oRequest, oResponse, oPayload) {
        var result = original.call(this._table, oRequest, oResponse, oPayload);
        rebuildIdToIndex.call(this);

        Whispercast.log('Updating after a onDataReturnSetRows()...', 'TABLE');
        this.updateAll();
        return result;
      }));

      // initial load
      setTimeout(Whispercast.closure(this, function() {
        this.reload();
      }), 0);
    },

    getTable: function() {
      return this._table;
    },

    isRunning: function() {
      return this._table != undefined;
    },

    getRecordIndexById: function(id) {
      return this._idToIndex[id];
    },
    getRecordById: function(id) {
      var index = this._idToIndex[id];
      if (index != undefined) {
        return this._table.getRecordSet().getRecords()[index].getData();
      }
      return null;
    },

    getDataSourceResponse: function() {
      if (this._table) {
        return this._table._response;
      }
      return undefined;
    },

    reload: function() {
      if (this._reloadTimeout) {
        clearTimeout(this._reloadTimeout);
        delete this._reloadTimeout;
      }
      this._table.showTableMessage(this._table.get('MSG_LOADING'), YAHOO.widget.DataTable.CLASS_LOADING);

      var oState = this._table.getState();
      if (oState.pagination) {
        oState.pagination.recordOffset = 0;
      }

      var dataCallback = function() {
        if (this._table) {
          var args = [];
          for (var i = 0; i < arguments.length; i++) {
            args.push(arguments[i]);
          }
          this._table.onDataReturnSetRows.apply(this._table, args);
        }
      };
      var callback = {
        success: dataCallback,
        failure: dataCallback,
        argument: oState, // Pass along the new state to the callback
        scope: this
      };

      var generateRequest = this._table.get('generateRequest');
      this._table.getDataSource().sendRequest(generateRequest ? generateRequest.call(this._table, oState, this._table) : '', callback);
    },

    _initialize: function(cfg) {
      var container = cfg.container;
      var columns = cfg.columns;
      var dataSource = cfg.dataSource;
      var config = cfg.config;

      this._links = '';
      if (cfg.links) {
        for (var i = 0; i < cfg.links.length; i++) {
          this._links += '<td width="1%" style="border:none;white-space:nowrap">'+cfg.links[i]+'</td>';
        }
      }
      this._filters = '';
      if (cfg.filters) {
        for (var i = 0; i < cfg.filters.length; i++) {
          this._filters += '<td style="border:none;white-space:nowrap">'+cfg.filters[i]+'</td>';
        }
      } else {
        this._filters += '<td style="border:none;white-space:nowrap">&nbsp;</td>';
      }
      if (this._links != '' && this._filters != '') {
        this._filters += '&nbsp;';
      }

      if (config.paginator === false) {
        delete config.paginator;
      } else {
        var paginator = {
          rowsPerPage: 10,
          template: 
            '<table border="0" cellpadding="0" cellspacing="0" width="100%" style="border:none"><tr class="form"><td width="1%" align="left" style="border:none;white-space:nowrap">'+
//          _('<span class="label">Page</span>&nbsp;{PagesDropdown}&nbsp;<span class="label">of</span>&nbsp;<b>{PagesTotal}</b>,&nbsp;{RowsPerPageDropdown}&nbsp;<span class="label">records per page</span>&nbsp;[&nbsp;{FirstPageLink}&nbsp;{PreviousPageLink}&nbsp;-&nbsp;{NextPageLink}&nbsp;{LastPageLink}&nbsp;]')+
            _('<span class="label">Page</span>&nbsp;{PagesDropdown}&nbsp;<span class="label">of</span>&nbsp;<b>{PagesTotal}</b>,&nbsp;{RowsPerPageDropdown}&nbsp;<span class="label">records per page</span>')+
            '</div></td>'+
            this._filters+
            this._links+
            '</tr></table>',
          rowsPerPageOptions: [5, 10, 25, 50, 100],

          firstPageLinkLabel: '&lt;&lt;',
          previousPageLinkLabel: '&lt;',
          nextPageLinkLabel: '&gt;',
          lastPageLinkLabel: '&gt;&gt;'
        };
        if (config.paginator != undefined) {
          for (var i in config.paginator) {
            paginator[i] = config.paginator[i]
          }
        }
        config.paginator = new Paginator(paginator);
      }
      if (config.generateRequest == undefined) {
        config.generateRequest = function(state, paginator) {
          var pageCurrent = (state.pagination) ? state.pagination.page : 0;
          var pagePerPage = (state.pagination) ? state.pagination.rowsPerPage : 10;
          var sortBy = (state.sortedBy) ? state.sortedBy.key : '';
          var sortOrder = (state.sortedBy) ? ((state.sortedBy.dir == YAHOO.widget.DataTable.CLASS_ASC) ? 1 : 0) : '';
          return 'pageCurrent='+pageCurrent+'&pagePerPage='+pagePerPage+'&sortBy='+sortBy+'&sortOrder='+sortOrder;
        };
      }
      config.initialLoad = false;

      var formatter = Whispercast.closure(this, function(el, oRecord, oColumn, oData) {
        this._formatCell(el, oRecord, oColumn, oData, true, true);
      });
      for (var i in columns) {
        if (columns[i].formatter == undefined) {
          columns[i].formatter = formatter;
        }
      }

      this._cfg = cfg;

      this._idToIndex = {};
    },

    _formatCell: function(el, oRecord, oColumn, oData, htmlEscape, tooltip) {
      if (tooltip === true) {
        tooltip = Whispercast.htmlEscape((oData != undefined) ? oData : '');
      } else
      if (tooltip === undefined || tooltip === false) {
        tooltip = '';
      }

      var style = oColumn.contentStyle ? ' style="'+oColumn.contentStyle+'"' : '';

      if (htmlEscape) {
        el.innerHTML = '<span class="table-cell"'+style+'title="'+tooltip+'">'+Whispercast.htmlEscape((oData != undefined && oData != null) ? oData : '')+'</span>';
      } else {
        el.innerHTML = '<span class="table-cell"'+style+'title="'+tooltip+'">'+((oData != undefined && oData != null) ? oData : '')+'</span>';
      }
    },

    filterRecord: function(record) {
      return true;
    },
    updateRecord: function(op, record) {
      var reload = false;

      switch (op) {
        case 'add':
          if (!this.filterRecord(record)) {
            return false;
          }
          reload = true;        
          break;
        case 'update':
          if (!this.filterRecord(record)) {
            return false;
          }        

          var index = this._idToIndex[record.id];

          Whispercast.log('Update '+record.id+'->'+index, 'TABLE');
          if (index != undefined) {
            var existing = this._table.getRecord(index).getData();
            for (var i in record) {
              existing[i] = record[i];
            }
            this._table.updateRow(index, existing);
          } else {
            Whispercast.log('Record '+record.id+' not found, scheduling reload...', 'TABLE');
            reload = true;
          }
          break;
        default:
          reload = true;
      }

      if (reload) {
        Whispercast.log('Reloading...', 'TABLE');

        if (this._reloadTimeout == undefined) {
          Whispercast.log('Scheduling reload...', 'TABLE');
          this._reloadTimeout = setTimeout(Whispercast.closure(this, function() {
            Whispercast.log('Performing scheduled reload...', 'TABLE');
            delete this._reloadTimeout;
            this.reload();
          }), 1);
        }
      }
      return true;
    },

    updateAll: function() {
    }
  };
}

}());
