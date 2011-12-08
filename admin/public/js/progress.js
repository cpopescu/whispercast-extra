(function() {
/* YUI shortcuts */
var Dom = YAHOO.util.Dom;

/* The Progress Whispercast namespace */
if (Whispercast.Progres == undefined) {
  Whispercast.Progress = {
    _zIndex: 100,

    _containers: {},

    show: function(id, element) {
      var P = Whispercast.Progress;

      id = (arguments.length > 0) ? id : '';

      if (!element) {
        if (id == '') {
          element = document.body;
        } else {
          element = document.getElementById(id);
        }
      } else {
        element = Dom.get(element);
      }
      if (!element) {
        return false;
      }

      if (P._containers[id] != undefined) {
        if (P._containers[id].element == element) {
          if (P._containers[id].active) {
            P.layout(id);
          }
          return;
        }
        P.hide(id);
      }

      var zIndexBase = 0;
      for (var zElement = element; zElement && zElement != document.body; zElement = zElement.parentNode) {
        var zIndex = Dom.getStyle(zElement, 'z-index');
        switch (zIndex) {
          case 'auto':
          case 'inherit':
            break;
          default:
            zIndexBase += 1*zIndex;
        }
      }

      var container = {active:false,element:element};

      var mask = document.createElement('div');
      Dom.addClass(mask, 'mask');
      Dom.addClass(mask, 'progress-mask');
      Dom.setStyle(mask, 'display', 'none');
      Dom.setStyle(mask, 'position', 'absolute');
      Dom.setStyle(mask, 'opacity', 0.1);
      Dom.setStyle(mask, 'z-index', zIndexBase+P._zIndex++);
      container.mask = mask;
      var image = document.createElement('img');
      image.src = Whispercast.Uri.base+'/images/progress.gif';
      Dom.setStyle(image, 'display', 'none');
      Dom.setStyle(image, 'position', 'absolute');
      Dom.setStyle(image, 'z-index', zIndexBase+P._zIndex++);
      Dom.setStyle(image, 'width', '32px');
      Dom.setStyle(image, 'height', '32px');
      container.image = image;

      P._containers[id] = container;

      Dom.insertBefore(container.mask, document.body.firstChild);
      Dom.insertBefore(container.image, document.body.firstChild);
      container.active = true;
      P.layout(id);
    },
    hide: function(id) {
      var P = Whispercast.Progress;

      id = (arguments.length > 0) ? id : '';
      if (P._containers[id] == undefined) {
        return;
      }

      var container = P._containers[id];

      if (container.image.parentNode) {
        container.image.parentNode.removeChild(container.image);
      }
      if (container.mask.parentNode){ 
        container.mask.parentNode.removeChild(container.mask);
      }

      P._zIndex -= 2;
      delete(P._containers[id]);
    },

    layout: function(id) {
      var P = Whispercast.Progress;

      if (arguments.length == 0) {
        if (P._layoutTimer) {
          clearTimeout(P._layoutTimer);
          delete(P._layoutTimer);
        }
        P._layoutTimer = setTimeout(
          function() {
            delete(P._layoutTimer);

            for (var i in P._containers) {
              P.layout(i);
            }
          },
          10
        );
      }
      if (P._containers[id] == undefined) {
        return;
      } 

      var container = P._containers[id];

      var region;
      region = (container.element != document.body) ? Dom.getRegion(container.element) : Dom.getClientRegion();

      var visible = (container.element != document.body) ? container.element.offsetWidth > 0 && container.element.offsetHeight > 0 : true;
      if (visible) {
        for (var vElement = container.element; vElement && vElement != document.body; vElement = vElement.parentNode) {
          if (Dom.getStyle(vElement, 'visibility') == 'hidden') {
            visible = false;
            break;
          }
        }
      }

      Dom.setStyle(container.mask, 'top', region.top+'px');
      Dom.setStyle(container.mask, 'left', region.left+'px');
      Dom.setStyle(container.mask, 'width', region.width+'px');
      Dom.setStyle(container.mask, 'height', region.height+'px');
      Dom.setStyle(container.mask, 'display', visible ? 'block' : 'none');
      var iw = Math.min(32, region.width-2);
      var ih = Math.min(32, region.height-2);
      Dom.setStyle(container.image, 'width', Math.max(1,Math.min(iw, ih))+'px');
      Dom.setStyle(container.image, 'height', Math.max(1,Math.min(iw, ih))+'px');
      Dom.setStyle(container.image, 'top', region.top+(region.height-ih)/2+'px');
      Dom.setStyle(container.image, 'left', region.left+(region.width-iw)/2+'px');
      Dom.setStyle(container.image, 'display', visible ? 'block' : 'none');
    }
  };

  YAHOO.util.Event.addListener(window, 'resize', function() {
    Whispercast.Progress.layout();
  });
}
 
}());
