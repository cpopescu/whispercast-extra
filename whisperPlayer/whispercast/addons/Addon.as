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

package whispercast.addons
{
import flash.system.*;
import flash.net.*;
import flash.events.*;
import flash.display.*;

import whispercast.*;

public class Addon extends Loader {
	public static var NONE:String = 'NONE';
	public static var FAILED:String = 'FAILED';
	public static var LOADED:String = 'LOADED';
	public static var INITIALIZED:String = 'INITIALIZED';
	public static var PREROLL:String = 'PREROLL';
	public static var POSTROLL:String = 'POSTROLL';
	public static var PLAY:String = 'PLAY';
	
	private var state_:String = LOADED;
	public function get state():String {
		return state_;
	}
	
	protected function setState(state:String):void {
		state_ = state;
		dispatchEvent(new AddonEvent(AddonEvent.STATE, this));
	}
	
	public function initialize():void {
	}
	
	public function layout(ax:Number, ay:Number, w:Number, h:Number, control_bar_h:Number):void {
	}
	
	public function onPlay():Boolean {
		return false;
	}
	public function onStop():Boolean {
		return false;
	}
	public function onPlayheadUpdate(current:Number, total:Number):Boolean {
		return false;
	}
	public function onSetVolume(volume:Number, muted:Boolean):Boolean {
		return false;
	}
}
}