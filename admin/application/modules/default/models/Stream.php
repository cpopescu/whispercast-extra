<?php
class Model_Stream extends App_Model_Record {
  private $_original;
  private $_delegateObject = null;

  public function init() {
    parent::init();
    $this->_original = $this->toArray();
  }

  protected function _stopExport($export, $protocol) {
    $interface = Whispercast::getInterface($this->server_id);
    try {
      $interface->MediaElementService()->StopExportElement(
        $protocol,
        $interface->export.$export
      );
    } catch(Exception $e) {
    }
  }
  protected function _startExport($export, $path, $protocol, $authorizer = null, $downloadable = false) {
    $interface = Whispercast::getInterface($this->server_id);
    try {
      $fc_video = 0+Zend_Registry::get('Whispercast')->flow_control['video'];
      $fc_total = 0+Zend_Registry::get('Whispercast')->flow_control['total'];
      $interface->MediaElementService()->StartExportElement(
        new ElementExportSpec(
          $interface->getExportPrefix($downloadable).$path,
          $protocol,
          $interface->export.$export,
          null,
          null,
          $authorizer,
          $downloadable ? 0 : $fc_video,
          $downloadable ? 0 : $fc_total
        )
      );
    } catch(Exception $e) {
    }
  }

  public function delegate() {
    if ($this->_delegateObject === null) {
      if ($this->type) {
        $parts =  array_map('ucfirst', split('_', $this->type));
        if (count($parts) == 1) {
          $class = 'Model_Delegate_'.$parts[0];
        } else {
          $class = array_shift($parts).'_Model_Delegate_'.join('_', $parts);
        }
        $this->_delegateObject = new $class($this);
      }
    }
    return $this->_delegateObject;
  }

  public function setFromArray($values) {
    parent::setFromArray($values);

    $delegate = $this->delegate();
    if ($delegate) {
      $delegate->setFromArray($values);
    }
  }

  public function _postCreate(&$data, $resource) {
    $this->type = $resource;

    $delegate = $this->delegate();
    if ($delegate) {
      $delegate->_postCreate($data);
    }
  }
  public function _postFetch(&$data) {
    $delegate = $this->delegate();
    if ($delegate) {
      $delegate->_postFetch($data);
    }
  }

  protected function _insert() {
    $this->delegate()->_insert();
  }
  protected function _postInsert() {
    $delegate = $this->delegate();
    $delegate->_postInsert();

    $prefix = Whispercast::getInterface($this->server_id)->getElementPrefix();

    if (!$delegate->disableExport) {
      $this->_startExport('/download'.$this->export, $this->path, 'http', $prefix.'_authorizer_preview', true);
      $this->_startExport('/preview'.$this->export, $this->path, 'rtmp', $prefix.'_authorizer_preview');
      $this->_startExport('/preview'.$this->export, $this->path, 'http', $prefix.'_authorizer_preview');
      if ($this->public) {
        $this->_startExport($this->export, $this->path, 'http');
        $this->_startExport($this->export, $this->path, 'rtmp');
      }
    }
    if (!$delegate->disableSave) {
      Whispercast::getInterface($this->server_id)->MediaElementService()->SaveConfig();
    }
  }

  protected function _update() {
    $this->delegate()->_update();
  }
  protected function _postUpdate() {
    $delegate = $this->delegate();
    $delegate->_postUpdate();

    if (!$delegate->disableExport) {
      $prefix = Whispercast::getInterface($this->server_id)->getElementPrefix();

      if (($this->path != $this->_original['path']) || ($this->export != $this->_original['export'])) {
        $this->_stopExport('/download'.$this->_original['export'], 'http');
        $this->_stopExport('/preview'.$this->_original['export'], 'rtmp');
        $this->_stopExport('/preview'.$this->_original['export'], 'http');
        $this->_stopExport($this->_original['export'], 'http');
        $this->_stopExport($this->_original['export'], 'rtmp');

        $this->_startExport('/download'.$this->export, $this->path, 'http', $prefix.'_authorizer_preview', true);
        $this->_startExport('/preview'.$this->export, $this->path, 'rtmp', $prefix.'_authorizer_preview');
        $this->_startExport('/preview'.$this->export, $this->path, 'http', $prefix.'_authorizer_preview');
        if ($this->public) {
          $this->_startExport($this->export, $this->path, 'http');
          $this->_startExport($this->export, $this->path, 'rtmp');
        }
      } else
      if (((bool)$this->public != (bool)$this->_original['public'])) {
        if ($this->public) {
          $this->_startExport($this->export, $this->path, 'http');
          $this->_startExport($this->export, $this->path, 'rtmp');
        } else {
          $this->_stopExport($this->_original['export'], 'http');
          $this->_stopExport($this->_original['export'], 'rtmp');
        }
      }
    }
    if (!$delegate->disableSave) {
      Whispercast::getInterface($this->server_id)->MediaElementService()->SaveConfig();
    }
  }

  protected function _delete() {
    $this->delegate()->_delete();
  }
  protected function _postDelete() {
    $delegate = $this->delegate();
    $delegate->_postDelete();

    if (!$delegate->disableExport) {
      $this->_stopExport('/download'.$this->export, 'http');
      $this->_stopExport('/preview'.$this->export, 'rtmp');
      if ($this->public) {
        $this->_stopExport($this->export, 'rtmp');
      }
    }
    if (!$delegate->disableSave) {
      Whispercast::getInterface($this->server_id)->MediaElementService()->SaveConfig();
    }
  }

  public function updateState() {
    return $this->delegate()->updateState();
  }
}
?>
