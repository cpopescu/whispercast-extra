<?php
class Model_Delegate_Aliases extends Model_Delegate_Streams {
  protected function _setupFromData($data) {
    $setup = $data['setup'];

    if ($setup['stream']) {
      $streams = new Model_DbTable_Streams();
      $stream = $streams->fetchRow($streams->select()->where('id = ?', $setup['stream']));
      if ($stream) {
        return array('stream'=>$stream->path, 'export_as'=>$setup['export_as'], 'duration'=>$stream->duration);
      }
    }
    return array('stream'=>null, 'export_as'=>$setup['export_as'], 'duration'=>null);
  }

  protected function _setupFromElement($data) {
    $aliases = new Whispercast_Aliases(Whispercast::getInterface($this->_model->server_id));

    $cs = $aliases->getConfig($this->_model->id, $setup);
    if ($cs !== null) {
      throw new Exception($cs);
    }

    $streams = new Model_DbTable_Streams();
    $stream = $streams->fetchRow($streams->select()->where('path = ?', $setup['stream']));
    if ($stream) {
      return array('stream'=>$stream->toArray(), 'export_as'=>$data['export_as'], 'duration'=>$stream->duration);
    }
    return array('stream'=>null, 'export_as'=>$data['export_as'], 'duration'=>null);
  }

  public function _postFetch(&$data) {
    parent::_postFetch($data);
    $data['setup']['export_as'] = $data['export_as'];
  }

  public function _insert() {
    $aliases = new Model_DbTable_Aliases();
    $this->_alias = $aliases->createRow();

    $this->_alias->setFromArray(array('export_as' => $this->_data['export_as']));

    $this->_aliases = new Whispercast_Aliases(Whispercast::getInterface($this->_model->server_id));
  }
  public function _postInsert() {
    parent::_postInsert();

    $streams = new Model_DbTable_Streams();
    $streams->update(
      array(
        'path' => ($this->_model->path = $this->_aliases->getPath($this->_model->id)),
        'export' => ($this->_model->export = '/aliases/'.$this->_alias->export_as),
        'duration' => $this->_data['setup']['duration']
      ),
      $streams->getAdapter()->quoteInto('id = ?', $this->_model->id)
    );

    $this->_alias->stream_id = $this->_model->id;
    $this->_alias->save();

    $cs = $this->_aliases->create($this->_model->id, $this->_data['setup']);
    if ($cs !== null) {
      throw new Exception($cs);
    }
  }

  public function _update() {
    $this->_aliases = new Whispercast_Aliases(Whispercast::getInterface($this->_model->server_id));
  }
  public function _postUpdate() {
    parent::_postUpdate();

    $streams = new Model_DbTable_Streams();
    $streams->update(
      array(
        'duration' => $this->_data['setup']['duration']
      ),
      $streams->getAdapter()->quoteInto('id = ?', $this->_model->id)
    );

    $cs = $this->_aliases->setConfig($this->_model->id, $this->_data['setup']);
    if ($cs !== null) {
      throw new Exception($cs);
    }
  }

  public function _delete() {
    $this->_aliases = new Whispercast_Aliases(Whispercast::getInterface($this->_model->server_id));
  }
  public function _postDelete() {
    $cs = $this->_aliases->destroy($this->_model->id);
    if ($cs !== null) {
      throw new Exception($cs);
    }
  }
}
?>
