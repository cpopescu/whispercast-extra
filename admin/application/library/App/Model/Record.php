<?php
class App_Model_Record extends Zend_Db_Table_Row {
  public function save() {
    $readOnly = $this->isReadOnly();
    $this->setReadOnly(false);

    $modifiedFields = $this->_modifiedFields;

    $this->_modifiedFields = array();
    foreach ($this->_getTable()->info(Zend_Db_Table_Abstract::COLS) as $column) {
      $this->_modifiedFields[$column] = true;
    }

    $result = parent::save();
    
    $this->_modifiedFields = $modifiedFields;

    $this->setReadOnly($readOnly);
    return $result;
  }
  public function delete() {
    $readOnly = $this->isReadOnly();
    $this->setReadOnly(false);

    $result = parent::delete();
    
    $this->setReadOnly($readOnly);
    return $result;
  }

  public function _postCreate(&$data, $resource) {
  }
  public function _postFetch(&$data) {
  }
}

?>
