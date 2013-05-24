<?php
class Model_Delegate_Files extends Model_Delegate_Streams {
  public $disableExport = true;

  public function updateState() {
    $adapter = Zend_Db_Table::getDefaultAdapter();

    $adapter->beginTransaction();

    $files_table = new Model_DbTable_Files();
    $file = $files_table->fetchRow($files_table->select()->where('stream_id = ?', $this->_model->id));

    if ($file === null) {
      return array('result'=>true);
    }

    $state = $file->state;
    $internal_id = $file->internal_id;

    $server = Whispercast::getInterface($this->_model->server_id);

    try {
      switch ($file->state) {
        case null:
        case 'new':
        case 'failed':
          $adapter->rollBack();
          return array('result'=>true);
        case 'uploaded':
          if ($internal_id < 0) {
            $new = true;
            $path = Zend_Registry::get('Whispercast')->paths['upload'].'/'.$this->_model->server_id.'-'.$this->_model->id.'-'.$file->key;

            $cs = Whispercast::getInterface($this->_model->server_id.'_transcoder')->MasterControl()->AddFile($path, $file->processed ? 'post_process' : 'transcode', new stdClass);
            if (!$cs->success_) {
              $result['result'] = false;
              $adapter->rollBack();
              return $result;
            }
            $internal_id = $cs->result_;
          }
      }

      if ($internal_id >= 0) {
        $cs = Whispercast::getInterface($this->_model->server_id.'_transcoder')->MasterControl()->GetFileState((int)$internal_id);
        if ($cs->success_) {
          switch ($cs->result_->state_) {
            case 'Idle':
              $state = 'waiting';
              break;
            case 'Transferring':
              $state = 'transferring';
              break;
            case 'Transferred':
              $state = 'transferred';
              break;
            case 'Transcoding':
            case 'PostProcessing':
              $state = 'transcoding';

              if (count($cs->result_->transcoding_status_->out_) > 0) {
                $progress = 1;
                foreach ($cs->result_->transcoding_status_->out_ as $status) {
                  $progress = min($progress, $status->progress_);
                }
                $result['progress'] = $progress;
              } else {
                $result['progress'] = 0;
              }
              break;
            case 'Outputting':
            case 'Distributing':
              $state = 'transcoding';
              $result['progress'] = 1;
              break;
            case 'Ready':
              if ($state !== null) {
                $state = null;

                $result['progress'] = 1;
                $result['duration'] = 0;

                $path = null;
                $export = null;

                $name = null;

                // TODO: Define the flow for multiple outputs for the same file..
                $transcoded_table = new Model_DbTable_FilesTranscoded();
                foreach ($cs->result_->transcoding_status_->out_ as $status) {
                  $transcoded_id = $transcoded_table->insert(array(
                    'file_id' => $file->id,
                    'width' => $status->width_,
                    'height' => $status->height_,
                    'framerate_n' => $status->framerate_n_,
                    'framerate_d' => $status->framerate_d_,
                    'bitrate' => $status->bitrate_,
                    'duration' => $status->duration_,
                    'file' => basename($status->filename_out_),
                    // TODO: We assume that as soon as the file was transcoded.. it's output is immediately available - this is not necessarily correct.
                    'available' => true
                  ));

                  // we get the longest...
                  $result['duration'] = max($result['duration'], $status->duration_);

                  // TODO: What should these be in case of multiple outputs for the same file ?
                  $name = basename($status->filename_out_);
                  $prefix = Whispercast::getInterface($this->_model->server_id)->getElementPrefix();
                  $path = $prefix.'_files/'.$name;
                  $export = '/files/'.$name;
                }

                // update the stream...
                $this->_model->duration = $result['duration'];
                $this->_model->path = $path;
                $this->_model->export = $export;
                // save
                $this->_model->save();

                // TODO: Decide how to handle failure in executing the scripts...
                Whispercast::getInterface($this->_model->server_id.'_runner')->Cmd()->RunScript('admin/Files.Create', array(Zend_Registry::get('Whispercast')->name, $name, $this->_model->public ? 'true' : ''));

                // mark to be reloaded
                $result['reload'] = true;
              }
              break;
            case 'TransferError':
            case 'TranscodeError':
            case 'PostProcessError':
            case 'NotFound':
            case 'Duplicate':
            case 'InvalidParameter':
            case 'DiskError':
            case 'BadSlaveState':
              // fall through
            default:
              $state = 'failed';
          }
          $result['result'] = true;
        }
      }

      if (($internal_id != $file->internal_id) || ($state !== $file->state)) {
        $file->internal_id = $internal_id;
        $file->state = $state;
        $file->save();

         // mark to be reloaded
         $result['reload'] = true;
      }

      $commited = true;
      $adapter->commit();
    } catch (Exception $e) {
      if ($new) {
        Whispercast::getInterface($this->_model->server_id.'_transcoder')->MasterControl()->Delete((int)$internal_id, true, true, true);
      }
      if (!$commited) {
        $adapter->rollBack();
      }
      return array('result'=>false);
    }
    return $result;
  }

  public function _insert() {
    $files = new Model_DbTable_Files();
    $this->_file = $files->createRow();
  }
  public function _postInsert() {
    $this->_file->key = md5(uniqid('whispercast.default.files.upload'.$this->_model->server_id, true));
    $this->_file->stream_id = $this->_model->id;
    $this->_file->save();
  }

  public function _update() {
  }
  public function _postUpdate() {
    $name = basename($this->_model->export);

    // TODO: Decide how to handle failure in executing the scripts...
    if ($this->_model->public) {
      Whispercast::getInterface($this->_model->server_id.'_runner')->Cmd()->RunScript('admin/Files.StartExport', array(Zend_Registry::get('Whispercast')->name, $name));
    } else {
      Whispercast::getInterface($this->_model->server_id.'_runner')->Cmd()->RunScript('admin/Files.StopExport', array(Zend_Registry::get('Whispercast')->name, $name));
    }
  }

  public function _delete() {
    $files = new Model_DbTable_Files();
    $this->_file = $files->fetchRow($files->getAdapter()->quoteInto('stream_id = ?', $this->_model->id));

    $this->__name = basename($this->_model->export);
  }
  public function _postDelete() {
    if ($this->_file->internal_id >= 0) {
      $cs = Whispercast::getInterface($this->_model->server_id.'_transcoder')->MasterControl()->Delete((int)$this->_file->internal_id, true, true, true);
      if (!$cs->success_) {
        throw new Exception($cs->error_);
      }
    }
    // TODO: Decide how to handle failure in executing the scripts...
    Whispercast::getInterface($this->_model->server_id.'_runner')->Cmd()->RunScript('admin/Files.Destroy', array(Zend_Registry::get('Whispercast')->name, $this->__name));
    unset($this->__name);
  }
}
?>
