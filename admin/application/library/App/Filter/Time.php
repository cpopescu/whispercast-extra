<?php
class App_Filter_Time implements Zend_Filter_Interface {
  public function __construct() {
  }

  public function filter($value) {
    if (empty($value)) {
      return null;
    }

    if (is_array($value)) {
      $parsed = new Zend_Date(array('year'=>2000, 'month'=>1, 'day'=>1, 'hour'=>$value['hour'], 'minute'=>$value['minute'], 'second'=>$value['second']));
      $v = $parsed->toArray();
      if (
        $v['year'] != 2000 ||
        $v['month'] != 1 ||
        $v['day'] != 1 ||
        $v['hour'] != $value['hour'] ||
        $v['minute'] != $value['minute'] ||
        $v['second'] != $value['second']) {
        throw new Exception('Invalid time');
      }
    } else
    if (is_numeric($value)) {
      $parsed = new Zend_Date($value, Zend_Date::TIMESTAMP);
    } else {
      $parsed = new Zend_Date($value, Zend_Date::TIME_LONG);
    }
    return $parsed->toArray();
  }
}
?>
