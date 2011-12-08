<?php
class Whispercast_Osd extends Whispercast_Base {
  /**
   * Constructor for the Whispercast object.
   *
   * @param[in] object $interface The actual interface.
   *
   * @return void 
   */
  public function __construct($interface, $prefix = '') {
    parent::__construct($interface, $prefix);
  }

  /**
   * Retrieves the OSD data associated with a specific media (clip).
   *
   * @param[in] string $id The id of the OSD associator element.
   * @param[in] string $path The path of the media we want the OSD related data for.
   * @param[out] array $data On success an associative array with all the OSD related data for the given media, on failure left unchanged.
   *
   * The clip data is an array containing the overlays, crawlers and PIP data - no formatting, colors, etc (except for PIP). The overlays
   * and crawlers are indexed by the id of the overlay (crawler) with which the data is associated.
   *
   * The overlay data is actually a simple text to display (it might be HTML or whatever is understood by the player).
   * The crawler data is an associative array of strings to strings, the key being the id of the crawler item and the value being the text to display.
   * The PIP data is an associative array with the following keys:
   * url - string - the URL to be played in PIP.
   * x_location, y_location - float - the position of the PIP area, relative to the container (the  media).
   * width, height - float - the size of the PIP area, relative to the container (the  media).
   * play - boolean - if true, the player must run the media identified by 'url' when it receives the PIP state.
   *
   * Sample of a possible data:
   *
   * [
   * 'overlays' => ['phone' => '08989898989', 'contact' => '<a href="mailto:gg@email.com">Greta Garbo</a>', ...],
   * 'crawlers' => ['crawler1' => ['1' => 'First item crawler...', '2' => 'Second item crawler', ...], 'advertising' => ['1' => 'Great prices on Hyundai', '2' => 'Buy a chicken', ...], ...],
   * 'pip' => ['url' => 'rtmp://server/whispercast/topshop/clip1234.flv', 'x_location' => 0.7, 'y_location' => 0.2, 'width' => 0.3, 'height' => 0.3, 'play' => true]
   * ]
   *
   * @return string An error code or null if successful.
   */
  public function getData($id, $path, &$data) {
    $osda = $this->_interface->OsdAssociator($id);

    $cs = $osda->GetAssociatedOsds($path);
    if (!$cs->success_) {
      return $cs->error_;
    }

    if ($cs->result_->overlays !== null) {
      foreach ($cs->result_->overlays as $overlay) {
        $data['overlays'][$overlay->id] = $overlay->content;
      }
    } else {
      $data['overlays'] = array();
    }
    if ($cs->result_->crawlers !== null) {
      foreach ($cs->result_->crawlers as $crawler) {
        $data['crawlers'][$crawler->crawler_id][$crawler->item_id] = $crawler->item->content;
      }
    } else {
      $data['crawlers'] = array();
    }
    if ($cs->result_->pip !== null) {
      $data['pip'] = self::toArray($cs->result_->pip);
    } else {
      $data['pip'] = null;
    }
    if ($cs->result_->click_url !== null) {
      $data['click_url'] = $cs->result_->click_url->url;
    }
    return null;
  }
  /**
   * Sets the OSD data associated with a specific media (clip).
   *
   * @param[in] string $id The id of the OSD associator element.
   * @param[in] string $path The path of the media we want the OSD related data for.
   * @param[in] array $data The OSD related for the given media.
   *
   * @return string An error code or null if successful.
   */
  public function setData($id, $path, $data) {
    $osda = $this->_interface->OsdAssociator($id);

    $overlays = null;
    if (isset($data['overlays'])) {
      foreach ($data['overlays'] as $id=>$content) {
        $overlays[] = new AddOverlayParams(''.$id, $content);
      }
    }
    $crawlers = null;
    if (isset($data['crawlers'])) {
      foreach ($data['crawlers'] as $crawler_id=>$crawler_items) {
        foreach ($crawler_items as $id=>$content) {
          $crawlers[] = new AddCrawlerItemParams(''.$crawler_id, ''.$id, new CrawlerItem($content));
        }
      } 
    }
    $pip = null;
    if (isset($data['pip'])) {
      $pip = self::toObject($data['pip'], 'SetPictureInPictureParams', array('url','x_location','y_location','width','height','play'));
    }
    $click_url = null;
    if (isset($data['click_url'])) {
      $click_url = self::toObject(array('url'=>$data['click_url']), 'SetClickUrlParams', array('url'));
    }

    // TODO: Are these the right defaults for the eos_xxx members ?
    $params = new AssociatedOsds($path, $crawlers, $overlays, $pip, $click_url, true, true, false, true, true);
    $cs = $osda->SetAssociatedOsds($params);
    return $cs->success_ ? null : $cs->error_;
  }

  /**
   * Merges two sets of OSD data.
   *
   * @param[in] array $data1 The first OSD data set.
   * @param[in] array $data2 The second OSD data set.
   * @param[out] array $result The merge result.
   *
   * @return string An error code or null if successful.
   */
  public function mergeData($data1, $data2, &$result) {
    $result = $data1;

    if (isset($data2['overlays'])) {
      if ($data2['overlays']) {
        $result['overlays'] = $data2['overlays'];
/* TODO: Decide what 'merge' means for overlays...
        foreach ($data2['overlays'] as $id=>$content) {
          $result['overlays'][$id] = $content;
        }
*/
      } else {
        unset($result['overlays']);
      }
    }
    if (isset($data2['crawlers'])) {
      if ($data2['crawlers']) {
        $result['crawlers'] = $data2['crawlers'];
/* TODO: Decide what 'merge' means for crawlers...
        foreach ($data2['crawlers'] as $id=>$content) {
          $result['crawlers'][$id] = $content;
        }
*/
      } else {
        unset($result['crawlers']);
      }
    }
    if (isset($data2['pip'])) {
      if ($data2['pip']) {
        $result['pip'] = $data2['pip'];
      } else {
        unset($result['pip']);
      }
    }
    if (isset($data2['click_url'])) {
      if ($data2['click_url']) {
        $result['click_url'] = $data2['click_url'];
      } else {
        unset($result['click_url']);
      }
    }
    return null;
  }

  /**
   * Retrieves the ids of all the defined OSD overlays.
   *
   * @param[in] string $id The id of the OSD associator element.
   * @param[out] array $ids On success an array that contains the ids of all the OSD overlays associated with the given export, on failure left unchanged.
   *
   * @return string An error code or null if successful.
   */
  public function listOverlays($id, &$ids) {
    $osda = $this->_interface->OsdAssociator($id);

    $cs = $osda->GetAllOverlayTypeIds();
    if (!$cs->success_) {
      return $cs->error_;
    }

    $ids = $cs->result_;
    return null;
  }
  /**
   * Creates or updates an OSD overlay.
   *
   * @param[in] string $id The id of the OSD overlay we are creating or updating.
   * @param[in] array $overlay An associative array containing the setup of the OSD overlay being created or added.
   *
   * The overlay setup is an associative array with the following keys:
   *
   *   fore_color, back_color - PlayerColor - the foreground and background colors of the overlay.
   *   link_color, hover_color - PlayerColor - the color of any HTML links in the content.
   *
   *   x_location, y_location - float - the position of the overlay.
   *   width - float - the size of the overlay.
   *
   *   margins - PlayerMargin - the margins.
   *
   *   ttl - float - the time, in seconds, that the overlay will be displayed - after this, the overlay is destroyed.
   *   show - boolean - if true the overlay is visible, if false the overlay is hidden.
   *
   * @return string An error code or null if successful.
   */
  public function createOverlay($id, $overlay) {
    $osda = $this->_interface->OsdAssociator();

    $overlay['id'] = $id;
    unset($overlay['content']);

    $params = self::toObject(
      $overlay,
     'CreateOverlayParams',
      array(
       'id',
       'content',
        array('fore_color', 'PlayerColor', array('color','alpha')),
        array('back_color', 'PlayerColor', array('color','alpha')),
        array('link_color','PlayerColor',array('color','alpha')),
        array('hover_color','PlayerColor',array('color','alpha')),
       'x_location',
       'y_location',
       'width',
       array('margins','PlayerMargins',array('left','top','right','bottom')),
       'ttl',
       'show'
      )
    );

    $cs = $osda->CreateOverlayType($params);
    return $cs->success_ ? null : $cs->error_;
  }
  /**
   * Destroys an OSD overlay.
   *
   * @param[in] string $id The id of the OSD overlay we are destroying.
   *
   * @return string An error code or null if successful.
   */
  public function destroyOverlay($id) {
    $osda = $this->_interface->OsdAssociator();

    $cs = $osda->DestroyOverlayType(new DestroyOverlayParams($id));
    return $cs->success_ ? null : $cs->error_;
  }
  /**
   * Retrieves the OSD overlay setup having the given id.
   *
   * @param[in] string $id The id of the OSD overlay we are creating or updating.
   * @param[out] array $overlay On success an associative array with the OSD overlay setup for the given id, on failure left unchanged.
   *
   * @return string An error code or null if successful.
   */
  public function getOverlay($id, &$overlay) {
    $osda = $this->_interface->OsdAssociator();

    $cs = $osda->GetOverlayType($id);
    if (!$cs->success_) {
      return $cs->error_;
    }

    $overlay = self::toArray($cs->result_);
    return null;
  }

  /**
   * Retrieves the ids of all the OSD crawlers managed by the system.
   *
   * @param[in] string $id The id of the OSD associator element.
   * @param[out] array $ids On success an array that contains the ids of all the OSD crawlers associated with the given export, on failure left unchanged.
   *
   * @return string An error code or null if successful.
   */
  public function listCrawlers($id, &$ids) {
    $osda = $this->_interface->OsdAssociator($id);

    $cs = $osda->GetAllCrawlerTypeIds();
    if (!$cs->success_) {
      return $cs->error_;
    }

    $ids = $cs->result_;
    return null;
  }
  /**
   * Creates or updates an OSD crawler.
   *
   * @param[in] string $id The id of the OSD crawler we are creating or updating.
   * @param[in] array $crawler An associative array containing the setup of the OSD crawler being created or added.
   *
   * The crawler setup is an associative array with the following keys:
   *
   *   speed - float - the speed of the crawler's crawling :), expressed as how many times the width of the crawler
   *                   will be rotated in each second (ie. 1 means that in one second the crawler will move over the
   *                   entire width of the player).
   *
   *   fore_color, back_color - PlayerColor - the foreground and background colors of the overlay.
   *   link_color, hover_color - PlayerColor - the color of any HTML links in the content.
   *
   *   y_location - float - the position of the overlay.
   *
   *   margins - PlayerMargin - the margins.
   *
   *   items - array - an associative array (map) of item ids to item text.
   *
   *   ttl - float - the time, in seconds, that the overlay will be displayed - after this, the overlay is destroyed.
   *   show - boolean - if true the overlay is visible, if false the overlay is hidden.
   *
   * @return string An error code or null if successful.
   */
  public function createCrawler($id, $crawler) {
    $osda = $this->_interface->OsdAssociator();

    $crawler['id'] = $id;
    unset($crawler['items']);

    $params = self::toObject(
      $crawler,
      'CreateCrawlerParams',
      array(
        'id',
        'speed',
        array('fore_color', 'PlayerColor', array('color','alpha')),
        array('back_color', 'PlayerColor', array('color','alpha')),
        array('link_color', 'PlayerColor', array('color','alpha')),
        array('hover_color', 'PlayerColor', array('color','alpha')),
        'y_location',
        array('margins', 'PlayerMargins', array('left','top','right','bottom')),
        'items',
        'ttl',
        'show'
      )
    );

    $cs = $osda->CreateCrawlerType($params);
    return $cs->success_ ? null : $cs->error_;
  }
  /**
   * Destroys an OSD crawler.
   *
   * @param[in] string $crawler_id The id of the OSD crawler we are destroying.
   *
   * @return string An error code or null if successful.
   */
  public function destroyCrawler($id) {
    $osda = $this->_interface->OsdAssociator();

    $cs = $osda->DestroyCrawlerType(new DestroyCrawlerParams($id));
    return $cs->success_ ? null : $cs->error_;
  }
  /**
   * Retrieves the OSD crawler setup having the given id.
   *
   * @param[in] string $crawler_id The id of the OSD crawler we are creating or updating.
   * @param[out] array $crawler On success an associative array with the OSD crawler setup for the given id, on failure left unchanged.
   *
   * @return string An error code or null if successful.
   */
  public function getCrawler($id, &$crawler) {
    $osda = $this->_interface->OsdAssociator();

    $cs = $osda->GetCrawlerType($id);
    if (!$cs->success_) {
      return $cs->error_;
    }

    $crawler = self::toArray($cs->result_);
    return null;
  }
}
?>
