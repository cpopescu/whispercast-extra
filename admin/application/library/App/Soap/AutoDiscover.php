<?php
class App_Soap_AutoDiscover extends Zend_Soap_AutoDiscover {
  protected function _getUri($path) {
    $parsed = parse_url('http://'.$_SERVER['SERVER_NAME'].':'.$_SERVER['SERVER_PORT'].$_SERVER['REQUEST_URI']);
    return
    $parsed['scheme'].'://'.
    ($parsed['user'] ? ($parsed['user'].':'.$parsed['pass'].'@') : '').
    $parsed['host'].
    ($parsed['port'] ? (($parsed['port'] != 80) ? ':'.$parsed['port'] : '') : '').
    $path;
  }

  protected function _addServiceClass($class, $uri, $wsdl) {
    $port = $wsdl->addPortType($class . 'Port');
    $binding = $wsdl->addBinding($class . 'Binding', 'tns:' .$class. 'Port');

    $wsdl->addSoapBinding($binding, $this->_bindingStyle['style'], $this->_bindingStyle['transport']);
    $wsdl->addService($class . 'Service', $class . 'Port', 'tns:' . $class . 'Binding', $uri);
    foreach ($this->_reflection->reflectClass($class)->getMethods() as $method) {
      /* <wsdl:portType>'s */
      $portOperation = $wsdl->addPortOperation($port, $method->getName(), 'tns:' .$class.'_'.$method->getName(). 'Request', 'tns:' .$class.'_'.$method->getName(). 'Response');
      $desc = $method->getDescription();
      if (strlen($desc) > 0) {
        /** @todo check, what should be done for portoperation documentation */
        //$wsdl->addDocumentation($portOperation, $desc);
      }
      /* </wsdl:portType>'s */

      $this->_functions[] = $method->getName();

      $selectedPrototype = null;
      $maxNumArgumentsOfPrototype = -1;
      foreach ($method->getPrototypes() as $prototype) {
        $numParams = count($prototype->getParameters());
        if($numParams > $maxNumArgumentsOfPrototype) {
          $maxNumArgumentsOfPrototype = $numParams;
          $selectedPrototype = $prototype;
        }
      }

      if($selectedPrototype != null) {
        $prototype = $selectedPrototype;
        $args = array();
        foreach($prototype->getParameters() as $param) {
          $args[$param->getName()] = $wsdl->getType($param->getType());
        }
        $message = $wsdl->addMessage($class.'_'.$method->getName() . 'Request', $args);
        if (strlen($desc) > 0) {
          //$wsdl->addDocumentation($message, $desc);
        }
        if ($prototype->getReturnType() != "void") {
          $returnName = 'return';
          $message = $wsdl->addMessage($class.'_'.$method->getName() . 'Response', array($returnName => $wsdl->getType($prototype->getReturnType())));
        }

        /* <wsdl:binding>'s */
        $operation = $wsdl->addBindingOperation($binding, $method->getName(),  $this->_operationBodyStyle, $this->_operationBodyStyle);
        $wsdl->addSoapOperation($operation, $uri . '#' .$method->getName());
        /* </wsdl:binding>'s */
      }
    }
  }

  public function setService($name, $interfaces) {
    $wsdl = new Zend_Soap_Wsdl($name, $this->getUri(), $this->_strategy);

    // The wsdl:types element must precede all other elements (WS-I Basic Profile 1.1 R2023)
    $wsdl->addSchemaTypeSection();

    foreach ($interfaces as $class => $path) {
      $this->_addServiceClass($class, $this->_getUri($path), $wsdl);
    }

    $this->_wsdl = $wsdl;
  }
}
