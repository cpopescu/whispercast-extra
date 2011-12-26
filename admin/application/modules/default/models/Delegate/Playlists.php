<?php
class Model_Delegate_Playlists extends Model_Delegate_Streams {
  protected $_tableClass = 'Model_DbTable_Playlists';
 
  protected function _setupFromData($data) {
    $setup = $data['setup'];

    $interface = Whispercast::getInterface($this->_model->server_id);

    $playlists = new Whispercast_Playlists($interface);
    $prefix = $interface->getStreamPrefix();

    $streams = new Model_DbTable_Streams();

    $playlist = array();
    if ($setup['playlist']) {
      foreach ($setup['playlist'] as $id) {
        $stream = $streams->fetchRow($streams->select()->where('id = ?', $id));
        if ($stream) {
          $playlist[] = $prefix.$stream->path;
        }
      }
    }
    return array('playlist' => $playlist, 'loop' => $setup['loop'] ? true : false);
  }

  protected function _setupFromElement($data) {
    $interface = Whispercast::getInterface($this->_model->server_id);

    $playlists = new Whispercast_Playlists($interface);
    $prefix = $interface->getStreamPrefix();

    $cs = $playlists->getConfig($this->_model->id, $setup);
    if ($cs !== null) {
      throw new Exception($cs);
    }

    $streams = new Model_DbTable_Streams();

    $playlist = array();
    foreach ($setup['playlist'] as $path) {
      $stream = $streams->fetchRow($streams->select()->where('path = ?', substr($path, strlen($prefix)+1)));
      if ($stream) {
        $playlist[] = $stream->toArray();
      } else {
        // TODO: Remove this as it is a temporary hack
        $p = substr($path, strlen($prefix)+1);
        $p = preg_replace('/\/s\d+_\d+_normalizer\//', '/', $p);
        $stream = $streams->fetchRow($streams->select()->where('path = ?', $p));
        if ($stream) {
          $playlist[] = $stream->toArray();
        }
      }
    }
    return array('playlist' => $playlist, 'loop' => $setup['loop'] ? true : false);
  }

  protected function _computeDuration() {
    $streams = new Model_DbTable_Streams();

    $setup = $this->_data['_']['setup'];

    $result = null;
    if ($setup['playlist']) {
      $counts = array_count_values($setup['playlist']);
      if (count($counts) > 0) {
        $durations = $streams->fetchAll(
          $streams->select()->
            where('id IN (?)', array_keys($counts))
        );

        $result = 0;
        foreach ($durations as $duration) {
          if ($duration['duration'] == null) {
            $result = null;
            break;
          }
          $result += $counts[$duration['id']]*$duration['duration'];
        }
      }
    }
    return $result;
  }

  public function _insert() {
    $playlists = new Model_DbTable_Playlists();
    $this->_playlist = $playlists->createRow();

    $this->_playlists = new Whispercast_Playlists(Whispercast::getInterface($this->_model->server_id));
  }
  public function _postInsert() {
    parent::_postInsert();
   
    $streams = new Model_DbTable_Streams();
    $streams->update(
      array(
        'path' => ($this->_model->path = $this->_playlists->getPath($this->_model->id)),
        'export' => ($this->_model->export = '/playlists/'.$this->_model->id),
        'duration' => (($this->_model->duration = $this->_computeDuration()) === null) ? new Zend_Db_Expr('NULL') : $this->_model->duration
      ),
      $streams->getAdapter()->quoteInto('id = ?', $this->_model->id)
    );

    $this->_playlist->setFromArray(array('loop'=>$this->_data['loop']));
    
    $this->_playlist->stream_id = $this->_model->id;
    $this->_playlist->save();

    $cs = $this->_playlists->create($this->_model->id, $this->_data['setup']);
    if ($cs !== null) {
      throw new Exception($cs);
    }
  }

  public function _update() {
    $playlists = new Model_DbTable_Playlists();
    $this->_playlist = $playlists->fetchRow($playlists->getAdapter()->quoteInto('stream_id = ?', $this->_model->id));

    $this->_playlists = new Whispercast_Playlists(Whispercast::getInterface($this->_model->server_id));
  }
  public function _postUpdate() {
    parent::_postUpdate();
     
    $streams = new Model_DbTable_Streams();
    $streams->update(
      array(
        'duration' => (($this->_model->duration = $this->_computeDuration()) === null) ? new Zend_Db_Expr('NULL') : $this->_model->duration
      ),
      $streams->getAdapter()->quoteInto('id = ?', $this->_model->id)
    );

    $this->_playlist->setFromArray(array('loop'=>$this->_data['loop']));
    $this->_playlist->save();

    $cs = $this->_playlists->setConfig($this->_model->id, $this->_data['setup']);
    if ($cs !== null) {
      throw new Exception($cs);
    }
  }

  public function _delete() {
    $this->_playlists = new Whispercast_Playlists(Whispercast::getInterface($this->_model->server_id));
  }
  public function _postDelete() {
    $cs = $this->_playlists->destroy($this->_model->id);
    if ($cs !== null) {
      throw new Exception($cs);
    }
  }
}
?>
