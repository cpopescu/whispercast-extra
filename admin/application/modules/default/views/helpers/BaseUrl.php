<?php
/**
 * Helper that retuns the base url
 * 
 * @package View Helpers
 */
 
class Zend_View_Helper_BaseUrl {
    /**
     * returns the base url
     * 
     * @return string
     */
    public function baseUrl($path = '', $params = array()) {
      $result =  Zend_Controller_Front::getInstance()->getBaseUrl().$path;
      if ($params) {
        $quoted = array();
        foreach ($params as $k=>$v)
          $quoted[] = htmlentities($k,ENT_COMPAT,'UTF-8').'='.htmlentities($v,ENT_COMPAT,'UTF-8');
        $result .= '?'.implode($quoted,'&');
      }
      return $result;
    }
}
?>
