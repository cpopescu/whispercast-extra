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
	public class Logger {
		private static var enabled_:Array = [];
		private static var enabledAll_:Boolean = false;
		
		public static function enableAll(enable:Boolean) {
			enabledAll_ = enable;
		}
		
		public static function enable(...args) {
			for (var i:int = 0; i < args.length; i++) {
				enabled_.push(args[i]);
			}
		}
		public static function disable(...args) {
			for (var i:int = 0; i < args.length; i++) {
				var index = enabled_.indexOf(args[i]);
				if (index >= 0) {
					enabled_.splice(index, 1);
				}
			}
		}
		
		private static var loggers_:Object = {};
		public static function addLogger(id:String, logger:Function) {
			loggers_[id] = logger;
		}
		public static function removeLogger(id:String) {
			delete loggers_[id];
		}
		
		public static function log(category:String, message:String, indent:int = 0) {
			if (enabledAll_ || enabled_.indexOf(category) >= 0) {
				message = (new Date()).toString()+":\n"+message;
				trace(message);
				for (var i in loggers_) {
					loggers_[i].call(null, message);
				}
			}
		}
		public static function stringify(object, indent:int = 0, name:String = ''):String {
			var p:String = '';
			var t:int = 0;
			for (; t < indent; t++) {
				p += '\t';
			}
			
			var r:String = '';
			for (var i in object) {
				if (typeof(object[i]) == 'object') {
					r += stringify(object[i], indent+1, name+'['+i+']');
					continue;
				}
				r += '\n'+(p+name+'['+i+'] = '+object[i]);
			}
			return r;
		}
	}
}