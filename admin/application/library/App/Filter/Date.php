<?php
class App_Filter_Date implements Zend_Filter_Interface {
  public function __construct() {
  }

  public function filter($value) {
    if (empty($value)) {
      return null;
    }

    if (is_array($value)) {
      $parsed = new Zend_Date(array('year'=>$value['year'], 'month'=>$value['month'], 'day'=>$value['day'], 'hour'=>0, 'minute'=>0, 'second'=>0));
      $v = $parsed->toArray();
      if (
        $v['year'] != $value['year'] ||
        $v['month'] != $value['month'] ||
        $v['day'] != $value['day'] ||
        $v['hour'] != 0 ||
        $v['minute'] != 0 ||
        $v['second'] != 0) {
        throw new Exception('Invalid date');
      }
    } else
    if (is_numeric($value)) {
      $parsed = new Zend_Date($value, Zend_Date::TIMESTAMP);
    } else {
      $parsed = new Zend_Date($value, Zend_Date::DATE_LONG);
    }
    return $parsed->toArray();
  }
}
?>
