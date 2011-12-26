<?php
class Whispercast_Sources extends Whispercast_Base {
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
   * Creates or updates a source.
   *
   * @param[in] string $id The id of the source.
   * @param[in] string $config The source configuration, as an array.
   *
   * @return string An error code or null if successful.
   */
  public function create($id, $config) {
    /* $config:
     *
     * type AS string
     * authorizer AS string
     */
    $iid = $this->_interface->getElementPrefix().'_';

    $standard_library_service = $this->_interface->StandardLibraryService();

    $type = $config['type'];
    switch ($type) {
      case 'rtmp':
        $cs = $this->_interface->StandardLibraryService()->
          AddRtmpPublishingElementSpec($iid.'rtmp_source', true, false, new RtmpPublishingElementSpec(array(), $config['authorizer']));
        break;
      case 'http':
        $cs = $this->_interface->StandardLibraryService()->
          AddHttpServerElementSpec($iid.'http_source', true, false, new HttpServerElementSpec('/'.$iid.'http_source', array(), $config['authorizer']));
        break;
      default:
        return 'Not supported';
    }

    if (!$cs->success_) {
      return $cs->error_;
    }
    return null;
  }

  /**
   * Destroys an existing source.
   *
   * @param[in] string $id The id of the source.
   * @param[in] string $type The type of the source.
   *
   * @return string An error code or null if successful.
   */
  public function destroy($id, $type) {
    $iid = $this->_interface->getElementPrefix().'_';

    switch ($type) {
      case 'rtmp':
        $cs = $this->_interface->MediaElementService()->DeleteElementSpec($iid.'rtmp_source');
        break;
      case 'http':
        $cs = $this->_interface->MediaElementService()->DeleteElementSpec($iid.'http_source');
        break;
      default:
        ;
    }

    if (!$cs->success_) {
      return $cs->error_;
    }
    return null;
  }

  /**
   * Returns the internal path for the given stream data.
   *
   * @param[in] string $id The id of the source.
   * @param[in] string $type The type of the source.
   *
   * @return string The internal path or null on error.
   */
  public function getPath($id, $type) {
    $iid = $this->_interface->getElementPrefix().'_';

    switch ($type) {
      case 'rtmp':
        return $iid.'rtmp_source';
      case 'http':
        return $iid.'http_source';
    }
    return null;
  }

  /**
   * Retrieves the current configuration of the given source.
   *
   * @param[in] string $id The id of the source.
   * @param[out] array $config On success an array that contains the source configuration, on failure left unchanged.
   *
   * @return string An error code or null if successful.
   */
  public function getConfig($id, &$config) {
    return 'Not implemented';
  }
  /**
   * Sets the configuration of the given source.
   *
   * @param[in] string $id The id of the source.
   * @param[in] array $config The program configuration, as an array.
   *
   * @return string An error code or null if successful.
   */
  public function setConfig($id, $config) {
    return 'Not implemented';
  }

  /**
   * Retrieves all the streams defined on a source.
   *
   * @param[in] string $id The id of the source.
   * @param[in] string $type The type of the source.
   * @param[out] array $streams On success an array that contains all the streams defined on the given source, on failure left unchanged.
   *
   * @return string An error code or null if successful.
   */
  public function getStreams($id, $type, &$streams) {
    $iid = $this->_interface->getElementPrefix().'_';

    switch ($type) {
      case 'rtmp':
        $cs = $this->_interface->MediaElementService()->ListMedia($iid.'rtmp_source');
        break;
      case 'http':
        $cs = $this->_interface->MediaElementService()->ListMedia($iid.'http_source');
        break;
      default:
        return 'Not supported';
    }
    if (!$cs->success_) {
      return $cs->error_;
    }

    $streams = $cs->result_;
    return null;
  }

  /**
   * Returns the internal path for the given stream.
   *
   * @param[in] string $id The id of the source.
   * @param[in] string $type The id of the source.
   * @param[in] string $name The name of the stream.
   *
   * @return string The internal path or null on error.
   */
  public function getStreamPath($id, $type, $name) {
    $iid = $this->_interface->getElementPrefix().'_';
    switch ($type) {
      case 'rtmp':
        return '/'.$iid.'rtmp_source/'.$name;
      case 'http':
        return '/'.$iid.'http_source/'.$name;
    }
    return null;
  }

  /**
   * Creates a new stream on a source.
   *
   * @param[in] string $id The id of the source.
   * @param[in] string $type The type of the source.
   * @param[in] string $name The name of the stream we are creating.
   *
   * TODO: Explain parameters...
   *
   * @return string An error code or null if successful.
   */
  public function createStream($id, $type, $name, $save_dir = null, $append_only = null, $buildup_interval_sec_ = null, $buildup_delay_sec = null, $prefill_buffer_ms = null, $advance_media_ms = null) {
    $iid = $this->_interface->getElementPrefix().'_';

    switch ($type) {
      case 'rtmp':
        $cs = $this->_interface->RtmpPublishingElementService($iid.'rtmp_source')->
          AddImport(new RtmpPublishingElementDataSpec($name, 'flv', $save_dir, $append_only, false, $buildup_interval_sec_, $buildup_delay_sec, $prefill_buffer_ms, $advance_media_ms));
        break;
      case 'http':
        $cs = $this->_interface->HttpServerElementService($iid.'http_source')->
          AddImport(new HttpServerElementDataSpec($name, 'flv', $save_dir, $append_only, false, $buildup_interval_sec_, $buildup_delay_sec, $prefill_buffer_ms, $advance_media_ms));
        break;
      default:
        return 'Not supported';
    }

    if (!$cs->success_) {
      return $cs->error_;
    }
    return null;
  }
  /**
   * Destroys the stream having the given name, defined on the given source.
   *
   * @param[in] string $id The id of the source.
   * @param[in] string $type The type of the source.
   * @param[in] string $name The name of the stream we are destroying.
   *
   * @return string An error code or null if successful.
   */
  public function destroyStream($id, $type, $name) {
    $iid = $this->_interface->getElementPrefix().'_';

    switch ($type) {
      case 'rtmp':
        $cs = $this->_interface->RtmpPublishingElementService($iid.'rtmp_source')->
          DeleteImport($name);
        break;
      case 'http':
        $cs = $this->_interface->HttpServerElementService($iid.'http_source')->
          DeleteImport($name);
        break;
      default:
        return 'Not supported';
    }

    if (!$cs->success_) {
      return $cs->error_;
    }
    return null;
  }
}
?>
