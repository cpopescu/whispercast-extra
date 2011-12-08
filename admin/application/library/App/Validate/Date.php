<?php
class App_Validate_Date extends Zend_Validate_Abstract {
  const INVALID = 'dateInvalid';

  protected $_messageTemplates = array(
    self::INVALID => '\'%value%\' is not a valid date'
  );

  public function __construct($options = null) {
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
        $f = new App_Filter_Date();
        $f->filter($context[$this->_alternate]);
      } else {
        new Zend_Date($valueString, Zend_Date::DATE_LONG);
      }

    } catch(Exception $e) {
      $this->_error(self::INVALID);
      return false;
    }
    return true;
  }
}
?>
