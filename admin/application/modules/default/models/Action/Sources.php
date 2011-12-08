<?php
class Model_Action_Sources extends Model_Action_Streams {
  protected $_resource= 'sources';

  protected function _createForm() {
    $protocols = array('http_client'=>'HTTP', 'rtmp' => 'RTMP');

    parent::_createForm();

    $e = array();

    $e['protocol'] = ($this->_action == 'add') ? $this->_output->form->createElement('select', 'protocol') : $this->_output->form->createElement('select', 'protocol', array('disabled'=>'disabled'));
    $e['protocol']->
      setMultiOptions($protocols)->
      addValidator('StringLength', false, array(1, 32));
    if ($this->_action == 'add') {
      $e['protocol']->setRequired(true);
    }
    $e['source'] = ($this->_action == 'add') ? $this->_output->form->createElement('text', 'source') : $this->_output->form->createElement('text', 'source', array('readonly'=>'readonly'));
    $e['source']->
      setAttrib('size', 95)->
      addValidator('StringLength', false, array(1, 255))->
      setRequired(true);

    $e['saved'] = ($this->_action == 'add') ? $this->_output->form->createElement('checkbox', 'saved') : $this->_output->form->createElement('checkbox', 'saved', array('disabled'=>'disabled'));
    $e['saved']->
      setRequired(true);

    $e['password'] = $this->_output->form->createElement('text', 'password');
    $e['password']->
      addValidator('StringLength', false, array(0, 255));

    foreach ($e as $a) {
      $this->_output->form->addElement($a);
    }

    $this->_output->form->
      addDisplayGroup(array('protocol', 'source'), 'resource')->
      addDisplayGroup(array('saved'), 'features')->
      addDisplayGroup(array('password'), 'authorization');
  }

  protected function _parseData($record) {
    $result = parent::_parseData($record);

    $result['setup']['protocol'] = $result['protocol'];
    $result['setup']['source'] = $result['source'];
    $result['setup']['saved'] = (bool)$result['saved'];
    $result['setup']['password'] = $result['password'];

    return $result;
  }

  public function authorizeAction() {
    $sources = new Model_DbTable_Sources();

    $resource = explode('/', $this->_input['source']);
    if (count($resource) == 2) {
      $prefix = Whispercast::getInterface($this->getServer()->id)->getElementPrefix();
      if ($resource[0] = $prefix.'_rtmp_source') {
        $source = $sources->fetchRow($sources->select()->where('protocol = ?', 'rtmp')->where('source = ?', $resource[1]));
        if ($source) {
          if (($source->password === null) || ($source->password == $this->_input['password'])) {
            die('{"allowed_": true, "reauthorize_interval_ms_": 0, "time_limit_ms_": 0}');
          }
        }
      } else
      if ($resource[0] = $prefix.'_http_source') {
        $source = $sources->fetchRow($sources->select()->where('protocol = ?', 'http')->where('source = ?', $resource[1]));
        if ($source) {
          if (($source->password === null) || ($source->password == $this->_input['password'])) {
            die('{"allowed_": true, "reauthorize_interval_ms_": 0, "time_limit_ms_": 0}');
          }
        }
      }
    }
    die('{"allowed_": false, "reauthorize_interval_ms_": 0, "time_limit_ms_": 0}');
  }

  public static function getSoapClassmap() {
    return array(
      'Result'=>'Result',
      'SourceSetup'=>'SourceSetup'
    );
  }
}

class SourceSetup {
  /**
   * The protocol - one of 'http' or 'rtmp'.
   * @var string $protocol
   */
  public $protocol;
  /**
   * The internal source name.
   * @var string $source
   */
  public $source;
  /**
   * The password used to authenticate the uploader of the stream.
   * @var string $password
   */
  public $password;
  /**
   * The saved flag - if TRUE the source is automatically saved to disk.
   * @var boolean $saved
   */
  public $saved;

  /**
   * Transfer the setup from an array (produced by the action).
   *
   * @param array $array The data array.
   * @return SourceSetup
  */
  public static function fromArray($array) {
    $setup = new SourceSetup();
    if (isset($array['protocol'])) {
      $setup->protocol = $array['protocol'];
    }
    if (isset($array['source'])) {
      $setup->source = $array['source'];
    }
    if (isset($array['password'])) {
      $setup->password = $array['password'];
    }
    if (isset($array['saved'])) {
      $setup->saved = (bool)$array['saved'];
    }
    return $setup;
  }
}
class Sources extends Streams {
  protected $_setupClass = 'SourceSetup';

  /**
   * Adds a new source.
   *
   * @param string $token The session token.
   * @param int $server_id The server id.
   * @param string $name The stream name.
   * @param string $description The stream description.
   * @param string $tags The tags associated to this stream.
   * @param int $category_id The id of the stream category.
   * @param boolean $public The public flag - if TRUE the stream is exported as a public stream.
   * @param boolean $usable The usable flag - if TRUE the stream can be used in switches, playlists, etc.
   * @param SourceSetup $setup The source specific setup.
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
      'usable'=>$usable,
      'protocol'=>$setup->protocol,
      'source'=>$setup->source,
      'password'=>$setup->password,
      'saved'=>$setup->saved
    ));
  }
  /**
   * Modifies an existing source.
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
   * @param SourceSetup $setup The source specific setup.
   * @return Result
  */
  public function set($token, $server_id, $id, $name, $description, $tags, $category_id, $public, $usable, $setup) {
    $this->_login($token);
    return $this->_process('set', array(
      'sid'=>$server_id,
      'id='>$id,
      'name'=>$name,
      'description'=>$description,
      'tags'=>$tags,
      'category_id'=>$category_id,
      'public'=>$public,
      'usable'=>$usable,
      'protocol'=>$setup ? $setup->protocol : null,
      'source'=>$setup ? $setup->source : null,
      'password'=>$setup ? $setup->password : null,
      'saved'=>$setup ? $setup->saved : null
    ));
  }
}
?>
