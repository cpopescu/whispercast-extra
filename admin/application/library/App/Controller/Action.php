<?php
class App_Controller_Action extends Zend_Controller_Action {
  protected function _getUri($path) {
    $parsed = parse_url('http://'.$_SERVER['SERVER_NAME'].':'.$_SERVER['SERVER_PORT'].$_SERVER['REQUEST_URI']);
    return
    $parsed['scheme'].'://'.
    ($parsed['user'] ? ($parsed['user'].':'.$parsed['pass'].'@') : '').
    $parsed['host'].
    ($parsed['port'] ? ':'.$parsed['port'] : '').
    $path;
  }

  protected function _getDelegateClass($module, $controller) {
    if ($module != 'default') {
      return ucfirst($module).'_Model_Action_'.ucfirst($controller);
    }
    return 'Model_Action_'.ucfirst($controller);
  }
  protected function _getProxyClass($module, $controller) {
    if ($module != 'default') {
      return ucfirst($module).'_'.ucfirst($controller);
    } 
    return ucfirst($controller);
  }

  protected function _initContext($force = null) {
    if (isset($force)) {
      $this->_helper->contextSwitch->setContext($force, array('suffix'=>''));
      $this->_request->setParam('format', $force);
    } else {
      $this->_helper->contextSwitch->setContext('html', array('suffix'=>''));
    }
    $this->_helper->contextSwitch->
      setActionContext($this->_request->getActionName(), isset($force) ? array($force) : array('json','html'))->
      initContext();
  }

  public function preDispatch() {
    parent::preDispatch();

    $action = $this->_request->getActionName();
    if (!method_exists($this, $action.'Action')) {
      $this->_initContext($_SERVER['HTTP_SOAPACTION'] ? 'soap' : null);

      $delegate = $this->_getDelegateClass($this->_request->getModuleName(), $this->_request->getControllerName());
      if (class_exists($delegate, true)) {
        if ($_SERVER['HTTP_SOAPACTION']) {
          $proxy = $this->_getProxyClass($this->_request->getModuleName(), $this->_request->getControllerName());
          if (class_exists($proxy)) {
            $ss = new Zend_Soap_Server(
              $this->_getUri(Zend_Controller_Action_HelperBroker::getStaticHelper('url')->direct('wsdl', $this->_request->getControllerName(), $this->_request->getModuleName())),
              array(
                'classmap'=>call_user_func(array($delegate, 'getSoapClassmap')),
                'cache_wsdl'=>WSDL_CACHE_BOTH
              )
            );
            $ss->setObject(new $proxy(new $delegate($this->getRequest(), $this->getResponse(), array('context'=>'soap', 'phase'=>App_Model_Action::PHASE_PERFORM))));
            $ss->handle();
            die;
          }
          throw new Zend_Controller_Action_Exception('Invalid controller', 404);
        }
        if ($action == 'wsdl') {
          if (!method_exists($delegate, $action.'Action')) {
            $proxy = $this->_getProxyClass($this->_request->getModuleName(), $this->_request->getControllerName());
            if (class_exists($proxy)) {
              $strategy = new Zend_Soap_Wsdl_Strategy_Composite(array(
                'int[]' => 'Zend_Soap_Wsdl_Strategy_ArrayOfTypeSequence',
                'string[]' => 'Zend_Soap_Wsdl_Strategy_ArrayOfTypeSequence',
                'Production_ProgramEntry[]' => 'Zend_Soap_Wsdl_Strategy_ArrayOfTypeComplex'
              ));
              $ss = new App_Soap_AutoDiscover($strategy);
              $ss->setService('Whispercast', array($proxy=>$interfaces[$proxy] = 
Zend_Controller_Action_HelperBroker::getStaticHelper('url')->direct('', $this->_request->getControllerName(), $this->_request->getModuleName())));
              $ss->handle();
              die;
            }
            throw new Zend_Controller_Action_Exception('Invalid controller', 404);
          }
        }
        $this->_delegate = new $delegate($this->getRequest(), $this->getResponse());
        $this->_delegate->preDispatch();
      } else {
        throw new Zend_Controller_Action_Exception('Invalid controller', 404);
      }
    }
  }
  public function postDispatch() {
    parent::postDispatch();

    switch ($this->getRequest()->getActionName()) {
      case 'add':
      case 'set':
        $this->_helper->viewRenderer->setScriptAction('edit');
    }

    if ($this->_helper->contextSwitch->getCurrentContext() == 'json') {
      $this->view->clearVars();
    }

    // TODO: This is a hack to preserve existing templates - to be fixed.
    $output = $this->_delegate->postDispatch();
    $model = clone $output;
    unset($model->model);
    foreach ($output->model as $k=>&$v) {
      $model->$k = $v;
    }
    $this->view->model = $model;

    $user = $this->_helper->auth->user(false);
    $this->view->user = $user ? $user->toArray() : null;

    $server = $this->_helper->server->server($this->getRequest()->getParam('sid'), false);
    $this->view->server = $server ? $server->toArray() : null;
  }
  public function dispatch($action) {
    $this->_helper->notifyPreDispatch();

    $this->preDispatch();
    if ($this->getRequest()->isDispatched()) {
      if (method_exists($this, $action)) {
        $this->$action();
      } else
      if (isset($this->_delegate)) {
        if (!method_exists($this->_delegate, $action)) {
          throw new Zend_Controller_Action_Exception('Invalid action', 404);
        }
        $this->_delegate->$action();
      } else {
        throw new Zend_Controller_Action_Exception('Invalid controller', 404);
      }
      $this->postDispatch();
    }

    $this->_helper->notifyPostDispatch();
  }
}
?>
