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
import fl.transitions.*;
import fl.transitions.easing.*;

import flash.ui.*;
import flash.events.*;
import flash.utils.*;
import flash.display.*;
import flash.geom.*;

import whispercast.*;
import whispercast.player.*;

public class ControlBarNormal extends ControlBar  {
	private var volume_bar_timeout_:Number = 1.5;
	private var volume_bar_timer_id_:uint = 0;
	
	private var volume_bar_visible_:Boolean = false;
	private var volume_bar_tween_:Tween = null;
	
	private var volume_bar_y_:Number = NaN;
	
	private var position_:Number = NaN;
	private var length_:Number = NaN;
	
	private var clip_width_:Number = NaN;
	private var clip_height_:Number = NaN;
	
	private var can_seek_:Boolean = true;
	
	protected var has_mouse_:Boolean = false;
	
	public function ControlBarNormal() {
	}
	
	public override function create(client:Object, p:Object):void {
		client.addEventListener(PlayerEvent.STATE, function(e:PlayerEvent) {
			if (e.state_.is_main_) {
				var playing:Boolean = e.player_.isPlaying();
				UI.setButtonAppearance(play_pause_btn_mc, playing ? 'pause' : 'play');
				
				UI.enableButton(play_pause_btn_mc, e.player_.isControllable() && (e.url_.regular != null));
				UI.enableBar(seek_bar_mc.progress_mc, e.player_.isControllable() && e.state_.can_seek_ && playing && !isNaN(e.state_.length_) && (e.state_.length_ > 0));
			}
		});
		client.addEventListener(PlayerEvent.POSITION, function(e:PlayerEvent) {
			if (e.state_.is_main_) {
				if (!isNaN(e.state_.length_) && (e.state_.length_ > 0)) {
					var length:Number = e.state_.length_;
					
					var position:Number = isNaN(e.state_.position_) ? 0 : e.state_.position_;
					var offset:Number = isNaN(e.state_.offset_) ? 0 : e.state_.offset_;
					
					if (!seek_bar_mc.progress_mc.is_dragging_) {
						UI.setBarPosition(seek_bar_mc.progress_mc, (position-offset)/(length-offset), offset/length);
					}
					
					_showTime(true);
					_showPositionAndLength(seek_bar_mc.progress_mc.is_dragging_ ? position_ : position, length);
				}
				else {
					UI.setBarPosition(seek_bar_mc.progress_mc, 0, 0);
					
					_showTime(false);
					_showPositionAndLength(NaN, NaN);
				}
			}
		});
		client.addEventListener(DisplayEvent.DISPLAY, function(e:DisplayEvent) {
			UI.setButtonAppearance(fullscreen_btn_mc, e.fullscreen_ ? 'off' : 'on');
		});
		client.addEventListener(CommonEvent.VOLUME, function(e:CommonEvent) {
			UI.setButtonAppearance(mute_btn_mc, e.state_.muted_ ? 'off' : 'on');
			UI.setBarPosition(volume_bar_mc.volume_mc, e.state_.volume_);
		});
		client.addEventListener(CommonEvent.PIP, function(e:CommonEvent) {
			switch(e.state_.pip_state_) {
				case CommonState.PIP_NONE:
					UI.setButtonAppearance(pip_btn_mc, 'off');
					UI.enableButton(pip_btn_mc, false);
					pip_btn_mc.visible = false;
					break;
				case CommonState.PIP_AVAILABLE:
					pip_btn_mc.visible = true;
					UI.setButtonAppearance(pip_btn_mc, 'off');
					UI.enableButton(pip_btn_mc, true);
					break;
				case CommonState.PIP_ACTIVE:
					pip_btn_mc.visible = true;
					UI.setButtonAppearance(pip_btn_mc, 'on');
					UI.enableButton(pip_btn_mc, true);
					break;
			}
		});
		
		UI.addButton(play_pause_btn_mc, p.callbacks.play_pause.callback, 'play', false, 'Play/Stop');
		
		UI.addButton(mute_btn_mc,
			function(ctrl:MovieClip, kind:String) {
				switch (kind) {
					case 'roll_over':
						_showVolumeBar(true);
						break;
					case 'roll_out':
						_showVolumeBar(false);
						break;
				}
				if (p.callbacks.mute.callback) {
					p.callbacks.mute.callback(ctrl, kind);
				}
			}, 'on', true, null);
		UI.addButton(fullscreen_btn_mc, p.callbacks.fullscreen.callback, 'on', true, 'Fullscreen');
		
		UI.addButton(pip_btn_mc, p.callbacks.pip.callback, 'off', false, 'Picture In Picture');

		volume_bar_y_ = volume_bar_mc.y;
		volume_bar_mc.y = volume_bar_mc.height;
		volume_bar_mc.visible = false;
		
		UI.addBar(volume_bar_mc.volume_mc,
			function(bar:MovieClip, percent:Number, is_dragging:Boolean) {
				p.callbacks.volume.callback(bar, percent, is_dragging);
				
				if (!is_dragging) {
					if (volume_bar_timer_id_ != 0) {
						clearTimeout(volume_bar_timer_id_);
						volume_bar_timer_id_ = 0;
					}
					_checkVolumeBarVisibility();
				}
			}, true, p.callbacks.volume.value);

		seek_bar_mc.progress_mc.tooltip_ = function(p:Object) {
			var t:Point = new Point(seek_bar_mc.progress_mc.mouseX, seek_bar_mc.progress_mc.y-seek_bar_mc.progress_mc.height/2-10);
			t = seek_bar_mc.progress_mc.localToGlobal(t);
			p.x = t.x;
			p.y = t.y;
			
			var time:Number = isNaN(length_) ? NaN : length_*Math.max(0, Math.min(1, seek_bar_mc.progress_mc.mouseX/seek_bar_mc.progress_mc.width));
			
			var h = Math.floor(time/3600);
			time = time-3600*h;
			var m = Math.floor(time/60);
			time = time-60*m;
			var s = Math.floor(time);
			p.text = h+':'+(m<10 ? '0':'')+m+':'+(s<10 ? '0':'')+s;
		}
		UI.addBar(seek_bar_mc.progress_mc,
			function(bar:MovieClip, percent:Number, is_dragging:Boolean) {
				p.callbacks.seek.callback(bar, percent, is_dragging);
				
				if (is_dragging) {
					if (!isNaN(length_)) {
						_showTime(true);
						_showPositionAndLength(percent*length_, length_);
					}
				}
			}, true, p.callbacks.seek.value);
		UI.addBar(seek_bar_mc.downloaded_mc, null, false, 0, 0);
		
		this.addEventListener(Event.ADDED_TO_STAGE, function() {
			stage.addEventListener(MouseEvent.MOUSE_MOVE, onStageMouseMove);
			stage.addEventListener(Event.MOUSE_LEAVE, onStageMouseLeave);
			stage.addEventListener(MouseEvent.CLICK, onStageMouseClick, true)
        });
	}
	
	public override function layout(ax:Number, ay:Number, w:Number, h:Number):void {
		var ow:Number = background_mc.width;
		background_mc.width = w;
		
		time_mc.visible = !time_mc.visible;
		_showTime(!time_mc.visible, false);
		
		time_mc.x += w - ow;
		volume_bar_mc.x += w - ow;
		mute_btn_mc.x += w - ow;
		fullscreen_btn_mc.x += w - ow;
		pip_btn_mc.x += w - ow;
		
		var ps:Object = UI.getBarState(seek_bar_mc.progress_mc);
		var ds:Object = UI.getBarState(seek_bar_mc.downloaded_mc);
		
		if (time_mc.visible) {
			seek_bar_mc.progress_mc.background_mc.width =
			seek_bar_mc.downloaded_mc.background_mc.width =
			seek_bar_mc.progress_mc.value_mc.width =
			seek_bar_mc.downloaded_mc.value_mc.width =
			time_mc.x-seek_bar_mc.x-10;
		} else {
			seek_bar_mc.progress_mc.background_mc.width =
			seek_bar_mc.downloaded_mc.background_mc.width =
			seek_bar_mc.progress_mc.value_mc.width =
			seek_bar_mc.downloaded_mc.value_mc.width =
			mute_btn_mc.x-seek_bar_mc.x-10;
		}
		
		UI.setBarState(seek_bar_mc.downloaded_mc, ds);
		UI.setBarState(seek_bar_mc.progress_mc, ps);
		
		x = ax;
		y = ay;
	}
	
	public override function canHide():Boolean {
		if (volume_bar_visible_) {
			return false;
		}
		if (UI.capturedControl() != null) {
			return false;
		}
		if (has_mouse_)
			return !background_mc.hitTestPoint(stage.mouseX, stage.mouseY, false);
		return true;
	}
	
	public override function getHeight():Number {
		return background_mc.height;
	}
	
	private function _showVolumeBar(show:Boolean):void {
		if (show) {
			if (volume_bar_timer_id_ != 0) {
				clearTimeout(volume_bar_timer_id_);
				volume_bar_timer_id_ = 0;
			}
		}
			
		if (show != volume_bar_visible_) {
			if (show) {
				volume_bar_visible_ =
				volume_bar_mc.visible = true;
				
				_animateVolumeBar();
			}
			volume_bar_timer_id_ = setTimeout(_checkVolumeBarVisibility, 1000*volume_bar_timeout_);
		}
	}
	private function _animateVolumeBar() {
		if (volume_bar_tween_) {
			volume_bar_tween_.continueTo(volume_bar_visible_ ? volume_bar_y_ : volume_bar_mc.height, NaN);
		} else {
			volume_bar_tween_ = new Tween(volume_bar_mc, 'y', Strong.easeOut, volume_bar_mc.y, volume_bar_visible_ ? volume_bar_y_ : volume_bar_mc.height, 0.5, true);
			volume_bar_tween_.addEventListener(TweenEvent.MOTION_FINISH, function(e:Object) {
				volume_bar_tween_ = null;
				volume_bar_mc.visible = volume_bar_visible_;
			}, false, 0, true);	
		}
	}
	private function _checkVolumeBarVisibility() {
		volume_bar_timer_id_ = 0;
		if (!volume_bar_mc.volume_mc.is_dragging_) {
			if (
				!has_mouse_ ||
				(
				!volume_bar_mc.hitTestPoint(stage.mouseX, stage.mouseY, false) &&
				!mute_btn_mc.hitTestPoint(stage.mouseX, stage.mouseY, false) &&
				(UI.capturedControl() != mute_btn_mc)
				)
			) {
				volume_bar_visible_ = false;
				_animateVolumeBar();
				
				return;
			}
		}
		volume_bar_timer_id_ = setTimeout(_checkVolumeBarVisibility, 1000*volume_bar_timeout_);
	}
	
	private function _showTime(show:Boolean, doLayout:Boolean = true):void {
		if ((mute_btn_mc.x-seek_bar_mc.x) < 100) {
			show = false;
		}
		if (show == time_mc.visible) {
			return;
		}
			
		time_mc.visible = show;
		if (doLayout) {
			layout(x, y, width, height);
		}
	}
	
	private function _formatTime(time:Number, destination:MovieClip):void {
		var t:Number = time;
		var h:Number;
		var m:Number;
		var s:Number;
		
		if (!isNaN(time)) {
			h = Math.floor(t/3600);
			t = t-3600*h;
			m = Math.floor(t/60);
			t = t-60*m;
			s = t;
			
			destination.h1.text = Math.floor(h/10);
			destination.h0.text = h-10*Math.floor(h/10);
			destination.m1.text = Math.floor(m/10);
			destination.m0.text = m-10*Math.floor(m/10);
			destination.s1.text = Math.floor(s/10);
			destination.s0.text = s-10*Math.floor(s/10);
		} else {
			destination.h1.text = 
			destination.h0.text = 
			destination.m1.text = 
			destination.m0.text = 
			destination.s1.text = 
			destination.s0.text = '-';
		}
	}
	private function _showPositionAndLength(position:Number, length:Number):void {
		if ((position_ != position) || (length_ != length)) {
			_formatTime(position, time_mc.p);
			_formatTime(length, time_mc.l);
			
			position_ = position;
			length_ = length;
		}
	}
	
	protected function onStageMouseMove(e:MouseEvent):void {
		has_mouse_ = true;
	}
	protected function onStageMouseLeave(e:Event):void {
		has_mouse_ = false;
	}
	protected function onStageMouseClick(e:MouseEvent):void {
		has_mouse_ = true;
	}
}
}