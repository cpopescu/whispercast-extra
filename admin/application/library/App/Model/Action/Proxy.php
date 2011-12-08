<?php
class App_Model_Action_Proxy {
  protected $_action;

  public function __construct($action) {
    $this->_action = $action;
  }

  protected function _login($token) {
    Zend_Session::setId($token);
  }

  protected function _process($method, $input) {
    $this->_action->getRequest()->setActionName($method);

    $this->_action->preDispatch($input);
    $method .= 'Action';
    $this->_action->$method();
    return $this->_action->postDispatch();
  }
}
?>
