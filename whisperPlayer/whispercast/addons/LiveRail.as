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
import flash.geom.*;

import whispercast.*;
import whispercast.player.PlayerEvent;

public class LiveRail extends Addon {
	private var domain_:String = 'vox-static.liverail.com';
	public function get domain():String {
		return domain_;
	}
	private var url_:String = 'http://vox-static.liverail.com/swf/v4/admanager.swf';
	public function get url():String {
		return url_;
	}
	private var config_:Object = null;
	
	public function LiveRail(p:Object) {
		if (p.domain != undefined) {
			domain_ = p.domain;
		}
		if (p.url != undefined) {
			url_ = p.url;
		}
		if (p.config != undefined) {
			config_ = p.config;
		}
	}
	
	public override function initialize():void {
		Security.allowDomain(domain_);
		
		if (config_ != null) {
			contentLoaderInfo.addEventListener(Event.COMPLETE, onLiveRailLoadComplete);
			contentLoaderInfo.addEventListener(IOErrorEvent.IO_ERROR, onLiveRailLoadError);
			
			load(new URLRequest(url_));
		} else {
			setState(FAILED);
		}
	}
	
	public override function layout(ax:Number, ay:Number, w:Number, h:Number, control_bar_h:Number):void {
		var adManager = content;
		if (adManager) {
			adManager.setSize(new Rectangle(0, 0, w, h-control_bar_h), new Rectangle(0, 0, w, h),  1);
		}
	}
	
	public override function onPlay():Boolean {
		var adManager = content;
		if (adManager) {
			adManager.onContentStart();
			setState(PREROLL);
			return true;
		}
		return false;
	}
	public override function onStop():Boolean {
		var adManager = content;
		if (adManager) {
			adManager.onContentEnd();
			setState(POSTROLL);
			return true;
		}
		return false;
	}
	public override function onPlayheadUpdate(current:Number, total:Number):Boolean {
		var adManager = content;
		if (adManager) {
			whispercast.Logger.log('addons:addons', 'onPlayheadUpdate(): '+current+' of '+total);
			adManager.onContentUpdate(current, total);
			return true;
		}
		return false;
	}
	public override function onSetVolume(volume:Number, muted:Boolean):Boolean {
		var adManager = content;
		if (adManager) {
			adManager.setVolume(volume, muted);
			return true;
		}
		return false;
	}
	
	private function onLiveRailLoadComplete(e:Event) {
		whispercast.Logger.log('addons:addons', 'onLiveRailLoadComplete(): '+whispercast.Logger.stringify(e, 1));
		
		content.addEventListener('initComplete', onLiveRailInitComplete);
		content.addEventListener('initError', onLiveRailInitError);
		content.addEventListener('prerollComplete', onLiveRailPrerollComplete);
		content.addEventListener('postrollComplete', onLiveRailPostrollComplete);
		content.addEventListener('adStart', onLiveRailAdStart);
		content.addEventListener('adEnd', onLiveRailAdEnd);
		content.addEventListener('clickThru', onLiveRailClickThru);

		setState(LOADED);
		
		var adManager = content;
		adManager.initAds(config_);
	}
	private function onLiveRailLoadError(e:Event) {
		whispercast.Logger.log('addons:addons', 'onLiveRailLoadError(): '+whispercast.Logger.stringify(e, 1));
		setState(FAILED);
	}
	
	private function onLiveRailInitComplete(e:Object) : void {
		whispercast.Logger.log('addons:addons', 'onLiveRailInitComplete(): '+whispercast.Logger.stringify(e, 1));
		setState(INITIALIZED);
	}
	private function onLiveRailInitError(e:Object) : void {
		whispercast.Logger.log('addons:addons', 'onLiveRailInitError(): '+whispercast.Logger.stringify(e, 1));
		setState(FAILED);
	}
	private function onLiveRailPrerollComplete(e:Object) : void {
		whispercast.Logger.log('addons:addons', 'onLiveRailPrerollComplete(): '+whispercast.Logger.stringify(e, 1));
		setState(PLAY);
	}
	private function onLiveRailPostrollComplete(e:Object) : void {
		whispercast.Logger.log('addons:addons', 'onLiveRailPostrollComplete(): '+whispercast.Logger.stringify(e, 1));
		setState(INITIALIZED);
	}
	private function onLiveRailAdStart(e:Object) : void {
		whispercast.Logger.log('addons:addons', 'onLiveRailAdStart(): '+whispercast.Logger.stringify(e, 1));
	}
	private function onLiveRailAdEnd(e:Object) : void {
		whispercast.Logger.log('addons:addons', 'onLiveRailAdEnd(): '+whispercast.Logger.stringify(e, 1));
	}
	private function onLiveRailClickThru(e:Object) : void {
		whispercast.Logger.log('addons:addons', 'onLiveRailClickThru(): '+whispercast.Logger.stringify(e, 1));
	}
}
}