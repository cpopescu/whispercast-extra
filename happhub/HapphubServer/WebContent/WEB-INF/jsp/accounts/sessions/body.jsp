<div id="content-wrapper">
  <div id="content">
    <div id="session" class="ui-helper-clearfix">
      <div id="session-cameras-section" class="ui-helper-clearfix ui-separator-bottom">
        <div id="session-camera-output" class="ui-border ui-background"></div>
        <div id="session-cameras" class="ui-border ui-background">
          <div id="session-cameras-scroller" class="horizontal-list scrollable"></div>
        </div>
      </div>
      <div class="ui-border ui-background ui-helper-clearfix">
        <div id="session-preview-section">
          <div id="session-preview" class="ui-separator-right"></div>
        </div>
        <div id="session-assets-section">
          <div id="session-assets">
            <div id="session-assets-scroller" class="scrollable"></div>
          </div>
        </div>
      </div>
    </div>
  </div>
</div>

<script type="text/javascript">
 var ncid = 0;
</script>
<script type="text/javascript">
  $(function(){
    var output = happhub.ui.addPlayer(
      '#session-camera-output',
      null,
      '',
      happhub.sessions.previewUrlOutput(happhub.model.session),
      'Output',
      [{text:'Preview', event:'camera-preview'}],
      ['play-stop']
    );
    
    $.each(happhub.model.session.cameras, function(i, e) {
      happhub.ui.addPlayer(
        '#session-cameras-scroller',
        null,
        'horizontal-list-item',
        happhub.sessions.previewUrlCamera(happhub.model.session, e),
        e.name,
        [{text:'Live!', event:'camera-switch'},{text:'Preview', event:'camera-preview'}],
        ['play-stop']
      );
    })
    $('#session-cameras-scroller').scroller('update');
    
    var preview = happhub.ui.addPlayer(
      '#session-preview',
      null,
      '',
      '',
      'Output',
      [],
      ['play-stop','pause'],
      undefined,
      undefined,
      function() {
        setTimeout(function() {
          preview.player.video('player').setURL(preview.url = output.url);
          preview.player.video('player').play();
        }, 100);
      }
    );
    
    var assetCommands = [{text:'Live!', event:'asset-switch'},{text:'Preview', event:'asset-preview'}];
    
    happhub.ui.addAsset('#session-assets-scroller', null, '', '', '/images/logo.png', 'Asset '+ncid++, assetCommands);
    happhub.ui.addAsset('#session-assets-scroller', null, '', '', '/images/happhub.png', 'Asset '+ncid++, assetCommands);
    happhub.ui.addAsset('#session-assets-scroller', null, '', '', '', 'Asset '+ncid++, assetCommands);
    happhub.ui.addAsset('#session-assets-scroller', null, '', '', '/images/logo.png', 'Asset '+ncid++, assetCommands);
    happhub.ui.addAsset('#session-assets-scroller', null, '', '', '/images/happhub.png', 'Asset '+ncid++, assetCommands);
    happhub.ui.addAsset('#session-assets-scroller', null, '', '', '', 'Asset '+ncid++, assetCommands);
    happhub.ui.addAsset('#session-assets-scroller', null, '', '', '/images/logo.png', 'Asset '+ncid++, assetCommands);
    happhub.ui.addAsset('#session-assets-scroller', null, '', '', '/images/happhub.png', 'Asset '+ncid++, assetCommands);
    happhub.ui.addAsset('#session-assets-scroller', null, '', '', '', 'Asset '+ncid++, assetCommands);
    
    $(document).on('camera-preview', function(e, d) {
      preview.caption.html(d.caption.html());
      var player = preview.player.video('player');
      
      player.stop();
      
      player.setURL(preview.url = d.url);
      preview.updateUI();
      
      player.play();
      return false;
    });
    $(document).on('camera-switch', function(e, d) {
      return false;
    });
    
    $(document).on('asset-preview', function(e, d) {
      preview.caption.html(d.caption.html());
      return false;
    });
    $(document).on('asset-switch', function(e, d) {
      return false;
    });
    
    setTimeout(function() {
      $('#session-cameras-scroller').scroller('update');
      $('#session-assets-scroller').scroller('update');
    }, 1);
    
    var originalPreviewStateCallback = preview.stateCallback;
    preview.stateCallback = function(id, state) {
      
      originalPreviewStateCallback.call(this, id, state);
      preview.stateCallback = originalPreviewStateCallback;
      
    };
    var originalOutputStateCallback = output.stateCallback;
    output.stateCallback = function(id, state) {
      originalOutputStateCallback.call(this, id, state);
      output.stateCallback = originalOutputStateCallback;
      
      setTimeout(function() {
        preview.player.video('player').play();
      }, 1);
    };
  });
</script>