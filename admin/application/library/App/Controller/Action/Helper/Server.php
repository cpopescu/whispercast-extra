<?php
class App_Controller_Action_Helper_Server extends Zend_Controller_Action_Helper_Abstract {
  private $_server;
  private $_server_id = null;

  public function server($server_id, $required = true) {
    if ($server_id !== $this->_server_id) {
      if ($server_id !== null) {
        $table = new Model_DbTable_Servers();
        $this->_server = $table->fetchRow($table->getAdapter()->quoteInto('id = ?', $server_id));

        if ($this->_server !== null) {
          $this->_server->register();
        }
      }
      $this->_server_id = $server_id;

      if ($required) {
        if ($this->_server === null) {
          if (Zend_Controller_Action_HelperBroker::getStaticHelper('contextSwitch')->getCurrentContext() != null) {
            $this->getResponse()->setHttpResponseCode(400)->sendHeaders();
            exit();
          }
          throw new Zend_Controller_Action_Exception('Invalid server specified', 400);
        }
      }
    } 
    return $this->_server;
  }
}
?>
