<?php
class App_Controller_Action_Helper_Auth extends Zend_Controller_Action_Helper_Abstract {
  private $_user;

  public function user($required = true) {
    if (!$this->_user) {
      $this->_user = Zend_Auth::getInstance()->getIdentity();
      if ($required) {
        if (!$this->_user) {
          if (Zend_Controller_Action_HelperBroker::getStaticHelper('contextSwitch')->getCurrentContext() != null) {
            $this->getResponse()->setHttpResponseCode(401)->sendHeaders();
            die;
          }
          Zend_Controller_Action_HelperBroker::getStaticHelper('redirector')->gotoRouteAndExit(array('module'=>'default','controller'=>'users','action'=>'login'));
        }
      }
      if ($this->_user) {
        $class = $this->_user->getTableClass();
        $this->_user->setTable(new $class());
      }
    }
    return $this->_user;
  }
}
?>
