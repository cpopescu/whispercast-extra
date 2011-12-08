<?php
class App_Controller_Dispatcher extends Zend_Controller_Dispatcher_Standard {
  public function getControllerClass(Zend_Controller_Request_Abstract $request) {
    $controller = $request->getControllerName();
    if ($controller != 'error') {
      $request->setControllerName('index');
    }

    $result =  parent::getControllerClass($request);

    $request->setControllerName($controller);
    return $result;
  }
}
?>
