<?php
class Whispercast_Base {
  /**
   * Converts an object to an array.
   *
   * @param[in] object $object The object to be converted.
   *
   * @return array
   */
  public static function toArray($object) {
    if ($object) {
      $result = array();
      foreach ($object as $key=>$value) {
        if ($value !== null) {
          if (is_object($value))
            $result[$key] = self::toArray($value);
          else
            $result[$key] = $value;
        }
      }
      return $result;
    }
    return null;
  }
  /**
   * Converts an array into an object.
   *
   * @param[in] array $array The array to be converted.
   * @param[in] string $class The object's class name.
   * @param[in] string $parameters The object's constructor parameter names.
   *
   * @return object
   */
  public static function toObject($array, $class, $members) {
    $locals = array();
    if ($array !== null) {
      $arguments = array();
      foreach ($members as $key) {
        if (is_array($key)) {
          $locals[] = self::toObject($array[$key[0]], $key[1], $key[2]);
          $arguments[] = '$locals['.(count($locals)-1).']';
        } else {
          $arguments[] = var_export($array[$key], true);
        }
      }
      eval('$result = new '.$class.'('.implode(',', $arguments).');');
      return $result;
    }
    return null;
  }

  /**
   * The associated interface.
   *
   * @var array
   */
  protected $_interface;

  /**
   * Constructor for the Whispercast object.
   *
   * @param[in] object $interface The actual interface.
   *
   * @return void 
   */
  public function __construct($interface) {
    $this->_interface = $interface;
  }
}
?>
