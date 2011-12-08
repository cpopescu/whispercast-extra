<?php
class Model_Delegate_Switches extends Model_Delegate_Streams {
  public function _insert() {
    $switches = new Model_DbTable_Switches();
    $this->_switch = $switches->createRow();

    $this->_switches = new Whispercast_Switches(Whispercast::getInterface($this->_model->server_id));
  }
  public function _postInsert() {
    $streams = new Model_DbTable_Streams();
    $streams->update(
      array(
        'path' => ($this->_model->path = $this->_switches->getPath($this->_model->id)),
        'export' => ($this->_model->export = '/switches/'.$this->_model->id)
      ),
      $streams->getAdapter()->quoteInto('id = ?', $this->_model->id)
    );

    $this->_switch->stream_id = $this->_model->id;
    $this->_switch->save();

    $cs = $this->_switches->create($this->_model->id, array());
    if ($cs !== null) {
      throw new Exception($cs);
    }
  }

  public function _update() {
  }
  public function _postUpdate() {
  }

  public function _delete() {
    $this->_switches = new Whispercast_Switches(Whispercast::getInterface($this->_model->server_id));
  }
  public function _postDelete() {
    $cs = $this->_switches->destroy($this->_model->id);
    if ($cs !== null) {
      throw new Exception($cs);
    }
  }
}
?>
