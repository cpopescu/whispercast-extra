<?php
class App_Form extends Zend_Form {
  public function loadDefaultDecorators() {
    $this->setDecorators(
      array(
        array('FormElements'),
        array('ViewScript', array('viewScript' => 'form.phtml', 'placement' => false))
      )
    );
  }
  public function createElement($type, $name, $options = array()) {
    $options['disableLoadDefaultDecorators'] = true;
    switch ($type) {
      case 'file':
        $options['decorators'] = array(
          array('File'),
          array('ViewScript', array('viewScript' => 'form_element.phtml', 'placement' => false)),
        );
        break;
      default:
        $options['decorators'] = array(
          array('ViewHelper'),
          array('ViewScript', array('viewScript' => 'form_element.phtml', 'placement' => false)),
        );
    }

    $options['class'] = 'form-'.$type;
    return parent::createElement($type, $name, $options);
  }
  public function addDisplayGroup($elements, $name, $options = array()) {
    $options['disableLoadDefaultDecorators'] = true;

    $options['decorators'] = array(
      array('FormElements'),
      array('ViewScript', array('viewScript' => 'form_group.phtml', 'placement' => isset($options['placement']) ? $options['placement'] : Zend_Form_Decorator_Abstract::PREPEND))
    );

    $options['class'] = 'form-display-group';

    unset($options['placement']);
    return parent::addDisplayGroup($elements, $name, $options);
  }
}
?>
