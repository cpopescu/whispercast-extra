<?php
class Whispercast_Servers extends Whispercast_Base {
  /**
   * Constructor for the Whispercast object.
   *
   * @param[in] object $interface The actual interface.
   *
   * @return void 
   */
  public function __construct($interface, $prefix = '') {
    parent::__construct($interface, $prefix);
  }

  /**
   * Creates a server.
   *
   * @param[in] string $id The id of the server we are creating.
   * @param[in] array $config The server configuration, as an array.
   *
   * @return string An error code or null if successful.
   */
  public function create($id, $config) {
    $iid = $this->_interface->getElementPrefix();

    $media_element_service = $this->_interface->MediaElementService();
    $standard_library_service = $this->_interface->StandardLibraryService();
    $extra_library_service = $this->_interface->ExtraLibraryService();
    $osd_library_service = $this->_interface->OsdLibraryService();

    $sources = new Whispercast_Sources($this->_interface);

    try {
      // The flow control stuff
      $fc_video = 0+$config['flow_control']['video'];
      $fc_total = 0+$config['flow_control']['total'];
       
      // We're creating one instance of OSD encoder and one instance of OSD associator per setup.
      $cs = $osd_library_service->AddOsdAssociatorElementSpec($iid.'_osda', true, false, new OsdAssociatorElementSpec());
      if (!$cs->success_) {
        throw new Exception($cs->error_);
      }
      $cs = $osd_library_service->AddOsdEncoderElementSpec($iid.'_osde', true, false, new OsdEncoderElementSpec());
      if (!$cs->success_) {
        throw new Exception($cs->error_);
      }

      // The authorizer used for previewing the server internals.
      $cs = $standard_library_service->AddSimpleAuthorizerSpec($iid.'_authorizer_preview', new SimpleAuthorizerSpec());
      if (!$cs->success_) {
        throw new Exception($cs->error_);
      }

      // Add the preview user/preview password.
      $p = $config['preview'];
      $cs = $this->_interface->SimpleAuthorizerService($iid.'_authorizer_preview')->SetUserPassword($p['user'], $p['password']);
      if (!$cs->success_) {
        throw new Exception($cs->error_);
      }

      // The authorizer used for publishing.
      $p = $config['publish'];
      $cs = $extra_library_service->AddHttpAuthorizerSpec($iid.'_authorizer_publish', new HttpAuthorizerSpec(
        array($p['server_address'].':'.$p['server_port']),
        $p['query'],
        array(),
        array(),
        false,
        null,
        null,
        null,
        null,
        true,
        $p['server_host']
      ));
      if (!$cs->success_) {
        throw new Exception($cs->error_);
      }

      // Create the private files renamer.
      $cs = $standard_library_service->AddStreamRenamerElementSpec($iid.'_files_renamer', true, false, new StreamRenamerElementSpec('^\(.*\)_public_files\(.*\)$', '$1_files$2'));
      if (!$cs->success_) {
        throw new Exception($cs->error_);
      }

      // Create the private files sources.
      $file_sources = array();
      for ($b = 0; $b < $config['disks']; $b++) {
        $cs = $standard_library_service->AddAioFileElementSpec($iid.'_files_'.$b, true, false, new AioFileElementSpec('flv', '/'.$b.'/private/files', '.*\.flv$'));
        if (!$cs->success_) {
          throw new Exception($cs->error_);
        }
        $file_sources[] = $iid.'_files_'.$b;
      }
      // Create the private files sources balancer.
      $cs = $standard_library_service->AddLoadBalancingElementSpec($iid.'_files', true, false, new LoadBalancingElementSpec($file_sources));
      if (!$cs->success_) {
        throw new Exception($cs->error_);
      }
      // Export the private files for preview.
      $cs = $media_element_service->StartExportElement(new ElementExportSpec(
        $this->_interface->getExportPrefix().'/'.$iid.'_files',
        'rtmp',
        $this->_interface->export.'/preview/files',
        null,
        null,
        $iid.'_authorizer_preview',
        $fc_video,
        $fc_audio
      ));
      $cs = $media_element_service->StartExportElement(new ElementExportSpec(
        $this->_interface->getExportPrefix().'/'.$iid.'_files',
        'http',
        $this->_interface->export.'/preview/files',
        null,
        null,
        $iid.'_authorizer_preview',
        $fc_video,
        $fc_audio
      ));
      // Export the private files for download.
      $cs = $media_element_service->StartExportElement(new ElementExportSpec(
        $this->_interface->getExportPrefix().'/'.$iid.'_files',
        'http',
        $this->_interface->export.'/download/files',
        null,
        null,
        $iid.'_authorizer_preview',
        0,
        0
      ));
      if (!$cs->success_) {
        throw new Exception($cs->error_);
      }

      // Create the public files sources.
      $file_sources = array();
      for ($b = 0; $b < $config['disks']; $b++) {
        $cs = $standard_library_service->AddAioFileElementSpec($iid.'_public_files_'.$b, true, false, new AioFileElementSpec('flv', '/'.$b.'/public/files', '.*\.flv$'));
        if (!$cs->success_) {
          throw new Exception($cs->error_);
        }
        $file_sources[] = $iid.'_public_files_'.$b;
      }
      // Create the public files sources balancer.
      $cs = $standard_library_service->AddLoadBalancingElementSpec($iid.'_public_files', true, false, new LoadBalancingElementSpec($file_sources));
      if (!$cs->success_) {
        throw new Exception($cs->error_);
      }
      // Export the public files.
      $cs = $media_element_service->StartExportElement(new ElementExportSpec(
        $this->_interface->getExportPrefix().'/'.$iid.'_files_renamer/'.$iid.'_public_files',
        'rtmp',
        $this->_interface->export.'/files',
        null,
        null,
        null,
        $fc_video,
        $fc_audio
      ));
      if (!$cs->success_) {
        throw new Exception($cs->error_);
      }
      $cs = $media_element_service->StartExportElement(new ElementExportSpec(
        $this->_interface->getExportPrefix().'/'.$iid.'_files_renamer/'.$iid.'_public_files',
        'http',
        $this->_interface->export.'/files',
        null,
        null,
        null,
        $fc_video,
        $fc_audio
      ));
      if (!$cs->success_) {
        throw new Exception($cs->error_);
      }

      // Create the thumbnails sources.
      $thumb_sources = array();
      for ($b = 0; $b < $config['disks']; $b++) {
        $cs = $standard_library_service->AddAioFileElementSpec($iid.'_thumbnails_'.$b, true, false, new AioFileElementSpec('raw', '/'.$b.'/private/files', '.*\.jpg$'));
        if (!$cs->success_) {
          throw new Exception($cs->error_);
        }
        $thumb_sources[] = $iid.'_thumbnails_'.$b;
      }
      // Create the thumbnails sources balancer.
      $cs = $standard_library_service->AddLoadBalancingElementSpec($iid.'_thumbnails', true, false, new LoadBalancingElementSpec($thumb_sources));
      if (!$cs->success_) {
        throw new Exception($cs->error_);
      }
      // Export the thumbnails.
      $cs = $media_element_service->StartExportElement(new ElementExportSpec(
        $iid.'_thumbnails',
        'http',
        $this->_interface->export.'/thumbnails',
        null,
        null,
        null,
        $fc_video,
        $fc_audio
      ));
      if (!$cs->success_) {
        throw new Exception($cs->error_);
      }

      // Create the live file sources.
      $live_sources = array();
      for ($b = 0; $b < $config['disks']; $b++) {
        $cs = $standard_library_service->AddAioFileElementSpec($iid.'_live_'.$b, true, false, new AioFileElementSpec('flv', '/'.$b.'/private/live', '.*\.flv$'));
        if (!$cs->success_) {
          throw new Exception($cs->error_);
        }
        $live_sources[] = $iid.'_live_'.$b;
      }
      // Create the live file sources balancer.
      $cs = $standard_library_service->AddLoadBalancingElementSpec($iid.'_live', true, false, new LoadBalancingElementSpec($live_sources));
      if (!$cs->success_) {
        throw new Exception($cs->error_);
      }
      // Create the live file sources balancer.
      $cs = $standard_library_service->AddTimeSavingElementSpec($iid.'_ts', true, false, new TimeSavingElementSpec(3600));
      if (!$cs->success_) {
        throw new Exception($cs->error_);
      }

      // The RTMP sources.
      $cs = $sources->create($id, array('type'=>'rtmp', 'authorizer'=>$iid.'_authorizer_publish'));
      if ($cs !== null) {
        throw new Exception($cs);
      }
      // The HTTP sources.
      $cs = $sources->create($id, array('type'=>'http', 'authorizer'=>$iid.'_authorizer_publish'));
      if ($cs !== null) {
        throw new Exception($cs);
      }

      return null;
    } catch(Exception $e) {
      $result = $e->getMessage();
    }

    $this->destroy($id);
    return $result;
  }
  /**
   * Destroys the server having the given id.
   *
   * @param[in] string $id The id of the server.
   *
   * @return void
   */
  public function destroy($id) {
    $iid = $this->_interface->getElementPrefix();

    $media_element_service = $this->_interface->MediaElementService();

    $sources = new Whispercast_Sources($this->_interface);

    try {
      $media_element_service->StopExportElement('http', $this->_interface->export.'/files');
    } catch(Exception $e) {
    }
    try {
      $media_element_service->StopExportElement('rtmp', $this->_interface->export.'/files');
    } catch(Exception $e) {
    }
    try {
      $media_element_service->StopExportElement('http', $this->_interface->export.'/download/files');
    } catch(Exception $e) {
    }
    try {
      $media_element_service->StopExportElement('rtmp', $this->_interface->export.'/preview/files');
    } catch(Exception $e) {
    }
    try {
      $media_element_service->StopExportElement('http', $this->_interface->export.'/thumbnails');
    } catch(Exception $e) {
    }
    try {
      $sources->destroy($id, 'http');
    } catch(Exception $e) {
    }
    try {
      $sources->destroy($id, 'rtmp');
    } catch(Exception $e) {
    }
    try {
      $media_element_service->DeleteElementSpec($iid.'_ts');
    } catch(Exception $e) {
    }
    try {
      $media_element_service->DeleteElementSpec($iid.'_live');
    } catch(Exception $e) {
    }
    for ($b = 0; $b < 32; $b++) {
      try {
        $media_element_service->DeleteElementSpec($iid.'_live_'.$b);
      } catch(Exception $e) {
      }
    }
    try {
      $media_element_service->DeleteElementSpec($iid.'_thumbnails');
    } catch(Exception $e) {
    }
    for ($b = 0; $b < 32; $b++) {
      try {
        $media_element_service->DeleteElementSpec($iid.'_thumbnails_'.$b);
      } catch(Exception $e) {
      }
    }
    try {
      $media_element_service->DeleteElementSpec($iid.'_public_files');
    } catch(Exception $e) {
    }
    for ($b = 0; $b < 32; $b++) {
      try {
        $media_element_service->DeleteElementSpec($iid.'_public_files_'.$b);
      } catch(Exception $e) {
      }
    }
    try {
      $media_element_service->DeleteElementSpec($iid.'_files');
    } catch(Exception $e) {
    }
    for ($b = 0; $b < 32; $b++) {
      try {
        $media_element_service->DeleteElementSpec($iid.'_files_'.$b);
      } catch(Exception $e) {
      }
    }
    try {
      $media_element_service->DeleteElementSpec($iid.'_files_renamer');
    } catch(Exception $e) {
    }
    try {
      $media_element_service->DeleteAuthorizerSpec($iid.'_authorizer_publish');
    } catch(Exception $e) {
    }
    try {
      $media_element_service->DeleteAuthorizerSpec($iid.'_authorizer_preview');
    } catch(Exception $e) {
    }
    try {
      $media_element_service->DeleteElementSpec($iid.'_osde');
    } catch(Exception $e) {
    }
    try {
      $media_element_service->DeleteElementSpec($iid.'_osda');
    } catch(Exception $e) {
    }
  }
}
?>
