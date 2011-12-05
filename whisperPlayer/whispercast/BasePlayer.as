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

package whispercast {
import flash.display.*;
import flash.events.*;
import flash.ui.*;
import flash.geom.*;

import whispercast.*;
import whispercast.player.*;

public class BasePlayer extends Sprite {
	protected var player_:Player;
	protected var state_:CommonState;

	protected var hardware_accelerated_fullscreen_:Boolean = false;
	
	protected var json_:JSON = new JSON();
		
	public function BasePlayer() {
		create();
	}
	
	protected function loadString(name:String, index:int, def:String):String {
		var value:String;
		name = (index >= 0) ? (name+index) : name;
		try {
		if (loaderInfo.parameters[name] != undefined) {
			value = String(loaderInfo.parameters[name]);
			if (value == '') {
				value = def;
			}
		} else {
			value = def;
		}
		} catch(e) {
			value = def;
		}
		return value;
	}
	protected function loadNumber(name:String, index:int, min:Number, max:Number, def:Number):Number {
		var value;
		name = (index >= 0) ? (name+index) : name;
		try {
		if (loaderInfo.parameters[name] != undefined) {
			value = Number(loaderInfo.parameters[name]);
			if (!isNaN(max)) {
				value = Math.min(max, value);
			}
			if (!isNaN(min)) {
				value = Math.max(min, value);
			}
		} else {
			value = def;
		}
		} catch(e) {
			value = def;
		}
		return value;
	}
	protected function loadBoolean(name:String, index:int, def:Boolean):Boolean {
		var value;
		name = (index >= 0) ? (name+index) : name;
		try {
		if (loaderInfo.parameters[name] != undefined) {
			value = Boolean((loaderInfo.parameters[name] != 0));
		} else {
			value = def;
		}
		} catch(e) {
			value = def;
		}
		return value;
	}
	protected function loadObject(name:String, index:int, def:Object = null):Object {
		var value;
		name = (index >= 0) ? (name+index) : name;
		try {
		if (loaderInfo.parameters[name] != undefined) {
			value = String(loaderInfo.parameters[name]);
			value = json_.parse(value);
		} else {
			value = def;
		}
		} catch(e) {
			value = def;
		}
		return value;
	}
		
	public function setVolume(volume:Number) {
		if (volume != state_.volume_ || state_.muted_) {
			player_.setVolume(state_.volume_ = state_.muted_volume_ = volume, false);
			
			state_.muted_ = false;
			dispatchEvent(new CommonEvent(CommonEvent.VOLUME, state_));
		}
	}
	public function getVolume():Number {
		return state_.muted_ ? state_.muted_volume_ : state_.volume_;
	}
	
	public function setMute(mute:Boolean) {
		if (mute != state_.muted_) {
			if (mute) {
				state_.volume_ = state_.muted_volume_ = player_.getVolume();
				
				player_.setVolume(0, true);
				state_.muted_ = true;
			} else {
				player_.setVolume(state_.volume_ = state_.muted_volume_, false);
				state_.muted_ = false;
			}
			
			dispatchEvent(new CommonEvent(CommonEvent.VOLUME, state_));
		}
	}
	public function getMute():Boolean {
		return  state_.muted_;
	}
	
	public function setFullscreen(fullscreen:Boolean) {
		if (stage.displayState == StageDisplayState.FULL_SCREEN) {
			stage.displayState = StageDisplayState.NORMAL;
		}
		else {
			if (hardware_accelerated_fullscreen_) {
				stage.fullScreenSourceRect = new Rectangle(0,0,stage.stageWidth,stage.stageHeight);
			}
			stage.displayState = StageDisplayState.FULL_SCREEN;
		}
	}
	public function getFullscreen():Boolean {
		return stage.displayState == StageDisplayState.FULL_SCREEN;
	}

	protected function checkInitialize():void {
		initialize();
	}
	
	protected function create():void {
		stage.scaleMode = StageScaleMode.NO_SCALE;
		stage.align = StageAlign.TOP_LEFT;
		
		state_ = new CommonState();
		state_.volume_ = 
		state_.muted_volume_ = loadNumber('volume', -1, 0, 1, 0.5);
		
		state_.muted_ = loadBoolean('muted', -1, false);
		
		stage.addEventListener(Event.ENTER_FRAME, function(e:Event) {
			if (stage.stageWidth > 0 && stage.stageHeight > 0) {
				stage.removeEventListener(Event.ENTER_FRAME, arguments.callee);
				loaded();
			}
		});
	}
	protected function loaded():void {
		checkInitialize();
	}
	
	protected function initialize():void {
		stage.addEventListener(Event.RESIZE, onStageResize);
		stage.addEventListener(FullScreenEvent.FULL_SCREEN, onStageResize);
	}
	protected function layout():void {
	}
	
	protected function onStageResize(e:Event):void {
		layout();

		Mouse.show();
		dispatchEvent(new DisplayEvent(DisplayEvent.DISPLAY, getFullscreen()));
	}
}
}
include "../JSON.as"