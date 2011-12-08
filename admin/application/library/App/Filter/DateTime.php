<?php
class App_Filter_DateTime implements Zend_Filter_Interface {
  public function __construct() {
  }

  public function filter($value) {
    if (empty($value)) {
      return null;
    }

    if (is_array($value)) {
      $parsed = new Zend_Date(array('year'=>$value['year'], 'month'=>$value['month'], 'day'=>$value['day'], 'hour'=>$value['hour'], 'minute'=>$value['minute'], 'second'=>$value['second']));
      $v = $parsed->toArray();
      if (
        $v['year'] != $value['year'] ||
        $v['month'] != $value['month'] ||
        $v['day'] != $value['day'] ||
        $v['hour'] != $value['hour'] ||
        $v['minute'] != $value['minute'] ||
        $v['second'] != $value['second']) {
        throw new Exception('Invalid date or time');
      }
    } else
    if (is_numeric($value)) {
      $parsed = new Zend_Date($value, Zend_Date::TIMESTAMP);
    } else {
      $parsed = new Zend_Date($value, Zend_Date::DATETIME_LONG);
    }
    return $parsed->toArray();
  }
}
?>
