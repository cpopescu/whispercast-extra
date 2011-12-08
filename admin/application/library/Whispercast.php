<?php
class Whispercast {
  protected $_options;
  protected $_interfaces;

  public function __construct($options) {
    $this->_options = $options;
  }

  public function __get($name) {
    return $this->_options[$name];
  }

  public static function registerServer($id, $options) {
    $w = Zend_Registry::get('Whispercast');
    $w->_options['servers'][$id] = $options;
  }

  public static function getInterface($id) {
    $w = Zend_Registry::get('Whispercast');
    if (!isset($w->_interfaces[$server])) {
      $w->_interfaces[$id] = new Whispercast_Interface($w->_options['servers'][$id]);
    }
    return $w->_interfaces[$id];
  }
}
?>
