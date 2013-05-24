<?php
class Model_Delegate_Programs extends Model_Delegate_Streams {
  protected function _setupFromData($data) {
    $setup = $data['setup'];

    $interface = Whispercast::getInterface($this->_model->server_id);

    $programs = new Whispercast_Programs($interface);

    $streams = new Model_DbTable_Streams();
    $stream = $streams->fetchRow($streams->select()->where('id = ?', $setup['default_stream']));

    $program = array(
      'playlist'=>array(),
      'default_media'=>$stream ? $interface->getStreamPath($stream->path) : null,
      'switch_now'=>isset($setup['switch_now']) ? ($setup['switch_now'] ? true : false) : false
    );

    $date_f = new App_Filter_Date();
    $time_f = new App_Filter_Time();

    if ($setup['program']) {
      foreach ($setup['program'] as $element) {
        $stream = $streams->fetchRow($streams->select()->where('id = ?', $element['stream']));
        if ($stream) {
          $program['playlist'][] = array(
            'media'=>$interface->getStreamPath($stream->path),
            'time'=>$time_f->filter($element['time']),
            'date'=>$date_f->filter($element['date']),
            'weekdays'=>isset($element['weekdays']) ? $element['weekdays'] : null
          );
        }
      }
    }
    return $program;
  }

  protected function _setupFromElement($data) {
    $interface = Whispercast::getInterface($this->_model->server_id);

    $programs = new Whispercast_Programs($interface);

    $cs = $programs->getConfig($this->_model->id, $setup);
    if ($cs !== null) {
      throw new Exception($cs);
    }

    $streams = new Model_DbTable_Streams();
    $stream = $streams->fetchRow($streams->select()->where('path LIKE ?', $interface->getPathForLike($setup['default_media'])));

    $program = array(
      'program'=>array(),
      'default_stream'=>$stream ? $stream->toArray() : null
    );

    $date_f = new App_Filter_Date();
    $time_f = new App_Filter_Time();

    if ($setup['playlist']) {
      foreach ($setup['playlist'] as $element) {
        $stream = $streams->fetchRow($streams->select()->where('path LIKE ?', $interface->getPathForLike($element['media'])));
        $program['program'][] = array(
          'stream' => $stream ? $stream->toArray() : null,
          'time' => $time_f->filter($element['time']),
          'date' => $date_f->filter($element['date']),
          'weekdays' => isset($element['weekdays']) ? $element['weekdays'] : null
        );
      }
    }
    return $program;
  }

  public function _insert() {
    $programs = new Model_DbTable_Programs();
    $this->_program = $programs->createRow();

    $this->_programs = new Whispercast_Programs(Whispercast::getInterface($this->_model->server_id));
  }
  public function _postInsert() {
    parent::_postInsert();

    $streams = new Model_DbTable_Streams();
    $streams->update(
      array(
        'path' => ($this->_model->path = $this->_programs->getPath($this->_model->id)),
        'export' => ($this->_model->export = '/programs/'.$this->_model->id)
      ),
      $streams->getAdapter()->quoteInto('id = ?', $this->_model->id)
    );

    $this->_program->stream_id = $this->_model->id;
    $this->_program->save();

    $cs = $this->_programs->create($this->_model->id, $this->_data['setup']);
    if ($cs !== null) {
      throw new Exception($cs);
    }
  }

  public function _update() {
    $this->_programs = new Whispercast_Programs(Whispercast::getInterface($this->_model->server_id));
  }
  public function _postUpdate() {
    parent::_postUpdate();

    $cs = $this->_programs->setConfig($this->_model->id, $this->_data['setup']);
    if ($cs !== null) {
      throw new Exception($cs);
    }
  }

  public function _delete() {
    $this->_programs = new Whispercast_Programs(Whispercast::getInterface($this->_model->server_id));
  }
  public function _postDelete() {
    $cs = $this->_programs->destroy($this->_model->id);
    if ($cs !== null) {
      throw new Exception($cs);
    }
  }
}
?>
