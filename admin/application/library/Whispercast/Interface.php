<?php
require_once 'rpc_types.php';
require_once 'rpc_json.php';
require_once 'rpc_connection.php';

require_once 'rpc_log.php';
InitializeLog(-1);

class Whispercast_Interface {
  /**
   * The currently instantiated interfaces to the underlying RPC stuff.
   *
   * @var array
   */
  private $_services = array();

  /**
   * The server host.
   *
   * @var string
   */
  private $_host;
  /**
   * The server port.
   *
   * @var string
   */
  private $_port;
  /**
   * The user name.
   *
   * @var string
   */
  private $_user;
  /**
   * The password.
   *
   * @var string
   */
  private $_password;

  /**
   * Accessor for configuration attributes.
   *
   * @param[in] string $name The function name.
   * @param[in] string $args The arguments passed on to the function.
   *
   * @return mixed
   */
  public function __get($name) {
    // private stuff
    switch ($name) {
      case 'services':
        return null;
    }

    $name = '_'.$name;
    if (isset($this->$name)) {
      return $this->$name;
    }
    return null;
  }

  /**
   * Overriden in order to allow retrieval of RPC interfaces as $this->Xxx([$param, ....]).
   *
   * @param[in] string $name The function name.
   * @param[in] string $args The arguments passed on to the function.
   *
   * @return mixed
   */
  public function __call($name, $args) {
    switch ($this->_kind) {
      // whispercast
      case 'whispercast':
        switch ($name) {
          // whispercast
          case 'MediaElementService':
            if (!isset($this->_services[$name])) {
              require_once 'auto/whisperstreamlib/base/request_types.php';
              require_once 'auto/whisperstreamlib/elements/factory_types.php';
              require_once 'auto/whisperstreamlib/elements/factory_invokers.php';
              $connection = new RpcHttpConnection("http://$this->_user:$this->_password@$this->_host:$this->_port/rpc-config/");
              eval('$this->_services[$name] = new RpcServiceWrapper'.$name.'($connection, $name);');
            }
            return $this->_services[$name]; 

          case 'MediaStats':
            if (!isset($this->_services[$name])) {
              require_once 'auto/whisperstreamlib/stats2/media_stats_types.php';
              require_once 'auto/whisperstreamlib/stats2/media_stats_invokers.php';
              if ($this->_user) {
                $connection = new RpcHttpConnection("http://$this->_user:$this->_password@$this->_host:$this->_port/rpc-stats/");
              } else {
                $connection = new RpcHttpConnection("http://$this->_host:$this->_port/rpc-stats/");
              }
              eval('$this->_services[$name] = new RpcServiceWrapper'.$name.'($connection, $name);');
            }
            return $this->_services[$name]; 

          case 'StandardLibraryService':
            if (!isset($this->_services[$name])) {
              require_once 'auto/whisperstreamlib/elements/factory_types.php';
              require_once 'auto/whisperstreamlib/elements/standard_library/standard_library_types.php';
              require_once 'auto/whisperstreamlib/elements/standard_library/standard_library_invokers.php';
              $connection = new RpcHttpConnection("http://$this->_user:$this->_password@$this->_host:$this->_port/rpc-config/standard_library/");
              eval('$this->_services[$name] = new RpcServiceWrapper'.$name.'($connection, $name);');
            }
            return $this->_services[$name]; 
          case 'ExtraLibraryService':
            if (!isset($this->_services[$name])) {
              require_once 'auto/whisperstreamlib/elements/factory_types.php';
              require_once 'auto/private/extra_library/extra_library_types.php';
              require_once 'auto/private/extra_library/extra_library_invokers.php';
              $connection = new RpcHttpConnection("http://$this->_user:$this->_password@$this->_host:$this->_port/rpc-config/extra_library/");
              eval('$this->_services[$name] = new RpcServiceWrapper'.$name.'($connection, $name);');
            }
            return $this->_services[$name]; 
          case 'OsdLibraryService':
            if (!isset($this->_services[$name])) {
              require_once 'auto/private/osd/osd_library/osd_library_types.php';
              require_once 'auto/private/osd/osd_library/osd_library_invokers.php';

              $connection = new RpcHttpConnection("http://$this->_user:$this->_password@$this->_host:$this->_port/rpc-config/osd_library/");
              eval('$this->_services[$name] = new RpcServiceWrapper'.$name.'($connection, $name);');
            }
            return $this->_services[$name]; 

          case 'PlaylistPolicyService':
          case 'SwitchPolicyService':
            if (!isset($this->_services[$name][$args[0]])) {
              require_once 'auto/whisperstreamlib/elements/factory_types.php';
              require_once 'auto/whisperstreamlib/elements/standard_library/standard_library_types.php';
              require_once 'auto/whisperstreamlib/elements/standard_library/standard_library_invokers.php';
              $connection = new RpcHttpConnection("http://$this->_user:$this->_password@$this->_host:$this->_port/rpc-config/standard_library/$args[0]/$args[0]/");
              eval('$this->_services[$name][$args[0]] = new RpcServiceWrapper'.$name.'($connection, $name);');
            }
            return $this->_services[$name][$args[0]]; 
          case 'SwitchingElementService':
            if (!isset($this->_services[$name][$args[0]])) {
              require_once 'auto/whisperstreamlib/elements/factory_types.php';
              require_once 'auto/whisperstreamlib/elements/standard_library/standard_library_types.php';
              require_once 'auto/whisperstreamlib/elements/standard_library/standard_library_invokers.php';
              $connection = new RpcHttpConnection("http://$this->_user:$this->_password@$this->_host:$this->_port/rpc-config/standard_library/$args[0]/");
              eval('$this->_services[$name][$args[0]] = new RpcServiceWrapper'.$name.'($connection, $name);');
            }
            return $this->_services[$name][$args[0]]; 
          case 'RtmpPublishingElementService':
            if (!isset($this->_services[$name][$args[0]])) {
              require_once 'auto/whisperstreamlib/elements/factory_types.php';
              require_once 'auto/whisperstreamlib/elements/standard_library/standard_library_types.php';
              require_once 'auto/whisperstreamlib/elements/standard_library/standard_library_invokers.php';
              $connection = new RpcHttpConnection("http://$this->_user:$this->_password@$this->_host:$this->_port/rpc-config/standard_library/$args[0]");
              eval('$this->_services[$name][$args[0]] = new RpcServiceWrapper'.$name.'($connection, $name);');
            }
            return $this->_services[$name][$args[0]]; 
          case 'HttpServerElementService':
            if (!isset($this->_services[$name][$args[0]])) {
              require_once 'auto/whisperstreamlib/elements/factory_types.php';
              require_once 'auto/whisperstreamlib/elements/standard_library/standard_library_types.php';
              require_once 'auto/whisperstreamlib/elements/standard_library/standard_library_invokers.php';
              $connection = new RpcHttpConnection("http://$this->_user:$this->_password@$this->_host:$this->_port/rpc-config/standard_library/$args[0]");
              eval('$this->_services[$name][$args[0]] = new RpcServiceWrapper'.$name.'($connection, $name);');
            }
            return $this->_services[$name][$args[0]]; 
          case 'SimpleAuthorizerService':
            if (!isset($this->_services[$name][$args[0]])) {
              require_once 'auto/whisperstreamlib/elements/factory_types.php';
              require_once 'auto/whisperstreamlib/elements/standard_library/standard_library_types.php';
              require_once 'auto/whisperstreamlib/elements/standard_library/standard_library_invokers.php';
              $connection = new RpcHttpConnection("http://$this->_user:$this->_password@$this->_host:$this->_port/rpc-config/standard_library/$args[0]");
              eval('$this->_services[$name][$args[0]] = new RpcServiceWrapper'.$name.'($connection, $name);');
            }
            return $this->_services[$name][$args[0]]; 

          case 'SchedulePlaylistPolicyService':
            if (!isset($this->_services[$name][$args[0]])) {
              require_once 'auto/whisperstreamlib/elements/factory_types.php';
              require_once 'auto/private/extra_library/extra_library_types.php';
              require_once 'auto/private/extra_library/extra_library_invokers.php';
              $connection = new RpcHttpConnection("http://$this->_user:$this->_password@$this->_host:$this->_port/rpc-config/extra_library/$args[0]/$args[0]/");
              eval('$this->_services[$name][$args[0]] = new RpcServiceWrapper'.$name.'($connection, $name);');
            }
            return $this->_services[$name][$args[0]]; 
          
          case 'FlavouringElementService':
            if (!isset($this->_services[$name][$args[0]])) {
              require_once 'auto/whisperstreamlib/elements/factory_types.php';
              require_once 'auto/private/extra_library/extra_library_types.php';
              require_once 'auto/private/extra_library/extra_library_invokers.php';
              $connection = new RpcHttpConnection("http://$this->_user:$this->_password@$this->_host:$this->_port/rpc-config/extra_library/$args[0]/");
              eval('$this->_services[$name][$args[0]] = new RpcServiceWrapper'.$name.'($connection, $name);');
            }
            return $this->_services[$name][$args[0]]; 
          
          case 'OsdAssociator':
            if (!isset($this->_services[$name][$args[0]])) {
              require_once 'auto/private/osd/base/osd_types.php';
              require_once 'auto/private/osd/base/osd_invokers.php';
              $connection = new RpcHttpConnection("http://$this->_user:$this->_password@$this->_host:$this->_port/rpc-config/osd_library/osd_associator/$args[0]/");
              eval('$this->_services[$name][$args[0]] = new RpcServiceWrapper'.$name.'($connection, $name);');
            }
            return $this->_services[$name][$args[0]];

          case 'TimeRangeElementService':
            if (!isset($this->_services[$name][$args[0]])) {
              require_once 'auto/whisperstreamlib/elements/factory_types.php';
              require_once 'auto/private/extra_library/extra_library_types.php';
              require_once 'auto/private/extra_library/extra_library_invokers.php';
              $connection = new RpcHttpConnection("http://$this->_user:$this->_password@$this->_host:$this->_port/rpc-config/extra_library/$args[0]/");
              eval('$this->_services[$name][$args[0]] = new RpcServiceWrapper'.$name.'($connection, $name);');
            }
            return $this->_services[$name][$args[0]]; 
        }
        break;
      // transcoder
      case 'transcoder':
        switch ($name) {
          case 'MasterControl':
            if (!isset($this->_services[$name])) {
              require_once 'auto/private/transcoder/types.php';
              require_once 'auto/private/transcoder/server_invokers.php';
              if ($this->_user) {
                $connection = new RpcHttpConnection("http://$this->_user:$this->_password@$this->_host:$this->_port/rpc_master_manager/");
              } else {
                $connection = new RpcHttpConnection("http://$this->_host:$this->_port/rpc_master_manager/");
              }
              eval('$this->_services[$name] = new RpcServiceWrapper'.$name.'($connection, $name);');
            }
            return $this->_services[$name]; 
        }
        break;
      // runner
      case 'runner':
        switch ($name) {
          case 'Cmd':
            if (!isset($this->_services[$name])) {
              require_once 'auto/private/whispercmd/types.php';
              require_once 'auto/private/whispercmd/server_invokers.php';
              if ($this->_user) {
                $connection = new RpcHttpConnection("http://$this->_user:$this->_password@$this->_host:$this->_port/rpc_cmd/");
              } else {
                $connection = new RpcHttpConnection("http://$this->_host:$this->_port/rpc_cmd/");
              }
              eval('$this->_services[$name] = new RpcServiceWrapper'.$name.'($connection, $name);');
            }
            return $this->_services[$name]; 
        }
    }
    throw new Exception('Service "'.$name.'" not exposed by the server interface');
  }

  /**
   * Constructor for the interface object.
   *
   * @param[in] array $options The options needed to instantiate the interface:
   *  host: the host name of the server
   *  port: the port on which the server is running
   *  user: the user name
   *  password: the password
   *
   * @return void 
   */
  public function __construct($options) {
    $this->_kind = $options['kind'];

    switch ($this->_kind) {
      case 'whispercast':
        $this->_id = $options['id'];
        $this->_version = $options['version'];
        $this->_export = $options['export'];
        break;
      case 'transcoder':
        break;
      case 'runner':
        break;
    }
    $this->_host = $options['host'];
    $this->_port = $options['port'];
    $this->_user = $options['user'];
    $this->_password = $options['password'];
  }

  public function getElementPrefix() {
    if ($this->_kind != 'whispercast') {
      throw new Exception('Element prefix is exposed only by the streaming server interface');
    }
    switch ($this->_version) {
      case 1:
        return 's'.$this->_id;
        break;
      case 2:
      default:
        return 's'.$this->_id.'_'.$this->_version;
    }
  }
  public function getStreamPrefix() {
    if ($this->_kind != 'whispercast') {
      throw new Exception('Stream prefix is exposed only by the streaming server interface');
    }
    switch ($this->_version) {
      case 1:
        return 's'.$this->_id.'_ts';
      case 2:
      default:
        return 's'.$this->_id.'_'.$this->_version.'_ts';
    }
  }
  public function getStreamPath($path) {
    if ($this->_kind != 'whispercast') {
      throw new Exception('Stream path is exposed only by the streaming server interface');
    }
    if (substr($path, 0, 1) == '/') {
      return $this->getStreamPrefix().$path;
    }
    return $this->getStreamPrefix().'/'.$path;
  }
  public function getExportPrefix() {
    if ($this->_kind != 'whispercast') {
      throw new Exception('Export prefix is exposed only by the streaming server interface');
    }
    switch ($this->_version) {
      case 1:
        return 's'.$this->_id.'_osde/s'.$this->_id.'_osda/'.$this->getStreamPrefix();
      case 2:
      default:
        return 's'.$this->_id.'_'.$this->_version.'_osde/s'.$this->_id.'_'.$this->_version.'_osda/'.$this->getStreamPrefix();
    }
  }
  
  public function getPathForLike($path) {
    if ($this->_kind != 'whispercast') {
      throw new Exception('Path functions are exposed only by the streaming server interface');
    }
    return '%'.str_replace(array('%','_'), array('\%', '\_'), substr($path, strlen($this->getStreamPrefix())+1));
  }
}
?>
