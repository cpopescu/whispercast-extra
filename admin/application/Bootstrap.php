<?php
class Bootstrap extends Zend_Application_Bootstrap_Bootstrap {
  protected static $_logger;
  public static function log() {
    if (self::$_logger) {
      $arguments = func_get_args();
      call_user_func_array(array(self::$_logger, 'info'), $arguments);
    }
  }

  protected function _initApplication() {
    if (APPLICATION_ENV != 'production') {
      self::$_logger = new Zend_Log();
      self::$_logger->addWriter(new Zend_Log_Writer_Firebug());
    }

    $options = $this->getOptions();
    if (isset($options['resources']['locale'])) {
      Zend_Locale::setDefault($options['resources']['locale']['default'], 100);
    }

    Zend_Controller_Action_HelperBroker::addPrefix('App_Controller_Action_Helper');

    new Zend_Application_Module_Autoloader(
      array(
        'basePath'  => APPLICATION_PATH.DIRECTORY_SEPARATOR.'modules'.DIRECTORY_SEPARATOR.'default',
        'namespace' => '',
      )
    );

    $front = Zend_Controller_Front::getInstance();
    $front->setDispatcher(new App_Controller_Dispatcher());
  }
}
?>
