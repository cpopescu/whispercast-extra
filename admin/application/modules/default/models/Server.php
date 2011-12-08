<?php
class Model_Server extends App_Model_Record {
  private $_isRegistered = false;
  public function register() {
    if (!$this->_isRegistered) {
      Whispercast::registerServer(
        $this->id,
        array(
          'kind'=>'whispercast',
          'host'=>$this->host,
          'port'=>$this->admin_port,
          'user'=>$this->admin_user,
          'password'=>$this->admin_password,
          'id'=>$this->id,
          'version'=>$this->version,
          'export'=>$this->export_base
        )
      );
      Whispercast::registerServer(
        $this->id.'_transcoder',
        array(
          'kind'=>'transcoder',
          'host'=>'localhost',
          'port'=>$this->transcoder_port,
          'user'=>$this->transcoder_user,
          'password'=>$this->transcoder_password,
        )
      );
      Whispercast::registerServer(
        $this->id.'_runner',
        array(
          'kind'=>'runner',
          'host'=>'localhost',
          'port'=>$this->runner_port,
          'user'=>$this->runner_user,
          'password'=>$this->runner_password,
        )
      );
      $this->_isRegistered = true;
    }
  }

  public function getInterface($kind = 'whispercast') {
    $this->register();
    if ($kind == 'whispercast')
      return Whispercast::getInterface($this->id);
    return Whispercast::getInterface($this->id.'_'.$kind);
  }

  protected function _insert() {
    $this->preview_user = 'preview';
    $this->preview_password = md5(uniqid('whispercast.default.servers.preview'.$this->id, true));

    $this->version = Zend_Registry::get('Whispercast')->version;
  }
  protected function _postInsert() {
    $this->register();
    $servers = new Whispercast_Servers($this->getInterface());

    $user_id = Zend_Controller_Action_HelperBroker::getExistingHelper('auth')->user()->id;

    $adapter = $this->_table->getAdapter();
    foreach (Zend_Registry::get('Whispercast')->resources as $resource => $actions) {
      $adapter->insert('acl', array(
        'server_id' => $this->id,
        'user_id' => $user_id,
        'resource' => $resource,
        'resource_id' => 0,
        'action' => implode(',',$actions )
      ));
    }

    $adapter->insert('acl', array(
      'server_id' => 0,
      'user_id' => $user_id,
      'resource' => 'servers',
      'resource_id' => $this->id,
      'action' => 'set,get,delete'
    ));

    $setup = array(
      'preview' => array(
        'user' => $this->preview_user,
        'password' => $this->preview_password
      ),
      'publish' => array(
        'server_address' => $_SERVER['SERVER_ADDR'],
        'server_host' => $_SERVER['SERVER_HOST'] ? $_SERVER['SERVER_HOST'] : $_SERVER['SERVER_ADDR'],
        'server_port' => $_SERVER['SERVER_PORT'],
        'query' => Zend_Controller_Front::getInstance()->getBaseUrl().'/sources/authorize?sid='.$this->id.'&format=json&action=${ACTION}&source=${RES}&user=${USER}&password=${PASSWD}'
      ),
      'disks' => $this->disks,
      'flow_control' => array(
        'video' => 0+Zend_Registry::get('Whispercast')->flow_control['video'],
        'total' => 0+Zend_Registry::get('Whispercast')->flow_control['total'],
      )
    );

    $cs = $servers->create($this->id, $setup);
    if ($cs != null) {
      throw new Exception($cs);
    }
    $this->getInterface()->MediaElementService()->SaveConfig();
  }

  protected function _delete() {
    $this->register();
    $servers = new Whispercast_Servers($this->getInterface());

    $user_id = Zend_Controller_Action_HelperBroker::getExistingHelper('auth')->user()->id;

    $adapter = $this->_table->getAdapter();
    $adapter->delete('acl', array(
      'server_id' => 0,
      'user_id' => $user_id,
      'resource' => 'servers',
      'resource_id' => $this->id,
      'action' => 'set,get,delete'
    ));

    // TODO: Move out all the streams deletion to an external script, run as a cron job...
    $clips_table = new Model_DbTable_Streams();
    $clips = $clips_table->fetchAll(
      $clips_table->select()->where($clips_table->getAdapter()->quoteIdentifier('server_id').' = ?', $this->id)
    );

    foreach ($clips as $clip) {
      $clip->delete();
    }

    $cs = $servers->destroy($this->id);
    if ($cs != null) {
      throw new Exception($cs);
    }
    $this->getInterface()->MediaElementService()->SaveConfig();
  }
  protected function _postDelete() {
  }
}
?>
