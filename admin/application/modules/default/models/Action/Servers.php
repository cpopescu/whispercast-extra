<?php
class Model_Action_Servers extends App_Model_Action_Record_Db {
  protected $_module = 'default';
  protected $_table = 'servers';
  protected $_order =
  array(
    'name'=>true,
    'host'=>true
  );

  public function postDispatch() {
    $user = $this->getUser();

    $acl = array();
    switch ($this->_action) {
      case 'index':
        if ($this->_output->model['records']) {
          foreach($this->_output->model['records'] as $record) {
            $racl = $user->getAcl($record['id'], array_keys(Zend_Registry::get('Whispercast')->resources), null, null);
            if ($racl) {
              foreach ($racl[$record['id']] as $resource=>$action) {
                $acl[$resource][$record['id']] = $action;
              }
            }
            $racl = $user->getAcl(0, 'servers', $record['id'], null);
            if ($racl) {
              $acl['servers'][$record['id']] = $racl[0]['servers'];
            }
          }
        }
        break;
      case 'get':
      case 'set':
        if ($this->_output->model['record']) {
          $racl = $user->getAcl($this->_output->model['record']['id'], array_keys(Zend_Registry::get('Whispercast')->resources), null, null);
          if ($racl) {
            foreach ($racl[$this->_output->model['record']['id']] as $resource=>$action) {
              $acl[$resource][$record['id']] = $action;
            }
          }
          $racl = $user->getAcl(0, 'servers', $this->_output->model['record']['id'], null);
          if ($racl) {
            $acl['servers'][$this->_output->model['record']['id']] = $racl[0]['servers'];
          }
        }
        break;
      case 'add':
        break;
    }

    $racl = $user->getAcl(0, 'servers', 0, null);
    if ($racl) {
      $acl['servers'][0] = $racl[0]['servers'];
    }
    $this->_output->model['acl'] = $acl;

    return parent::postDispatch();
  }

  protected function _createForm() {
    $this->_output->form = new App_Form();
    $this->_output->form->setMethod('post');

    $e = array();

    if ($this->_action != 'add') {
      $e['id'] = $this->_output->form->createElement('hidden', 'id');
    }

    $e['name'] = $this->_output->form->createElement('text', 'name');
    $e['name']->
      addValidator('StringLength', false, array(1, 255))->
      setRequired(true);
    $e['host'] = $this->_output->form->createElement('text', 'host', array('size'=>30));
    $e['host']->
      addValidator('Hostname', false, array(Zend_Validate_Hostname::ALLOW_DNS | Zend_Validate_Hostname::ALLOW_IP))->
      addValidator('StringLength', false, array(1, 255))->
      setRequired(true);
    $e['http_port'] = $this->_output->form->createElement('text', 'http_port', array('size'=>10));
    $e['http_port']->
      addValidator('Int', false)->
      addValidator('Int', false, array(null, 0, 65535))->
      setRequired(true);
    $e['rtmp_port'] = $this->_output->form->createElement('text', 'rtmp_port', array('size'=>10));
    $e['rtmp_port']->
      addValidator('Int', false)->
      addValidator('Int', false, array(null, 0, 65535))->
      setRequired(true);

    $e['export_host'] = $this->_output->form->createElement('text', 'export_host', array('size'=>30));
    $e['export_host']->
      addValidator('Hostname', false, array(Zend_Validate_Hostname::ALLOW_DNS | Zend_Validate_Hostname::ALLOW_IP))->
      addValidator('StringLength', false, array(1, 255))->
      setRequired(true);
    $e['export_base'] = ($this->_action == 'add') ?
      $this->_output->form->createElement('text', 'export_base') :
      $this->_output->form->createElement('text', 'export_base', array('readonly'=>'readonly'));
    $e['export_base']->
      addValidator('StringLength', false, array(1, 255));

    $e['disks'] = ($this->_action == 'add') ?
    $this->_output->form->createElement('text', 'disks', array('size'=>10)) : 
    $this->_output->form->createElement('text', 'disks', array('size'=>10, 'readonly'=>'readonly'));
    $e['disks']->
      addValidator('Int', false)->
      addValidator('Int', false, array(null, 1, 8))->
      setRequired(true);

    $e['admin_port'] = $this->_output->form->createElement('text', 'admin_port', array('size'=>10));
    $e['admin_port']->
      addValidator('Int', false)->
      addValidator('Int', false, array(null, 0, 65535))->
      setRequired(true);
    $e['admin_user'] = $this->_output->form->createElement('text', 'admin_user');
    $e['admin_user']->
      addValidator('StringLength', false, array(0, 255));
    $e['admin_password'] = $this->_output->form->createElement('text', 'admin_password');
    $e['admin_password']->
      addValidator('StringLength', false, array(0, 255));

    $e['transcoder_port'] = $this->_output->form->createElement('text', 'transcoder_port', array('size'=>10));
    $e['transcoder_port']->
      addValidator('Int', false)->
      addValidator('Int', false, array(null, 0, 65535))->
      setRequired(true);
    $e['transcoder_user'] = $this->_output->form->createElement('text', 'transcoder_user');
    $e['transcoder_user']->
      addValidator('StringLength', false, array(0, 255));
    $e['transcoder_password'] = $this->_output->form->createElement('text', 'transcoder_password');
    $e['transcoder_password']->
      addValidator('StringLength', false, array(0, 255));

    $e['runner_port'] = $this->_output->form->createElement('text', 'runner_port', array('size'=>10));
    $e['runner_port']->
      addValidator('Int', false)->
      addValidator('Int', false, array(null, 0, 65535))->
      setRequired(true);
    $e['runner_user'] = $this->_output->form->createElement('text', 'runner_user');
    $e['runner_user']->
      addValidator('StringLength', false, array(0, 255));
    $e['runner_password'] = $this->_output->form->createElement('text', 'runner_password');
    $e['runner_password']->
      addValidator('StringLength', false, array(0, 255));

    foreach ($e as $a) {
      $this->_output->form->addElement($a);
    }
    $this->_output->form->addElement('submit', 'submit');

    $action = $this->_action;
    switch ($action) {
      case 'add':
      case 'set':
        $action = 'edit';
    }

    $this->_output->form->
      setAttrib('name', $this->_module.'-'.$this->_resource.'-'.$action.'-form')->
      addDisplayGroup(array('host','http_port','rtmp_port'), 'machine')->
      addDisplayGroup(array('export_host','export_base'), 'export')->
      addDisplayGroup(array('disks'), 'setup')->
      addDisplayGroup(array('admin_port', 'admin_user','admin_password'), 'credentials')->
      addDisplayGroup(array('transcoder_port', 'transcoder_user','transcoder_password'), 'transcoder')->
      addDisplayGroup(array('runner_port', 'runner_user','runner_password'), 'runner')->
      setAction(Zend_Controller_Action_HelperBroker::getStaticHelper('url')->direct($this->_request->getActionName(), $this->_request->getControllerName(), $this->_request->getModuleName()));
  }

  protected function _checkAccess($id, $action) {
    if (!is_array($action)) {
      $action = array($action);
    }
    foreach ($action as $k=>$a) {
      if ($a == 'index') {
        unset($action[$k]);
      }
    }

    $acl = $this->getUser()->getAcl(0, $this->_resource, $id);
    foreach ($action as $a) {
      if (!$acl[0][$this->_resource][$a]) {
        throw new Zend_Controller_Action_Exception('Not enough privileges', 403);
      }
    }
  }

  protected function _createSelect($action = null, $params = array()) {
    $servers_id_ = $this->_tableAdapter->quoteIdentifier('servers.id');

    return
    $this->_tableObject->select()->
      from('servers')->
      setIntegrityCheck(false)->
      joinInner(
        'acl',
        '('.$servers_id_.' = '.$this->_tableAdapter->quoteIdentifier('acl.resource_id').
        ') OR ('.$this->_tableAdapter->quoteIdentifier('acl.resource_id').' = 0)',
        array()
      )->
      where(
        'FIND_IN_SET(?, '.$this->_tableAdapter->quoteIdentifier('acl.action').')',
        ($action == null) ? $this->_action : $action
      )->
      where(
        $this->_tableAdapter->quoteIdentifier('acl.resource').' = ?',
        'servers'
      )->
      where(
        $this->_tableAdapter->quoteIdentifier('acl.user_id').' = ?',
        (int)$this->getUser()->id
      );
  }

  public function consoleAction() {
  }

  public function pingAction() {
    die('{server: '.time().'}');
  }
}

class Servers extends App_Model_Action_Proxy {
  /**
   * Returns a list of the defined servers.
   *
   * @param string $token The session token.
   * @return Result
  */
  public function index($token) {
    $this->_login($token);
    return $this->_process('index', array());
  }
  /**
   * Returns a server.
   *
   * @param string $token The session token.
   * @param int $id The stream id.
   * @return Result
  */
  public function get($token, $id) {
    $this->_login($token);
    return $this->_process('get', array('id'=>$id));
  }
  /**
   * Deletes a server.
   *
   * @param string $token The session token.
   * @param int $id The stream id.
   * @return Result
  */
  public function delete($token, $id) {
    $this->_login($token);
    return $this->_process('delete', array('id'=>$id));
  }
}
?>
