<?php
class Whispercast_Programs extends Whispercast_Base {
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
   * Converts from a config structure to the actual spec.
   *
   * @param[in] array $config The config.
   *
   * @return object
   */
  private function _configToSpec($config) {
    $playlist = array();
    if ($config['playlist']) {
      foreach ($config['playlist'] as $kid=>$p) {
        $date = null;
        if (isset($p['date'])) {
          $date = array(
            array(
              'year_' =>(int)$p['date']['year'],
              'month_' =>(int)$p['date']['month'],
              'day_' =>(int)$p['date']['day']
            )
          );
        }
        $weekdays = null;
        if (isset($p['weekdays'])) {
          $weekdays = $p['weekdays'];
          foreach ($weekdays as $k=>$v) {
            $weekdays[$k] = (int)$v;
          }
        }

        $playlist[] = new SchedulePlaylistItemSpec($kid, $p['media'], new TimeSpec(
          (int)$p['time']['minute'], (int)$p['time']['hour'], (int)$p['time']['second'],
          $date,
          $weekdays,
          $p['duration'] ? (int)$p['duration'] : 0
        ));
      }
    }
    return array(
      'default_media' => isset($config['default_media']) ? $config['default_media'] : null,
      'playlist' => $playlist,
      'switch_now' => isset($config['switch_now']) ? ($config['switch_now'] ? true : false) : null
    );
  }

  /**
   * Converts from an actual spec to the config structure.
   *
   * @param[in] object $spec The spec.
   *
   * @return array
   */
  private function _specToConfig($param) {
    $config = array();
    $config['default_media'] = $param->default_media_;
    $config['playlist'] = array();
    foreach ($param->playlist_ as $p) {
      $record = array(
        'id' => $p->id_,
        'media' => $p->media_,
        'time' => array(
          'hour' => $p->start_time_->start_hour_,
          'minute' => $p->start_time_->start_minute_,
          'second' => isset($p->start_time_->start_second_) ? $p->start_time_->start_second_ : 0,
         ),
        'duration' => $p->start_time_->duration_in_seconds_
      );

      if (isset($p->start_time_->weekdays_)) {
        $record['weekdays'] = $p->start_time_->weekdays_;
      }
      if (isset($p->start_time_->dates_)) {
        if (count($p->start_time_->dates_) > 0) {
          $record['date'] = array(
            'year' => $p->start_time_->dates_[0]->year_,
            'month' => $p->start_time_->dates_[0]->month_,
            'day' => $p->start_time_->dates_[0]->day_
          );
        }
      }

      $config['playlist'][] = $record;
    }
    return $config;
  }

  /**
   * Creates a program.
   *
   * @param[in] string $id The id of the program we are creating.
   * @param[in] array $config The program configuration, as an array.
   *
   * @return string An error code or null if successful.
   */
  public function create($id, $config) {
    /* $config:
     *
     * default_media AS string
     * playlist AS array<
     *   media AS string
     *   time AS array(
     *     hour AS int
     *     minute AS int
     *     second AS int
     *   )
     *   date AS array(
     *     year AS int
     *     month AS int
     *     day AS int
     *   )
     *   weekdays AS array(
     *      day_of_week AS int
     *      ...
     *      day_of_week AS int
     *   )
     *   switch_now AS boolean
     * >
     */
    $iid = $this->_interface->getElementPrefix().'_'.$id;

    $media_element_service = $this->_interface->MediaElementService();
    $standard_library_service = $this->_interface->StandardLibraryService();
    $extra_library_service = $this->_interface->ExtraLibraryService();

    $spec = $this->_configToSpec($config);

    $cs = $extra_library_service->AddSchedulePlaylistPolicySpec($iid.'_program', new SchedulePlaylistPolicySpec($spec['default_media'], $spec['playlist'], null, false));
    if (!$cs->success_) {
      return $cs->error_;
    }

    $cs = $standard_library_service->AddSwitchingElementSpec($iid.'_program', true, false, new SwitchingElementSpec($iid.'_program', 'flv', 255, 1, 0, false));
    if (!$cs->success_) {
      $media_element_service->DeletePolicySpec($iid.'_program');
      return $cs->error_;
    }

    return null;
  }
  /**
   * Destroys the program having the given id.
   *
   * @param[in] string $id The id of the program.
   *
   * @return string An error code or null if successful.
   */
  public function destroy($id) {
    $iid = $this->_interface->getElementPrefix().'_'.$id;

    $media_element_service = $this->_interface->MediaElementService();

    $cs = $media_element_service->DeleteElementSpec($iid.'_program');
    if ($cs->success_) {
      $media_element_service->DeletePolicySpec($iid.'_program');
      return null;
    }
    return $cs->error_;
  }

  /**
   * Returns the internal path for the given program.
   *
   * @param[in] string $id The id of the program element.
   *
   * @return string The internal path or null on error.
   */
  public function getPath($id) {
    $iid = $this->_interface->getElementPrefix().'_'.$id;
    return '/'.$iid.'_program';
  }

  /**
   * Retrieves the current configuration of the given program element.
   *
   * @param[in] string $id The id of the program element.
   * @param[out] array $config On success an array that contains the program configuration, on failure left unchanged.
   *
   * @return string An error code or null if successful.
   */
  public function getConfig($id, &$config) {
    $iid = $this->_interface->getElementPrefix().'_'.$id;

    $cs = $this->_interface->SchedulePlaylistPolicyService($iid.'_program')->GetPlaylist();
    if (!$cs->success_) {
      return $cs->error_;
    }

    $config = $this->_specToConfig($cs->result_);
    return null;
  }
  /**
   * Sets the configuration of the given program element.
   *
   * @param[in] string $id The id of the program element.
   * @param[in] string $config The program config.
   *
   * @return string An error code or null if successful.
   */
  public function setConfig($id, $config) {
    $iid = $this->_interface->getElementPrefix().'_'.$id;

    $extra_library_service = $this->_interface->ExtraLibraryService();

    $spec = $this->_configToSpec($config);

    $cs = $this->_interface->SchedulePlaylistPolicyService($iid.'_program')->SetPlaylist(new SetSchedulePlaylistPolicySpec($spec['default_media'], $spec['switch_now'], $spec['playlist']));
    if (!$cs->success_) {
      return $cs->error_;
    }

    return null;
  }

  /**
   * Returns the media currently playing in the given program.
   *
   * @param[in] string $id The id of the program.
   * @param[out] string $media The currently playing media.
   *
   * @return string An error code or null if successful.
   */
  public function getMedia($id, &$media) {
    $iid = $this->_interface->getElementPrefix().'_'.$id;

    $cs = $this->_interface->SwitchingElementService($iid.'_program')->GetCurrentMedia();
    if (!$cs->success_) {
      return $cs->error_;
    }

    $media = $cs->result_;
    return null;
  }

}
?>
