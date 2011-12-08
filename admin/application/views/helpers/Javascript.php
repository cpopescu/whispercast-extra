<?php
/**
 * Helper that retuns the base url
 * 
 * @package View Helpers
 */
 
class Zend_View_Helper_Javascript {
    /**
     * Returns the given data as a Javscript expression.
     * 
     * @return string
     */
    public function javascript($data, $options = array()) {
      return Zend_Json::encode($data, null, $options);
    }
}
?>
