<?php
class Result {
  /**
   * Result code (>0 success, else failure)
   * @var int $result
   */
  public $result = 1;
  /**
   * Errors, as an array
   * @var array $errors
   */
  public $errors = array();

  /**
   * The model, as an array
   * @var array $model
   */
  public $model = array();
}

class App_Model_Action {
  const PHASE_PREPARE = 'prepare';
  const PHASE_PERFORM = 'perform';

  protected $_request;
  protected $_response;

  protected $_context;
  protected $_phase;

  protected $_action;
  protected $_module;

  protected $_input;
  protected $_output;

  public function __construct(Zend_Controller_Request_Abstract $request, Zend_Controller_Response_Abstract $response, array $invokeArgs = array()) {
    $this->_request = $request;
    $this->_response = $response;

    $this->_params = $invokeArgs;

    $this->_init();
  }

  protected function _init() {
    $this->_context = isset($this->_params['context']) ? $this->_params['context'] : Zend_Controller_Action_HelperBroker::getStaticHelper('contextSwitch')->getCurrentContext();
    $this->_phase = isset($this->_params['phase']) ? $this->_params['phase'] : (($this->_request->getMethod() == 'POST') ? App_Model_Action::PHASE_PERFORM : App_Model_Action::PHASE_PREPARE);
  }

  protected function _parseData($record) {
    $result = array();
    foreach ($record as $k=>$v) {
      $result[$k] = ($v == '') ? null : $v;
    }
    return $result;
  }

  public function getRequest() {
    return $this->_request;
  }
  public function getResponse() {
    return $this->_response;
  }

  public function getContext() {
    return $this->_context;
  }
  public function getPhase() {
    return $this->_phase;
  }

  public function getUser($required = true) {
    return Zend_Controller_Action_HelperBroker::getStaticHelper('auth')->user($required);
  }
  public function getServer($required = true) {
    return Zend_Controller_Action_HelperBroker::getStaticHelper('server')->server($this->_input['sid'], $required);
  }

  public function preDispatch($input = null) {
    $this->_module = $this->_request->getModuleName();
    $this->_action = $this->_request->getActionName();
    if (isset($input)) {
      //TODO: Should we worry about param injection here ?
      //$this->_request->clearParams();
      $this->_request->setParams($input);
    }

    $this->_input = $this->_parseData($this->_request->getParams());
    $this->_output = new Result();

    Bootstrap::log('App_Model_Action::preDispatch');
    Bootstrap::log($this->_input);
  }

  public function postDispatch() {
    $output = $this->_output;

    $this->_output = null;
    $this->_input = null;

    $this->_action = null;
    $this->_module = null;

    Bootstrap::log('App_Model_Action::postDispatch');
    Bootstrap::log(array('result'=>$output->result, 'errors'=>$output->errors, 'model'=>$output->model));

    return $output;
  }

  public static function getSoapClassmap() {
    return array();
  }
}
?>
