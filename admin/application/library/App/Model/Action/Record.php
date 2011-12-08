<?php
abstract
class App_Model_Action_Record extends App_Model_Action {
  protected function _init() {
    if (!isset($this->_key)) { 
      $this->_key = 'id';
    }

    parent::_init();
  }

  public function postDispatch() {
    if ($this->_output->form) {
      switch ($this->_context) {
        case 'json':
        case 'soap':
          unset($this->_output->form);
          break;
        default:
          if ($this->_output->model['record']) {
            $this->_populateForm($this->_output->model['record']);
          }
      }
    }

    if ($this->_output->model['record']) {
      unset($this->_output->model['record']['_']);
    }
    return parent::postDispatch();
  }

  abstract
  protected function _createForm();

  protected function _populateForm($record) {
    $this->_output->form->populate($record);
  }

  protected function _processForm() {
    if ($this->_output->form->isValid($this->_input)) {
      $this->_output->result = 1;
    } else {
      $this->_output->result = 0;
      foreach ($this->_output->form->getElements() as $element) {
        if ($element->hasErrors()) {
          $this->_output->errors[$element->getId()] = array_values($element->getMessages());
        }
      }
    }
  }

  abstract
  protected function _checkAccess($id, $action);

  abstract
  protected function _index();
  abstract
  protected function _add();
  abstract
  protected function _get($id);
  abstract
  protected function _set($id);
  abstract
  protected function _delete($id);

  public function indexAction() {
    $this->_checkAccess(null, 'index');

    $this->_index();
  }
  public function addAction() {
    $this->_checkAccess(null, 'add');

    $this->_createForm();
    $this->_add();
  }
  public function getAction() {
    if (!isset($this->_input[$this->_key])) {
      throw new Zend_Controller_Action_Exception('Invalid record id', 400);
    }
    $this->_checkAccess($this->_input[$this->_key], 'get');

    $this->_createForm();
    $this->_get($this->_input[$this->_key]);
  }
  public function setAction() {
    if (!isset($this->_input[$this->_key])) {
      throw new Zend_Controller_Action_Exception('Invalid record id', 400);
    }
    $this->_checkAccess($this->_input[$this->_key], 'set');

    $this->_createForm();
    $this->_set($this->_input[$this->_key]);
  }
  public function deleteAction() {
    if (!isset($this->_input[$this->_key])) {
      throw new Zend_Controller_Action_Exception('Invalid record id', 400);
    }
    $this->_checkAccess($this->_input[$this->_key], 'delete');

    $this->_createForm();
    $this->_delete($this->_input[$this->_key]);
  }
}
?>
