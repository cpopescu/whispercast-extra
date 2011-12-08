<?php
class ErrorController extends Zend_Controller_Action {
  public function errorAction() {
    $this->_helper->layout()->disableLayout();

    $errors = $this->_getParam('error_handler');

    $code = 0;
    if ($errors->exception) {
      $code = $errors->exception->getCode();
      if ($code == 0) {
        if (is_a($errors->exception, 'Zend_Controller_Action_Exception')) {
          $code = 400;
        }
      }
    }
    $code = $code ? $code : 500;

    $contextSwitch = Zend_Controller_Action_HelperBroker::hasHelper('contextSwitch') ? Zend_Controller_Action_HelperBroker::getExistingHelper('contextSwitch') : null;
    if ($contextSwitch) {
      if ($contextSwitch->getCurrentContext() != null) {
        $this->getResponse()->setHttpResponseCode($code)->sendHeaders();
        if (APPLICATION_ENV != 'production') {
          die('<pre>'.$errors->exception.'</pre>');
        }
        die;
      }
    }

    $this->getResponse()->setHttpResponseCode($code);
    $this->view->message = 'Error';
    
    $this->view->exception = $errors->exception;
    $this->view->request = $errors->request;
  }
}
?>
