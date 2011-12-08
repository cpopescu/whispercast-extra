<?php
/**
 * ##LIBRARY_NAME##
 *
 * ##LICENSE##
 * 
 *
 * @category   ##LIBRARY_NAME## 
 * @package    PHPTAL_Zend
 * @copyright  ##COPYRIGHT##
 * @license    ##LICENSE_SHORT##
 */

require_once 'PHPTAL/PHPTAL.php';
require_once 'PHPTAL_Zend_View_Translator.php';

/**
 * Integration of PHPTAL and Zend Framework
 *
 * @category   ##LIBRARY_NAME##
 * @package    PHPTAL_Zend
 * @copyright  ##COPYRIGHT##
 * @license    ##LICENSE_SHORT##
 */
class PHPTAL_Zend_View extends PHPTAL implements Zend_View_Interface {
  protected $_helperLoader;
  protected $_helper = array();

  /**
    * Constructor.
    *
    * @param array $config Configuration key-value pairs.
    */
  public function __construct($config = array())
  {
    // call the base class member
    parent::__construct();

    // encoding
    if (array_key_exists('encoding', $config)) {
      $this->setEncoding($config['encoding']);
    }

    // base path
    if (array_key_exists('basePath', $config)) {
      $prefix = 'Zend_View';
      if (array_key_exists('basePathPrefix', $config)) {
        $prefix = $config['basePathPrefix'];
      }
      $this->setBasePath($config['basePath'], $prefix);
    }
    // script path
    if (array_key_exists('scriptPath', $config)) {
      $this->addScriptPath($config['scriptPath']);
    }
    // helper path
    if (array_key_exists('helperPath', $config)) {
      if (is_array($config['helperPath'])) {
        foreach ($config['helperPath'] as $prefix => $path) {
          $this->addHelperPath($path, $prefix);
        }
      } else {
        $prefix = 'Zend_View_Helper';
        if (array_key_exists('helperPathPrefix', $config)) {
          $prefix = $config['helperPathPrefix'];
        }
        $this->addHelperPath($config['helperPath'], $prefix);
      }
    }

    // compiled path
    if (array_key_exists('compiledPath', $config)) {
      $this->setPhpCodeDestination($config['compiledPath']);
    }

    $this->setTranslator(new PHPTAL_Zend_View_Translator());
  }

  /**
    * Return the helpers loader
    *
    * @return mixed
    */
  protected function _getHelperLoader() {
    if (!$this->_helperLoader) {
      $this->_helperLoader = new Zend_Loader_PluginLoader(array('Zend_View_Helper' => 'Zend/View/Helper'));
    }
    return $this->_helperLoader;
  }
  /**
    * Return a named helper
    *
    * @param string The helper name. 
    * @return object|false
    */
  protected function _getHelper($name) {
    if (!isset($this->_helper[$name])) {
      $loader = $this->_getHelperLoader();
      if ($loader->isLoaded($name)) {
        $class = $loader->getClassName($name);
      } else {
        try {
          $class = $loader->load($name);
          if ($class === false) {
            return false;
          }
          $class = $loader->getClassName($name);
        } catch (Zend_Loader_Exception $e) {
          return false;
        }
      }

      $this->_helper[$name] = new $class();
      if (method_exists($this->_helper[$name], 'setView')) {
         $this->_helper[$name]->setView($this);
      }
    }
    return $this->_helper[$name];
  }

  /**
    * Accesses a helper object from within a script.
    *
    * If the helper class has a 'view' property, sets it with the current view
    * object.
    *
    * @param string $name The helper name.
    * @param array $args The parameters for the helper.
    * @return string The result of the helper output.
    */
  public function __call($name, $args) {
    $helper = $this->_getHelper($name);
    if ($helper === false) {
      return '';
    }
    return call_user_func_array(
      array($helper, $name),
      $args
    );
  }

  /**
    * Returns true if a variable with the given name has been set.
    *
    *
    * @param string $name The variable name.
    * @return boolean True if the variable was set.
    */
  public function __isset($name) {
    return isset($this->getContext()->$name);
  }
  /**
    * Clears the variable with the given name.
    *
    *
    * @param string $name The variable name.
    * @return
    */
  public function __unset($name) {
    unset($this->getContext()->$name);
  }

  /**
    * Return the template engine object, if any
    *
    * If using a third-party template engine, such as Smarty, patTemplate,
    * phplib, etc, return the template engine object. Useful for calling
    * methods on these objects, such as for setting filters, modifiers, etc.
    *
    * @return mixed
    */
  public function getEngine() {
    return $this;
  }

  /**
    * Set the path to find the view script used by render()
    *
    * @param string|array The directory (-ies) to set as the path. Note that
    * the concrete view implentation may not necessarily support multiple
    * directories.
    * @return PHPTAL_Zend_View
    */
  public function setScriptPath($path) {
    $this->setTemplateRepository(is_array($path) ? $path : array($path));
    return $this;
  }

  /**
    *  Adda a path to find the view script used by by render()
    *
    * @param string|array The directory (-ies) to set as the path. Note that
    * the concrete view implentation may not necessarily support multiple
    * directories.
    * @return PHPTAL_Zend_View
    */
  public function addScriptPath($path) {
    if (is_array($path)) {
      foreach($path as $value) {
        $this->setTemplateRepository($value);
      }
    } else {
      $this->setTemplateRepository($path);
    }
    return $this;
  }
  /**
    * Retrieve all view script paths
    *
    * @return array
    */
  public function getScriptPaths() {
    return $this->getTemplateRepositories();
  }

  /**
    * Set a base path to all view resources
    *
    * @param  string $path
    * @param  string $classPrefix
    * @return PHPTAL_Zend_View
    */
  public function setBasePath($path, $classPrefix = 'Zend_View') {
    $this->setTemplateRepository();
    return $this;
  }

  /**
    * Add an additional path to view resources
    *
    * @param  string $path
    * @param  string $classPrefix
    * @return PHPTAL_Zend_View
    */
  public function addBasePath($path, $classPrefix = 'Zend_View') {
    $path = rtrim($path, '/');
    $path = rtrim($path, '\\');
    $path .= DIRECTORY_SEPARATOR;
    $classPrefix = rtrim($classPrefix, '_') . '_';
    $this->addScriptPath($path . 'scripts');
    $this->addHelperPath($path . 'helpers', $classPrefix . 'Helper');
    return $this;
  }

  /**
    * Adds to the stack of helper paths in LIFO order.
    *
    * @param string|array The directory (-ies) to add.
    * @param string $classPrefix Class prefix to use with classes in this
    * directory; defaults to Zend_View_Helper
    * @return PHPTAL_Zend_View
    */
  public function addHelperPath($path, $classPrefix = 'Zend_View_Helper_') {
    $this->_getHelperLoader()->addPrefixPath($classPrefix, $path);
    return $this;
  }

  /**
    * Resets the stack of helper paths.
    *
    * To clear all paths, use Zend_View::setHelperPath(null).
    *
    * @param string|array $path The directory (-ies) to set as the path.
    * @param string $classPrefix The class prefix to apply to all elements in
    * $path; defaults to Zend_View_Helper
    * @return PHPTAL_Zend_View
    */
  public function setHelperPath($path, $classPrefix = 'Zend_View_Helper_') {
    unset($this->_helperLoader);
    return $this->addHelperPath($path, $classPrefix);
  }

  /**
    * Get full path to a helper class file specified by $name
    *
    * @param  string $name
    * @return string|false False on failure, path on success
    */
  public function getHelperPath($name) {
    return $this->_getHelperLoader()->getClassPath($name);
  }

  /**
    * Returns an array of all currently set helper paths
    *
    * @return array
    */
  public function getHelperPaths() {
    return $this->getPluginLoader('helper')->getPaths();
  }

  /**
    * Assign variables to the view script via differing strategies.
    *
    * Suggested implementation is to allow setting a specific key to the
    * specified value, OR passing an array of key => value pairs to set en
    * masse.
    *
    * @see __set()
    * @param string|array $spec The assignment strategy to use (key or array of key
    * => value pairs)
    * @param mixed $value (Optional) If assigning a named variable, use this
    * as the value.
    * @return void
    */
  public function assign($spec, $value = null) {
    if (is_array($spec)) {
      foreach ($spec as $key=>$value) {
        $this->$key = $value;
      }
    } else {
      $this->$spec = $value;
    }
  }

  /**
    * Return list of all assigned variables
    *
    * Returns all public properties of the object. Reflection is not used
    * here as testing reflection properties for visibility is buggy.
    *
    * @return array
    */
  public function getVars() {
    $context = $this->getContext();
    $vars = get_object_vars($context);
    foreach ($vars as $key => $value) {
      if ('_' == substr($key, 0, 1)) {
        unset($vars[$key]);
      }
    }
    return $vars;
  }
  /**
    * Clear all assigned variables
    *
    * Clears all variables assigned to Zend_View either via {@link assign()} or
    * property overloading ({@link __get()}/{@link __set()}).
    *
    * @return void
    */
  public function clearVars() {
    $context = $this->getContext();
    $vars = get_object_vars($context);
    foreach ($vars as $key => $value) {
      if ('_' != substr($key, 0, 1)) {
        unset($context->$key);
      }
    }
  }

  /**
    * Processes a view script and returns the output.
    *
    * @param string $name The script script name to process.
    * @return string The script output.
    */
  public function render($name) {
    $previous = $this->get('view');
    $this->set('view', $this);

    try {
      $this->setTemplate(func_get_arg(0));
      $result =  $this->execute();

      $this->set('view', $previous);
      return $result;
    } catch (PHPTAL_IOException $e) {
      $this->set('view', $previous);

      // TODO: Hacky - we assume that the template file was not found
      // and we treat this as a dispatcher error (ie. HTTP 404)..
      throw new Zend_Controller_Dispatcher_Exception($e->getMessage(), 404);
    } catch (PHPTAL_Exception $e) {
      $this->set('view', $previous);
      throw new Zend_View_Exception($e->getMessage(), $this);
    }
  }

  /**
    * Escapes a value for output in a view script.
    *
    * If escaping mechanism is one of htmlspecialchars or htmlentities, uses
    * {@link $_encoding} setting.
    *
    * @param mixed $var The output to escape.
    * @return mixed The escaped value.
    */
  public function escape($var) {
    return htmlentities($var, ENT_COMPAT, 'UTF-8'); 
  }
}

function phptal_tales_json($src, $nothrow) {
  require_once PHPTAL_DIR.'PHPTAL/Php/Transformer.php';
  $src = PHPTAL_Php_Transformer::transform($src, '$ctx->');
  return 'json_encode('.$src.')';
}
?>
