<?php
class Model_Action_Osd extends App_Model_Action_Record {
  private function _getStream($id) {
    if (!$this->_stream) {
      $streams = new Model_DbTable_Streams();
      $this->_stream = $streams->fetchRow($streams->select()->where('id = ?', $id)->where('server_id = ?', $this->getServer()->id));

      if ($this->_stream === null) {
        throw new Zend_Controller_Action_Exception('Record not found', 400);
      }
    }
    return $this->_stream;
  }

  protected function _createForm() {
    $this->_output->form = new App_Form();
    $this->_output->form->
      setMethod('post');

    $e = array();

    $e['id'] = $this->_output->form->createElement('hidden', 'id');

    $e['click_url'] = $this->_output->form->createElement('text', 'click_url');
    $e['click_url']->
      setAttrib('size', 95)->
      addValidator('StringLength', false, array(1, 255));

    foreach ($e as $a) {
      $this->_output->form->addElement($a);
    }
    $this->_output->form->addElement('submit', 'submit');

    $action = $this->_action;
    switch ($action) {
      case 'add':
      case 'set':
        $action = 'edit';
    }

    $this->_output->form->
      setAttrib('name', 'default-osd-'.$action.'-form')->
      setAction(Zend_Controller_Action_HelperBroker::getStaticHelper('url')->direct($this->_request->getActionName(), $this->_request->getControllerName(), $this->_request->getModuleName()).'?sid='.$this->getServer()->id);
  }

  protected function _populateForm($record) {
    parent::_populateForm($record);

    if ($record['osd']) {
      $this->_populateForm($record['osd']);
    }
  }

  protected function _checkAccess($id, $action) {
    $server = $this->getServer();
    $stream = $this->_getStream($id);

    $acl = $this->getUser()->getAcl($server->id, $stream->type, $id);
    if (!$acl[$server->id][$stream->type]['osd']) {
      throw new Zend_Controller_Action_Exception('Not enough privileges', 403);
    }
  }

  protected function _index() {
  }
  protected function _add() {
    throw new Zend_Controller_Action_Exception('Invalid action', 400);
  }
  protected function _get($id) {
    $server = $this->getServer();
    $stream = $this->_getStream($id);

    $interface = Whispercast::getInterface($server->id);
    $osd = new Whispercast_Osd($interface);

    $osda_id = $interface->getElementPrefix().'_osda';

    $path = $interface->getElementPrefix().'_ts'.$stream->path;
    $cs = $osd->getData($osda_id, $path, $data);
    if ($cs !== null) {
      throw new Exception($cs);
    }
    $cs = $osd->listOverlays($osda_id, $overlays);
    if ($cs !== null) {
      throw new Exception($cs);
    }
    $cs = $osd->listCrawlers($osda_id, $crawlers);
    if ($cs !== null) {
      throw new Exception($cs);
    }

    $this->_output->model['record'] = $stream->toArray();
    $this->_output->model['record']['osd'] = $data;

    $this->_output->model['templates'] = array(
      'overlays'=>$overlays,
      'crawlers'=>$crawlers
    );
    $this->_output->result = 1;
  }
  protected function _set($id) {
    $this->_get($id);
    if ($this->_output->result > 0) {
      if ($this->_phase == App_Model_Action::PHASE_PERFORM) {
        $this->_processForm();
        if ($this->_output->result > 0) {
          try {
            $values = $this->_output->form->getValues();
            if (is_array($this->_input['overlays'])) {
              foreach($this->_input['overlays'] as $k=>$v) {
                $values['overlays'][htmlspecialchars_decode($k)] = htmlspecialchars_decode($v);
              }
            } else {
              $values['overlays'] = array();
            }
            if (is_array($this->_input['crawlers'])) {
              foreach($this->_input['crawlers'] as $k=>$v) {
                if (!is_array($v)) {
                  $v = htmlspecialchars_decode($v);
                  $v = preg_replace('/<ul(\s*[^>]*)>(.*?)<\/ul>/', '$2', $v);
                  $v = preg_split('/(<li>)|(<\/li>\s*<li>)|(<\/li>)/', $v, -1, PREG_SPLIT_NO_EMPTY);
                }
                $values['crawlers'][htmlspecialchars_decode($k)] = $v;
              }
            } else {
              $values['crawlers'] = array();
            }
            if (is_object($this->_input['pip'])) {
	      $pip = array();
	      $pip['url'] = $this->_input['pip']->url;
	      $pip['x_location'] = $this->_input['pip']->x_location;
	      $pip['y_location'] = $this->_input['pip']->y_location;
	      $pip['width'] = $this->_input['pip']->width;
	      $pip['height'] = $this->_input['pip']->height;
	      $pip['play'] = $this->_input['pip']->play;

	      $values['pip'] = $pip;
            } else
            if (is_array($this->_input['pip'])) {
	      $values['pip'] = $this->_input['pip'];
            }
            $values = $this->_parseData($values);

            $current = array(
              'overlays' => $values['overlays'],
              'crawlers' => $values['crawlers'],
              'pip' => $values['pip'],
              'click_url' => isset($values['click_url']) ? $values['click_url'] : null
            );

            $server = $this->getServer();
            $stream = $this->_getStream($id);

            $interface = Whispercast::getInterface($server->id);
            $osd = new Whispercast_Osd($interface);

            $path = $interface->getElementPrefix().'_ts'.$stream->path;
            if ((bool)$this->_input['overwrite']) {
              $final = $current;
            } else {
              $cs = $osd->getData($interface->getElementPrefix().'_osda', $path, $previous);
              if ($cs !== null) {
                throw new Exception($cs);
              }
              $cs = $osd->mergeData($previous, $current, $final);
              if ($cs !== null) {
                throw new Exception($cs);
              }
            }
            $cs = $osd->setData($interface->getElementPrefix().'_osda', $path, $final);
            if ($cs !== null) {
              throw new Exception($cs);
            }

            $this->_output->model['record'] = $stream->toArray();
            $this->_output->model['record']['osd'] = $final;

            $this->_output->result = 1;
          } catch (Exception $e) {
            $this->_output->errors[] = $e->getMessage();
            $this->_output->result = -1;
          }
        }
      }
    }
  }
  protected function _delete($id) {
    throw new Zend_Controller_Action_Exception('Invalid action', 400);
  }

  public static function getSoapClassmap() {
    return array(
      'Result'=>'Result',
      'OsdData_Crawler'=>'OsdData_Crawler',
      'OsdData_Pip'=>'OsdData_Pip',
      'OsdData_ClickUrl'=>'OsdData_ClickUrl',
      'OsdData'=>'OsdData'
    );
  }
}

class OsdData_Crawler {
  /**
   * The crawler items.
   * @var string[] $items
   */
  public $items;
}
class OsdData_Pip {
  /**
   * The URL to be played as PIP (Picture In Picture).
   * @var string $url
   */
  public $url;

  /**
   * The X location of the PIP box.
   * @var float $x_location
   */
  public $x_location;
  /**
   * The Y location of the PIP box.
   * @var float $y_location
   */
  public $y_location;

  /**
   * The width of the PIP box.
   * @var float $width
   */
  public $width;
  /**
   * The height of the PIP box.
   * @var float $height
   */
  public $height;

  /**
   * The play flag - if TRUE the PIP stream will start playing right away.
   * @var boolean $play
   */
  public $play;
}
class OsdData_ClickUrl {
  /**
   * The URL to navigate to on player click.
   * @var string $url
   */
  public $url;
}
class OsdData {
  /**
   * The overlays.
   * @var array $overlays
   */
  public $overlays;
  /**
   * The crawlers.
   * @var array $crawlers
   */
  public $crawlers;
  /**
   * The PIP setup.
   * @var OsdData_Pip $pip
   */
  public $pip;
  /**
   * The click URL.
   * @var OsdData_ClickUrl $click_url
   */
  public $click_url;

  /**
   * Transfer the OSD data to an array.
   *
   * @param OsdData $data The OSD data.
   * @return array
  */
  public static function toArray($data) {
    $array = array();

    // transfer overlays
    if ($data->overlays) {
      $array['overlays'] = $data->overlays;
    }
    // transfer crawlers
    if ($data->crawlers) {
      $array['crawlers'] = array();
      foreach ($data['crawlers'] as $k=>$v) {
        $array[$k] = $v->items;
      }
    }
    // transfer pip
    if (isset($data->pip)) {
      $pip = array();
      $pip['url'] = $data->pip->url;
      $pip['x_location'] = $data->pip->x_location;
      $pip['y_location'] = $data->pip->y_location;
      $pip['width'] = $data->pip->width;
      $pip['height'] = $data->pip->height;
      $pip['play'] = $data->pip->play;

      $array['pip'] = $pip;
    }
    // transfer click url
    if (is_set($data->click_url)) {
      $array['click_url'] = $data->click_url->url;
    }
  }

  /**
   * Transfer the OSD data from an array (produced by the action).
   *
   * @param array $array The OSD data array.
   * @return OsdData
  */
  public static function fromArray($array) {
    $data = new OsdData();

    // transfer overlays
    if (isset($array['overlays'])) {
      $data->overlays = $array['overlays'];
    }
    // transfer crawlers
    if (isset($array['crawlers'])) {
      $data->crawlers = array();
      foreach ($array['crawlers'] as $k=>$v) {
        $c = new OsdData_Crawler();
        $c->items = $v;
        $data->crawlers[$k] = $c;
      }
    }
    // transfer pip
    if (isset($array['pip'])) {
      $data->pip = new OsdData_Pip();
      $data->pip->url = $array['pip']['url'];
      $data->pip->x_location = (float)$array['pip']['x_location'];
      $data->pip->y_location = (float)$array['pip']['y_location'];
      $data->pip->width = (float)$array['pip']['width'];
      $data->pip->height = (float)$array['pip']['height'];
      $data->pip->play = (bool)$array['pip']['play'];
    }
    // transfer click url
    if (isset($array['click_url'])) {
      $data->click_url = new OsdData_ClickUrl();
      $data->click_url->url = $array['click_url'];
    }

    return $data;
  }
}
class Osd extends App_Model_Action_Proxy {
  /**
   * Retrieves the OSD data for a given stream.
   *
   * @param string $token The session token.
   * @param int $server_id The server id.
   * @param int $id The id of the stream.
   * @return Result
  */
  public function get($token, $server_id, $id) {
    $this->_login($token);
    $result = $this->_process('get', array(
      'sid'=>$server_id,
      'id'=>$id
    ));
    if (isset($result->model['record']['osd'])) {
      $result->model['record']['osd'] = OsdData::fromArray($result->model['record']['osd']);
    }
    return $result;
  }
  /**
   * Sets the OSD data for a given stream.
   *
   * @param string $token The session token.
   * @param int $server_id The server id.
   * @param int $id The id of the stream.
   * @param OsdData $data The OSD data to be associated to the stream.
   * @param boolean $overwrite The overwrite flag - if TRUE the entire OSD data associated
                               to the stream will be overwritten.
   * @return Result
  */
  public function set($token, $server_id, $id, $data, $overwrite) {
    $this->_login($token);
    $result = $this->_process('set', array(
      'sid'=>$server_id,
      'id'=>$id,
      'overlays'=>$data->overlays,
      'crawlers'=>$data->crawlers,
      'pip'=>$data->pip,
      'click_url'=>$data->click_url->url,
      'overwrite'=>$overwrite
    ));
    if (isset($result->model['record']['osd'])) {
      $result->model['record']['osd'] = OsdData::fromArray($result->model['record']['osd']);
    }
    return $result;
  }
}
?>
