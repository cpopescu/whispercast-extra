<?php
class Model_Action_Programs extends Model_Action_Streams {
  protected $_resource= 'programs';

  protected function _createForm() {
    parent::_createForm();

    $e = array();

    $e['default_stream'] = $this->_output->form->createElement('hidden', 'default_stream');
    $e['default_stream']->
      setRequired(true);

    foreach ($e as $a) {
      $this->_output->form->addElement($a);
    }
  }

  protected function _parseData($record) {
    $result = parent::_parseData($record);

    $result['setup']['default_stream'] = $result['default_stream'];

    return $result;
  }

  public static function getSoapClassmap() {
    return array(
      'Result'=>'Result',
      'ProgramSetup'=>'ProgramSetup',
      'ProgramEntry'=>'ProgramEntry',
      'Date'=>'Date',
      'Time'=>'Time',
      'DateAndTime'=>'DateAndTime'
    );
  }
}

class ProgramEntry {
  /**
   * The id of the stream.
   * @var int $stream
   */
  public $stream;
  /**
   * The time.
   * @var Time $time
   */
  public $time;
  /**
   * The date.
   * @var Date $date
   */
  public $date;
  /**
   * The weekdays.
   * @var int[] $date
   */
  public $weekdays;
}
class ProgramSetup {
  /**
   * The id of the default stream.
   * @var int  $default_stream
   */
  public $default_stream;
  /**
   * The actual program entries.
   * @var ProgramEntry[] $program
   */
  public $program;

  /**
   * Transfer the setup from an array (produced by the action).
   *
   * @param array $array The data array.
   * @return ProgramSetup
  */
  public static function fromArray($array) {
    $setup = new ProgramSetup();
    if (isset($array['default_stream'])) {
      $setup->default_stream = $array['default_stream'];
    }
    if (isset($array['program'])) {
      $setup->program = array();
      foreach ($array['program'] as $k=>$v) {
        $e = new ProgramEntry();
        if (isset($v['stream'])) {
          $e->stream = $v['stream'];
        }
        if (isset($v['time'])) {
          $e->time = new Time();
          $e->time->hour = $v['time']['hour'];
          $e->time->minute = $v['time']['minute'];
          $e->time->second = $v['time']['second'];
        }
        if (isset($v['date'])) {
          $e->date = new Date();
          $e->date->year = $v['date']['year'];
          $e->date->month = $v['date']['month'];
          $e->date->day = $v['date']['day'];
        }
        if (isset($v['weekdays'])) {
          $e->weekdays = $v['weekdays'];
        }
        $setup->program[$k] = $e;
      }
    }
    return $setup;
  }
}
class Programs extends Streams {
  protected $_setupClass = 'ProgramSetup';

  /**
   * Adds a new program.
   *
   * @param string $token The session token.
   * @param int $server_id The server id.
   * @param string $name The stream name.
   * @param string $description The stream description.
   * @param string $tags The tags associated to this stream.
   * @param int $category_id The id of the stream category.
   * @param boolean $public The public flag - if TRUE the stream is exported as a public stream.
   * @param boolean $usable The usable flag - if TRUE the stream can be used in switches, playlists, etc.
   * @param ProgramSetup $setup The program specific setup.
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
      'default_stream'=>$setup->default_stream,
      'setup'=>array(
        'program'=>$setup->program
      )
    ));
  }
  /**
   * Modifies an existing program.
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
   * @param ProgramSetup $setup The program specific setup.
   * @return Result
  */
  public function set($token, $server_id, $id, $name, $description, $tags, $category_id, $public, $usable, $setup) {
    $this->_login($token);
    return $this->_process('set', array(
      'sid'=>$server_id,
      'name'=>$name,
      'id'=>$id,
      'description'=>$description,
      'tags'=>$tags,
      'category_id'=>$category_id,
      'public'=>$public,
      'usable'=>$usable,
      'loop'=>$setup ? $setup->loop : null,
      'default_stream'=>$setup->default_stream,
      'setup'=>array(
        'program'=>$setup->program
      )
    ));
  }
}
?>
