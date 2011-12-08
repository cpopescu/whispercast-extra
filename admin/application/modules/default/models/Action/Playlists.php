<?php
class Model_Action_Playlists extends Model_Action_Streams {
  protected $_resource= 'playlists';

  protected function _createForm() {
    parent::_createForm();

    $e = array();
    $e['loop'] = $this->_output->form->createElement('checkbox', 'loop');
    $e['loop']->
      setRequired(true);

    foreach ($e as $a) {
      $this->_output->form->addElement($a);
    }

    $this->_output->form->
      addDisplayGroup(array('loop'), 'playlist-group');
  }
      
  protected function _parseData($record) {
    $result = parent::_parseData($record);

    $result['setup']['loop'] = $result['loop'];
    
    return $result;
  }

  public static function getSoapClassmap() {
    return array(
      'Result'=>'Result',
      'PlaylistSetup'=>'PlaylistSetup'
    );
  }
}

class PlaylistSetup {
  /**
   * The loop flag.
   * @var boolean $loop
   */
  public $loop;
  /**
   * The actual playlist.
   * @var int[] $playlist
   */
  public $playlist;

  /**
   * Transfer the setup from an array (produced by the action).
   *
   * @param array $array The data array.
   * @return PlaylistSetup
  */
  public static function fromArray($array) {
    $setup = new PlaylistSetup();
    if (isset($array['loop'])) {
      $setup->loop = (bool)$array['loop'];
    }
    if (isset($array['playlist'])) {
      $setup->playlist = $array['playlist'];
    }
    return $setup;
  }
}
class Playlists extends Streams {
  protected $_setupClass = 'PlaylistSetup';

  /**
   * Adds a new playlist.
   *
   * @param string $token The session token.
   * @param int $server_id The server id.
   * @param string $name The stream name.
   * @param string $description The stream description.
   * @param string $tags The tags associated to this stream.
   * @param int $category_id The id of the stream category.
   * @param boolean $public The public flag - if TRUE the stream is exported as a public stream.
   * @param boolean $usable The usable flag - if TRUE the stream can be used in switches, playlists, etc.
   * @param PlaylistSetup $setup The playlist specific setup.
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
      'loop'=>$setup->loop,
      'setup'=>array(
        'playlist'=>$setup->playlist
      )
    ));
  }
  /**
   * Modifies an existing playlist.
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
   * @param PlaylistSetup $setup The playlist specific setup.
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
      'usable'=>$usable,
      'loop'=>$setup->loop,
      'setup'=>array(
        'playlist'=>$setup->playlist
      )
    ));
  }
}
?>
