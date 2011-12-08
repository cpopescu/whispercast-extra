<?php
class App_Controller_Router_Route extends Zend_Controller_Router_Route_Abstract {
  public static function getInstance(Zend_Config $config) {
    $defs = ($config->defaults instanceof Zend_Config) ? $config->defaults->toArray() : array();
    return new self($defs);
  }
  public function __construct($defaults = array()) {
    $this->_defaults = $defaults;
  }

  public function getVersion() {
    return 1;
  }
  public function match($path) {
    $path = ltrim($path, '/');
    if ($path == '') {
      return array('module'=>'default', 'controller'=>'index', 'action'=>'index');
    }
    $parts = split('/', $path);

    switch (count($parts)) {
      case 1:
        return array('module'=>'default', 'controller'=>'index', 'action'=>urldecode($parts[0]));
      case 2:
        return array('module'=>'default', 'controller'=>urldecode($parts[0]), 'action'=>($parts[1] ? urldecode($parts[1]) : 'index'));
      case 3:
        if ($parts[0] != 'default') {
          return array('module'=>urldecode($parts[0]), 'controller'=>urldecode($parts[1]), 'action'=>($parts[2] ? urldecode($parts[2]) : 'index'));
        }
    }
    return false;
  }
  public function assemble($data = array(), $reset = false, $encode = false) {
    $result = '';

    if ($data['module'] != 'default') {
      $result .= urlencode($data['module']).'/';
      $result .= urlencode($data['controller']).'/';
      $result .= ($data['action'] != 'index') ? urlencode($data['action']) : '';
    } else {
      if ($data['action'] != 'index') {
        if ($data['controller'] != 'index') {
          $result .= urlencode($data['controller']).'/';
        }
        $result .= urlencode($data['action']);
      } else {
        if ($data['controller'] != 'index') {
          $result .= urlencode($data['controller']).'/';
          $result .= urlencode($data['action']);
        }
      }
    }
    unset($data['action']);
    unset($data['controller']);
    unset($data['module']);

    if (!empty($data)) {
      $get = array();
      if ($encode)
        foreach ($data as $key=>$item)
          $get[] = urlencode($key).'='.urlencode($item);
      else
        foreach ($data as $key=>$item)
          $get[] = $key.'='.$item;
      $result .= '?'.implode('&', $get);
    }
    return $result;
  }
}
?>
