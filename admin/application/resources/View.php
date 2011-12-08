<?php
class App_Resource_View extends Zend_Application_Resource_ResourceAbstract {
  protected $_view;

  public function init() {
    if ($this->_view === null) {
      // the view (using PHPTAL as the rendering engine)
      require_once 'PHPTAL_Zend_View.php';
      $this->_view = new PHPTAL_Zend_View($this->getOptions());
      // set it as the default view
      Zend_Controller_Action_HelperBroker::getStaticHelper('ViewRenderer')->setView($this->_view);
    }
    return $this->_view;
  }
}
