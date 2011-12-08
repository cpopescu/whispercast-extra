<?php
class App_Resource_Whispercast extends Zend_Application_Resource_ResourceAbstract {
  protected $_whispercast;

  public function init() {
    if ($this->_whispercast === null) {
      require_once 'Whispercast.php';
      $this->_whispercast = new Whispercast($this->getOptions());

      Zend_Registry::set('Whispercast', $this->_whispercast);
      return $this->_whispercast;
    }
  }
}
?>
