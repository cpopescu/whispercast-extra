<?php
class Whispercast_Aliases extends Whispercast_Base {
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
   * Creates an alias.
   *
   * @param[in] string $id The id of the alias.
   * @param[in] array $config The alias configuration, as an array.
   *
   * @return string An error code or null if successful.
   */
  public function create($id, $config) {
    /* $config:
     *
     * stream AS string
     */
    $iid = $this->_interface->getElementPrefix().'_'.$id;

    $cs = $this->_interface->MediaElementService()->SetMediaAlias($iid.'_alias', $config['stream']);
    if (!$cs->success_) {
      return $cs->error_;
    }
    return null;
  }

  /**
   * Destroys an existing alias.
   *
   * @param[in] string $id The id of the alias.
   *
   * @return string An error code or null if successful.
   */
  public function destroy($id) {
    $iid = $this->_interface->getElementPrefix().'_'.$id;

    $cs = $this->_interface->MediaElementService()->SetMediaAlias($iid.'_alias', '');
    if (!$cs->success_) {
      return $cs->error_;
    }
    return null;
  }

  /**
   * Returns the internal path for the given alias.
   *
   * @param[in] string $id The id of the alias.
   *
   * @return string The internal path or null on error.
   */
  public function getPath($id) {
    $iid = $this->_interface->getElementPrefix().'_'.$id;
    return '/'.$iid.'_alias';
  }

  /**
   * Retrieves the configuration of the given alias.
   *
   * @param[in] string $id The id of the alias.
   * @param[out] array $config On success an array that contains the alias configuration, on failure left unchanged.
   *
   * @return string An error code or null if successful.
   */
  public function getConfig($id, &$config) {
    $iid = $this->_interface->getElementPrefix().'_'.$id;

    $cs = $this->_interface->MediaElementService()->GetMediaAlias($iid.'_alias');
    if (!$cs->success_) {
      return $cs->error_;
    }

    $config['stream'] = $cs->result_;
    return null;
  }
  /**
   * Sets the configuration of the given alias.
   *
   * @param[in] string $id The id of the alias.
   * @param[in] array $config The alias configuration, as an array.
   *
   * @return string An error code or null if successful.
   */
  public function setConfig($id, $config) {
    return self::create($id, $config);
  }
}
?>
