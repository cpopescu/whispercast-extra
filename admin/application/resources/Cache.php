<?php
class App_Resource_Cache extends Zend_Application_Resource_ResourceAbstract {
  protected $_cache;

  public function init() {
    if ($this->_cache === null) {
      $this->_cache = Zend_Cache::factory(
        'Core',
        'File',
        array(
          'automatic_serialization' => true
        ),
        $this->getOptions()
      );
      Zend_Db_Table_Abstract::setDefaultMetadataCache($this->_cache);

      Zend_Registry::set('Cache', $this->_cache);
      return $this->_cache;
    }
  }
}
?>
