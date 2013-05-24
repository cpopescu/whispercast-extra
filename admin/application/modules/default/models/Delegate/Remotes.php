<?php
class Model_Delegate_Remotes extends Model_Delegate_Streams {
  protected function _setupFromData($data) {
    $setup = $data['setup'];

    return array(
      'host_ip' => (string)$setup['host_ip'],
      'port' => (int)$setup['port'],
      'path_escaped' => (string)$setup['path_escaped'],
      'should_reopen' => (bool)$setup['should_reopen'],
      'fetch_only_on_request' => (bool)$setup['fetch_only_on_request'],
      'media_type' => (string)$setup['media_type'],
      'remote_user' => (string)$setup['remote_user'],
      'remote_password' => (string)$setup['remote_password']
    );
  }

  protected function _setupFromElement($data) {
    $interface = Whispercast::getInterface($this->_model->server_id);

    $remotes = new Whispercast_Remotes($interface);

    $cs = $remotes->getConfig($this->_model->id, $setup);
    if ($cs !== null) {
      throw new Exception($cs);
    }

    return array(
      'host_ip' => (string)$setup['host_ip'],
      'port' => (int)$setup['port'],
      'path_escaped' => (string)$setup['path_escaped'],
      'should_reopen' => (bool)$setup['should_reopen'],
      'fetch_only_on_request' => (bool)$setup['fetch_only_on_request'],
      'media_type' => (string)$setup['media_type'],
      'remote_user' => (string)$setup['remote_user'],
      'remote_password' => (string)$setup['remote_password']
    );
  }
  
  public function _insert() {
    $remotes = new Model_DbTable_Remotes();
    $this->_remote = $remotes->createRow();

    $this->_remotes = new Whispercast_Remotes(Whispercast::getInterface($this->_model->server_id));
  }
  public function _postInsert() {
    $streams = new Model_DbTable_Streams();
    $streams->update(
      array(
        'path' => ($this->_model->path = $this->_remotes->getPath($this->_model->id)),
        'export' => ($this->_model->export = '/remotes/'.$this->_model->id)
      ),
      $streams->getAdapter()->quoteInto('id = ?', $this->_model->id)
    );

    $this->_remote->stream_id = $this->_model->id;
    $this->_remote->save();
    
    $setup = $this->_setupFromData($this->_data);
    
    $cs = $this->_remotes->create($this->_model->id, $setup);
    if ($cs !== null) {
      throw new Exception($cs);
    }
  }

  public function _update() {
  }
  public function _postUpdate() {
  }

  public function _delete() {
    $this->_remotes = new Whispercast_Remotes(Whispercast::getInterface($this->_model->server_id));
  }
  public function _postDelete() {
    $cs = $this->_remotes->destroy($this->_model->id);
    if ($cs !== null) {
      throw new Exception($cs);
    }
  }
}
?>
