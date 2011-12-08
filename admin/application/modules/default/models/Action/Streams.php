<?php
class Model_Action_Streams extends App_Model_Action_Record_Db {
  protected $_module = 'default';

  protected $_table = 'streams';
  protected $_order =
  array(
    'type'=>true,
    'name'=>true
  );

  public function preDispatch($input = null) {
    parent::preDispatch($input);
    $this->getServer();
  }

  public function postDispatch() {
    $acl = array();

    switch ($this->_action) {
      case 'index':
        if ($this->_output->model['records']) {
          $server_id = $this->getServer()->id;

          foreach($this->_output->model['records'] as $record) {
            $racl = $this->getUser()->getAcl($server_id, $record['type'], $record['id'], null);
            if ($racl) {
              $acl[$record['id']] = $racl[$server_id][$record['type']];
            }
          }
        }
        break;
      case 'get':
      case 'set':
        if ($this->_output->model['record']) {
          $server_id = $this->getServer()->id;

          $racl = $this->getUser()->getAcl($server_id, $this->_output->model['record']['type'], $this->_output->model['record']['id'], null);
          if ($racl) {
            $acl[$this->_output->model['record']['id']] = $racl[$server_id][$this->_output->model['record']['type']];
          }
        }
        break;
      case 'add':
        break;
    }
    $this->_output->model['acl'] = $acl;

    return parent::postDispatch();
  }

  protected function _createForm() {
    $categories[0] = '-';
    $categories += $this->_getCategories();

    $this->_output->form = new App_Form();
    $this->_output->form->
      setMethod('post');

    $e = array();

    if ($this->_action != 'add') {
      $e[$this->_key] = $this->_output->form->createElement('hidden', $this->_key);
    }

    $e['key'] = $this->_output->form->createElement('hidden', 'key');

    $e['name'] = $this->_output->form->createElement('text', 'name');
    $e['name']->
      setAttrib('size', 95)->
      addValidator('StringLength', false, array(1, 255))->
      setRequired(true);

    $e['description'] = $this->_output->form->createElement('textarea', 'description');
    $e['description']->
      setAttrib('rows', 3)->
      setAttrib('cols', 95);

    $e['tags'] = $this->_output->form->createElement('textarea', 'tags');
    $e['tags']->
      setAttrib('rows', 3)->
      setAttrib('cols', 95);

    $e['category_id'] = $this->_output->form->createElement('select', 'category_id');
    $e['category_id']->
      setMultiOptions($categories)->
      addValidator('StringLength', false, array(1, 32))->
      setRequired(false);

    $e['public'] = $this->_output->form->createElement('checkbox', 'public');
    $e['public']->
        setRequired(true);
    $e['usable'] = $this->_output->form->createElement('checkbox', 'usable');
    $e['usable']->
        setRequired(true);
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
      setAction(Zend_Controller_Action_HelperBroker::getStaticHelper('url')->direct($this->_request->getActionName(), $this->_request->getControllerName(), $this->_request->getModuleName()).'?sid='.$this->getServer()->id);
  }

  protected function _parseData($record) {
    $result = parent::_parseData($record);

    $result['type'] = $this->_resource;
    $result['category_id'] = ($result['category_id'] == 0) ? null : $result['category_id'];
    
    if ($result['tags']) {
      $tags = preg_split('/[^\p{L}\p{N}]/u', $result['tags'], -1, PREG_SPLIT_NO_EMPTY);
      if ($tags && count($tags) > 0) {
        $result['tags'] = implode(',', $tags);
      } else {
        $result['tags'] = null;
      }
    } else {
      $result['tags'] = null;
    }
    
    return $result;
  }

  protected function _checkAccess($id, $action) {
    if ($this->_resource != 'streams') {
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
  }

  protected function _createSelect($action = null, $params = array()) {
    $action = ($action == null) ? $this->_action : $action;

    $streams_id_ = $this->_tableAdapter->quoteIdentifier('streams.id');
    $acl_resource_id_ = $this->_tableAdapter->quoteIdentifier('acl.resource_id');
    
    $select =
    $this->_tableObject->select()->
      setIntegrityCheck(false)->
      from('streams', array(Zend_Db_Select::SQL_WILDCARD, new Zend_Db_Expr('1')));

    if ($this->_resource != 'streams') {
      $streams = Zend_Registry::get('Whispercast')->streams;
      if (isset($streams[$this->_resource])) {
        $stream = $this->_resource;
        $fields = $streams[$this->_resource];
        $select->
          joinLeft(
            $stream,
            $this->_tableAdapter->quoteIdentifier($stream.'.stream_id').' = '.$streams_id_.' AND '.
            $this->_tableAdapter->quoteIdentifier($stream.'.id').' IS NOT NULL',
            $fields
          );
      }
      $hasState = ($this->_resource == 'files');
    } else {
      foreach(Zend_Registry::get('Whispercast')->streams as $stream => $fields) {
        $select->
          joinLeft(
            $stream,
            $this->_tableAdapter->quoteIdentifier($stream.'.stream_id').' = '.$streams_id_.' AND '.
            $this->_tableAdapter->quoteIdentifier($stream.'.id').' IS NOT NULL',
            $fields
          );
      }
      $hasState = true;
    }

    $select->
      joinLeft(
        'streamsCategories',
        $this->_tableAdapter->quoteIdentifier('streams.category_id').' = '.$this->_tableAdapter->quoteIdentifier('streamsCategories.id'),
        'streamsCategories.name AS category'
      )->
      joinInner(
        'acl',
        $this->_tableAdapter->quoteIdentifier('streams.type').' = '.$this->_tableAdapter->quoteIdentifier('acl.resource'),
        array()
      )->
      joinInner(
        'servers',
        $this->_tableAdapter->quoteIdentifier('streams.server_id').' = '.$this->_tableAdapter->quoteIdentifier('servers.id'),
        array()
      )->
      where(
        $this->_tableAdapter->quoteIdentifier('acl.server_id').' = ? AND ('.$acl_resource_id_.' = '.$streams_id_.' OR '.$acl_resource_id_.' = 0)',
        (int)$this->getServer()->id
      )->
      where(
        $this->_tableAdapter->quoteIdentifier('acl.user_id').' = ?',
        (int)$this->getUser()->id
      )->
      where(
        $this->_tableAdapter->quoteIdentifier('streams.server_id').' = ?',
        (int)$this->getServer()->id
      )->
      where(
        'FIND_IN_SET(?, '.$this->_tableAdapter->quoteIdentifier('acl.action').')',
        $action
      );

    $state_ = 'CONCAT('.$this->_tableAdapter->quoteIdentifier('type').',\'.\','.$this->_tableAdapter->quoteIdentifier('state').')';
    $type_ = $this->_tableAdapter->quoteIdentifier('type');

    $filters = $params['f'] ? $params['f'] : array();

    if ($hasState) {
      // filter by state/negated state
      if (isset($filters['state'])) {
        $where = '';

        $state = $filters['state'];
        if (!is_array($state)) {
          $state = array($state);
        }
        foreach($state as $k=>$v) {
          if ($v == '*') {
            $where .= ($where ? ' OR ':'') . $state_ . ' IS NOT NULL';
            unset($state[$k]);
          } else
          if ($v == '') {
            $where .= ($where ? ' OR ':'') . $state_ . ' IS NULL';
            unset($state[$k]);
          }
        }
        if ($where || $state) {
          if ($state) {
            $where .= ($where ? ' OR ':'') . $state_.' IN (?)';
            $select->where(
              $where, $state
            );
          } else {
            $select->where(
              $where
            );
          }
        }
      }
      if (isset($filters['nstate'])) {
        $where = '';

        $nstate = $filters['nstate'];
        if (!is_array($nstate)) {
          $nstate = array($nstate);
        }
        foreach($nstate as $k=>$v) {
          if ($v == '*') {
            $where .= ($where ? ' OR ':'') . $state_ . ' IS NULL';
            unset($nstate[$k]);
          } else
          if ($v == '') {
            $where .= ($where ? ' OR ':'') . $state_ . ' IS NOT NULL';
            unset($nstate[$k]);
          }
        }
        if ($where || $nstate) {
          if ($nstate) {
            $where .= ($where ? ' OR ':'') . $state_.' NOT IN (?)';
            $select->where(
              $where, $nstate
            );
          } else {
            $select->where(
              $where
            );
          }
        }
      }
      if (!isset($filters['state']) && !isset($filters['nstate'])) {
        $select->where(
          $state_ .' IS NULL'
        );
      }
    }

    // filter by type/negated type
    if ($this->_resource == 'streams') {
      if (isset($filters['type'])) {
        $type = $filters['type'];
        if (!is_array($type)) {
          $type = array($type);
        }
      }
      if (isset($filters['ntype'])) {
        $ntype = $filters['ntype'];
        if (!is_array($ntype)) {
          $ntype = array($ntype);
        }
      }
    } else {
      $type = array($this->_resource);
    }
    if ($ntype) {
      $select->where(
        $type_.' NOT IN (?)', $ntype
      );
    }
    if ($type) {
      $select->where(
        $type_.' IN (?)', $type
      );
    }

    // filter by the saved flag
    if (isset($filters['saved'])) {
      $saved = $filters['saved'] ? 1 : 0;
      $select->where(
        $this->_tableAdapter->quoteIdentifier('saved').' = ?', $saved
      );
    }
    // filter by the usable flag
    if (isset($filters['usable'])) {
      $usable = $filters['usable'] ? 1 : 0;
      $select->where(
        $this->_tableAdapter->quoteIdentifier('usable').' = ?', $usable
      );
    }
    // filter by the public flag
    if (isset($filters['public'])) {
      $public = $filters['public'] ? 1 : 0;
      $select->where(
        $this->_tableAdapter->quoteIdentifier('public').' = ?', $public
      );
    }

    // filter by keywords
    if (isset($filters['query'])) {
      $query = $filters['query'];
      if (!is_array($query)) {
        $query = array($query);
      }
      foreach ($query as $k=>$v) {
        $query[$k] = $v.'*';
      }
    }
    if ($query && (count($query) > 0)) {
      $select->joinInner(
        'streamsMeta',
        $this->_tableAdapter->quoteIdentifier('streams.id').' = '.$this->_tableAdapter->quoteIdentifier('streamsMeta.id'),
        array()
      )->where(
        new Zend_Db_Expr('MATCH(`streamsMeta`.`name`,`streamsMeta`.`description`,`streamsMeta`.`tags`) AGAINST(? IN BOOLEAN MODE)'),
        $query
      );
    }
    return $select;
  }

  protected function _adjustOrderBy($orderBy) {
    switch ($orderBy) {
      case 'name':
      case 'type':
      case 'duration':
      case 'public':
      case 'usable':
        return 'streams.'.$orderBy;
    }
    return $orderBy;
  }

  protected function _getCategories() {
    if (!isset($this->_output->model['categories'])) {
      $this->_output->model['categories'] = 
      $this->_tableAdapter->fetchPairs(
        $this->_tableAdapter->select()->
          from('streamsCategories', array('id', 'name'))->
          where(
            $this->_tableAdapter->quoteIdentifier('server_id').' = '. $this->_tableAdapter->quote($this->getServer()->id)
          )
      );
    }
    return $this->_output->model['categories'];
  }

  public function displayAction() {
  }

  public function updateAction() {
    $id = $this->_input[$this->_key];
    if ($id === null) {
      throw new Zend_Controller_Action_Exception('Invalid record id', 400);
    }
    if (!is_array($id)) {
      $id = array($id);
    }

    $this->_output->results = array();
    $this->_output->model['records'] = array();

    $this->_output->result = 1;

    $server = $this->getServer();

    foreach ($id as $rid) {
      $lock_name = 'whispercast.default.streams.'.$rid;
      if ($this->_tableAdapter->fetchOne('SELECT GET_LOCK(?, 5)', $lock_name) == 1) {
        try {
          $select = $this->_createSelect('set', array_merge($this->_input, array('f'=>array('nstate'=>array()))));

          $record = $this->_tableObject->fetchRow($select->where($this->_quotedKey.' = ?', $rid));
          if ($record) {
            try {
              $result = $record->updateState($server);
              if ($result['reload']) {
                $record = $this->_tableObject->fetchRow($select);
                unset($result['reload']);
              }

              $this->_output->model['results'][$rid] = $result;
              $this->_output->model['records'][$rid] = $record->toArray();
            } catch (Exception $e) {
              $this->_output->model['results'][$rid] = array('result'=>false);
            }
          }
        } catch(Exception $e) {
          $this->_output->model['results'][$rid] = array('result'=>false);
        }
        $this->_tableAdapter->fetchOne('SELECT RELEASE_LOCK(?)', $lock_name);
      } else {
        $this->_output->model['results'][$rid] = array('result'=>false);
      }
    }
  }
}

class Time {
  /**
   * The hour.
   * @var int $hour
   */
  public $hour;
  /**
   * The minute.
   * @var int $minute
   */
  public $minute;
  /**
   * The second.
   * @var int $second
   */
  public $second;
}
class Date {
  /**
   * The year.
   * @var int $year
   */
  public $year;
  /**
   * The month.
   * @var int $month
   */
  public $month;
  /**
   * The day.
   * @var int $day
   */
  public $day;
}
class DateAndTime {
  /**
   * The date part.
   * @var Date $date
   */
  public $date;
  /**
   * The time part.
   * @var Time $time
   */
  public $time;
}

class Streams extends App_Model_Action_Proxy {
  protected function _process($method, $input) {
    $result = parent::_process($method, $input);
    if ($this->_setupClass) {
      switch ($method) {
        case 'index':
          if (isset($result->model['records'])) {
            foreach ($result->model['records'] as $k=>&$v) {
              if (isset($v['setup'])) {
                $v['setup'] = call_user_func(array($this->_setupClass, 'fromArray'), $v['setup']);
              }
            }
          }
          break;
        default:
          if (isset($result->model['record']['setup'])) {
            $result->model['record']['setup'] = call_user_func(array($this->_setupClass, 'fromArray'), $result->model['record']['setup']);
          }
      }
    }
    return $result;
  }

  /**
   * Returns a list of streams, based on the given filtering criteria.
   *
   * @param string $token The session token.
   * @param int $server_id The server id.
   * @param array $filters The filtering criteria.
   * @param string $sortBy The name of the field used in sorting the results.
   * @param boolean $sortOrder The sort ordering (true is ascending, false is descending).
   * @param int $pagePerPage The number of items per page.
   * @param int $pageCurrent The current page.
   * @return Result
  */
  public function index($token, $server_id, $filters, $sortBy, $sortOrder, $pagePerPage, $pageCurrent) {
    $this->_login($token);
    return $this->_process('index', array('sid'=>$server_id,'f'=>$filters,'sortBy'=>$sortBy,'sortOrder'=>$sortOrder,'pagePerPage'=>$pagePerPage,'pageCurrent'=>$pageCurrent));
  }
  /**
   * Returns a stream.
   *
   * @param string $token The session token.
   * @param int $server_id The server id.
   * @param int $id The stream id.
   * @return Result
  */
  public function get($token, $server_id, $id) {
    $this->_login($token);
    return $this->_process('get', array('sid'=>$server_id,'id'=>$id));
  }
  /**
   * Deletes a stream.
   *
   * @param string $token The session token.
   * @param int $server_id The server id.
   * @param int $id The stream id.
   * @return Result
  */
  public function delete($token, $server_id, $id) {
    $this->_login($token);
    return $this->_process('delete', array('sid'=>$server_id,'id'=>$id));
  }
}
?>
