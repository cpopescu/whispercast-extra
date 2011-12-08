<?php
abstract
class App_Model_Action_Record_Db extends App_Model_Action_Record {
  protected function _init() {
    if (!isset($this->_resource)) {
      $this->_resource = $this->_table;
    }

    if (!isset($this->_tableClass)) { 
      $this->_tableClass = (($this->_module == 'default') ? '' : ucfirst($this->_module).'_').'Model_DbTable_'.ucfirst($this->_table);
    }
    $this->_table = ($this->_module == 'default') ? $this->_table : $this->_module.'_'.$this->_table;

    $this->_tableObject = new $this->_tableClass();
    $this->_tableAdapter = $this->_tableObject->getAdapter();

    parent::_init();

    $this->_quotedTable = $this->_tableAdapter->quoteIdentifier($this->_table);
    $this->_quotedKey = $this->_tableAdapter->quoteIdentifier($this->_table.'.'.$this->_key);
  }

  public function preDispatch($input = null) {
    parent::preDispatch($input);

    switch ($this->_action) {
      case 'add':
      case 'set':
      case 'delete':
        $this->_tableAdapter->beginTransaction();
        break;  
    }  
  } 

  public function postDispatch() {
    switch ($this->_action) {
      case 'index':
        $records = array();
        if ($this->_context == 'soap') {
          if ($this->_output->model['records']) {
            foreach ($this->_output->model['records'] as $record) {
              $data = $record->toArray();
              $record->_postFetch($data);
              $records[] = $data;
            }
          }
        } else {
          if ($this->_output->model['records']) {
            foreach ($this->_output->model['records'] as $record) {
              $records[] = $record->toArray();
            }
          }
        }
        $this->_output->model['records'] = $records;
        break;
      case 'add':
      case 'get':
      case 'set':
      case 'delete':
        if ($this->_output->model['record']) {
          $data = $this->_output->model['record']->toArray();
          switch ($this->_action) {
            case 'add':
              if ($this->_phase == App_Model_Action::PHASE_PREPARE) {
                $this->_output->model['record']->_postCreate($data, $this->_resource);
              }
              break;
            case 'get':
            case 'set':
              $this->_output->model['record']->_postFetch($data);
              break;
          }
          $this->_output->model['record'] = $data;
        } else {
          $this->_output->model['record'] = array();
        }
        break;
    }

    switch ($this->_action) {
      case 'add':
      case 'set':
      case 'delete':
        if ($this->_output->result > 0) {
          $this->_tableAdapter->commit();
        } else {
          $this->_tableAdapter->rollBack();
        }
        break;  
    }

    return parent::postDispatch();
  }

  protected function _createSelect($action = null, $params = array()) {
    $servers_id_ = $this->_tableAdapter->quoteIdentifier('servers.id');

    return
    $this->_tableObject->select()->
      from($this->_table)->
      setIntegrityCheck(false)->
      joinInner(
        'servers',
        $servers_id_.' = '.$this->_tableAdapter->quoteIdentifier($this->_table.'.server_id'),
        array()
      )->
      joinInner(
        'acl',
        $servers_id_.' = '.$this->_tableAdapter->quoteIdentifier('acl.server_id'),
        array()
      )->
      where(
        $servers_id_.' = ?',
        (int)$this->getServer()->id
      )->
      where(
        'FIND_IN_SET(?, '.$this->_tableAdapter->quoteIdentifier('acl.action').')',
        ($action == null) ? $this->_action : $action
      )->
      where(
        $this->_tableAdapter->quoteIdentifier('acl.user_id').' = ?',
        (int)$this->getUser()->id
      );
  }

  protected function _adjustOrderBy($orderBy) {
    return $orderBy;
  }

  protected function _createRecord() {
    return $this->_tableObject->createRow(array(), Zend_Db_Table::DEFAULT_DB);
  }
  protected function _fetchRecord($id) {
    return $this->_tableObject->fetchRow($this->_createSelect(null, $this->_input)->where($this->_quotedKey.' = ?', $id));
  }

  protected function _index() {
    $order = $this->_order;
    $orderBy = $this->_input['sortBy'];
    if ($orderBy) {
      $orderDir = $this->_input['sortOrder'];
      $orderDir = ($orderDir === null) ? true : ($orderDir ? true : false);

      unset($order[$orderBy]);
      $order = array($orderBy=>$orderDir)+$order;
    }

    if ($order) {
      foreach ($order as $key=>&$value) {
        $value = $this->_adjustOrderBy($key).' '.($value ? 'ASC' : 'DESC');
      }
    } else {
      $order = array();
    }
    $perPage = $this->_input['pagePerPage'];
    if ($perPage > 0) {
      $paginator = Zend_Paginator::factory($this->_createSelect('get', $this->_input)->order($order));
      $paginator->setItemCountPerPage((int)$perPage);

      $page = $this->_input['pageCurrent'];
      if ($page !== null) {
        $paginator->setCurrentPageNumber((int)$page);
      }

      $this->_output->model['records'] = $paginator->getCurrentItems();

      $this->_output->model['itemsTotal'] = $paginator->getTotalItemCount();
      $this->_output->model['itemsCurrent'] = $paginator->getCurrentItemCount();
      $this->_output->model['pagesTotal'] = $paginator->count();
      $this->_output->model['pagesCurrent'] = $paginator->getCurrentPageNumber();
      $this->_output->model['itemsPerPage'] = $paginator->getItemCountPerPage();
    } else {
      $this->_output->model['records'] = $this->_tableObject->fetchAll($this->_createSelect('get', $this->_input)->order($order));
    }

    $this->_output->result = 1;
  }

  protected function _add() {
    $this->_output->model['record'] = $this->_createRecord();
    if ($this->_phase == App_Model_Action::PHASE_PERFORM) {
      $this->_processForm();
      if ($this->_output->result > 0) {
        try {
          $values = $this->_output->form->getValues();
          unset($values[$this->_key]);
          $values['server_id'] = $this->getServer()->id;
          $values['user_id'] = $this->getUser()->id;

          $this->_output->model['record']->setFromArray(array_merge($this->_input, $values));

          $id = $this->_output->model['record']->save();
          if (isset($id)) {
            $this->_output->model['record'] = $this->_tableObject->fetchRow($this->_createSelect(null, $this->_input)->where($this->_quotedKey.' = ?', $id));
            $this->_output->result = 1;
          } else {
            $this->_output->result = 0;
          }
        } catch (Exception $e) {
          $this->_output->errors[] = $e->getMessage();
          $this->_output->result = -1;
        }
      }
    }
  }
  protected function _get($id) {
    $this->_output->model['record'] = $this->_fetchRecord($id);
    if ($this->_output->model['record'] === null) {
      throw new Zend_Controller_Action_Exception('Record not found', 400);
    }
    $this->_output->result = 1;
  }
  protected function _set($id) {
    $this->_get($id);
    if ($this->_output->result > 0) {
      if ($this->_phase == App_Model_Action::PHASE_PERFORM) {
        $this->_processForm();
        if ($this->_output->result > 0) {
          try {
            $values = $this->_output->form->getValues();
            $values['server_id'] = $this->getServer()->id;
            $values['user_id'] = $this->getUser()->id;

            $this->_output->model['record']->setFromArray(array_merge($this->_input, $values));

            $id = $this->_output->model['record']->save();
            if (isset($id)) {
              $this->_output->model['record'] = $this->_tableObject->fetchRow($this->_createSelect(null, $this->_input)->where($this->_quotedKey.' = ?', $id));
            }

            $this->_output->result = 1;
          } catch (Exception $e) {
            $this->_output->errors[] = $e->getMessage();
            $this->_output->result = -1;
          }
        }
      }
    }
  }
  protected function _delete($id) {
    $this->_get($id);
    if ($this->_output->result > 0) {
      if ($this->_phase == App_Model_Action::PHASE_PERFORM) {
        try {
          $deleted = $this->_output->model['record']->delete();
          if ($deleted > 0) {
            $this->_output->result = 1;
          } else {
            $this->_output->result = 0;
          }
        } catch (Exception $e) {
          $this->_output->errors[] = $e->getMessage();
          $this->_output->result = -1;
        }
      }
    }
  }
}
?>
