/*
Copyright (c) 2005 JSON.org

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The Software shall be used for Good, not Evil.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

/*
Ported to Actionscript 2 May 2005 by Trannie Carter <tranniec@designvox.com>,
wwww.designvox.com

Updated 2007-03-30

USAGE:
            var json = new JSON();
    try {
        var o:Object = json.parse(jsonStr);
        var s:String = json.stringify(obj);
    } catch(ex) {
        trace(ex.name + ":" + ex.message + ":" + ex.at_ + ":" + ex.text_);
    }

*/

package org.whispercast
{
	public class Json {
        private var ch_:String;
		private var at_:Number;
		private var text_:String;
	
	    public function stringify(arg:*):String {
	        var s:String = '', v:String;
	
	        switch (typeof arg) {
	        case 'object':
	            if (arg) {
	                if (arg is Array) {
	                    for (var i:int = 0; i < arg.length; ++i) {
	                        v = stringify(arg[i]);
	                        if (s) {
	                            s += ',';
	                        }
	                        s += v;
	                    }
	                    return '[' + s + ']';
	                } else if (typeof arg.toString != 'undefined') {
	                    for (var k:String in arg) {
	                        v = arg[k];
	                        if (typeof v != 'undefined' && typeof v != 'function') {
	                            v = stringify(v);
	                            if (s) {
	                                s += ',';
	                            }
	                            s += stringify(k) + ':' + v;
	                        }
	                    }
	                    return '{' + s + '}';
	                }
	            }
	            return 'null';
	        case 'number':
	            return isFinite(arg) ? String(arg) : 'null';
	        case 'string':
	            var l:int = arg.length;
	            s = '"';
	            for (i = 0; i < l; i += 1) {
	                var c:String = arg.charAt(i);
	                if (c >= ' ') {
	                    if (c == '\\' || c == '"') {
	                        s += '\\';
	                    }
	                    s += c;
	                } else {
	                    switch (c) {
	                        case '\b':
	                            s += '\\b';
	                            break;
	                        case '\f':
	                            s += '\\f';
	                            break;
	                        case '\n':
	                            s += '\\n';
	                            break;
	                        case '\r':
	                            s += '\\r';
	                            break;
	                        case '\t':
	                            s += '\\t';
	                            break;
	                        default:
	                            var n:Number = c.charCodeAt();
	                            s += '\\u00' + Math.floor(n/16).toString(16) +
	                                (n % 16).toString(16);
	                    }
	                }
	            }
	            return s + '"';
	        case 'boolean':
	            return String(arg);
	        default:
	            return 'null';
	        }
	    }
		
        private function white():void {
            while (ch_) {
                if (ch_ <= ' ') {
                    this.next();
                } else if (ch_ == '/') {
                    switch (this.next()) {
                        case '/':
                            while (this.next() && ch_ != '\n' && ch_ != '\r') {}
                            break;
                        case '*':
                            this.next();
                            for (;;) {
                                if (ch_) {
                                    if (ch_ == '*') {
                                        if (this.next() == '/') {
                                            next();
                                            break;
                                        }
                                    } else {
                                        this.next();
                                    }
                                } else {
                                    error("Unterminated comment");
                                }
                            }
                            break;
                        default:
                            this.error("Syntax error");
                    }
                } else {
                    break;
                }
            }
        }
        private function error(m:String):void {
            throw new Error(m);
        }
        private function next():String {
            ch_ = text_.charAt(at_);
            at_ += 1;
            return ch_;
        }
        private function str():String {
            var s:String = '', u:Number;
            var outer:Boolean = false;

            if (ch_ == '"') {
                while (this.next()) {
                    if (ch_ == '"') {
                        this.next();
                        return s;
                    } else if (ch_ == '\\') {
                        switch (this.next()) {
                        case 'b':
                            s += '\b';
                            break;
                        case 'f':
                            s += '\f';
                            break;
                        case 'n':
                            s += '\n';
                            break;
                        case 'r':
                            s += '\r';
                            break;
                        case 't':
                            s += '\t';
                            break;
                        case 'u':
                            u = 0;
                            for (var i:int = 0; i < 4; i += 1) {
                                var t:Number = parseInt(this.next(), 16);
                                if (!isFinite(t)) {
                                    outer = true;
                                    break;
                                }
                                u = u * 16 + t;
                            }
                            if(outer) {
                                outer = false;
                                break;
                            }
                            s += String.fromCharCode(u);
                            break;
                        default:
                            s += ch_;
                        }
                    } else {
                        s += ch_;
                    }
                }
            }
            this.error("Bad string");
			return null;
        }
        private function arr():Array {
            var a:Array = [];

            if (ch_ == '[') {
                this.next();
                this.white();
                if (ch_ == ']') {
                    this.next();
                    return a;
                }
                while (ch_) {
                    a.push(this.value());
                    this.white();
                    if (ch_ == ']') {
                        this.next();
                        return a;
                    } else if (ch_ != ',') {
                        break;
                    }
                    this.next();
                    this.white();
                }
            }
            this.error("Bad array");
			return null;
        }
        private function obj():Object {
            var k:Object, o:Object = {};

            if (ch_ == '{') {
                this.next();
                this.white();
                if (ch_ == '}') {
                    this.next();
                    return o;
                }
                while (ch_) {
                    k = this.str();
                    this.white();
                    if (ch_ != ':') {
                        break;
                    }
                    this.next();
                    o[k] = this.value();
                    this.white();
                    if (ch_ == '}') {
                        this.next();
                        return o;
                    } else if (ch_ != ',') {
                        break;
                    }
                    this.next();
                    this.white();
                }
            }
            this.error("Bad object");
			return null;
        }
        private function num():Number {
            var n:String = '', v:Number;

            if (ch_ == '-') {
                n = '-';
                this.next();
            }
            while (ch_ >= '0' && ch_ <= '9') {
                n += ch_;
                this.next();
            }
            if (ch_ == '.') {
                n += '.';
                this.next();
                while (ch_ >= '0' && ch_ <= '9') {
                    n += ch_;
                    this.next();
                }
            }
            if (ch_ == 'e' || ch_ == 'E') {
                n += ch_;
                this.next();
                if (ch_ == '-' || ch_ == '+') {
                    n += ch_;
                    this.next();
                }
                while (ch_ >= '0' && ch_ <= '9') {
                    n += ch_;
                    this.next();
                }
            }
            v = Number(n);
            if (!isFinite(v)) {
                this.error("Bad number");
            }
            return v;
        }
        private function word():* {
            switch (ch_) {
                case 't':
                    if (this.next() == 'r' && this.next() == 'u' &&
                            this.next() == 'e') {
                        this.next();
                        return true;
                    }
                    break;
                case 'f':
                    if (this.next() == 'a' && this.next() == 'l' &&
                            this.next() == 's' && this.next() == 'e') {
                        this.next();
                        return false;
                    }
                    break;
                case 'n':
                    if (this.next() == 'u' && this.next() == 'l' &&
                            this.next() == 'l') {
                        this.next();
                        return null;
                    }
                    break;
            }
            this.error("Syntax error");
			return null;
        }
        private function value():* {
            this.white();
            switch (ch_) {
                case '{':
                    return this.obj();
                case '[':
                    return this.arr();
                case '"':
                    return this.str();
                case '-':
                    return this.num();
                default:
                    return ch_ >= '0' && ch_ <= '9' ? this.num() : this.word();
            }
        }
			
	    public function parse(text:String):Object {
	        text_ = text;
            at_ = 0;
	        ch_ = ' ';
	        return value();
	    }
	}
}