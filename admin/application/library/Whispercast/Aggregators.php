<?php
class Whispercast_Aggregators extends Whispercast_Base {
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
    $flavours = array();
    foreach($config['flavours'] as $mask => $stream) {
      $flavours[] = new FlavouringSpec((int)$mask, substr($stream, 1));
    }
    return $flavours;
  }

  /**
   * Converts from an actual spec to the config structure.
   *
   * @param[in] object $spec The spec.
   *
   * @return array
   */
  private function _specToConfig($param) {
    $config = array();
    foreach($param as $mask=>$spec) {
      $config['flavours'][$spec->flavour_mask_] = '/'.$spec->media_prefix_;
    }
    return $config;
  }

  /**
   * Creates an aggregator.
   *
   * @param[in] string $id The id of the aggregator we are creating.
   * @param[in] array $config The aggregator configuration, as an array.
   *
   * @return string An error code or null if successful.
   */
  public function create($id, $config) {
    /* $config:
     *
     * flavours AS map<flavour AS int, media AS string>
     */
    $iid = $this->_interface->getElementPrefix().'_'.$id;

    $extra_library_service = $this->_interface->ExtraLibraryService();

    $spec = $this->_configToSpec($config);

    $cs = $extra_library_service->AddFlavouringElementSpec($iid.'_aggregator', true, false, new FlavouringElementSpec($spec));
    if (!$cs->success_) {
      return $cs->error_;
    }

    return null;
  }
  /**
   * Destroys the aggregator having the given id.
   *
   * @param[in] string $id The id of the aggregator.
   *
   * @return string An error code or null if successful.
   */
  public function destroy($id) {
    $iid = $this->_interface->getElementPrefix().'_'.$id;

    $media_element_service = $this->_interface->MediaElementService();

    $cs = $media_element_service->DeleteElementSpec($iid.'_aggregator');
    if ($cs->success_) {
      return null;
    }
    return $cs->error_;
  }

  /**
   * Returns the internal path for the given aggregator.
   *
   * @param[in] string $id The id of the aggregator element.
   *
   * @return string The internal path or null on error.
   */
  public function getPath($id) {
    $iid = $this->_interface->getElementPrefix().'_'.$id;
    return $iid.'_aggregator';
  }

  /**
   * Retrieves the configuration of the given aggregator.
   *
   * @param[in] string $id The id of the aggregator.
   * @param[out] array $config On success an array that contains the configuration, on failure left unchanged.
   *
   * @return string An error code or null if successful.
   */
  public function getConfig($id, &$config) {
    $iid = $this->_interface->getElementPrefix().'_'.$id;

    $cs = $this->_interface->FlavouringElementService($iid.'_aggregator')->GetFlavours();
    if (!$cs->success_) {
      return $cs->error_;
    }

    $config = $this->_specToConfig($cs->result_);
    return null;
  }
  /**
   * Sets the configuration of the given aggregator.
   *
   * @param[in] string $id The id of the aggregator.
   * @param[in] array $config The aggregator configuration, as an array.
   *
   * @return string An error code or null if successful.
   */
  public function setConfig($id, $config) {
    $iid = $this->_interface->getElementPrefix().'_'.$id;

    $extra_library_service = $this->_interface->ExtraLibraryService();

    $spec = $this->_configToSpec($config);

    $cs = $this->_interface->FlavouringElementService($iid.'_aggregator')->SetFlavours($spec);
    if (!$cs->success_) {
      return $cs->error_;
    }

    return null;
  }
}
?>
