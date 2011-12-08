<?php
class Model_User extends App_Model_Record {
  protected $_acl = array();

  protected static $_actions = array( 
    1 => 'add',
    2 => 'set',
    4 => 'get',
    8 => 'delete',
    16 => 'operate',
    32 => 'osd'
  );

  public function getAcl($server_id, $resource = null, $resource_id = null) {
    $adapter = $this->getTable()->getAdapter();

    if (is_array($resource)) {
      $select = $adapter->select()->
                          from('acl', array('server_id','resource','action'=>'BIT_OR(action)'))->
                          where($adapter->quoteIdentifier('user_id').' = ?', (int)$this->id)->
                          where($adapter->quoteIdentifier('server_id').' = ?', (int)$server_id)->
                          where($adapter->quoteIdentifier('resource').' IN (?)', $resource)->
                          group(array('user_id', 'server_id', 'resource'));
    } else {
      $select = $adapter->select()->
                          from('acl', array('server_id','resource','action'=>'BIT_OR(action)'))->
                          where($adapter->quoteIdentifier('user_id').' = ?', (int)$this->id)->
                          where($adapter->quoteIdentifier('server_id').' = ?', (int)$server_id)->
                          where($adapter->quoteIdentifier('resource').' = ?', $resource)->
                          group(array('user_id', 'server_id', 'resource'));
    }

    if (isset($resource_id)) {
      $resource_id_ = $adapter->quoteIdentifier('resource_id');

      $select->
      where('('.$resource_id_.' = ? OR '.$resource_id_.' = 0)', (int)$resource_id);
    }

    $acls = $adapter->fetchAll($select);
    foreach ($acls as $ac) {
      foreach(Model_User::$_actions as $m=>$a) {
        $acl[$ac['server_id']][$ac['resource']][$a] = ($ac['action'] & $m) ? true : false;
      }
    }

    return $acl;
  }
}
?>
