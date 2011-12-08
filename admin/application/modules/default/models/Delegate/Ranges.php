<?php
class Model_Delegate_Ranges extends Model_Delegate_Streams {
  protected function _setupFromData($data) {
    $setup = $data['setup'];

    $date_f = new App_Filter_DateTime();

    return array(
      'begin' => $date_f->filter($setup['begin']),
      'end' => $date_f->filter($setup['end']),
      'source' => $setup['source']
    );
  }

  protected function _setupFromElement($data) {
    $ranges = new Whispercast_Rangers(Whispercast::getInterface($this->_model->server_id));

    $cs = $ranges->getRange($this->_model->id, $data['source_id'], $begin, $end);
    if ($cs !== null) {
      throw new Exception($cs);
    }

    $date_f = new App_Filter_DateTime();

    $streams = new Model_DbTable_Streams();
    if (isset($data['source_id'])) {
      $streams = new Model_DbTable_Streams();
      $stream = $streams->fetchRow($streams->select()->where('id = ?', $data['source_id']));
      if ($stream) {
        $source = $stream->toArray();
      }
    }
    return array(
      'begin' => $date_f->filter((float)$begin),
      'end' => $date_f->filter((float)$end),
      'source' => $source
    );
  }

  public function _insert() {
    $ranges = new Model_DbTable_Ranges();
    $this->_range = $ranges->createRow();

    $this->_rangers = new Whispercast_Rangers(Whispercast::getInterface($this->_model->server_id));
  }
  public function _postInsert() {
    parent::_postInsert();

    $setup = $this->_data['setup'];

    $begin = new Zend_Date($setup['begin']);
    $begin = $begin->get(Zend_Date::TIMESTAMP);
    $end = new Zend_Date($setup['end']);
    $end = $end->get(Zend_Date::TIMESTAMP);    

    $this->_range->setFromArray(array('source_id'=>$setup['source']));

    $this->_range->stream_id = $this->_model->id;
    $this->_range->save();

    $streams = new Model_DbTable_Streams();
    $streams->update(
      array(
        'path' => ($this->_model->path =  $this->_rangers->getRangePath($this->_model->id, $this->_range->source_id)),
        'export' => ($this->_model->export = '/ranges/'.$this->_model->id),
        'duration' => ($this->_model->duration = 1000*($end-$begin))
      ),
      $streams->getAdapter()->quoteInto('id = ?', $this->_model->id)
    );

    $sources = new Model_DbTable_Sources();

    $source = $streams->fetchRow($streams->select()->where('id = ?', $this->_range->source_id));
    if ($source) {
      $cs = $this->_rangers->createRange($this->_model->id, $this->_range->source_id, $begin, $end);
      if ($cs !== null) {
        throw new Exception($cs);
      }
    } else {
      throw new Exception('Couldn\'t find the associated source', 400);
    }
  }

  public function _update() {
    $ranges = new Model_DbTable_Ranges();
    $this->_range = $ranges->fetchRow($ranges->getAdapter()->quoteInto('stream_id = ?', $this->_model->id));

    $this->_rangers = new Whispercast_Rangers(Whispercast::getInterface($this->_model->server_id));
  }
  public function _postUpdate() {
    parent::_postUpdate();
    
    $setup = $this->_data['setup'];

    $begin = new Zend_Date($setup['begin']);
    $begin = $begin->get(Zend_Date::TIMESTAMP);
    $end = new Zend_Date($setup['end']);
    $end = $end->get(Zend_Date::TIMESTAMP);    

    $streams = new Model_DbTable_Streams();
    $streams->update(
      array(
        'duration' => ($this->_model->duration = 1000*($end - $begin))
      ),
      $streams->getAdapter()->quoteInto('id = ?', $this->_model->id)
    );

    $cs = $this->_rangers->setRange($this->_model->id, $this->_range->source_id, $begin, $end);
    if ($cs !== null) {
      throw new Exception($cs);
    }
  }

  public function _delete() {
    $ranges = new Model_DbTable_Ranges();
    $this->_range = $ranges->fetchRow($ranges->getAdapter()->quoteInto('stream_id = ?', $this->_model->id));

    $this->_rangers = new Whispercast_Rangers(Whispercast::getInterface($this->_model->server_id));
  }
  public function _postDelete() {
    $cs = $this->_rangers->destroyRange($this->_model->id, $this->_range->source_id);
    if ($cs !== null) {
      throw new Exception($cs);
    }
  }
}
?>
