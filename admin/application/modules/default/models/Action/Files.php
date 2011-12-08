<?php
class Model_Action_Files extends Model_Action_Streams {
  protected $_resource = 'files';

  public function addAction() {
    $this->_input['f'] = array('state'=>'files.new');
    parent::addAction();
  }

  public function uploadAction() {
    $this->_output->result = 0;

    if ($this->getPhase() == App_Model_Action::PHASE_PERFORM) {
      if (isset($this->_input['key'])) {
        $files_table = new Model_DbTable_Files();
        $file = $files_table->fetchRow($this->_tableAdapter->quoteInto($this->_tableAdapter->quoteIdentifier('key').' = ?', $this->_input['key']));

        if ($file) {
          // we only need the record id
          $this->_output->model['record'] = array('id'=>$file->stream_id);

          if ($file->state == 'new') {
            $lock_name = 'whispercast.default.streams.'.$this->_output->model['record']['id'];
            if ($this->_tableAdapter->fetchOne('SELECT GET_LOCK(?, 5)', $lock_name) == 1) {
              try {
                $base = Zend_Registry::get('Whispercast')->paths['upload'].'/';
                $name = $this->getServer()->id.'-'.$this->_output->model['record']['id'].'-'.$file->key;

                $adapter = new Zend_File_Transfer_Adapter_Http();
                $adapter->addFilter('Rename', $base.$name);
                if (@$adapter->receive()) {
                  $received = $adapter->getFileName();
                  chmod($received, 0666);
                  try {
                    $file->state = 'uploaded';
                    $file->processed = (bool)$this->_input['processed'] ? 1 : 0;
                    $file->save();

                    $this->_output->result = 1;
                  } catch (Exception $e) {
                    unlink($received);
                    throw $e;
                  }
                } else {
                  $this->_output->errors[] = $adapter->getMessages();
                  $this->_output->result = -1;
                }
              } catch(Exception $e) {
                $this->_output->errors[] = $e->getMessage();
                $this->_output->result = -1;
              }
              $this->_tableAdapter->fetchOne('SELECT RELEASE_LOCK(?)', $lock_name);
            } else {
              throw new Zend_Controller_Action_Exception('Timed out getting a lock on the uploaded file', 503);
            }
          }
        } else {
          throw new Zend_Controller_Action_Exception('The upload key is invalid', 400);
        }
      } else {
        throw new Zend_Controller_Action_Exception('The upload key must be provided', 400);
      }
    }
  }

  public function retryAction() {
    $id = $this->_input[$this->_key];
    if ($id === null) {
      throw new Zend_Controller_Action_Exception('Invalid record id', 400);
    }
    if (!is_array($id)) {
      $id = array($id);
    }

    $files_table = new Model_DbTable_Files();

    $this->_output->model['results'] = array();
    $this->_output->model['records'] = array();

    $this->_output->result = 1;

    $server = $this->getServer();

    foreach ($id as $rid) {
      try {
        $this->_checkAccess($rid, 'set');
      } catch(Exception $e) {
        continue;
      }

      $lock_name = 'whispercast.default.streams.'.$rid;
      if ($this->_tableAdapter->fetchOne('SELECT GET_LOCK(?, 5)', $lock_name) == 1) {

        $result['result'] = false;
        $this->_tableAdapter->beginTransaction();

        try {
          $file = $files_table->fetchRow($this->_tableAdapter->quoteInto('stream_id = ?', $rid));
          if ($file) {
            try {
              if ($file->state == 'failed') {
                if ($file->internal_id >= 0) {
                  $cs = Whispercast::getInterface($server->id.'_transcoder')->MasterControl()->Retry((int)$file->internal_id);
                  if ($cs->success_) {
                    $file->state = 'uploaded';
                    $file->save();

                    $result['result'] = true;
                  }
                }
              }
              $this->_output->model['results'][$rid] = $result;
              $this->_output->model['records'][$rid] = $this->_tableObject->fetchRow($this->_createSelect('set', $this->_input)->where($this->_quotedKey.' = ?', $rid));
            } catch (Exception $e) {
              $this->_output->model['results'][$rid] = array('result'=>false);
            }
          }
        } catch(Exception $e) {
          $this->_output->model['results'][$rid] = array('result'=>false);
        }

        if ($result['result']) {
          $this->_tableAdapter->commit();
        } else {
          $this->_tableAdapter->rollBack();
        }

        $this->_tableAdapter->fetchOne('SELECT RELEASE_LOCK(?)', $lock_name);
      } else {
        $this->_output->model['results'][$rid] = array('result'=>false);
      }
    }
  }

  public static function getSoapClassmap() {
    return array(
      'Result'=>'Result',
      'FileSetup'=>'FileSetup'
    );
  }
}

class FileSetup {
  /**
   * Transfer the setup from an array (produced by the action).
   *
   * @param array $array The data array.
   * @return FileSetup
  */
  public static function fromArray($array) {
    $setup = new FileSetup();
    return $setup;
  }
}
class Files extends Streams {
  protected $_setupClass = 'FileSetup';

  /**
   * Adds a new file.
   *
   * @param string $token The session token.
   * @param int $server_id The server id.
   * @param string $name The stream name.
   * @param string $description The stream description.
   * @param string $tags The tags associated to this stream.
   * @param int $category_id The id of the stream category.
   * @param boolean $public The public flag - if TRUE the stream is exported as a public stream.
   * @param boolean $usable The usable flag - if TRUE the stream can be used in switches, playlists, etc.
   * @param FileSetup $setup The alias specific setup.
   * @return Result
  */
  public function add($token, $server_id, $name, $description, $tags, $category_id, $public, $usable, $setup) {
    $this->_login($token);
    return $this->_process('add', array(
      'sid'=>$server_id,
      'name'=>$name,
      'description'=>$description,
      'tags'=>$tags,
      'category_id'=>$category_id,
      'public'=>$public,
      'usable'=>$usable
    ));
  }
  /**
   * Modifies an existing file.
   *
   * @param string $token The session token.
   * @param int $server_id The server id.
   * @param int $id The stream id.
   * @param string $name The stream name.
   * @param string $description The stream description.
   * @param string $tags The tags associated to this stream.
   * @param int $category_id The id of the stream category.
   * @param boolean $public The public flag - if TRUE the stream is exported as a public stream.
   * @param boolean $usable The usable flag - if TRUE the stream can be used in switches, playlists, etc.
   * @param FileSetup $setup The alias specific setup.
   * @return Result
  */
  public function set($token, $server_id, $id, $name, $description, $tags, $category_id, $public, $usable, $setup) {
    $this->_login($token);
    return $this->_process('add', array(
      'sid'=>$server_id,
      'id'=>$id,
      'name'=>$name,
      'description'=>$description,
      'tags'=>$tags,
      'category_id'=>$category_id,
      'public'=>$public,
      'usable'=>$usable
    ));
  }
}
?>
