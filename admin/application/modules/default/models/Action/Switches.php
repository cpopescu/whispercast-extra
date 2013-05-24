<?php
class Model_Action_Switches extends Model_Action_Streams {
  protected $_resource= 'switches';

  public function currentAction() {
    $this->_action = 'operate';
    $server = $this->getServer();

    if (!isset($this->_input['id'])) {
      throw new Zend_Controller_Action_Exception('Invalid record id', 400);
    }
    $this->_checkAccess($this->_input['id'], 'operate');

    $this->_get($this->_input['id']);
    if ($this->_output->result <= 0) {
      return;
    }

    $interface = Whispercast::getInterface($server->id);
    $switches = new Whispercast_Switches($interface);

    $error = $switches->getMedia($this->_output->model['record']->id, $media);
    if ($error === null) {
      $this->_output->model['media'] = array(
      );

      $streams = new Model_DbTable_Streams();

      $this->_output->model['media']['current'] = $streams->fetchRow($streams->select()->where('path LIKE ?', $interface->getPathForLike($media['current'])));
      if ($this->_output->model['media']['current']) {
        $this->_output->model['media']['current'] = $this->_output->model['media']['current']->toArray();
      }
      $this->_output->model['media']['next'] = $streams->fetchRow($streams->select()->where('path LIKE ?', $interface->getPathForLike($media['next'])));
      if ($this->_output->model['media']['next']) {
        $this->_output->model['media']['next'] = $this->_output->model['media']['next']->toArray();
      }

      $this->_output->result = 1;
    } else {
      $this->_output->errors[] = $error;
      $this->_output->result = 0;
    }
  }
  public function switchAction() {
    $this->_action = 'operate';
    $server = $this->getServer();

    if (!isset($this->_input['id'])) {
      throw new Zend_Controller_Action_Exception('Invalid record id', 400);
    }
    if (!isset($this->_input['stream'])) {
      throw new Zend_Controller_Action_Exception('Invalid media id', 400);
    }
    $this->_checkAccess($this->_input['id'], 'operate');

    $this->_get($this->_input['id']);
    if ($this->_output->result <= 0) {
      return;
    }

    $streams = new Model_DbTable_Streams();
    $source = $streams->fetchRow($streams->select()->where('id = ?', $this->_input['stream'])->where('server_id = ?', $server->id));
    if ($source) {
      $interface = Whispercast::getInterface($server->id);
      $switches = new Whispercast_Switches($interface);
      
      $error = $switches->setMedia($this->_output->model['record']->id, $interface->getStreamPath($source->path), (bool)$this->_input['set_as_default'], (bool)$this->_input['switch_now']); 
      if ($error === null) {
        $this->_output->result = 1;
      } else {
        $this->_output->errors[] = $error;
        $this->_output->result = 0;
      }

      $this->_output->result = 1;
    } else {
      $this->_output->result = -1;
    }
  }

  public static function getSoapClassmap() {
    return array(
      'Result'=>'Result',
      'SwitchSetup'=>'SwitchSetup'
    );
  }
}

class SwitchSetup {
  /**
   * Transfer the setup from an array (produced by the action).
   *
   * @param array $array The data array.
   * @return SwitchSetup
  */
  public static function fromArray($array) {
    $setup = new SwitchSetup();
    return $setup;
  }
}
class Switches extends Streams {
  protected $_setupClass = 'SwitchSetup';

  /**
   * Adds a new switch.
   *
   * @param string $token The session token.
   * @param int $server_id The server id.
   * @param string $name The stream name.
   * @param string $description The stream description.
   * @param string $tags The tags associated to this stream.
   * @param int $category_id The id of the stream category.
   * @param boolean $public The public flag - if TRUE the stream is exported as a public stream.
   * @param boolean $usable The usable flag - if TRUE the stream can be used in switches, playlists, etc.
   * @param SwitchSetup $setup The switch specific setup.
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
   * @param SwitchSetup $setup The switch specific setup.
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
  /**
   * Interrogates the switch for the current/next stream.
   *
   * @param string $token The session token.
   * @param int $server_id The server id.
   * @param int $id The switch id.
   * @return Result
  */
  public function getCurrent($token, $server_id, $id) {
    $this->_login($token);
    return $this->_process('current', array(
      'sid'=>$server_id,
      'id'=>$id
    ));
  }
  /**
   * Operates the switch, switching to the given stream.
   *
   * @param string $token The session token.
   * @param int $server_id The server id.
   * @param int $id The switch id.
   * @param int $stream The id of the stream to switch to.
   * @param boolean $set_as_default If TRUE the given stream becomes the default stream for this switch.
   * @param boolean $switch_now If TRUE the switch will be performed immediately.
   * @return Result
  */
  public function setCurrent($token, $server_id, $id, $stream, $set_as_default, $switch_now) {
    $this->_login($token);
    return $this->_process('switch', array(
      'sid'=>$server_id,
      'id'=>$id,
      'stream'=>$stream,
      'set_as_default'=>$set_as_default,
      'switch_now'=>$switch_now
    ));
  }
}
?>
