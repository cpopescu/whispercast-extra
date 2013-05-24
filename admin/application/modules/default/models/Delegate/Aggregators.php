<?php
class Model_Delegate_Aggregators extends Model_Delegate_Streams {
  protected function _setupFromData($data) {
    $streams = new Model_DbTable_Streams();

    $setup = $data['setup'];

    $flavours = array();
    if ($setup['flavours']) {
      $mask = 0;
      foreach ($setup['flavours'] as $id) {
        $stream = $streams->fetchRow($streams->select()->where('id = ?', $id));
        if ($stream) {
          if (isset($flavours[$stream->path])) {
            $flavours[$stream->path] |= (1 << $mask);
          } else {
            $flavours[$stream->path] = (1 << $mask);
          }
        } else {
          throw new Exception('Stream not found');
        }
        $mask++;
      }
    }
    return array('flavours' => array_flip($flavours));
  }

  protected function _setupFromElement($data) {
    $interface = Whispercast::getInterface($data['server_id']);
    $aggregators = new Whispercast_Aggregators($interface);

    $cs = $aggregators->getConfig($data['id'], $setup);
    if ($cs !== null) {
      throw new Exception($cs);
    }

    $streams = new Model_DbTable_Streams();

    $flavours = array();
    foreach ($setup['flavours'] as $mask => $path) {
      $stream = $streams->fetchRow($streams->select()->where('path LIKE ?', $interface->getPathForLike($path)));
      if ($stream) {
        $pow = 0;
        while ($mask && ($pow < 16)) {
          if (($mask & 1)) {
            $flavours[1 << $pow] = array('id'=>$stream->id, 'name'=>$stream->name);
          }

          $pow++;
          $mask = $mask >> 1;
        }
      }
    }
    return array('flavours' => $flavours);
  }

  public function _insert() {
    $aggregators = new Model_DbTable_Aggregators();
    $this->_aggregator = $aggregators->createRow();

    $this->_aggregators = new Whispercast_Aggregators(Whispercast::getInterface($this->_model->server_id));
  }
  public function _postInsert() {
    parent::_postInsert();

    $streams = new Model_DbTable_Streams();
    $streams->update(
      array(
        'path' => ($this->_model->path =  $this->_aggregators->getPath($this->_model->id)),
        'export' => ($this->_model->export = '/aggregators/'.$this->_model->id)
      ),
      $streams->getAdapter()->quoteInto('id = ?', $this->_model->id)
    );

    $this->_aggregator->stream_id = $this->_model->id;
    $this->_aggregator->save();

    $cs = $this->_aggregators->create($this->_model->id, $this->_data['setup']);
    if ($cs !== null) {
      throw new Exception($cs);
    }
  }

  public function _update() {
    $this->_aggregators = new Whispercast_Aggregators(Whispercast::getInterface($this->_model->server_id));
  }
  public function _postUpdate() {
    parent::_postUpdate();
    
    $cs = $this->_aggregators->setConfig($this->_model->id, $this->_data['setup']);
    if ($cs !== null) {
      throw new Exception($cs);
    }
  }

  public function _delete() {
    $this->_aggregators = new Whispercast_Aggregators(Whispercast::getInterface($this->_model->server_id));
  }
  public function _postDelete() {
    $cs = $this->_aggregators->destroy($this->_model->id);
    if ($cs !== null) {
      throw new Exception($cs);
    }
  }
}
?>
