<?php
class App_Validate_Time extends Zend_Validate_Abstract {
  const INVALID  = 'timeInvalid';

  protected $_messageTemplates = array(
    self::INVALID => '\'%value%\' is not a valid time'
  );

  public function __construct($options) {
    $this->_alternate = $options ? $options['alternate'] : null;
    $this->_alternateFormat = $options ? $options['alternateFormat'] : null;
  }

  public function isValid($value, $context = null) {
    if (empty($value)) {
      return true;
    }
   
    $valueString = (string)$value;
    $this->_setValue($valueString);

    try {
      if ($this->_alternate) {
        $f = new App_Filter_Time();
        $f->filter($context[$this->_alternate]);
      } else {
        new Zend_Date($valueString, Zend_Date::TIME_LONG);
      }

    } catch(Exception $e) {
      $this->_error(self::INVALID);
      return false;
    }
    return true;
  }
}
?>
