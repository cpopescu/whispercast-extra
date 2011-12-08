<?php
class Model_Action_Index extends App_Model_Action {
  public function indexAction() {
    $this->getUser();
  }
  public function wsdlAction() {
    $router = Zend_Controller_Front::getInstance()->getRouter();

    $interfaces = array();
    foreach (Zend_Registry::get('Whispercast')->services as $module => $controllers) {
      foreach ($controllers as $controller) {
        if ($module != 'default') {
          $action = ucfirst($module).'_Model_Action_'.ucfirst($controller);
        } else {
          $action = 'Model_Action_'.ucfirst($controller);
        }
        if ($module != 'default') {
          $proxy = ucfirst($module).'_'.ucfirst($controller);
        } else {
          $proxy = ucfirst($controller);
        }

        if (class_exists($action, true)) {
          if (class_exists($proxy, false)) {
            $interfaces[$proxy] = $router->assemble(array('module'=>strtolower($module),'controller'=>strtolower($controller), 'action'=>''));
          }
        }
      }
    }

    $strategy = new Zend_Soap_Wsdl_Strategy_Composite(array(
      'int[]' => 'Zend_Soap_Wsdl_Strategy_ArrayOfTypeSequence',
      'string[]' => 'Zend_Soap_Wsdl_Strategy_ArrayOfTypeSequence',
      'ProgramEntry[]' => 'Zend_Soap_Wsdl_Strategy_ArrayOfTypeComplex',
      'OsdData_Crawler[]' => 'Zend_Soap_Wsdl_Strategy_ArrayOfTypeComplex'
    ));
    $sad = new App_Soap_AutoDiscover($strategy);
    $sad->setService('Whispercast', $interfaces);
    $sad->handle();
    die;
  }
}
?>
