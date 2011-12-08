<?php
class Model_Action_Ranges extends Model_Action_Streams {
  protected $_resource= 'ranges';

  protected function _createForm() {
    parent::_createForm();

    $e = array();

    $e['begin_date'] = $this->_output->form->createElement('text', 'begin_date');
    $e['begin_date']->
      addValidator(new App_Validate_Date(array('alternate'=>'begin')))->
      setAttrib('size', 40)->
      setAttrib('class', 'form-date')->
      setRequired(true);
    $e['begin_time'] = $this->_output->form->createElement('text', 'begin_time');
    $e['begin_time']->
      addValidator(new App_Validate_Time(array('alternate'=>'begin')))->
      setAttrib('size', 15)->
      setAttrib('class', 'form-time')->
      setRequired(true);

    $e['end_date'] = $this->_output->form->createElement('text', 'end_date');
    $e['end_date']->
      addValidator(new App_Validate_Date(array('alternate'=>'end')))->
      setAttrib('size', 40)->
      setAttrib('class', 'form-date')->
      setRequired(true);
    $e['end_time'] = $this->_output->form->createElement('text', 'end_time');
    $e['end_time']->
      addValidator(new App_Validate_Time(array('alternate'=>'end')))->
      setAttrib('size', 15)->
      setAttrib('class', 'form-time')->
      setRequired(true);

    $e['source'] = $this->_output->form->createElement('hidden', 'source');
    $e['source']->
      setRequired(true);

    foreach ($e as $a) {
      $this->_output->form->addElement($a);
    }

    $this->_output->form->
      addDisplayGroup(array('begin_date', 'begin_time'), 'begin')->
      addDisplayGroup(array('end_date', 'end_time'), 'end');
  }

  protected function _processForm() {
    parent::_processForm();

    if ($this->_output->result > 0) {
      $begin = new Zend_Date($this->_input['setup']['begin']);
      $end = new Zend_Date($this->_input['setup']['end']);

      if ($begin->get(Zend_Date::TIMESTAMP) >= $end->get(Zend_Date::TIMESTAMP)) {
        $this->_output->errors[] = _('The date range is invalid.');
        $this->_output->result = 0;
      }
    }
  }

  protected function _parseData($record) {
    $result = parent::_parseData($record);

    $result['setup']['source'] = $result['source'];

    return $result;
  }

  public static function getSoapClassmap() {
    return array(
      'Result'=>'Result',
      'RangeSetup'=>'RangeSetup',
      'Date'=>'Date',
      'Time'=>'Time',
      'DateAndTime'=>'DateAndTime'
    );
  }
}

class RangeSetup {
  /**
   * The associated source stream.
   * @var int  $source
   */
  public $source;

  /**
   * The range begin.
   * @var DateAndTime $begin
   */
  public $begin;
  /**
   * The range end.
   * @var DateAndTime $end
   */
  public $end;

  /**
   * Transfer the setup from an array (produced by the action).
   *
   * @param array $array The data array.
   * @return RangeSetup
  */
  public static function fromArray($array) {
    $setup = new RangeSetup();
    if (isset($array['source'])) {
      $setup->source = $array['source'];
    }
    if (isset($array['begin'])) {
      $setup->begin = new DateAndTime();
      $setup->begin->date = new Date();
      $setup->begin->time = new Time();
      $setup->begin->date->year = (int)$array['begin']['year'];
      $setup->begin->date->month = (int)$array['begin']['month'];
      $setup->begin->date->day = (int)$array['begin']['day'];
      $setup->begin->time->hour = (int)$array['begin']['hour'];
      $setup->begin->time->minute = (int)$array['begin']['minute'];
      $setup->begin->time->second = (int)$array['begin']['second'];
    }
    if (isset($array['end'])) {
      $setup->end = new DateAndTime();
      $setup->end->date = new Date();
      $setup->end->time = new Time();
      $setup->end->date->year = (int)$array['end']['year'];
      $setup->end->date->month = (int)$array['end']['month'];
      $setup->end->date->day = (int)$array['end']['day'];
      $setup->end->time->hour = (int)$array['end']['hour'];
      $setup->end->time->minute = (int)$array['end']['minute'];
      $setup->end->time->second = (int)$array['end']['second'];
    }
    return $setup;
  }
}
class Ranges extends Streams {
  protected $_setupClass = 'RangeSetup';

  /**
   * Adds a new range.
   *
   * @param string $token The session token.
   * @param int $server_id The server id.
   * @param string $name The stream name.
   * @param string $description The stream description.
   * @param string $tags The tags associated to this stream.
   * @param int $category_id The id of the stream category.
   * @param boolean $public The public flag - if TRUE the stream is exported as a public stream.
   * @param boolean $usable The usable flag - if TRUE the stream can be used in switches, playlists, etc.
   * @param RangeSetup $setup The program specific setup.
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
      'begin'=>array(
        'year' => $setup->begin->year,
        'month' => $setup->begin->month,
        'day' => $setup->begin->day,
        'hour' => $setup->begin->hour,
        'minute' => $setup->begin->minute,
        'second' => $setup->begin->second
      ),
      'end'=>array(
        'year' => $setup->end->year,
        'month' => $setup->end->month,
        'day' => $setup->end->day,
        'hour' => $setup->end->hour,
        'minute' => $setup->end->minute,
        'second' => $setup->end->second
      ),
      'source'=>$setup->source
    ));
  }
  /**
   * Modifies an existing range.
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
   * @param RangeSetup $setup The program specific setup.
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
      'begin'=>array(
        'year' => $setup->begin->year,
        'month' => $setup->begin->month,
        'day' => $setup->begin->day,
        'hour' => $setup->begin->hour,
        'minute' => $setup->begin->minute,
        'second' => $setup->begin->second
      ),
      'end'=>array(
        'year' => $setup->end->year,
        'month' => $setup->end->month,
        'day' => $setup->end->day,
        'hour' => $setup->end->hour,
        'minute' => $setup->end->minute,
        'second' => $setup->end->second
      ),
      'source'=>$setup->source
    ));
  }
}
?>
