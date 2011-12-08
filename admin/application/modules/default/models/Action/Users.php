<?php
class Model_Action_Users extends App_Model_Action_Record_Db {
  protected $_module = 'default';
  protected $_table = 'users';
  protected $_order =
  array(
    'user'=>true
  );

  public function postDispatch() {
    $acl = array();

    switch ($this->_action) {
      case 'index':
        if ($this->_output->model['records']) {
          $server_id = $this->getServer()->id;

          foreach($this->_output->model['records'] as $record) {
            $racl = $this->getUser()->getAcl($server_id, 'users', $record['id'], null);
            if ($racl) {
              $acl[$record['id']] = $racl[$server_id]['users'];
            }
          }
        }
        break;
      case 'get':
      case 'set':
        if ($this->_output->model['record']) {
          $server_id = $this->getServer()->id;

          $racl = $this->getUser()->getAcl($server_id, 'users', $this->_output->model['record']['id'], null);
          if ($racl) {
            $acl[$this->_output->model['record']['id']] = $racl[$server_id]['users'];
          }
        }
        break;
      case 'add':
        break;
    }
    $this->_output->model['acl'] = $acl;

    if ($this->_output->result > 0) {
      if ($this->getPhase() == App_Model_Action::PHASE_PERFORM) {
        switch ($this->_action) {
          case 'set':
              $this->_tableAdapter->delete('acl', $this->_tableAdapter->quoteInto('user_id = ?', $this->_output->model['record']->id));
              // fall through
          case 'add':
            $this->_tableAdapter->insert('acl', array(
              'server_id' => 0,
              'user_id' => $this->_output->model['record']->id,
              'resource' => 'servers',
              'resource_id' => (int)$this->getServer()->id,
              'action' => 'get'
            ));
            if (isset($this->_input['resources'])) {
              foreach ($this->_input['resources'] as $resource=>$actions) {
                $this->_tableAdapter->insert('acl', array(
                  'server_id' => (int)$this->getServer()->id,
                  'user_id' => $this->_output->model['record']->id,
                  'resource' => $resource,
                  'resource_id' => 0,
                  'action' => implode(',',array_keys($actions))
                ));
              }
            }
        }
      } else {
          switch ($this->_action) {
            case 'set':
              $server_id = $this->getServer()->id;

              $acls = $this->_output->model['record']->getAcl($server_id, array_keys($this->_output->resources), 0);
              if ($acls && $acls[$server_id]) {
                $this->_output->acls = $acls[$server_id];
              } else {
                $this->_output->acls = array();
              }
              break;
          }
      }
    }
    return parent::postDispatch();
  }

  protected function _createForm() {
    $this->_output->form = new App_Form();
    $this->_output->form->
      setMethod('post');

    $e = array();

    switch ($this->_action) {
      case 'login':
        $e['user'] = $this->_output->form->createElement('text', 'user');
        $e['user']->
          addValidator('StringLength', false, array(1, 255))->
          setRequired(true);
        $e['password'] = $this->_output->form->createElement('password', 'password');
        $e['password']->
          addValidator('StringLength', false, array(1, 255))->
          setRequired(true);
        break;
      case 'logout':
        break;
      default:
        if ($this->_action != 'add') {
          $e[$this->_key] = $this->_output->form->createElement('hidden', $this->_key);
        }

        $e['user'] = $this->_output->form->createElement('text', 'user');
        $e['user']->
          addValidator('StringLength', false, array(1, 255))->
          setRequired(true);
        $e['update_password'] = $this->_output->form->createElement('checkbox', 'update_password', ($this->_action == 'add') ? array('disabled'=>'disabled', 'checked'=>true) : array());
        $e['update_password']->
          setRequired(true);
        $e['password'] = $this->_output->form->createElement('password', 'password', array('size'=>30));
        $e['password']->
          addValidator('StringLength', false, array(1, 255));
        $e['password_confirmation'] = $this->_output->form->createElement('password', 'password_confirmation', array('size'=>30));
        $e['password_confirmation']->
          addValidator('StringLength', false, array(1, 255));
    }

    foreach ($e as $a) {
      $this->_output->form->addElement($a);
    }
    $this->_output->form->addElement('submit', 'submit');

    switch ($this->_action) {
      case 'login':
        $this->_output->form->
          setAttrib('name', $this->_module.'-'.$this->_resource.'-login-form')->
          setAction(Zend_Controller_Action_HelperBroker::getStaticHelper('url')->direct($this->_request->getActionName(), $this->_request->getControllerName(), $this->_request->getModuleName()));
        break;
      case 'logout':
        break;
      default:
        $this->_output->form->addDisplayGroup(array('update_password'), 'update_passwords', array('placement'=>'PREPEND'));
        $this->_output->form->addDisplayGroup(array('password', 'password_confirmation'), 'passwords', array('placement'=>'PREPEND'));
        $this->_output->form->
          setAttrib('name', $this->_module.'-'.$this->_resource.'-edit-form')->
          setAction(Zend_Controller_Action_HelperBroker::getStaticHelper('url')->direct($this->_request->getActionName(), $this->_request->getControllerName(), $this->_request->getModuleName()).'?sid='.$this->getServer()->id);

        $this->_output->resources = Zend_Registry::get('Whispercast')->resources;
    }

  }
  protected function _processForm() {
    switch ($this->_action) {
      case 'login':
      case 'logout':
        break;
      default:
        if ($this->_input['update_password']) {
          $this->_output->form->getElement('password')->
            setRequired(true);
          $this->_output->form->getElement('password_confirmation')->
            setRequired(true)->
            addValidator('Identical', false, array(Zend_Controller_Front::getInstance()->getRequest()->getParam('password')));
        } else {
          $this->_output->form->removeElement('password');
          $this->_output->form->removeElement('password_confirmation');
        }
    }

    parent::_processForm();
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

    $server = $this->getServer();

    $acl = $this->getUser()->getAcl($server->id, $this->_resource, $id);
    foreach ($action as $a) {
      if (!$acl[$server->id][$this->_resource][$a]) {
        throw new Zend_Controller_Action_Exception('Not enough privileges', 403);
      }
    }
  }

  protected function _createSelect($action = null, $params = array()) {
    $select =
    $this->_tableObject->select()->
      from($this->_table, array('id', 'user', 'password'))->
      setIntegrityCheck(false)->
      where(
        $this->_tableAdapter->quoteIdentifier('users.id').' <> ?',
        1
      );

    if ($this->_action != 'add') {
      $select->
        joinInner(
          'acl AS acl',
          $this->_tableAdapter->quoteIdentifier($this->_table.'.id').' = '.$this->_tableAdapter->quoteIdentifier('acl.user_id'),
          array()
        )->
        where(
          $this->_tableAdapter->quoteIdentifier('acl.server_id').' = ?',
          0
        )->
        where(
          $this->_tableAdapter->quoteIdentifier('acl.resource').' = ?',
          'servers'
        )->
        where(
          $this->_tableAdapter->quoteIdentifier('acl.resource_id').' = ?',
          $this->getServer()->id
        )->
        where(
          'FIND_IN_SET(?, '.$this->_tableAdapter->quoteIdentifier('acl.action').')',
          'get'
        );
    }
    return $select;
  }

  protected function _parseData($record) {
    $result = parent::_parseData($record);

    switch ($this->_action) {
      case 'login':
      case 'logout':
        break;
      default:
        if ($this->_action == 'add') {
          $result['update_password'] = true;
        } else {
          $result['update_password'] = (bool)$result['update_password'];
        }
        if (!$result['update_password']) {
          unset($result['password']);
          unset($result['password_confirmation']);
        } else {
          if (isset($result['password'])) {
            $result['password'] = md5($result['password']);
          }
        }
    }

    return $result;
  }

  public function displayAction() {
  }

  public function loginAction() {
    $this->_createForm();

    if ($this->getPhase() == App_Model_Action::PHASE_PERFORM) {
      if ($this->_output->form->isValid($this->_input)) {
        $adapter = new Zend_Auth_Adapter_DbTable(Zend_Db_Table::getDefaultAdapter(), 'users', 'user', 'password', 'MD5(?)');
        $adapter->
        setIdentity($this->_output->form->getValue('user'))->
        setCredential($this->_output->form->getValue('password'));

        $result = Zend_Auth::getInstance()->authenticate($adapter);
        if ($result->isValid()) {
          $user = new Model_User(array('table'=>new Model_DbTable_Users(), 'data'=>(array)$adapter->getResultRowObject()));
          Zend_Auth::getInstance()->getStorage()->write($user);

          if ($this->getContext() === null) {
            Zend_Controller_Action_HelperBroker::getStaticHelper('redirector')->setCode(302)->gotoRouteAndExit(array('module'=>'default','controller'=>'index', 'action'=>'index'), null, true);
          }

          $this->_output->result = 1;
          $this->_output->model['token'] = Zend_Session::getId();
          return;
        }

        $this->_output->result = 0;
        $this->_output->errors = $result->getMessages();
        return;
      }

      $this->_output->result = 0;
    } else {
      if ($this->getContext() === null) {
        if ($this->getUser(false)) {
          Zend_Controller_Action_HelperBroker::getStaticHelper('redirector')->setCode(302)->gotoRouteAndExit(array('module'=>'default','controller'=>'index', 'action'=>'index'), null, true);
        }
      }
    }
  }
  public function logoutAction() {
    Zend_Auth::getInstance()->clearIdentity();
    $this->_output->result = 1;

    if ($this->getContext() === null) {
      Zend_Controller_Action_HelperBroker::getStaticHelper('redirector')->setCode(302)->setCloseSessionOnExit(true)->gotoRouteAndExit(array('module'=>'default','controller'=>'users', 'action'=>'login'), null, true);
    }
  }
}

class Users extends App_Model_Action_Proxy {
  /**
   * Initiates a user session.
   *
   * @param string $user The user's name.
   * @param string $password The user's password.
   * @return Result
  */
  public function login($user, $password) {
    return $this->_process('login', array('user'=>$user, 'password'=>$password));
  }
  /**
   * Terminate a user session.
   *
   * @param string $token The session token to terminate.
   * @return Result
  */
  public function logout($token) {
    $this->_login($token);
    return $this->_process('logout', array());
  }
}
?>
