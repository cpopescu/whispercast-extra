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

package whispercast.player
{
import flash.external.*;

import flash.geom.*;
import flash.display.*;
import flash.media.*;
import flash.net.*;
import flash.events.*;
import flash.utils.*;

import flash.filters.*;
	
import fl.transitions.*;
import fl.transitions.easing.*;

import whispercast.*;
import whispercast.player.*;
import whispercast.osd.*;
import whispercast.addons.*;

// Force the import of the addon classes...
whispercast.addons.LiveRail;

public class Player extends Sprite {
	protected var initialized_:Boolean = false;
	
	protected var url_mapper_:String = null;
	protected var placeholder_mapper_:String = null;
	
	protected var url_regular_:String = null;
	protected var url_standby_:String = null;
	
	protected var url_current_:String = null;
	protected var url_current_parsed_:ParsedURL = null;
	
	protected var url_placeholder_:String = null;
	
	protected var progress_callback_:String = null;
	protected var media_callback_:String = null;
	
	protected var current_media_:String = null;
	
	protected var background_mc_:Sprite;
	protected var frame_mc_:Sprite;
	
	protected var placeholder_mc_:Loader = null;
	
	protected var placeholder_width_:Number = 0;
	protected var placeholder_height_:Number = 0;
	
	protected var buffering_mc_:BufferingIcon;
	
	protected var x_:Number;
	protected var y_:Number;
	protected var width_:Number;
	protected var height_:Number;
	protected var control_bar_h_:Number;
	
	protected var video_align_x_:String = 'center';
	protected var video_align_y_:String = 'center';
	
	protected var seek_pos_current_:Number = NaN;
	protected var seek_pos_key_:String = 'wsp';
	
	protected var clear_osd_on_play_:Boolean = true;
	
	protected var buffer_time_while_connecting_:Number = 0.1;
	protected var buffer_time_while_playing_:Number = 0.1;
	
	protected var show_osd_:Boolean = true;
	protected var always_show_osd_:Boolean = false;
		
	protected var state_:PlayerState;
	protected var stopped_:Boolean = true;
	
	protected var stop_on_first_frame_:Boolean = false;
	
	protected var buffering_:Boolean = false;
	protected var opening_:Boolean = false;
	protected var closing_:Boolean = false;
	
	protected var json_:JSON;
	protected var scripting_hooks_:Object;
	
	protected var osd_:Osd;
	
	protected var initialize_timeout_:Number = 1;
	protected var initialize_timer_id_:uint = 0;
	
	protected var restart_timeout_:Number = 1;
	protected var restart_timer_id_:uint = 0;
	
	protected var seek_timeout_:Number = 0.05;
	protected var seek_timer_id_:uint = 0;
	
	protected var seek_pending_:Number = NaN;
	
	protected var playhead_update_interval_:Number = 0.3;
	protected var playhead_timer_id_:uint = 0;
	
	protected var playhead_offset_:Number = NaN;
	protected var playhead_locked_:Boolean = false;
	
	protected var video_:Video = null;
	protected var audio_:SoundTransform = null;
	
	protected var ns_:NetStream = null;
	protected var nc_:NetConnection = null;
	
	protected var movies_:Object = {};
	protected var movies_holder_mc_:Sprite = null;

	protected var addons_:Object = {};
	
	protected var click_url_:String = null;
	
	public function Player(index:int = 0) {
		state_ = new PlayerState();
		state_.index_ = index;
	}
	
	private function notifyListeners(event:String, params:Object = null) {
		dispatchEvent(new PlayerEvent(event, {regular:url_regular_, standby:url_standby_}, state_, this, params));
	}
	
	public function create(p:Object) {
		url_mapper_ = (p.url_mapper != undefined) ? p.url_mapper : null;
		placeholder_mapper_ = (p.placeholder_mapper != undefined) ? p.placeholder_mapper : null;
		
		progress_callback_ = (p.progress_callback != undefined) ? p.progress_callback : null;
		media_callback_ = (p.media_callback != undefined) ? p.media_callback : null;
		
		video_align_x_ = (p.video_align_x != undefined) ? p.video_align_x : video_align_x_;
		video_align_y_ = (p.video_align_y != undefined) ? p.video_align_y : video_align_y_;
		
		clear_osd_on_play_ = (p.clear_osd_on_play != undefined) ? p.clear_osd_on_play : clear_osd_on_play_;
		
		seek_pos_key_ = (p.seek_pos_key != undefined) ? p.seek_pos_key : seek_pos_key_;

		buffer_time_while_connecting_ = (!isNaN(p.buffer_time_while_connecting)) ? p.buffer_time_while_connecting : buffer_time_while_connecting_;
		buffer_time_while_playing_ = (!isNaN(p.buffer_time_while_playing)) ? p.buffer_time_while_playing : buffer_time_while_playing_;
		
		show_osd_ = (p.show_osd != undefined) ? p.show_osd : show_osd_;
		always_show_osd_ = (p.always_show_osd != undefined) ? p.always_show_osd : always_show_osd_;
	
		restart_timeout_ = (!isNaN(p.restart_timeout)) ? p.restart_timeout : restart_timeout_;
		seek_timeout_ = (!isNaN(p.seek_timeout)) ? p.seek_timeout : seek_timeout_;
		playhead_update_interval_ = (!isNaN(p.playhead_update_interval)) ? p.playhead_update_interval : playhead_update_interval_;
		
		restart_timeout_ = (!isNaN(p.restart_timeout)) ? p.restart_timeout : restart_timeout_;
		
		background_mc_ = new Sprite();
		addChild(background_mc_);

		placeholder_mc_ = new Loader();
		placeholder_mc_.visible = false;
		addChild(placeholder_mc_);
		
		video_ = new Video();
		video_.smoothing = true;
		addChild(video_);
		
		frame_mc_ = new Sprite();
		addChild(frame_mc_);
		
		audio_ = new SoundTransform();
		audio_.volume = 0;

		buffering_mc_ = new BufferingIcon();
		addChild(buffering_mc_);
		
		buffering_mc_.visible = false;
		
		json_ = new JSON();
		scripting_hooks_ = new Object();
		
		scripting_hooks_['flash'] = new Object();
		scripting_hooks_['javascript'] = new Object();
		
		// create the OSD only if requested
		if (show_osd_) {
			osd_ = new Osd(p);
			addChild(osd_);
			
			// enumerate all the osd related interface methods and hook them up
			var d = describeType(osd_)..method;
			for (var i in d) {
				var n:String = d[i].@name;
				if (n.search('_osd_') == 0) {
					scripting_hooks_['flash'][n.substr(5)] = {f:osd_[n], c:osd_};
				}
			}
			
			// enumerate our osd related interface methods and hook them up
			d = describeType(this)..method;
			for (i in d) {
				n = d[i].@name;
				if (n.search('_osd_') == 0) {
					scripting_hooks_['flash'][n.substr(5)] = {f:this[n], c:this};
				}
			}
			
			// register the ExternalInterface stuff
			if (ExternalInterface.available) {
				ExternalInterface.addCallback('call'+state_.index_+'_', _call);
			}
		}
		
		addEventListener(MouseEvent.CLICK, _onMouseClick);
		
		if (p.addons != undefined) {
			setTimeout(function() {
				for (i in p.addons) {
					try {
						var aclass:Class = getDefinitionByName('whispercast.addons.'+i) as Class;
						addons_[i] = new aclass(p.addons[i]);
						addons_[i].addEventListener(AddonEvent.STATE, _onAddonEvent);
						
						addons_[i].initialize();
					} catch(e) {
					}
				}
			}, 1);
		} else {
			setTimeout(function() {
				initialized_ = true;
				notifyListeners(PlayerEvent.STATE);
			}, 1);
		}
		
		/* TODO: Proxy mouse events to the player..
		movies_holder_mc_ = new Sprite();
		addChild(movies_holder_mc_);
		*/
		movies_holder_mc_ = this;
	}
	
	protected function _call(t:String, f:String) {
		if (scripting_hooks_[t] != undefined) {
			if (scripting_hooks_[t][f] != undefined) {
				var args:Array = new Array();
				for (var i = 2; i < arguments.length; i++) {
					args.push(arguments[i]);
				}
				scripting_hooks_[t][f].f.apply(scripting_hooks_[t][f].c, args);
			}
		}
	}
	
	protected function _layoutMovie(id:String):void {
		whispercast.Logger.log('player:layout','Player::_layoutMovie(): '+id);
		if (movies_[id]) {
			if (movies_[id].mask_) {
				movies_holder_mc_.removeChild(movies_[id].mask_);
			}
			
			var m:Loader = movies_[id].movie_;
			var p:Object = movies_[id].parameters_;
			
			var ow:Number = m.content.width;
			var oh:Number = m.content.height;
			
			var mw:Number;
			var mh:Number;
			
			if (p.width_ == undefined && p.height_ == undefined) {
				mw = ow;
				mh = oh;
			} else
			if (p.width_ == undefined) {
				mh = height_*p.height_;
				mw = ow*mh/oh;
			} else
			if (p.height_ == undefined) {
				mw = width_*p.width_;
				mh = oh*mw/ow;
			} else {
				mw = width_*p.width_;
				mh = height_*p.height_;
			}
			
			var mx:Number = (width_ - mw)*p.x_location_;
			var my:Number = (height_ - mh)*p.y_location_;
			
			movies_[id].mask_ = new Sprite()
			movies_[id].mask_.graphics.clear();
			movies_[id].mask_.graphics.beginFill(0xff0000, 0.5);
			movies_[id].mask_.graphics.drawRect(0, 0, mw, mh);
			movies_[id].mask_.graphics.endFill();
			
			movies_[id].mask_.x = mx;
			movies_[id].mask_.y = my;
			
			movies_holder_mc_.addChild(movies_[id].mask_);
			m.mask = movies_[id].mask_;
			
			m.width = mw;
			m.height = mh;
			
			m.x = mx;
			m.y = my;
			
			movies_holder_mc_.addChild(m);
	
			whispercast.Logger.log('player:layout','Player::_layoutMovie(): '+mx+','+my+','+mw+','+mh);
			m.visible = true;
		}
	}
	protected function _layoutPlaceholder(show):void {
		if (placeholder_width_ > 0 && placeholder_height_ > 0) {
			var scale:Number = Math.min(width_/placeholder_width_, height_/placeholder_height_);
			
			placeholder_mc_.x = (width_ - placeholder_width_*scale)/2;
			placeholder_mc_.y = (height_ - placeholder_height_*scale)/2;
			
			placeholder_mc_.scaleX =
			placeholder_mc_.scaleY = scale;
			
			placeholder_mc_.visible = (show != null) ? show : placeholder_mc_.visible;
		}
	}
	protected function _layoutAddon(id:String):void {
		var w:Number = state_.is_main_ ? stage.stageWidth : width_;
		var h:Number = state_.is_main_ ? stage.stageHeight: height_;
		addons_[id].layout(0, 0, w, h, control_bar_h_);
	}
	
	public function layout(ax:Number, ay:Number, w:Number, h:Number, control_bar_h:Number):void {
		whispercast.Logger.log('player:layout','Player::layout() - Initial ['+ax+','+ay+', '+(ax+w)+','+(ay+h)+', '+scaleX+','+scaleY+']');
		
		control_bar_h_ = control_bar_h;
		
		var id:String;
		for (id in movies_) {
			try {
				movies_holder_mc_.removeChild(movies_[id].movie);
			} catch (e) {
			}
		}
		
		background_mc_.graphics.clear();
		background_mc_.graphics.beginFill(0x000000, state_.is_main_ ? 0 : 0.5);
		background_mc_.graphics.drawRect(0, 0, w, h);
		background_mc_.graphics.endFill();
		
		movies_holder_mc_.graphics.clear();
		movies_holder_mc_.graphics.beginFill(0x000000, 0);
		movies_holder_mc_.graphics.drawRect(0, 0, w, h);
		movies_holder_mc_.graphics.endFill();

		if (state_.is_main_) {
			frame_mc_.visible = false;
		} else {
			frame_mc_.graphics.clear();
			frame_mc_.graphics.lineStyle(1, 0x606060);
			frame_mc_.graphics.drawRect(-1, -1, w+1, h+1);
			frame_mc_.graphics.endFill();
			frame_mc_.visible = true;
		}
		
		if (!state_.is_main_) {
			var sf:DropShadowFilter = new DropShadowFilter();
			sf.distance = 0;
			sf.blurX = 3;
			sf.blurY = 3;
			sf.quality = 5;
				
			frame_mc_.filters = [
				sf
			];
		}
		
		buffering_mc_.x = w/2;
		buffering_mc_.y = h/2;
		buffering_mc_.width =
		buffering_mc_.height = Math.min(Math.max(20, 60*w/320), Math.max(20, 60*h/240));
		
		x = x_ = ax;
		y = y_ = ay;
		width_ = w;
		height_ = h;
		
		_updateVideoSize();
		
		_layoutPlaceholder(null);
		
		for (id in movies_) {
			_layoutMovie(id);
		}
		for (id in addons_) {
			_layoutAddon(id);
		}
		
		whispercast.Logger.log('player:layout','Player::layout() - Final ['+x+','+y+', '+(x+width)+','+(y+height)+', '+scaleX+','+scaleY+']');
	}
	
	public function setMain(main:Boolean, state:CommonState, force:Boolean = false):void {
		if (force || (main != state_.is_main_)) {
			state_.is_main_ = main;
			
			if (state_.is_main_)
				if (state.muted_) {
					setVolume(0, true);
				} else {
					setVolume(state.volume_, false);
				}
			else
				setVolume(0, false);
				
			
			notifyListeners(PlayerEvent.STATE);
			notifyListeners(PlayerEvent.POSITION);
		}
	}
	
	public function setURL(regular:String, standby:String, start:Boolean = true, initial_position:Number = NaN):void {
		url_regular_ = (regular == '') ? null : regular;
		url_standby_ = (standby == '') ? null : standby;
		
		url_placeholder_ = null;
		if (placeholder_mapper_ != null) {
			var mapped:String = ExternalInterface.call(placeholder_mapper_, regular);
			if (mapped != '') {
				if (url_placeholder_ != mapped) {
					url_placeholder_ = mapped;
					
					placeholder_width_ =
					placeholder_height_ = 0;
				}
			} else {
				placeholder_width_ =
				placeholder_height_ = 0;
			}
		} else {
			placeholder_width_ =
			placeholder_height_ = 0;
		}
		
		seek_pos_current_ = NaN;
			
		state_.length_ = NaN;
		
		notifyListeners(PlayerEvent.STATE);
		notifyListeners(PlayerEvent.POSITION);
		
		if (start) {
			if (!stopped_) {
				stop();
			}
			play();
		}
		else {
			stopped_ = false;
			stop();
		}
	}
	public function getURL():Object{
		return {regular:url_regular_, standby:url_standby_};
	}
	
	public function setVolume(volume:Number, muted:Boolean):void {
		audio_.volume = volume;
		if (ns_ != null) {
			ns_.soundTransform = audio_;
		}
		
		for (var id in addons_) {
			addons_[id].onSetVolume(volume, muted);
		}
	}
	public function getVolume():Number {
		return audio_.volume;	
	}
	
	public function isInitialized():Boolean {
		return initialized_;
	}
	
	public function isControllable():Boolean {
		return initialized_ && (state_.state_ != PlayerState.PREROLL) && (state_.state_ != PlayerState.POSTROLL);
	}
	
	public function isStopped():Boolean {
		return stopped_;
	}
	public function isPlaying():Boolean {
		return initialized_ && !stopped_ && ((state_.state_ == PlayerState.LOADING) || (state_.state_ == PlayerState.PLAYING) || (state_.state_ == PlayerState.SEEKING));
	}
	public function isPaused():Boolean {
		return initialized_ && !stopped_ && (state_.state_ == PlayerState.PAUSED);
	}
	
	public function play():void {
		if (stopped_) {
			whispercast.Logger.log('player:state','Player::play() - state: '+state_.state_);
			stopped_ = false;
			
			_setCurrentURL(url_regular_);
			stop_on_first_frame_ = false;
			
			if (url_current_parsed_ != null) {
				var preroll:Boolean = false;
				for (var id in addons_) {
					if (addons_[id].onPlay()) {
						preroll = true;
					}
				}
				if (preroll) {
					_setState(PlayerState.PREROLL);
				}
				
				_open();
			}
		}
	}
	public function stop():void {
		if (!stopped_) {
			whispercast.Logger.log('player:state','Player::stop()');
			stopped_ = true;
		
			var postroll:Boolean = false;
			for (var id in addons_) {
				if (addons_[id].onStop()) {
					postroll = true;
				}
			}
			
			if (postroll) {
				_setState(PlayerState.POSTROLL);
			}
			
			_close();
		}
	}
	
	private function pause():void {
		if (ns_) {
			_setState(PlayerState.PAUSED);
			
			ns_.pause();
			_updateBuffering();
		}
	}
	private function resume():void {
		if (ns_) {
			_setState(PlayerState.PLAYING);
			
			ns_.resume();
			_updateBuffering();
		}
	}
	
	public function seek(position:Number, instant:Boolean = false) {
		_seekClear();
		_lockPlayhead(true);
		
		seek_pending_ = position;
		_updateBuffering();
		
		if (instant)
			_seekPerform();
		else {
			seek_timer_id_ = flash.utils.setTimeout(function() {seek_timer_id_=0;_seekPerform()}, 1000*seek_timeout_);
		}
	}
	public function seekRelative(percent:Number, instant:Boolean = false) {
		if (!isNaN(state_.length_) && (state_.length_ > 0)) {
			seek(percent*state_.length_, instant);
		}
	}
	
	protected function _setStateCheck(state:String):Boolean {
		switch (state_.state_) {
			case PlayerState.PREROLL:
				switch (state) {
					case PlayerState.PREROLL:
						return false;
					case PlayerState.LOADING:
						return true;
					case PlayerState.PLAYING:
					case PlayerState.POSTROLL:
						return false;
					case PlayerState.STOPPED:
						return true;
					case PlayerState.PAUSED:
					case PlayerState.SEEKING:
						return false;
				}
				break;
			case PlayerState.LOADING:
				switch (state) {
					case PlayerState.PREROLL:
					case PlayerState.LOADING:
						return false;
					case PlayerState.PLAYING:
					case PlayerState.POSTROLL:
					case PlayerState.STOPPED:
						return true;
					case PlayerState.PAUSED:
					case PlayerState.SEEKING:
						return false;
				}
				break;
			case PlayerState.PLAYING:
				switch (state) {
					case PlayerState.PREROLL:
					case PlayerState.LOADING:
					case PlayerState.PLAYING:
						return false;
					case PlayerState.POSTROLL:
					case PlayerState.STOPPED:
					case PlayerState.PAUSED:
					case PlayerState.SEEKING:
						return true;
				}
				break;
			case PlayerState.POSTROLL:
				switch (state) {
					case PlayerState.PREROLL:
					case PlayerState.LOADING:
					case PlayerState.PLAYING:
					case PlayerState.POSTROLL:
						return false;
					case PlayerState.STOPPED:
						return true;
					case PlayerState.PAUSED:
					case PlayerState.SEEKING:
						return false;
				}
				break;
			case PlayerState.STOPPED:
				switch (state) {
					case PlayerState.PREROLL:
					case PlayerState.LOADING:
						return true;
					case PlayerState.PLAYING:
					case PlayerState.POSTROLL:
					case PlayerState.STOPPED:
					case PlayerState.PAUSED:
					case PlayerState.SEEKING:
						return false;
				}
				break;
			case PlayerState.PAUSED:
				switch (state) {
					case PlayerState.PREROLL:
					case PlayerState.LOADING:
						return false;
					case PlayerState.PLAYING:
					case PlayerState.POSTROLL:
					case PlayerState.STOPPED:
						return true;
					case PlayerState.PAUSED:
					case PlayerState.SEEKING:
						return false;
				}
				break;
			case PlayerState.SEEKING:
				switch (state) {
					case PlayerState.PREROLL:
					case PlayerState.LOADING:
						return false;
					case PlayerState.PLAYING:
					case PlayerState.POSTROLL:
					case PlayerState.STOPPED:
						return true;
					case PlayerState.PAUSED:
					case PlayerState.SEEKING:
						return false;
				}
				break;
		}
		return false;
	}
	protected function _setState(state:String):void {
		if (state_.state_ == state) {
			return;
		}
		if (!_setStateCheck(state)) {
			whispercast.Logger.log('player:state','Player::_setState('+state+') is invalid, current state is: '+state_.state_);
			return;
		}
		whispercast.Logger.log('player:state','Player::_setState('+state+')');

		var previous:String = state_.state_;
		state_.state_ = state;
		
		switch (state) {
			case PlayerState.PREROLL:
			case PlayerState.LOADING:
				if (clear_osd_on_play_) {
					if (osd_) {
						osd_.reset();
					}
					setClickUrlHelper(null);
				}
				for (var id in movies_) {
					_destroyMovieHelper(id);
				}
				buffering_ = true;
				_lockPlayhead(true);
				break;
			case PlayerState.PLAYING:
				video_.visible = true;
				placeholder_mc_.visible = false;
				_lockPlayhead(false);
				break;
			case PlayerState.POSTROLL:
			case PlayerState.STOPPED:
				buffering_ = false;
				_lockPlayhead(true);
				break;
			case PlayerState.PAUSED:
				_lockPlayhead(true);
				break;
			case PlayerState.SEEKING:
				var time:Date = new Date();
				if (url_current_parsed_.is_rtmp_) {
					playhead_offset_ = seek_pending_ - time.getTime()/1000;
				} else {
					playhead_offset_ = 0;
				}
				break;
			default:
				whispercast.Logger.log('player:state','Player::_setState() - Unknown state: '+state_.state_);
		}
		
		notifyListeners(PlayerEvent.STATE);
		_updateBuffering();
	}

	protected function _setCurrentURL(url:String) {
		if (url != null) {
			if (url != url_current_) {
				if (osd_) {
					osd_.reset();
				}
				for (var id in movies_) {
					_destroyMovieHelper(id);
				}
				setClickUrlHelper(null);
			}

			url_current_parsed_ = ParsedURL.parse(url);
			if (url_current_parsed_ == null)
				url_current_ = null;
			else {
				if (url_current_parsed_.protocol_ == null) {
					if (url_mapper_ != null) {
						var mapped:String = ExternalInterface.call(url_mapper_, url);
						if (mapped != '') {
							url_current_parsed_ = ParsedURL.parse(mapped);
						}
					}
				}
			}
			
			if (url != null) {
				url_current_ = url;
			} else {
				url_current_ = null;
			}
		} else {
			url_current_parsed_ = null;
			url_current_ = null;
		}
	}
	
	protected function _cleanup() {
		whispercast.Logger.log('player:state','Player::_cleanup()');
		_seekClear();
		
		if (initialize_timer_id_) {
			flash.utils.clearTimeout(initialize_timer_id_);
			initialize_timer_id_ = 0;
		}
		if (restart_timer_id_) {
			flash.utils.clearTimeout(restart_timer_id_);
			restart_timer_id_ = 0;
		}
		
		if (playhead_timer_id_ != 0) {
			flash.utils.clearInterval(playhead_timer_id_);
			playhead_timer_id_ = 0;
		}
		
		if (nc_ != null) {
			nc_.close();
			nc_ = null;
		}
		if (ns_ != null) {
			ns_.close();
			ns_ = null;
		}
		video_.attachNetStream(null);
	}
	
	protected function _open() {
		if (!opening_) {
			whispercast.Logger.log('player:state','Player::_open()');
			current_media_ = null;
			
			state_.complete_ = false;
			if (url_current_parsed_ != null) {
				opening_ = true;
				_cleanup();
				
				playhead_offset_ =
				state_.position_ = NaN;
				
				if (state_.state_ != PlayerState.PREROLL) {
					_openPerform();
					return;
				}
			}
			notifyListeners(PlayerEvent.STATE);
		}
	}
	protected function _openPerform() {
		whispercast.Logger.log('player:state','Player::_openPerform()');
		if (!stopped_) {
			_setState(PlayerState.LOADING);
		}
		
		nc_ = new NetConnection();
		nc_.client = {
			onBWDone: function() {
				whispercast.Logger.log('player:state','Player::onBWDone');
			}
		};
		nc_.addEventListener(NetStatusEvent.NET_STATUS, onNetStatus);
		
		var url:String = null;
		if (url_current_parsed_.is_rtmp_) {
			url = url_current_parsed_.protocol_ + ((url_current_parsed_.host_ == null) ? '' : '/' + url_current_parsed_.host_ + ((url_current_parsed_.port_ == null) ? '' : (':' + url_current_parsed_.port_)) + '/') + url_current_parsed_.app_name_+'/';
		}
			
		whispercast.Logger.log('player:state','Player::_openPerform() - NetConnection.connect("'+url+'")');
		nc_.connect(url);
		
		opening_ = false;
	}
	
	protected function _close() {
		if (!closing_) {
			whispercast.Logger.log('player:state','Player::_close()');
			_cleanup();
		
			closing_ = true;
			if (state_.state_ != PlayerState.POSTROLL) {
				_closePerform();
			}
		}
	}
	protected function _closePerform() {
		whispercast.Logger.log('player:state','Player::_closePerform()');
		_setState(PlayerState.STOPPED);
		closing_ = false;
		
		if (url_standby_ != null) {
			_setCurrentURL(url_standby_);
			stop_on_first_frame_ = (url_standby_ == url_regular_);
		} else {
			_setCurrentURL(null);
			stop_on_first_frame_ = false;

			if (url_placeholder_) {
				if (placeholder_width_ == 0 || placeholder_height_ == 0)  {
					placeholder_mc_.load(new URLRequest(url_placeholder_));
					placeholder_mc_.contentLoaderInfo.addEventListener(Event.COMPLETE,
						function() {
							video_.visible = false;
	
							placeholder_width_ = placeholder_mc_.width;
							placeholder_height_ = placeholder_mc_.height;
	
							_layoutPlaceholder(true);
						}
					);
				}
			}
		}
		
		if (stopped_) {
			_open();
		}
	}
	
	protected function _seekClear() {
		seek_pending_ = NaN;
		_updateBuffering();
		
		if (seek_timer_id_) {
			flash.utils.clearTimeout(seek_timer_id_);
		}
	}
	protected function _seekPerform() {
		if (isNaN(seek_pending_)) {
			return false;
		}
		
		switch (state_.state_) {
			case PlayerState.SEEKING:
				if (seek_timer_id_ != 0) {
					flash.utils.clearTimeout(seek_timer_id_);
				}
				seek_timer_id_ = flash.utils.setTimeout(function() {seek_timer_id_=0;_seekPerform()}, 1000*seek_timeout_);
				break;
			case PlayerState.PLAYING:
			case PlayerState.PAUSED:
				_setState(PlayerState.SEEKING);
				if (url_current_parsed_.is_rtmp_) {
					ns_.seek(seek_pending_);
				} else {
					if (seek_pending_ <  state_.offset_) {
						ns_.seek(2*state_.length_);
					} else {
						ns_.seek(seek_pending_ - state_.offset_);
					}
				}
				break;
			case PlayerState.STOPPED:
				break;
			default:
				whispercast.Logger.log('player:state','Player::_seekPerform() - Unknown state: '+state_.state_);
		}
		return true;
	}
	
	protected function _play() {
		var stream:String;
		if (!isNaN(seek_pos_current_)) {
			var seek_pos:int = 1000*seek_pos_current_;
			seek_pos_current_ = NaN;
		
			if (url_current_parsed_.stream_name_.indexOf('?') == -1) {
				stream = url_current_parsed_.stream_name_+'?'+seek_pos_key_+'='+seek_pos+'&rnd='+Math.random();
			}
			else {
				stream = url_current_parsed_.stream_name_+'&'+seek_pos_key_+'='+seek_pos+'&rnd='+Math.random();
			}
		} else {
			if (url_current_parsed_.stream_name_.indexOf('?') == -1)
				stream = url_current_parsed_.stream_name_+'?rnd='+Math.random();
			else
				stream = url_current_parsed_.stream_name_+'&rnd='+Math.random();
		}
		
		whispercast.Logger.log('player:state','Player::_play() - NetStream.play('+stream+')');
		if (url_current_parsed_.is_rtmp_) {
			ns_.play(escape(stream));
		} else {
			ns_.play(stream);
		}
	}
	protected function _stop() {
	}
	
	protected function _restart() {
		if (restart_timer_id_) {
			flash.utils.clearTimeout(restart_timer_id_);
			restart_timer_id_ = 0;
		}
		
		if (url_current_parsed_ != null) {
			if (restart_timeout_ > 0) {
				_close();
				
				whispercast.Logger.log('player:state', 'Player::_restart() - Scheduling restart in '+restart_timeout_+' seconds...');
				restart_timer_id_ = flash.utils.setTimeout(
					function() {
						whispercast.Logger.log('player:state', 'Player::_restart() - Performing restart...');
						restart_timer_id_ = 0;
						if (url_current_ != null) {
							_setCurrentURL(url_current_);
							_open();
						}
					},
					1000*restart_timeout_
				);
				return;
			}
		}
		stop();
	}

	protected function _complete() {
		state_.position_ = state_.length_;
		notifyListeners(PlayerEvent.POSITION);
		
		_close();
	}
	
	protected function _lockPlayhead(lock:Boolean) {
		if (lock == playhead_locked_) {
			return;
		}

		if (lock) {
			playhead_locked_ = true;
			if (playhead_timer_id_ != 0) {
				flash.utils.clearInterval(playhead_timer_id_);
				playhead_timer_id_ = 0;
			}
		} else {
			playhead_locked_ = false;
			if (playhead_timer_id_ == 0) {
				playhead_timer_id_ = flash.utils.setInterval(_updatePlayhead, 1000*playhead_update_interval_);
			}
		}
	}
	protected function _updatePlayhead() {
		if (!playhead_locked_) {
			if (url_current_parsed_.is_rtmp_) {
				var time:Date = new Date();
				state_.position_ = playhead_offset_ + time.getTime()/1000;
			} else {
				state_.position_ =  playhead_offset_ + ns_.time;
			}
			whispercast.Logger.log('player:playhead','Player::_updatePlayhead() - NetStream time: '+ns_.time+', position: '+state_.position_);
			
			state_.downloaded_bytes_ = ns_.bytesLoaded;
			if (!isNaN(state_.total_bytes_)) {
				whispercast.Logger.log('player:playhead','Player::_updatePlayhead() - Downloaded '+state_.downloaded_bytes_+' of '+state_.total_bytes_+'('+ns_.bytesTotal+')');
			}
			
			notifyListeners(PlayerEvent.POSITION);
			
			if (!stopped_) {
				for (var id in addons_) {
					addons_[id].onPlayheadUpdate(state_.position_, state_.length_);
				}
			}
		}
	}
	protected function _updatePlayheadOffset(offset:Number) {
		if (isNaN(offset)) {
			playhead_offset_ = NaN;
		} else
		if (url_current_parsed_.is_rtmp_) {
			playhead_offset_ = offset - (new Date()).getTime()/1000;
		} else {
			playhead_offset_ = 0;
		}
		whispercast.Logger.log('player:playhead','Player::_updatePlayheadOffset() - Offset: '+playhead_offset_);
	}
	
	protected function _updateVideoSize():void {
		var w:Number;
		var h:Number;
		
		if (!isNaN(state_.video_width_) && (state_.video_width_ > 0) && !isNaN(state_.video_height_) && (state_.video_height_ > 0)) {
			if ((state_.video_width_/state_.video_height_) > width_/height_) {
				w = width_;
				h = (w*state_.video_height_)/state_.video_width_;
			} else {
				h = height_;
				w = (h*state_.video_width_)/state_.video_height_;
			}

			switch (video_align_x_) {
				case 'left':
				  video_.x = 0;
				  break;
				case 'right':
				  video_.x = width_ - w;
				  break;
				default:
				  video_.x = (width_ - w)/2;
			}
			switch (video_align_y_) {
				case 'top':
				  video_.y = 0;
				  break;
				case 'bottom':
				  video_.y = height_ - h;
				  break;
				default:
				  video_.y = (height_ - h)/2;
			}
			video_.width = w;
			video_.height = h;
			
			if (osd_) {
				osd_.layout(video_.x, video_.y, video_.width, video_.height, video_.width/state_.video_width_, video_.height/state_.video_height_);
			}
		} else {
			video_.x = 0;
			video_.y = 0;
			video_.width = width_;
			video_.height = height_;
			
			w = width_;
			h = height_;
			
			if (osd_) {
				osd_.layout(0, 0, width_, height_, 1, 1);
			}
		}
		
		if (osd_) {
			osd_.visible = state_.is_main_ || always_show_osd_;
		}
	}
	
	protected function _updateBuffering():void {
		var buffering:Boolean = isPlaying() && (buffering_ || !isNaN(seek_pending_));
		whispercast.Logger.log('player:state','Player::_updateBuffering() - Buffering: '+buffering);
		
		if (buffering) {
			_lockPlayhead(true);
			
			buffering_mc_.visible = true;
			buffering_mc_.play();
		} else {
			if (isPlaying()) {
				_lockPlayhead(false);
				_updatePlayheadOffset(state_.position_);
			}
			
			buffering_mc_.visible = false;
			buffering_mc_.stop();
		}
	}
	
	protected function _onError(e:Object, restart:Boolean) {
		whispercast.Logger.log('player:state','Player::_onError() - Event: '+whispercast.Logger.stringify(e.info, 1));
		
		if (restart) {
			_restart();
			return;
		}
		stop();
	}
	protected function _onClose(e:Object) {
		whispercast.Logger.log('player:state','Player::_onClose() - Event: '+whispercast.Logger.stringify(e.info, 1));
		
		if (!stopped_) {
			if (!opening_) {
				if (!closing_) {
					if (!stop_on_first_frame_) {
						if (state_.state_ != PlayerState.PAUSED) {
							_restart();
							return;
						} else {
							seek_pos_current_ = state_.position_;
						}
					}
				}
				stop();
			}
		}
	}
	protected function _onStart(e:Object) {
		whispercast.Logger.log('player:state','Player::_onStart()');
	}
	protected function _onStop(e:Object) {
		whispercast.Logger.log('player:state','Player::_onStop()');
		
		stopped_ = true;
		if (!isNaN(seek_pending_)) {
			seek_pos_current_ = seek_pending_;
			seek_pending_ = NaN;
			
			_updateBuffering();
			_play();
		} else {
			state_.position_ = state_.length_;
			notifyListeners(PlayerEvent.POSITION);
			
			if (!url_current_parsed_.is_rtmp_) {
				_close();
			}
		}
	}
	
	protected function _onPause(e:Object, paused:Boolean) {
		whispercast.Logger.log('player:state','Player::_onPause('+paused+')');
		
		if (!paused) {
			_updatePlayheadOffset(state_.position_);
		}
		_setState(paused ? PlayerState.PAUSED : PlayerState.PLAYING);
	}
	
	protected function _onSeek(e:Object, completed:Boolean) {
		whispercast.Logger.log('player:state','Player::_onSeek('+completed+')');
		
		_lockPlayhead(false);
		
		seek_pending_ = NaN;
		_updateBuffering();
		
		if (completed) {
			_setState(PlayerState.PLAYING);
		}
	}
	
	protected function _onEvent(e:Object) {
		whispercast.Logger.log('player:state','Player::_onEvent() - Event: '+whispercast.Logger.stringify(e.info, 1));
		switch (e.info.code) {
			case 'NetConnection.Connect.Success':
				ns_ = new NetStream(nc_);
				ns_.client = this;
				
				var audio:SoundTransform = new SoundTransform();
				audio.volume = 0;
				ns_.soundTransform = audio;

				ns_.bufferTime = buffer_time_while_connecting_;
				
				ns_.addEventListener(NetStatusEvent.NET_STATUS, onNetStatus);
				ns_.addEventListener('onPlayStatus', onNetStatus);
				video_.attachNetStream(ns_);

				_play();
				break;
			case 'NetConnection.Connect.Failed':
			case 'NetConnection.Connect.Rejected':
			case 'NetConnection.Connect.AppShutdown':
			case 'NetConnection.Connect.InvalidApp':
				_onError(e, true);
				break;
			case 'NetConnection.Connect.Closed':
				_onClose(e);
				break;
				
			case 'NetStream.Buffer.Empty':
				if (state_.complete_) {
					_complete();
				} else {
					buffering_ = true;
					_updateBuffering();
				}
				break;
			case 'NetStream.Buffer.Full':
				buffering_ = false;
				_updateBuffering();
				break;
			case 'NetStream.Buffer.Flush':
				break;
			case 'NetStream.Play.Reset':
				break;
			case 'NetStream.Play.Start':
				_onStart(e);
				break;
			case 'NetStream.Play.Stop':
				_onStop(e);
				break;
			case 'NetStream.Play.Complete':
				state_.complete_ = true;
				if (buffering_) {
					_complete();
				}
				break;
				
			// no read access, no restart
			case 'NetStream.Play.Failed':
			// not found, no restart
			case 'NetStream.Play.StreamNotFound':
			// invalid file, no restart
			case 'NetStream.Play.FileStructureInvalid':
			case 'NetStream.Play.NoSupportedTrackFound':
				_onError(e, false);
				break;
			// we restart, as this might be a transient condition
			case 'NetStream.Play.InsufficientBW':
				_onError(e, true);
				break;
				
			case 'NetStream.Pause.Notify':
				_onPause(e, true);
				break;
			case 'NetStream.Unpause.Notify':
				_onPause(e, false);
				break;
			case 'NetStream.Seek.InvalidTime':
				if (!url_current_parsed_.is_rtmp_) {
					seek_pos_current_ = seek_pending_;
					seek_pending_ = NaN;
					
					_updateBuffering();
					_play();
				} else {
					_onSeek(e, false);
				}
				break;
			case 'NetStream.Seek.Failed':
				_onSeek(e, false);
				break;
			case 'NetStream.Seek.Notify':
				_onSeek(e, true);
				break;
		}
	}
	public function onPlayStatus(e:Object) {
		whispercast.Logger.log('player:event','Player::onPlayStatus(): '+whispercast.Logger.stringify(e.info, 1));
		_onEvent({info:e});
	}
	public function onNetStatus(e:NetStatusEvent) {
		whispercast.Logger.log('player:event','Player::onNetStatus(): '+whispercast.Logger.stringify(e.info, 1));
		_onEvent(e);
	}
	
	public function onMetaData(e:Object) {
		whispercast.Logger.log('player:event','Player::onMetaData(): '+whispercast.Logger.stringify(e.info, 1));
		state_.length_ = (e.duration == undefined) ? NaN : e.duration;
		
		if (url_current_parsed_.is_rtmp_) {
			state_.offset_ = 0;
			if ((e.offset != undefined) && !isNaN(e.offset)) {
				state_.position_ = e.offset;
			} else {
				state_.position_ = 0;
			}
			
		} else {
			if ((e.offset != undefined) && !isNaN(e.offset)) {
				state_.offset_ =
				state_.position_ = e.offset;
			} else {
				state_.offset_ =
				state_.position_ = 0;
			}
		}
		_updatePlayheadOffset(state_.position_);
		
		state_.downloaded_bytes_ = ns_.bytesLoaded;
		if ((e.datasize != undefined) && !isNaN(e.datasize)) {
			state_.total_bytes_ = e.datasize;
		} else
		if ((e.filesize != undefined) && !isNaN(e.filesize)) {
			state_.total_bytes_ = e.filesize;
		} else {
			state_.total_bytes_ = NaN;
		}
		
		if ((e .width != undefined) && !isNaN(e.width)) {
			state_.video_width_ = e.width;
		} else {
			state_.video_width_ = NaN;
		}
		if ((e.height != undefined) && !isNaN(e.height)) {
			state_.video_height_ = e.height;
		} else {
			state_.video_height_ = NaN;
		}

		if (e.unseekable != undefined) {
			state_.can_seek_ = !e.unseekable;
		} else {
			state_.can_seek_ = true;
		}
		
		if (e.wc_mn != undefined) {
			if (e.wc_mn !== current_media_) {
				current_media_ = e.wc_mn;
				whispercast.Logger.log('player:event','Player::onMetaData(): Media changed to: '+current_media_);
				if (media_callback_) {
					ExternalInterface.call(media_callback_, state_.index_, state_.is_main_, current_media_);
				}
			}
		}
		_updateVideoSize();
		
		notifyListeners(PlayerEvent.STATE);
		notifyListeners(PlayerEvent.POSITION);
		
		if (stop_on_first_frame_) {
			initialize_timer_id_ = flash.utils.setTimeout(
				function() {
					if (video_.videoWidth > 0) {
						initialize_timer_id_ = 0;
						stop();
					}
					else
						initialize_timer_id_ = flash.utils.setTimeout(arguments.callee, initialize_timeout_);
				},
				initialize_timeout_
			)
		}
		else {
			ns_.soundTransform = audio_;
			
			state_.complete_ = false;
			ns_.bufferTime = buffer_time_while_playing_;
			
			if (!stopped_) {
				_setState(PlayerState.PLAYING);
			}
			
			playhead_locked_ = true; // force
			_lockPlayhead(false);
		}
	}
	public function onXMPData(e:Object) {
		whispercast.Logger.log('player:event','Player::onXMPData(): '+whispercast.Logger.stringify(e.info, 1));
	}
	public function onTextData(e:Object) {
		whispercast.Logger.log('player:event','Player::onTextData(): '+whispercast.Logger.stringify(e.info, 1));
	}
	
	public function onCuePoint(e:Object) {
		whispercast.Logger.log('player:event','Player::onCuePoint() - '+whispercast.Logger.stringify(e.info, 1));
		switch (e.type) {
			case 'navigation':
				state_.position_ = e.time;
				
				_updatePlayheadOffset(state_.position_);
				_lockPlayhead(false);
				
				if (state_.state_ == PlayerState.PAUSED || state_.state_ == PlayerState.SEEKING) {
					_setState(PlayerState.PLAYING);
				}
				
				buffering_ = false;
				_updateBuffering();
				
				whispercast.Logger.log('player:state','Player::onCuePoint() - NetStream time: '+ns_.time+', position: '+state_.position_+', CuePoint time: '+e.time);
				if (progress_callback_) {
					ExternalInterface.call(progress_callback_, state_.index_, state_.is_main_, e.time, e.abstime);
				}
				if (e.wc_mn != undefined) {
					if (e.wc_mn !== current_media_) {
						current_media_ = e.wc_mn;
						whispercast.Logger.log('player:state','Player::onCuePoint() - Media changed to: '+current_media_);
						if (media_callback_) {
							ExternalInterface.call(media_callback_, state_.index_, state_.is_main_, current_media_);
						}
					}
				}
				break;
			case 'actionscript':
				try {
					_call.call(this, e['parameters']['target'], e['parameters']['function'], json_.parse(e['parameters']['parameters']));
				} catch(e) {
					whispercast.Logger.log('player:event','Player::_onCuePoint(\'actionscript\') - Exception: '+e.message);
				}
				break;
		}
	}
	
	protected function _onMouseClick(e:MouseEvent):void {
		if (state_.is_main_ && click_url_) {
			ExternalInterface.call('window.open', click_url_, '_blank');
		} else {
			notifyListeners(PlayerEvent.CLICK);
		}
	}
	
	protected function _onAddonEvent(e:AddonEvent):void {
		whispercast.Logger.log('player:event','_onAddonEvent(): '+whispercast.Logger.stringify(e, 1));
		
		var id;
		switch (e.type) {
			case  AddonEvent.STATE:
				if (e.addon.state == Addon.LOADED) {
					if (e.addon.parent == null) {
						addChild(e.addon);
					}
				}
				
				if (!initialized_) {
					for (id in addons_) {
						if (addons_[id].state != Addon.INITIALIZED && addons_[id].state != Addon.FAILED) {
							return;
						} else {
							_layoutAddon(id);
						}
					}
					initialized_ = true;
					notifyListeners(PlayerEvent.STATE);
				} else
				if (state_.state_ == PlayerState.PREROLL) {
					for (id in addons_) {
						if (addons_[id].state == Addon.PREROLL) {
							return;
						}
					}
					_openPerform();
				} else
				if (state_.state_ == PlayerState.POSTROLL) {
					for (id in addons_) {
						if (addons_[id].state == Addon.POSTROLL) {
							return;
						}
					}
					_closePerform();
				}
				break;
		}
	}
	
	//OSD
	protected function setClickUrlHelper(url:String) {
		click_url_ = url ? url : null;
		this.buttonMode = (click_url_ != null);
	}
	public function _osd_SetClickUrl(p:Object) {
		try {
			whispercast.Logger.log('osd:player','Player::_osd_SetClickUrl() - '+whispercast.Logger.stringify(p, 1));
			setClickUrlHelper(p.url ? p.url : null);
		} catch(e) {
			whispercast.Logger.log('osd:player','Player::_osd_SetClickUrl() - Exception: '+e.message);
		}
	}
	public function _osd_SetPictureInPicture(p:Object) {
		try {
			whispercast.Logger.log('osd:player','Player::_osd_SetPictureInPicture() - '+whispercast.Logger.stringify(p, 1));
			notifyListeners(PlayerEvent.OSD, {type_:'SetPictureInPicture',params_:p});
		} catch(e) {
			whispercast.Logger.log('osd:player','Player::_osd_SetPictureInPicture() - Exception: '+e.message);
		}
	}
	public function _osd_CreateMovie(p:Object) {
		try {
			whispercast.Logger.log('osd:player','Player::_osd_CreateMovie() - '+whispercast.Logger.stringify(p, 1));
			/*
			notifyListeners(PlayerEvent.OSD, {type_:'CreateMovie',params_:p});
			*/
			var reload:Boolean = false;
			
			var parameters:Object = movies_[p.id] ? movies_[p.id].parameters_ : {x_location_:0,y_location_:0};
			if (p.url != undefined) {
				reload = (parameters.url_ != p.url);
				parameters.url_ = p.url;
			}
			if (p.width != undefined)
				if (p.width == 0)
					delete parameters.width_;
				else
					parameters.width_ = Math.max(0, Math.min(1, p.width));
			if (p.height != undefined)
				if (p.height == 0)
					delete parameters.height_;
				else
					parameters.height_ = Math.max(0, Math.min(1, p.height));
				
			if (p.x_location != undefined)
				parameters.x_location_ = Math.max(0, Math.min(1, p.x_location));
			if (p.y_location != undefined)
				parameters.y_location_ = Math.max(0, Math.min(1, p.y_location));
				
			if (movies_[p.id]) {
				if (reload) {
					_destroyMovieHelper(p.id);
				} else {
					movies_[p.id].parameters_ = parameters;
					_layoutMovie(p.id);
					return;
				}
			}
			
			var movie:Loader = new Loader();
			movies_[p.id] = {movie_:movie,parameters_:parameters};
			
			movie.visible = false;
			movie.contentLoaderInfo.addEventListener(
				flash.events.Event.COMPLETE,
				function(e:Event) {
					_layoutMovie(p.id);
				}
			);
			addChild(movie);
			movie.load(new URLRequest(parameters.url_));
		} catch(e) {
			whispercast.Logger.log('osd:player','Player::_osd_CreateMovie() - Exception: '+e.message);
		}
	}
	protected function _destroyMovieHelper(id:String) {
		if (movies_[id]) {
			try {
				if (movies_[id].mask_) {
					movies_holder_mc_.removeChild(movies_[id].mask_);
				}
				movies_holder_mc_.removeChild(movies_[id].movie_);
			} catch(e) {
			}
			delete movies_[id];
		}
	}
	public function _osd_DestroyMovie(p:Object) {
		try {
			whispercast.Logger.log('osd:player','Player::_osd_DestroyMovie() - '+whispercast.Logger.stringify(p, 1));
			_destroyMovieHelper(p.id);
		} catch(e) {
			whispercast.Logger.log('osd:player','Player::_osd_DestroyMovie() - Exception: '+e.message);
		}
	}
}
}
include "../../JSON.as"
class ParsedURL extends Object {
	public var is_rtmp_:Boolean = false;
	
	public var protocol_:String = null;
	public var host_:String = null;
	public var port_:String = null;
	
	public var app_name_:String = null;
	public var stream_name_:String = null;
	
	public var query_:Object = {};
	
	public static function parse(url:String):ParsedURL {
		if (url) {
			var results:ParsedURL = new ParsedURL();
				
			// get protocol
			var start_index:Number = 0;
			var end_index:Number = url.indexOf(':/', start_index);
			if (end_index >= 0) {
				end_index += 2;
				results.protocol_ = url.slice(start_index, end_index);
			}
			
			if (results.protocol_ == 'rtmp:/' ||
				results.protocol_ == 'rtmpt:/' ||
				results.protocol_ == 'rtmps:/' ||
				results.protocol_ == 'rtmpe:/' ||
				results.protocol_ == 'rtmpte:/') {
				results.is_rtmp_ = true;
				
				start_index = end_index;
		
				if (url.charAt(start_index) == '/') {
					start_index++;
					// get server (and maybe port)
					var colon_index:Number = url.indexOf(':', start_index);
					var slash_index:Number = url.indexOf('/', start_index);
					if (slash_index < 0) {
						if (colon_index < 0) {
							results.host_ = url.slice(start_index);
						} else {
							end_index = colon_index;
							results.port_ = url.slice(start_index, end_index);
							start_index = end_index + 1;
							results.host_ = url.slice(start_index);
						}
						return results;
					}
					if (colon_index >= 0 && colon_index < slash_index) {
						end_index = colon_index;
						results.host_ = url.slice(start_index, end_index);
						start_index = end_index + 1;
						end_index = slash_index;
						results.port_ = url.slice(start_index, end_index);
					} else {
						end_index = slash_index;
						results.host_ = url.slice(start_index, end_index);
					}
					start_index = end_index + 1;
				}
		
				// get application name
				end_index = url.indexOf('/', start_index);
				if (end_index < 0) {
					results.app_name_ = url.slice(start_index);
					return results;
				}
				
				results.app_name_ = url.slice(start_index, end_index);
				start_index = end_index + 1;
		
				results.stream_name_ = url.slice(start_index);
				results.query_ = new Object();
				
				start_index = results.stream_name_.indexOf('?', 0);
				if (start_index >= 0) {
					var pairs:Array = results.stream_name_.slice(start_index+1).split('&');
					for (var p:String in pairs) {
						var pair:Array = pairs[p].split('=');
						results.query_[pair[0]] = unescape(pair[1]);
					}
				}
				
				return results;
			} else {
				// is http, just return the full url received as stream_name_
				results.is_rtmp_ = false;
				results.stream_name_ = url;
			}
			return results;
		}
		return null;
	}
}