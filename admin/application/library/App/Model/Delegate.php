<?php
class App_Model_Delegate {
  protected $_model;
  protected $_data = array();

  public function __construct($model) {
    $this->_model = $model;
  }

  public function setFromArray(array $data) {
    $this->_data = $data;
  }
}
?>
