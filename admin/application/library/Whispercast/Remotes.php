<?php
class Whispercast_Remotes extends Whispercast_Base {
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
   * Converts from a config structure to the actual spec.
   *
   * @param[in] array $config The config.
   *
   * @return object
   */
  private function _configToSpec($config) {
    return $config;
  }

  /**
   * Converts from an actual spec to the config structure.
   *
   * @param[in] object $spec The spec.
   *
   * @return array
   */
  private function _specToConfig($spec) {
    return $spec;
  }
  
  /**
   * Creates a new remote stream.
   *
   * @param[in] string $id The id of the remote stream.
   * @param[in] array $config The remote stream configuration, as an array.
   *
   * @return string An error code or null if successful.
   */
  public function create($id, $config) {
    $iid = $this->_interface->getElementPrefix().'_'.$id;

    $standard_library_service = $this->_interface->StandardLibraryService();

    $spec = $this->_configToSpec($config);

    $cs = $standard_library_service->AddHttpClientElementSpec($iid.'_remote', true, false, new HttpClientElementSpec(array(new HttpClientElementDataSpec($id, $spec['host_ip'], $spec['port'], $spec['path_escaped'], $spec['should_reopen'], $spec['fetch_only_on_request'], $spec['media_type'], $spec['remote_user'], $spec['remote_password']))));
    if (!$cs->success_) {
      return $cs->error_;
    }
    return null;
  }

  /**
   * Destroys an existing remote stream.
   *
   * @param[in] string $id The id of the remote stream.
   *
   * @return string An error code or null if successful.
   */
  public function destroy($id) {
    $iid = $this->_interface->getElementPrefix().'_'.$id;

    $media_element_service = $this->_interface->MediaElementService();

    $cs = $media_element_service->DeleteElementSpec($iid.'_remote');
    if (!$cs->success_) {
      return $cs->error_;
    }
    return null;
  }

  /**
   * Returns the internal path for the given remote stream.
   *
   * @param[in] string $id The id of the remote stream.
   *
   * @return string The internal path or null on error.
   */
  public function getPath($id) {
    $iid = $this->_interface->getElementPrefix().'_'.$id;
    return '/'.$iid.'_remote/'.$id;
  }

  /**
   * Retrieves the current configuration of the given remote stream.
   *
   * @param[in] string $id The id of the remote stream.
   * @param[out] array $config On success an array that contains the remote stream configuration, on failure left unchanged.
   *
   * @return string An error code or null if successful.
   */
  public function getConfig($id, &$config) {
    return 'Not implemented';
  }
  /**
   * Sets the configuration of the given switch.
   *
   * @param[in] string $id The id of the program element.
   * @param[in] string $config The program config.
   *
   * @return string An error code or null if successful.
   */
  public function setConfig($id, $config) {
    return 'Not implemented';
  }
}
?>
