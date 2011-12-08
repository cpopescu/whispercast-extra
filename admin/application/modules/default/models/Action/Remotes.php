<?php
class Model_Action_Remotes extends Model_Action_Streams {
  protected $_resource= 'remotes';

  protected function _createForm() {
    parent::_createForm();

    $e = array();

    $e['host_ip'] = $this->_output->form->createElement('text', 'host_ip', array('size'=>30));
    $e['host_ip']->
      addValidator('Hostname', false, array(Zend_Validate_Hostname::ALLOW_IP))->
      addValidator('StringLength', false, array(1, 255))->
      setRequired(true);
    $e['port'] = $this->_output->form->createElement('text', 'port', array('size'=>10));
    $e['port']->
      addValidator('Int', false)->
      addValidator('Int', false, array(null, 0, 65535))->
      setRequired(true);
    $e['path_escaped'] = $this->_output->form->createElement('text', 'path_escaped', array('size'=>30));
    $e['path_escaped']->
      setRequired(true);
      
    $e['should_reopen'] = $this->_output->form->createElement('checkbox', 'should_reopen');
    $e['should_reopen']->
      setRequired(true);
    $e['fetch_only_on_request'] = $this->_output->form->createElement('checkbox', 'fetch_only_on_request');
    $e['fetch_only_on_request']->
      setRequired(true);
      
    $e['media_type'] = $this->_output->form->createElement('text', 'media_type', array('size'=>30));
    $e['media_type']->
      addValidator('StringLength', false, array(1, 255))->
      setRequired(true);
    $e['remote_user'] = $this->_output->form->createElement('text', 'remote_user', array('size'=>30));
    $e['remote_user']->
      addValidator('StringLength', false, array(1, 255));
    $e['remote_password'] = $this->_output->form->createElement('text', 'remote_password', array('size'=>30));
    $e['remote_password']->
      addValidator('StringLength', false, array(1, 255));
      
    foreach ($e as $a) {
      $this->_output->form->addElement($a);
    }

    $this->_output->form->
      addDisplayGroup(array('host_ip', 'port', 'path_escaped'), 'url')->
      addDisplayGroup(array('should_reopen', 'fetch_only_on_request'), 'behaviour')->
      addDisplayGroup(array('media_type'), 'media')->
      addDisplayGroup(array('remote_user', 'remote_password'), 'authentication');
  }

  protected function _processForm() {
    parent::_processForm();
  }

  protected function _parseData($record) {
    $result = parent::_parseData($record);

    $result['setup']['host_ip'] = $result['host_ip'];
    $result['setup']['port'] = $result['port'];
    $result['setup']['path_escaped'] = $result['path_escaped'];
    $result['setup']['should_reopen'] = $result['should_reopen'];
    $result['setup']['fetch_only_on_request'] = $result['fetch_only_on_request'];
    $result['setup']['media_type'] = $result['media_type'];
    $result['setup']['remote_user'] = $result['remote_user'];
    $result['setup']['remote_password'] = $result['remote_password'];

    return $result;
  }

  public static function getSoapClassmap() {
    return array(
      'Result'=>'Result',
      'RemoteSetup'=>'RemoteSetup'
    );
  }
}

class RemoteSetup {
  /**
   * The host IP address, in numeric form.
   * @var string $host_ip
   */
  public $host_ip;
  /**
   * The port.
   * @var int $port
   */
  public $port;
  /**
   * The path, escaped.
   * @var string $path_escaped
   */
  public $path_escaped;
  /**
   * If true, the stream is fetched only on request.
   * @var string $fetch_only_on_request
   */
  public $fetch_only_on_request;
  /**
   * The media type of the stream.
   * @var string $media_type
   */
  public $media_type;
  /**
   * The user name for HTTP authentication.
   * @var string $remote_user
   */
  public $remote_user;
  /**
   * The password for HTTP authentication.
   * @var string $remote_password
   */
  public $remote_password;
  
  /**
   * Transfer the setup from an array (produced by the action).
   *
   * @param array $array The data array.
   * @return RemoteSetup
  */
  public static function fromArray($array) {
    $setup = new RemoteSetup();
    return $setup;
  }
}
class Remotes extends Streams {
  protected $_setupClass = 'RemoteSetup';

  /**
   * Adds a new remote stream.
   *
   * @param string $token The session token.
   * @param int $server_id The server id.
   * @param string $name The stream name.
   * @param string $description The stream description.
   * @param string $tags The tags associated to this stream.
   * @param int $category_id The id of the stream category.
   * @param boolean $public The public flag - if TRUE the stream is exported as a public stream.
   * @param boolean $usable The usable flag - if TRUE the stream can be used in remotes, playlists, etc.
   * @param RemoteSetup $setup The switch specific setup.
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
   * Modifies an existing switch.
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
   * @param RemoteSetup $setup The switch specific setup.
   * @return Result
  */
  public function set($token, $server_id, $id, $name, $description, $tags, $category_id, $public, $usable, $setup) {
    $this->_login($token);
    return $this->_process('set', array(
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
