<?php
class Whispercast_Rangers extends Whispercast_Base {
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
   * Creates or updates a range element.
   *
   * @param[in] string $id The id of the range element.
   * @param[in] array $config The range element configuration, as an array.
   *
   * @return string An error code or null if successful.
   */
  public function create($id, $config) {
    /* $config:
     *
     * home_dir AS string 
     * root_media_path AS string 
     * file_prefix AS string 
     * utc_requests AS boolean
    */
    $iid = $this->_interface->getElementPrefix().'_'.$id;

    $cs = $this->_interface->ExtraLibraryService()->AddTimeRangeElementSpec($iid.'_ranger', true, false, new TimeRangeElementSpec($config['home_dir'], $config['root_media_path'], $config['file_prefix'], $config['utc_requests']));
    if (!$cs->success_) {
      return $cs->error_;
    }
    return null;
  }

  /**
   * Destroys an existing range element.
   *
   * @param[in] string $id The id of the range element.
   *
   * @return string An error code or null if successful.
   */
  public function destroy($id) {
    $iid = $this->_interface->getElementPrefix().'_'.$id;

    $media_element_service = $this->_interface->MediaElementService();

    $cs = $media_element_service->DeleteElementSpec($iid.'_ranger');
    if (!$cs->success_) {
      return $cs->error_;
    }
    return null;
  }

  /**
   * Returns the internal path for the given range.
   *
   * @param[in] string $id The id of the range element.
   *
   * @return string The internal path or null on error.
   */
  public function getPath($id) {
    $iid = $this->_interface->getElementPrefix().'_'.$id;
    return $iid.'_ranger';
  }

  /**
   * Retrieves the current configuration of the given range element.
   *
   * @param[in] string $id The id of the range element.
   * @param[out] array $config On success an array that contains the range configuration, on failure left unchanged.
   *
   * @return string An error code or null if successful.
   */
  public function getConfig($id, &$config) {
    return 'Not implemented';
  }
  /**
   * Sets the configuration of the given range element.
   *
   * @param[in] string $id The id of the range element.
   * @param[in] array $config The program config.
   *
   * @return string An error code or null if successful.
   */
  public function setConfig($id, $config) {
    return 'Not implemented';
  }

  /**
   * Creates a range.
   *
   * @param[in] string $id The id of the range.
   * @param[in] string $ranger_id The id of the ranger on which the range is defined.
   * @param[in] string $begin The begin of the interval.
   * @param[in] string $end The end of the interval.
   *
   * @return string An error code or null if successful.
   */
  public function createRange($id, $ranger_id, $begin, $end) {
    $iid = $this->_interface->getElementPrefix().'_'.$ranger_id;

    $cs = $this->_interface->TimeRangeElementService($iid.'_ranger')->SetRange($id, $begin.'/'.$end);
    if (!$cs->success_) {
      return $cs->error_;
    }

    return null;
  }
  /**
   * Destroys the range having the given id.
   *
   * @param[in] string $id The id of the range.
   * @param[in] string $ranger_id The id of the ranger on which the range is defined.
   *
   * @return string An error code or null if successful.
   */
  public function destroyRange($id, $ranger_id) {
    $iid = $this->_interface->getElementPrefix().'_'.$ranger_id;

    $cs = $this->_interface->TimeRangeElementService($iid.'_ranger')->SetRange($id, '');
    if (!$cs->success_) {
      return $cs->error_;
    }

    return null;
  }

  /**
   * Returns the internal path for the given range.
   *
   * @param[in] string $id The id of the range.
   * @param[in] string $ranger_id The id of the ranger on which the range is defined.
   *
   * @return string The internal path or null on error.
   */
  public function getRangePath($id, $ranger_id) {
    $iid = $this->_interface->getElementPrefix().'_'.$ranger_id;
    return '/'.$iid.'_ranger/'.$id;
  }

  /**
   * Retrieves the range having the given id.
   *
   * @param[in] string $id The id of the range.
   * @param[in] string $ranger_id The id of the ranger on which the range is defined.
   * @param[out] string $begin The begin of the interval.
   * @param[out] string $end The end of the interval.
   *
   * @return string An error code or null if successful.
   */
  public function getRange($id, $ranger_id, &$begin, &$end) {
    $iid = $this->_interface->getElementPrefix().'_'.$ranger_id;

    $cs = $this->_interface->TimeRangeElementService($iid.'_ranger')->GetRange($id);
    if (!$cs->success_) {
      return $cs->error_;
    }

    list($begin, $end) = explode('/', $cs->result_);
    return null;
  }
  /**
   * Updates the range having the given id.
   *
   * @param[in] string $id The id of the range.
   * @param[in] string $ranger_id The id of the ranger with which the range serves.
   * @param[in] string $begin The begin of the interval.
   * @param[in] string $end The end of the interval.
   *
   * @return string An error code or null if successful.
   */
  public function setRange($id, $ranger_id, $begin, $end) {
    return self::createRange($id, $ranger_id, $begin, $end);
  }
}
?>
