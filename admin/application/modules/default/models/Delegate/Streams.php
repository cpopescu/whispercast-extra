<?php
class Model_Delegate_Streams extends App_Model_Delegate {
  protected function _create(&$data) {
    if ($this->_tableClass) {
      $table = new $this->_tableClass();
      $row = $table->createRow(array(), Zend_Db_Table::DEFAULT_DB);
      $row = $row->toArray();

      foreach ($row as $k=>$v) {
        switch ($k) {
          case 'id':
          case 'stream_id':
            break;
          default:
            $setup[$k] = $data[$k] = $v;
        }
      }
      return $setup;
    }
    return null;
  }

  protected function _setupFromData($data) {
    return array();
  }
  protected function _setupFromElement($data) {
    return array();
  }

  public function _postCreate(&$data) {
    Bootstrap::log('Model_Delegate_Streams::_postCreate');
    Bootstrap::log($data);

    $data['setup'] = $this->_create($data);

    Bootstrap::log($data);
  }
  public function _postFetch(&$data) {
    Bootstrap::log('Model_Delegate_Streams::_postFetch');
    Bootstrap::log($data);

    $data['_']['setup'] = $data['setup'];
    $data['setup'] = $this->_setupFromElement($data);

    Bootstrap::log($data);
  }

  public function _postInsert() {
    Bootstrap::log('Model_Delegate_Streams::_postInsert');
    Bootstrap::log($this->_data);

    $this->_data['_']['setup'] = $this->_data['setup'];
    $this->_data['setup'] = $this->_setupFromData($this->_data);

    Bootstrap::log($this->_data);
  }
  public function _postUpdate() {
    Bootstrap::log('Model_Delegate_Streams::_postUpdate');
    Bootstrap::log($this->_data);

    $this->_data['_']['setup'] = $this->_data['setup'];
    $this->_data['setup'] = $this->_setupFromData($this->_data);

    Bootstrap::log($this->_data);
  }
}
?>
