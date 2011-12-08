<?php
class Model_Delegate_Sources extends Model_Delegate_Streams {
  protected function _setupFromData($data) {
    $setup = $data['setup'];
    return array('protocol'=>$setup['protocol'], 'source'=>$setup['source'], 'password'=>$setup['password'], 'saved'=>$setup['saved']);
  }

  protected function _setupFromElement($data) {
    return array('protocol'=>$data['protocol'], 'source'=>$data['source'], 'password'=>$data['password'], 'saved'=>$data['saved']);
  }

  public function _postFetch(&$data) {
    parent::_postFetch($data);
    $data['setup']['protocol'] = $data['protocol'];
    $data['setup']['source'] = $data['source'];
    $data['setup']['password'] = $data['password'];
    $data['setup']['saved'] = $data['saved'];
  }

  public function _insert() {
    $sources = new Model_DbTable_Sources();
    $this->_source = $sources->createRow();

    $this->_source->setFromArray($this->_data);
    $this->_source->server_id = $this->_model->server_id;

    $this->_sources = new Whispercast_Sources(Whispercast::getInterface($this->_model->server_id));

    $this->_model->path = $this->_sources->getStreamPath($this->_model->server_id, $this->_source->protocol, $this->_source->source);
    $this->_model->export = '/sources/'.$this->_source->protocol.'/'.$this->_source->source;
  }
  public function _postInsert() {
    parent::_postInsert();

    $this->_source->stream_id = $this->_model->id;
    $this->_source->save();
    
    $buildup_interval_sec = 0+Zend_Registry::get('Whispercast')->savers['buildup_interval_sec'];
    $buildup_delay_sec = 0+Zend_Registry::get('Whispercast')->savers['buildup_delay_sec'];
    $prefill_buffer_ms = 0+Zend_Registry::get('Whispercast')->savers['prefill_buffer_ms'];
    $advance_media_ms = 0+Zend_Registry::get('Whispercast')->savers['advance_media_ms'];

    $cs = $this->_sources->createStream($this->_model->server_id, $this->_source->protocol, $this->_source->source, $this->_source->saved ? '0/private/live/s'.$this->_model->server_id.'_source_'.$this->_model->id : null, $this->_source->saved ? true : false, $buildup_interval_sec, $buildup_delay_sec, $prefill_buffer_ms, $advance_media_ms);
    if ($cs !== null) {
      throw new Exception($cs);
    }
    if ($this->_source->saved) {
      $interface = Whispercast::getInterface($this->_model->server_id);

      $rangers = new Whispercast_Rangers($interface);
      $cs = $rangers->create($this->_model->id, array(
        'home_dir' => '0/private/live/s'.$this->_model->server_id.'_source_'.$this->_model->id.'/media',
        'root_media_path' => $interface->getElementPrefix().'_live/s'.$this->_model->server_id.'_source_'.$this->_model->id.'/media',
        'file_prefix' => 'media',
        'utc_requests' => true
      ));
      if ($cs !== null) {
        $this->_sources->destroyStream($this->_model->server_id, $this->_source->protocol, $this->_source->source);
        throw new Exception($cs);
      }
    }
  }

  public function _update() {
    $sources = new Model_DbTable_Sources();
    $this->_source = $sources->fetchRow($sources->getAdapter()->quoteInto('stream_id = ?', $this->_model->id));
  }
  public function _postUpdate() {
    parent::_postUpdate();

    $this->_source->setFromArray(array('password' => $this->_data['password']));
    $this->_source->save();
  }

  public function _delete() {
    $sources = new Model_DbTable_Sources();
    $this->_source = $sources->fetchRow($sources->getAdapter()->quoteInto('stream_id = ?', $this->_model->id));

    $this->_sources = new Whispercast_Sources(Whispercast::getInterface($this->_model->server_id));
  }
  public function _postDelete() {
    $cs = $this->_sources->destroyStream($this->_model->server_id, $this->_source->protocol, $this->_source->source);
    if ($cs !== null) {
      throw new Exception($cs);
    }

    $rangers = new Whispercast_Rangers(Whispercast::getInterface($this->_model->server_id));
    $rangers->destroy($this->_model->id);
  }
}
?>
