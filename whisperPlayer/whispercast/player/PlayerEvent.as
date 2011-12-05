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
import flash.events.*;
import whispercast.player.*;
	
public class PlayerEvent extends Event {
	public static var STATE:String = 'player_state';
	public static var POSITION:String = 'player_position';
	public static var CLICK:String = 'player_click';
	
	public static var OSD:String = 'player_osd';
	
	public var url_:Object;
	
	public var state_:PlayerState;
	public var player_:Player;
	
	public var params_:Object;
	
	function PlayerEvent(type:String, url:Object, state:PlayerState, player:Player, params:Object = null) {
		super(type);
		
		url_ = url;
		state_ = state;
		
		player_ = player;
		
		params_ = params;
	}
	public override function clone():Event {
		return new PlayerEvent(type, url_, state_, player_, params_);
	}
}
}