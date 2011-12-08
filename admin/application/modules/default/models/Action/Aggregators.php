<?php
class Model_Action_Aggregators extends Model_Action_Streams {
  protected $_resource= 'aggregators';

  public static function getSoapClassmap() {
    return array(
      'Result'=>'Result',
      'AggregatorSetup'=>'AggregatorSetup'
    );
  }
}

class AggregatorSetup {
  /**
   * The flavours.
   * @var int[] $flavours
   */
  public $flavours;

  /**
   * Transfer the setup from an array (produced by the action).
   *
   * @param array $array The data array.
   * @return AggregatorSetup
  */
  public static function fromArray($array) {
    $setup = new AggregatorSetup();
    if (isset($array['flavours'])) {
      $setup->flavours = $array['flavours'];
    }
    return $setup;
  }
}
class Aggregators extends Streams {
  protected $_setupClass = 'AggregatorsSetup';

  /**
   * Adds a new aggregator.
   *
   * @param string $token The session token.
   * @param int $server_id The server id.
   * @param string $name The stream name.
   * @param string $description The stream description.
   * @param string $tags The tags associated to this stream.
   * @param int $category_id The id of the stream category.
   * @param boolean $public The public flag - if TRUE the stream is exported as a public stream.
   * @param boolean $usable The usable flag - if TRUE the stream can be used in switches, playlists, etc.
   * @param AggregatorSetup $setup The aggregator specific setup.
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
      'setup' => array(
        'flavours'=>$setup->flavours
      )
    ));
  }
  /**
   * Modifies an existing aggregator.
   *
   * @param string $token The session token.
   * @param int $server_id The server id.
   * @param string $id The stream id.
   * @param string $name The stream name.
   * @param string $description The stream description.
   * @param string $tags The tags associated to this stream.
   * @param int $category_id The id of the stream category.
   * @param boolean $public The public flag - if TRUE the stream is exported as a public stream.
   * @param boolean $usable The usable flag - if TRUE the stream can be used in switches, playlists, etc.
   * @param AggregatorSetup $setup The aggregator specific setup.
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
      'setup' => array(
        'flavours'=>$setup->flavours
      )
    ));
  }
}
?>
