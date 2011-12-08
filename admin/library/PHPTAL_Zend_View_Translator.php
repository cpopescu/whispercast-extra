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

require_once 'PHPTAL/PHPTAL/TranslationService.php';

/**
 * Integration of PHPTAL and Zend Framework
 *
 * @category   ##LIBRARY_NAME##
 * @package    PHPTAL_Zend
 * @copyright  ##COPYRIGHT##
 * @license    ##LICENSE_SHORT##
 */
class PHPTAL_Zend_View_Translator implements PHPTAL_TranslationService {
    /**
     * Set the target language for translations.
     *
     * When set to '' no translation will be done.
     *
     * You can specify a list of possible language for exemple :
     *
     * setLanguage('fr_FR', 'fr_FR@euro')
     *
     * @return string - chosen language
     */
    function setLanguage(/*...*/) {
    }

    /**
     * PHPTAL will inform translation service what encoding page uses.
     * Output of translate() must be in this encoding.
     */
    function setEncoding($encoding) {
    }

    /**
     * Set the domain to use for translations.
     */
    function useDomain($domain) {
    }

    /**
     * Set an interpolation var.
     *
     * Replace all ${key}s with values in translated strings.
     */
    function setVar($key, $value) {
     }

    /**
     * Translate a gettext key and interpolate variables.
     */
    function translate($key, $htmlescape=true) {
      return $key;
    }
}
?>
