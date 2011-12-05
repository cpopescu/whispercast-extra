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
public class PlayerState extends Object {
	public static var PREROLL:String = 'preroll';
	public static var LOADING:String = 'loading';
	public static var PLAYING:String = 'playing';
	public static var POSTROLL:String = 'postroll';
	public static var STOPPED:String = 'stopped';
	public static var PAUSED:String = 'paused';
	public static var SEEKING:String = 'seeking';
	
	public var state_:String = STOPPED;
	
	public var index_:int = -1;
	public var is_main_:Boolean = false;
	
	public var can_seek_:Boolean = true;
	
	public var length_:Number = NaN;
	public var offset_:Number = NaN;
	public var position_:Number = NaN;
	
	public var total_bytes_:Number = NaN;
	public var downloaded_bytes_:Number = NaN;
	
	public var complete_:Boolean = false;
	
	public var video_width_:Number = NaN;
	public var video_height_:Number = NaN;
}
}