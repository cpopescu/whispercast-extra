<?php
class Whispercast_Playlists extends Whispercast_Base {
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
   * Creates a playlist.
   *
   * @param[in] string $id The id of the playlist we are creating or updating.
   * @param[in] array $config The playlist configuration, as an array.
   *
   * @return string An error code or null if successful.
   */
  public function create($id, $config) {
    /* $config:
     *
     * playlist AS array<media AS string>
     * loop AS boolean
     */
    $iid = $this->_interface->getElementPrefix().'_'.$id;

    $media_element_service = $this->_interface->MediaElementService();
    $standard_library_service = $this->_interface->StandardLibraryService();

    $playlist = array();
    foreach($config['playlist'] as $p) {
      $playlist[] = $p;
    }

    $cs = $standard_library_service->AddPlaylistPolicySpec($iid.'_playlist', new PlaylistPolicySpec((array)$playlist, (bool)$config['loop'], false));
    if (!$cs->success_)
      return $cs->error_;

    $cs = $standard_library_service->AddSwitchingElementSpec($iid.'_playlist', false, false, new SwitchingElementSpec($iid.'_playlist', 'flv', 255, 1, 0, false, 0));
    if (!$cs->success_) {
      $media_element_service->DeletePolicySpec($iid.'_playlist');
      return $cs->error_;
    }

    return null;
  }
  /**
   * Destroys the playlist having the given id.
   *
   * @param[in] string $id The id of the playlist.
   *
   * @return string An error code or null if successful.
   */
  public function destroy($id) {
    $iid = $this->_interface->getElementPrefix().'_'.$id;

    $media_element_service = $this->_interface->MediaElementService();

    $cs = $media_element_service->DeleteElementSpec($iid.'_playlist');
    if ($cs->success_) {
      $media_element_service->DeletePolicySpec($iid.'_playlist');
      return null;
    }
    return $cs->error_;
  }

  /**
   * Returns the internal path for the given playlist.
   *
   * @param[in] string $id The id of the playlist element.
   *
   * @return string The internal path or null on error.
   */
  public function getPath($id) {
    $iid = $this->_interface->getElementPrefix().'_'.$id;
    return $iid.'_playlist';
  }

  /**
   * Retrieves the configuration of the given playlist.
   *
   * @param[in] string $id The id of the playlist.
   * @param[out] array $config The playlist configuration on success, not changed on error.
   *
   * @return string An error code or null if successful.
   */
  public function getConfig($id, &$config) {
    $iid = $this->_interface->getElementPrefix().'_'.$id;

    $cs = $this->_interface->PlaylistPolicyService($iid.'_playlist')->GetPlaylist();
    if (!$cs->success_) {
      return $cs->error_;
    }

    $playlist = array();
    foreach ($cs->result_->playlist_ as $p) {
      $playlist[] = '/'.$p;
    }

    $config = array (
      'playlist' => $playlist,
      'loop' => (bool)$cs->result_->loop_playlist_
    );
    return null;
  }
  /**
   * Sets the configuration of the given playlist.
   *
   * @param[in] string $id The id of the playlist.
   * @param[in] array $config The playlist configuration, as an array.
   *
   * @return string An error code or null if successful.
   */
  public function setConfig($id, $config) {
    $iid = $this->_interface->getElementPrefix().'_'.$id;

    $playlist = array();
    foreach($config['playlist'] as $p) {
      $playlist[] = $p;
    }

    $cs = $this->_interface->PlaylistPolicyService($iid.'_playlist')->SetPlaylist(new SetPlaylistPolicySpec(new PlaylistPolicySpec($playlist, $config['loop'], false, 0)));
    if (!$cs->success_) {
      return $cs->error_;
    }

    return null;
  }

  /**
   * Returns the media currently playing in the given playlist.
   *
   * @param[in] string $id The id of the playlist.
   * @param[out] string $media The currently playing media on success, not changed on error.
   *
   * @return string An error code or null if successful.
   */
  public function getMedia($id, &$media) {
    $iid = $this->_interface->getElementPrefix().'_'.$id;

    $cs = $this->_interface->SwitchingElementService($iid.'_playlist')->GetCurrentMedia();
    if (!$cs->success_) {
      return $cs->error_;
    }

    $media = $cs->result_;
    return null;
  }
}
?>
