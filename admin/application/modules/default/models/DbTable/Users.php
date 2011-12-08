<?php
class Model_DbTable_Users extends Zend_Db_Table_Abstract {
  protected $_name = 'users';
  protected $_rowClass = 'Model_User';
}
?>
