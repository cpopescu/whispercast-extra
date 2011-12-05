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

import whispercast.player.*;

public class ControlBarSimple extends ControlBar {
	public function ControlBarSimple() {
	}
	
	public override function create(client:Object, p:Object):void {
		client.addEventListener(PlayerEvent.STATE, function(e:PlayerEvent) {
			if (e.state_.is_main_) {
				var playing:Boolean = e.player_.isPlaying();
				
				UI.setButtonAppearance(play_pause_btn_mc, playing ? 'pause' : 'play');
				UI.enableButton(play_pause_btn_mc, e.player_.isInitialized() && (e.url_.regular != null));
			}
		});
		client.addEventListener(CommonEvent.VOLUME, function(e:CommonEvent) {
			UI.setButtonAppearance(mute_btn_mc, e.state_.muted_ ? 'off' : 'on');
		});
		
		UI.addButton(play_pause_btn_mc, p.callbacks.play_pause.callback, 'play', false, 'Play/Stop');
		UI.addButton(mute_btn_mc, p.callbacks.mute.callback, 'on', true, 'Mute/Unmute');
	}
	
	public override function layout(ax:Number, ay:Number, w:Number, h:Number):void {
		x = ax;
		y = ay;
	}
	
	public override function canHide():Boolean {
		return false;
	}
	
	public override function getHeight():Number {
		return height+1;
	}
}
}