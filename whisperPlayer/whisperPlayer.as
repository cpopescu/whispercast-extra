// Copyright (c) 2009, Whispersoft s.r.l.
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
// * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
// * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
// * Neither the name of Whispersoft s.r.l. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

package {
import fl.transitions.*;
import fl.transitions.easing.*;

import flash.external.*;

import flash.ui.*;
import flash.events.*;
import flash.text.*;
import flash.utils.*;
import flash.net.*;
import flash.display.*;

import whispercast.*;
import whispercast.player.*;

public class whisperPlayer extends BasePlayer {
	protected var player_pip_:Player;
	
	protected var player_main_:Player;
	protected var player_secondary_:Player;
	
	protected var player_secondary_closed_:Boolean = false;
	
	protected var control_bar_:ControlBar;
	protected var control_bar_created_:Boolean = false;
	
	protected var control_bar_visible_:Boolean = true;
	protected var control_bar_tween_:Tween = null;
	
	protected var secondary_x_:Number;
	protected var secondary_y_:Number;
	protected var secondary_width_:Number;
	protected var secondary_height_:Number;
	
	protected var hide_ui_timer_ms_:Number = 1000;
	protected var hide_ui_timer_id_:uint = 0;
	
	protected var show_ui_:Boolean = true;
	protected var static_ui_:Boolean = true;
	
	protected var locked_ui_:Boolean = false;
	
	protected var simple_ui_:Boolean = false;
	
	protected var has_mouse_:Boolean = false;
	protected var hide_mouse_with_ui_:Boolean = true;
	
	protected var popup_url_:String = null;
	
	protected var style_sheet_:StyleSheet = null;
	
	protected var play_icon_mc_:PlayIcon = null;
	protected var play_icon_disabled_:Boolean = false;
	
	protected var css_loaded_:Boolean = false;
	
	public function whisperPlayer() {
		player_main_ = new Player(0);
		player_main_.addEventListener(PlayerEvent.STATE, onPlayerChanged);
		player_main_.addEventListener(PlayerEvent.POSITION, onPlayerChanged);
		player_main_.addEventListener(PlayerEvent.CLICK, onPlayerClicked);
		
		player_main_.addEventListener(PlayerEvent.OSD, onPlayerOSD);
		
		player_secondary_ = new Player(1);
		player_secondary_.addEventListener(PlayerEvent.STATE, onPlayerChanged);
		player_secondary_.addEventListener(PlayerEvent.POSITION, onPlayerChanged);
		player_secondary_.addEventListener(PlayerEvent.CLICK, onPlayerClicked);
		player_secondary_.visible = false;
		
		// register the ExternalInterface stuff
		if (ExternalInterface.available) {
			ExternalInterface.addCallback('setURL_', setURL);
			ExternalInterface.addCallback('getURL_', getURL);
			ExternalInterface.addCallback('play_', play);
			ExternalInterface.addCallback('stop_', stop);
			ExternalInterface.addCallback('isPlaying_', isPlaying);
			ExternalInterface.addCallback('setVolume_', setVolume);
			ExternalInterface.addCallback('getVolume_', getVolume);
			ExternalInterface.addCallback('setMute_', setMute);
			ExternalInterface.addCallback('getMute_', getMute);
		}
		super();
		
		show_ui_ = loadBoolean('show_ui', -1, show_ui_);
		simple_ui_ = loadBoolean('simple_ui', -1, simple_ui_);
		
		play_icon_disabled_ = !loadBoolean('play_icon', undefined, true);
		
		if (show_ui_) {
			control_bar_ = simple_ui_ ? new ControlBarSimple() : new ControlBarNormal();
		}
		
		var tooltip:MovieClip = new Tooltip();
		tooltip.visible = false;
		addChild(tooltip);
		UI.initializeTooltipSupport(tooltip);
	}
	
	public function getPlayer(index:int) {
		switch (index) {
			case 0:
			  return player_main_;
			case 1:
			  return player_secondary_;
		}
		return null;
	}
	
	public function setMainPlayer(index:int, force:Boolean = false, start:Boolean = false) {
		var main, secondary;
		switch (index) {
			case 0:
			  main = player_main_;
			  secondary = player_secondary_;
			  
			  if (player_secondary_closed_) {
				setURL(1, null, null, false, false);
			  }
			  break;
			case 1:
			  main = player_secondary_;
			  secondary = player_main_;
			  break;
		}
		
		if (force || (main != player_)) {
			main.setMain(true, state_);
			secondary.setMain(false, state_);
			
			if (main != player_)
				swapChildren(main, secondary);
			
			player_ = main;
			player_pip_ = secondary;
			
			layout();
			
			main.buttonMode = (popup_url_ != null);
			secondary.buttonMode = true;
		
			_updatePiPState();
			
			if (start && !player_.isPlaying())
				player_.play();
			
			if (player_.isPlaying() || !player_.isControllable()) {
				play_icon_mc_.visible = false;
			} else {
				play_icon_mc_.visible = !play_icon_disabled_ && (player_.getURL().regular != null);
			}
		}
	}
	public function getMainPlayer():Player {
		return player_;
	}
		
	public function switchPlayers(start:Boolean = false) {
		if (player_ == player_main_) {
			setMainPlayer(1, false, start);
		} else {
			setMainPlayer(0, false, start);
		}
	}
	
	public function setURL(index:int, regular:String, standby:String, play:Boolean = true, show:Boolean = false, initial_position:Number = NaN) {
		switch (index) {
			case 0:
				player_main_.setURL(regular, standby, play, initial_position);
				break;
			case 1:
			  	player_secondary_closed_ = false;
				player_secondary_.visible = show;
				player_secondary_.setURL(regular, standby, play && show, initial_position);
				break;
		}
		_updatePiPState();
	}
	public function getURL(index:int):Object {
		switch (index) {
			case 0:
				return player_main_.getURL();
			case 1:
				return player_secondary_.getURL();
		}
		return null;
	}

	public function play(index:int) {
		var player:Player = getPlayer(index);
		if (player)
			player.play();
	}
	public function stop(index:int) {
		var player:Player = getPlayer(index);
		if (player)
			player.stop();
	}
	
	public function isPlaying(index:int):Boolean {
		var player:Player = getPlayer(index);
		if (player)
			return player.isPlaying();
		return false;
	}
	
	protected function _updatePiPState() {
		if (player_secondary_.getURL().regular != null) {
			state_.pip_state_ = player_pip_.visible ? CommonState.PIP_ACTIVE : CommonState.PIP_AVAILABLE;
		} else {
			state_.pip_state_ = CommonState.PIP_NONE;
		}
		dispatchEvent(new CommonEvent(CommonEvent.PIP, state_));
	}
	
	protected function _showUI(show:Boolean, force:Boolean = false) {
		if (locked_ui_ || (static_ui_ && !force))
			return;
			
		if (show != control_bar_visible_ || force)
			if (show) {
				control_bar_visible_ =
				control_bar_.visible = true;
				_animateControlBar();
				
				if (hide_mouse_with_ui_ && getFullscreen())
					Mouse.show();
			} else {
				control_bar_visible_ = false;
				_animateControlBar();
				
				if (hide_mouse_with_ui_ && getFullscreen())
					Mouse.hide();
			}
	}
	protected function _animateControlBar() {
		if (control_bar_tween_) {
			control_bar_tween_.continueTo(control_bar_visible_ ? stage.stageHeight-control_bar_.getHeight() : stage.stageHeight, NaN);
		} else {
			control_bar_tween_ = new Tween(control_bar_, 'y', Strong.easeOut, control_bar_.y, control_bar_visible_ ? stage.stageHeight-control_bar_.getHeight() : stage.stageHeight, 0.5, true);
			control_bar_tween_.addEventListener(TweenEvent.MOTION_FINISH, function(e:Object) {
				control_bar_tween_ = null;
				control_bar_.visible = control_bar_visible_;
			});	
		}
	}
	protected function _checkUIVisibility() {
		_showUI(true);
		
		if (hide_ui_timer_id_) {
			clearTimeout(hide_ui_timer_id_);
		}
		hide_ui_timer_id_ = setTimeout(function(){
			hide_ui_timer_id_ = 0;
			if (control_bar_created_) {
				if (control_bar_.canHide()) {
					_showUI(false);
				} else {
					_checkUIVisibility();
				}
			}
		},
		hide_ui_timer_ms_,
		this
		);
	}
	
	protected override function checkInitialize():void {
		if (!css_loaded_) {
			return;
		}
		super.checkInitialize();
	}
	
	protected override function create():void {
		whispercast.Logger.enable('player:state', 'player:playhead');
		
		var logger:String = loadString('logger', -1, null);
		if (logger) {
			Logger.addLogger('external', function(message:String) { ExternalInterface.call(logger, message); });
		}
		
		static_ui_ = loadBoolean('static_ui', -1, static_ui_);
		
		hide_mouse_with_ui_ = loadBoolean('hide_mouse_with_ui', -1, hide_mouse_with_ui_);
		
		hardware_accelerated_fullscreen_ = loadBoolean('hardware_accelerated_fullscreen', -1, hardware_accelerated_fullscreen_);
		
		secondary_x_ = loadNumber('secondary_x', -1, 0, 1, 0.95);
		secondary_y_ = loadNumber('secondary_y', -1, 0, 1, 0.05);
		secondary_width_ = loadNumber('secondary_width', -1, 0, 1, 0.25);
		secondary_height_ = loadNumber('secondary_height', -1, 0, 1, 0.25);

		popup_url_ = loadString('popup_url', -1, null);
		
		player_ = player_main_;
		player_pip_ = player_secondary_;

		var p:Object = {};
		p.url_mapper = loadString('url_mapper', -1, undefined);
		p.placeholder_mapper = loadString('placeholder_mapper', -1, undefined);
		p.progress_callback = loadString('progress_callback', -1, undefined);
		p.media_callback = loadString('media_callback', -1, undefined);
		p.buffer_time_while_connecting = loadNumber('buffer_time_while_connecting', -1, 0, NaN, 1);
		p.buffer_time_while_playing = loadNumber('buffer_time_while_playing', -1, 0, NaN, 0.1);
		p.video_align_x = loadString('video_align_x', -1, 'center');
		p.video_align_y = loadString('video_align_y', -1, 'center');
		p.seek_pos_key = loadString('seek_pos_key', -1, 'wsp');
		p.font_size = loadNumber('font_size', -1, 5, NaN, 12);
		p.show_osd = loadBoolean('show_osd', -1, true);
		p.always_show_osd = loadBoolean('always_show_osd', -1, false);
		p.clear_osd_on_play = loadBoolean('clear_osd_on_play', -1, true);
		p.restart_timeout = loadNumber('restart_timeout', -1, 0, NaN, NaN);
		p.restore_btn_x = loadNumber('restore_btn_x', -1, 0, 1, 1);
		p.restore_btn_y = loadNumber('restore_btn_y', -1, 0, 1, 0);
		p.addons = loadObject('addons', -1, null);
		whispercast.Logger.log('addons:addons', 'Addons: '+whispercast.Logger.stringify(p.addons));
		
		var p0:Object = {};
		p0.url_mapper = loadString('url_mapper', 0, p.url_mapper);
		p0.placeholder_mapper = loadString('placeholder_mapper', 0, p.placeholder_mapper);
		p0.progress_callback = loadString('progress_callback', 0, p.progress_callback);
		p0.media_callback = loadString('media_callback', 0, p.media_callback);
		p0.buffer_time_while_connecting = loadNumber('buffer_time_while_connecting', 0, 0, NaN, p.buffer_time_while_connecting);
		p0.buffer_time_while_playing = loadNumber('buffer_time_while_playing', 0, 0, NaN, p.buffer_time_while_playing);
		p0.video_align_x = loadString('video_align_x', 0, p.video_align_x);
		p0.video_align_y = loadString('video_align_y', 0, p.video_align_y);
		p0.seek_pos_key = loadString('seek_pos_key', 0, p.seek_pos_key);
		p0.font_size = loadNumber('font_size', 0, 5, NaN, p.font_size);
		p0.show_osd = loadBoolean('show_osd', 0, p.show_osd);
		p0.always_show_osd = loadBoolean('always_show_osd', 0, p.always_show_osd);
		p0.clear_osd_on_play = loadBoolean('clear_osd_on_play', 0, p.clear_osd_on_play);
		p0.restart_timeout = loadNumber('restart_timeout', 0, 0, NaN, p.restart_timeout);
		p0.restore_btn_x = loadNumber('restore_btn_x', 0, 0, 1, p.restore_btn_x);
		p0.restore_btn_y = loadNumber('restore_btn_y', 0, 0, 1, p.restore_btn_y);
		p0.addons = loadObject('addons', 0, p.addons);
		if (style_sheet_) {
			p0.style_sheet = style_sheet_;
		}
		player_main_.create(p0);
		addChild(player_main_);
		
		var p1:Object = {};
		p1.url_mapper = loadString('url_mapper', 1, p.url_mapper);
		p1.placeholder_mapper = loadString('placeholder_mapper', 1, p.placeholder_mapper);
		p1.progress_callback = loadString('progress_callback', 1, p.progress_callback);
		p1.media_callback = loadString('media_callback', 1, p.media_callback);
		p1.buffer_time_while_connecting = loadNumber('buffer_time_while_connecting', 1, 0, NaN, p.buffer_time_while_connecting);
		p1.buffer_time_while_playing = loadNumber('buffer_time_while_playing', 1, 0, NaN, p.buffer_time_while_playing);
		p1.video_align_x = loadString('video_align_x', 1, p.video_align_x);
		p1.video_align_y = loadString('video_align_y', 1, p.video_align_y);
		p1.seek_pos_key = loadString('seek_pos_key', 1, p.seek_pos_key);
		p1.font_size = loadNumber('font_size', 1, 5, NaN, p.font_size);
		p1.show_osd = loadBoolean('show_osd', 1, p.show_osd);
		p1.always_show_osd = loadBoolean('always_show_osd', 1, p.always_show_osd);
		p1.clear_osd_on_play = loadBoolean('clear_osd_on_play', 1, p.clear_osd_on_play);
		p1.restart_timeout = loadNumber('restart_timeout', 1, 0, NaN, p.restart_timeout);
		p1.restore_btn_x = loadNumber('restore_btn_x', 1, 0, 1, p.restore_btn_x);
		p1.restore_btn_y = loadNumber('restore_btn_y', 1, 0, 1, p.restore_btn_y);
		p1.addons = loadObject('addons', 1, p.addons);
		if (style_sheet_) {
			p1.style_sheet = style_sheet_;
		}
		player_secondary_.create(p1);
		addChild(player_secondary_);
		
		play_icon_mc_ = new PlayIcon();
		addChild(play_icon_mc_);
		
		play_icon_mc_.buttonMode = true;
		play_icon_mc_.visible = false;
		
		play_icon_mc_.addEventListener(MouseEvent.CLICK, function(e) {
			if (play_icon_mc_.visible) {
				player_.play();
			}
		});
		
		super.create();
	}
	
	protected override function loaded():void {
		var css_url:String = loadString('css_url', -1, null);
		if (css_url) {
			var loader:URLLoader = new URLLoader();
			try {
				var s = super;
				loader.addEventListener(IOErrorEvent.IO_ERROR,
					function(e:IOErrorEvent) {
						css_loaded_ = true;
						s.loaded();
					}
				);
				loader.addEventListener(Event.COMPLETE,
					function(e:Event) {
						style_sheet_ = new StyleSheet();
						style_sheet_.parseCSS(loader.data);
						
						css_loaded_ = true;
						s.loaded();
					}
				);
				
	            loader.load(new URLRequest(css_url));
			} catch(e) {
				css_loaded_ = true;
				super.loaded();
			}
		} else {
			css_loaded_ = true;
			super.loaded();
		}
	}
	
	protected override function initialize():void {
		super.initialize();
		
		stage.addEventListener(MouseEvent.MOUSE_MOVE, onStageMouseMove);
		stage.addEventListener(Event.MOUSE_LEAVE, onStageMouseLeave);
		stage.addEventListener(MouseEvent.CLICK, onStageMouseClick, true);
		
		if (show_ui_) {
			control_bar_.create(
				this,
				{
					callbacks: {
						play_pause: {
							callback: function(ctrl:MovieClip, kind:String) {
								switch (kind) {
									case 'click':
										if (player_.isPlaying()) {
											player_.stop();
											player_pip_.stop();
										} else {
											player_.play();
										}
										break;
								}
							}
						},
						
						mute: {
							callback: function(ctrl:MovieClip, kind:String) {
								switch (kind) {
									case 'click':
										setMute(!getMute());
										break;
								}
							}
						},
						
						fullscreen: {
							callback: function(ctrl:MovieClip, kind:String) {
								switch (kind) {
									case 'click':
										setFullscreen(!getFullscreen());
										break;
								}
							}
						},
						
						pip: {
							callback: function(ctrl:MovieClip, kind:String) {
								switch (kind) {
									case 'click':
										if (player_pip_.getURL().regular != null) {
											player_pip_.visible = !player_pip_.visible;
											if (player_pip_.visible)
												player_pip_.play();
											else
												player_pip_.stop();
											_updatePiPState();
										}
										break;
								}
							}
						},
						
						volume: {
							callback: function(bar:MovieClip, volume:Number, is_dragging:Boolean) {
								player_.setVolume(state_.volume_ = volume, false);
		
								if (state_.muted_) {
									state_.muted_ = false;
									dispatchEvent(new CommonEvent(CommonEvent.VOLUME, state_));
								}
							}
						},
						
						seek: {
							callback: function(bar:MovieClip, percent:Number, is_dragging:Boolean) {
								if (!is_dragging) {
									player_.seekRelative(percent);
								}
							}
						}
					}
				}
			);
			
			addChild(control_bar_);
			control_bar_created_ = true;
		}
		
		setMainPlayer(0, true);
		
		dispatchEvent(new CommonEvent(CommonEvent.VOLUME, state_));
		dispatchEvent(new CommonEvent(CommonEvent.PIP, state_));

		var url0:String = loadString('url', 0, null);
		var url0_standby:String = loadString('url_standby', 0, null);
		var url1:String = loadString('url', 1, null);
		var url1_standby:String = loadString('url_standby', 1, null);
		
//		url0 = 'http://192.168.2.198/pinoccio2.m4v';
//		url0 = 'http://192.168.2.198/Spiderman3_trailer_300.flv';
//		url0 = 'http://localhost/video.flv';
//		url0 = 'http://192.168.2.199:2181/events_f4v/samples/backcountry.f4v';
		
		var play_on_load:Boolean = loadBoolean('play_on_load', -1, false);
		if (url0 != null || url0_standby != null) {
			setURL(0, url0, url0_standby, loadBoolean('play_on_load', 0, play_on_load), true);
		}
		if (url1 != null || url1_standby != null) {
			setURL(1, url1, url1_standby, loadBoolean('play_on_load', 1, play_on_load), true);
		}

		layout();
		_checkUIVisibility();
	}
	protected override function layout():void {
		var stage_w:Number = stage.stageWidth;
		var stage_h:Number = stage.stageHeight;
		var control_bar_h:Number = 0;
		if (static_ui_ && control_bar_created_ && !simple_ui_) {
			control_bar_h = control_bar_.getHeight();
		}
		stage_h -= control_bar_h;
		
		var main, secondary;
		if (player_ == player_main_) {
			main = player_main_;
			secondary = player_secondary_;
		} else {
			main = player_secondary_;
			secondary = player_main_;
		}
		
		var sec_w:Number;
		var sec_h:Number;	
		
		sec_w = stage_w*secondary_width_;
		sec_h = stage_h*secondary_height_;
		secondary.layout(secondary_x_*(stage_w-sec_w), secondary_y_*(stage_h-sec_h), sec_w, sec_h, 0);

		main.layout(0, 0, stage_w, stage_h, control_bar_h);
		
		if (control_bar_created_) {
			var animate:Boolean = false;
			if (control_bar_tween_) {
				control_bar_tween_.time = 1e10;
				animate = true;
			}
			if (simple_ui_) {
				control_bar_.layout(4, stage.stageHeight-control_bar_.getHeight()-4, stage.stageWidth, control_bar_.getHeight());
			} else {
				control_bar_.layout(0, stage.stageHeight-control_bar_.getHeight(), stage.stageWidth, control_bar_.getHeight());
			}
			if (animate) {
				_animateControlBar();
			}
		}
		
		play_icon_mc_.width =
		play_icon_mc_.height = Math.min(Math.max(20, 60*stage_w/320), Math.max(20, 60*stage_h/240));
		play_icon_mc_.x = (stage_w - play_icon_mc_.width)/2;
		play_icon_mc_.y = (stage_h - play_icon_mc_.height)/2;
	}
	
	protected function onPlayerChanged(e:PlayerEvent):void {
		if (e.player_ == player_) {
			if (player_.isPlaying() || !player_.isControllable()) {
				play_icon_mc_.visible = false;
			} else {
				play_icon_mc_.visible = !play_icon_disabled_ && player_.isInitialized() && (player_.getURL().regular != null);
			}
		}
		if (locked_ui_) {
			if (e.player_.isControllable()) {
				locked_ui_ = false;
				_showUI(true, true);
			}
		} else {
			if (!e.player_.isControllable()) {
				_showUI(false, true);
				locked_ui_ = true;
			}
		}
		dispatchEvent(e.clone());
	}
	protected function onPlayerClicked(e:PlayerEvent):void {
		if (e.player_ == ((player_ == player_main_) ? player_secondary_ : player_main_)) {
			switchPlayers(true);
		} else {
			if (popup_url_) {
				navigateToURL(new URLRequest(popup_url_), '_blank');
			}
		}
	}
	
	protected function onPlayerOSD(e:PlayerEvent):void {
		var p:Object = e.params_.params_;
		switch (e.params_.type_) {
			case 'SetPictureInPicture':
				if (e.player_ == player_main_) {
					var do_layout:Boolean = false;
					if (p.x_location != undefined) {
						secondary_x_ = p.x_location;
						do_layout = true;
					}
					if (p.y_location != undefined) {
						secondary_y_ = p.y_location;
						do_layout = true;
					}
					if (p.width != undefined) {
						secondary_width_ = p.width;
						do_layout = true;
					}
					if (p.height != undefined) {
						secondary_height_ = p.height;
						do_layout = true;
					}
					
					if (do_layout) {
						layout();
					}
					
					if ((p.url != undefined) && (p.url != '')) {
						player_secondary_closed_ = false;
						if (player_secondary_.getURL().regular != p.url) {
							setURL(1, p.url, null, (p.play != undefined) ? p.play : true, true);
						}
						else
							if (p.play != undefined) {
								if (p.play) {
									if (!player_secondary_.isPlaying())
										player_secondary_.play();
								}
								else
									player_secondary_.stop();
							}
					}
					else {
						player_secondary_closed_ = true;
						if (player_ != player_secondary_) {
							setURL(1, null, null, false, false);
						}
					}
				}
				break;
		}
	}
	
	protected function onPlayerCreateMovie(e:PlayerEvent):void {
	}
	protected function onPlayerDestroyMovie(e:PlayerEvent):void {
	}
	
	protected function onStageMouseMove(e:MouseEvent):void {
		has_mouse_ = true;
		_checkUIVisibility();
	}
	protected function onStageMouseLeave(e:Event):void {
		has_mouse_ = false;
		_checkUIVisibility();
	}
	protected function onStageMouseClick(e:MouseEvent):void {
		if (hide_mouse_with_ui_)
			if (!control_bar_visible_) {
				_checkUIVisibility();
				return;
			}
	}
}
}