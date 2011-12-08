<?php
class Model_Action_Aliases extends Model_Action_Streams {
  protected $_resource= 'aliases';

 protected function _createForm() {
    parent::_createForm();

    $e = array();

    $e['export_as'] = ($this->_action == 'add') ? $this->_output->form->createElement('text', 'export_as') : $this->_output->form->createElement('text', 'export_as', array('readonly'=>'readonly'));
    $e['export_as']->
      setAttrib('size', 95)->
      addValidator('StringLength', false, array(1, 255))->
      setRequired(true);

    $e['stream'] = $this->_output->form->createElement('hidden', 'stream');
    $e['stream']->
      setRequired(true);

    foreach ($e as $a) {
      $this->_output->form->addElement($a);
    }

    $this->_output->form->
      addDisplayGroup(array('export_as', 'stream'), 'alias-group');
  }

  protected function _parseData($record) {
    $result = parent::_parseData($record);

    $result['setup']['stream'] = $result['stream'];

    return $result;
  }

  public static function getSoapClassmap() {
    return array(
      'Result'=>'Result',
      'AliasSetup'=>'AliasSetup'
    );
  }
}

class AliasSetup {
  /**
   * The id of the aliased stream.
   * @var int $stream
   */
  public $stream;
  /**
   * The name under which the alias gets exported.
   * @var string $export_as
   */
  public $export_as;

  /**
   * Transfer the setup from an array (produced by the action).
   *
   * @param array $array The data array.
   * @return AliasSetup
  */
  public static function fromArray($array) {
    $setup = new AliasSetup();
    if (isset($array['stream'])) {
      $setup->stream = $array['stream'];
    }
    if (isset($array['export_as'])) {
      $setup->export_as = $array['export_as'];
    }
    return $setup;
  }
}
class Aliases extends Streams {
  protected $_setupClass = 'AliasSetup';

  /**
   * Adds a new alias.
   *
   * @param string $token The session token.
   * @param int $server_id The server id.
   * @param string $name The stream name.
   * @param string $description The stream description.
   * @param string $tags The tags associated to this stream.
   * @param int $category_id The id of the stream category.
   * @param boolean $public The public flag - if TRUE the stream is exported as a public stream.
   * @param boolean $usable The usable flag - if TRUE the stream can be used in switches, playlists, etc.
   * @param AliasSetup $setup The alias specific setup.
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
      'export_as'=>$setup->export_as,
      'stream'=>$setup->stream
    ));
  }
  /**
   * Modifies an existing alias.
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
   * @param AliasSetup $setup The alias specific setup.
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
      'usable'=>$usable,
      'export_as'=>$setup->export_as,
      'stream'=>$setup->stream
    ));
  }
}
?>
