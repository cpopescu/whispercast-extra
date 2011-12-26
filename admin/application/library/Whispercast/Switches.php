<?php
class Whispercast_Switches extends Whispercast_Base {
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
   * Creates a new switch.
   *
   * @param[in] string $id The id of the switch.
   * @param[in] array $config The switch configuration, as an array.
   *
   * @return string An error code or null if successful.
   */
  public function create($id, $config) {
    $iid = $this->_interface->getElementPrefix().'_'.$id;

    $standard_library_service = $this->_interface->StandardLibraryService();

    $cs = $standard_library_service->AddOnCommandPolicySpec($iid.'_switch', new OnCommandPolicySpec(5, ''));
    if (!$cs->success_) {
      return $cs->error_;
    }
    $cs = $standard_library_service->AddSwitchingElementSpec($iid.'_switch', true, false, new SwitchingElementSpec($iid.'_switch', 'flv', 255, 1, 0, false));
    if (!$cs->success_) {
      $this->_interface->MediaElementService()->DeletePolicySpec($iid.'_switch');
      return $cs->error_;
    }
    return null;
  }

  /**
   * Destroys an existing switch.
   *
   * @param[in] string $id The id of the switch.
   *
   * @return string An error code or null if successful.
   */
  public function destroy($id) {
    $iid = $this->_interface->getElementPrefix().'_'.$id;

    $media_element_service = $this->_interface->MediaElementService();

    $cs = $media_element_service->DeleteElementSpec($iid.'_switch');
    if ($cs->success_) {
      $media_element_service->DeletePolicySpec($iid.'_switch');
      return null;
    }
    return $cs->error_;
  }

  /**
   * Returns the internal path for the given switch.
   *
   * @param[in] string $id The id of the switch.
   *
   * @return string The internal path or null on error.
   */
  public function getPath($id) {
    $iid = $this->_interface->getElementPrefix().'_'.$id;
    return $iid.'_switch';
  }

  /**
   * Retrieves the current configuration of the given switch.
   *
   * @param[in] string $id The id of the switch.
   * @param[out] array $config On success an array that contains the switch configuration, on failure left unchanged.
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

  /**
   * Returns the media currently playing on the given switch. 
   *
   * @param[in] string $id The id of the switch.
   * @param[out] array $media The current/next playing media.
   *
   * @return string An error code or null if successful.
   */
  public function getMedia($id, &$media) {
    $iid = $this->_interface->getElementPrefix().'_'.$id;

    $cs = $this->_interface->SwitchPolicyService($iid.'_switch')->GetPlayInfo();
    if (!$cs->success_) {
      return $cs->error_;
    }

    $media = array(
      'current' => $cs->result_->current_media_,
      'next' => $cs->result_->next_media_
    );
    return null;
  }
  /**
   * Switch the given switch to the given media.
   *
   * @param[in] string $id The id of the switch.
   * @param[in] string $media The media to switch to.
   * @param[in] string $set_as_default Set the given media as the default.
   * @param[in] string $switch_now Switch now (as opposed to waiting the current media to terminate).
   *
   * @return string An error code or null if successful.
   */
  public function setMedia($id, $media, $set_as_default = true, $switch_now = true) {
    $iid = $this->_interface->getElementPrefix().'_'.$id;

    $cs = $this->_interface->SwitchPolicyService($iid.'_switch')->SwitchPolicy($media, $set_as_default ? true  : false, $switch_now ? true : false);
    if (!$cs->success_) {
      return $cs->error_;
    }
    return null;
  }
}
?>
