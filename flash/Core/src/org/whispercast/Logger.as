package org.whispercast
{
	import flash.external.ExternalInterface;
	import flash.system.Capabilities;
	import flash.utils.describeType;
	
	public class Logger
	{
		private static var levels_:Object = 
		{
			info:null,
			debug:null,
			warn:null,
			error:null
		};
		
		private static var buffer_:String = "";
		
		private static function write_(level:Function, message:String):void
		{
			buffer_ += message;
		}
		private static function flush_(level:Function):void
		{
			if (buffer_)
			{
				for (var i:int = 0; i < groupIndent_; ++i)
					buffer_ = "\t"+buffer_; 
				level.call(null, buffer_);
			}
			buffer_ = "";
		}
		
		private static var groupBegin_:Function = null;
		private static var groupEnd_:Function = null;
		
		private static var groupIndent_:int = 0;
		
		private static function beginGroup_(level:Function, kind:String):void
		{
			if (groupBegin_ != null && groupEnd_ != null)
			{
				groupBegin_.call(null, buffer_ + kind);
				buffer_ = "";
			}
			else
			{
				write_(level, "<"+kind+">");
				flush_(level);
				++groupIndent_;
			}
		}
		private static function endGroup_(level:Function, kind:String):void
		{
			if (groupBegin_ != null && groupEnd_ != null)
				groupEnd_.call(null, kind);
			else
			{
				flush_(level);
				--groupIndent_;
				write_(level, "</"+kind+">");
			}
		}
		
		private static function expand_(level:Function, object:*, type:String):void
		{
			if (type == "Object" || type == "Array")
			{
				beginGroup_(level, type);
				for (var id:String in object)
				{
					output_(level, object[id], id);
					flush_(level);
				}
				if (type == "Array")
					for (var i:int = 0; i < object.length; ++i)
					{
						output_(level, object[i], String(i));
						flush_(level);
					}
				endGroup_(level, type);
			}
			else
			{
				var description:XML = describeType(object);
				if (description..accessor.length())
				{
					beginGroup_(level, type);
					for each(var item: XML in description..accessor)
					{
						if (item.@access && item.@access != "writeonly") 
						{
							var value:*;
							try
							{
								value = object[item.@name]
							}
							catch(e:Error)
							{
								output_(level, "# "+e.message +" #", item.@name);
								flush_(level);
								
								continue;
							}
							
							output_(level, value, item.@name);
							flush_(level);
						}
					}					
					endGroup_(level, type);
				}
				else
					output_(level, String(object));
			}
		}
		private static function output_(level:Function, object:*, name:String = null):void
		{
			if (name)
				write_(level, name+" = ");
			
			if (object === undefined)
			{
				write_(level, "UNDEFINED");
				return;
			}
			
			var type:String = describeType(object).@name;
			switch (type)
			{
				case "void":
				case "null":
					write_(level, type.toUpperCase());
					return;
				case "Boolean":
					write_(level, object ? "TRUE" : "FALSE");
					return;
				case "int":
				case "uint":
				case "Number":
					write_(level, object);
					return;
				case "String":
					write_(level, '"'+object+'"');
					return;
			}
			expand_(level, object, type);
		}
		
		private static var initialized_:Boolean = false;
		private static function initialize_():void
		{
			var browser:Boolean = (Capabilities.playerType == "ActiveX" || Capabilities.playerType == "PlugIn");
			if (browser && ExternalInterface.available)
			{
				if (ExternalInterface.call( "function(){ return typeof window.console == 'object'}"))
				{
					var log:Function = null;
					if (ExternalInterface.call( "function(){ return typeof console.log == 'function'}"))
					{
						log = function(message:String):void {
							ExternalInterface.call("console.log", message);
						}
					}
					
					if (ExternalInterface.call( "function(){ return typeof console.groupCollapsed == 'function'}"))
					{
						groupBegin_ = function(kind:String):void {
							ExternalInterface.call("console.groupCollapsed", kind);
						}
					}
					if (ExternalInterface.call( "function(){ return typeof console.groupEnd == 'function'}"))
					{
						groupEnd_ = function(kind:String):void {
							ExternalInterface.call("console.groupEnd");
						}
					}
					
					if (ExternalInterface.call( "function(){ return typeof console.info == 'function'}"))
					{
						levels_.info = function(message:String):void {
							ExternalInterface.call("console.info", message);
						}
					}
					else
						levels_.info = log;
					if (ExternalInterface.call( "function(){ return typeof console.debug == 'function'}"))
					{
						levels_.debug = function(message:String):void {
							ExternalInterface.call("console.debug", message);
						}
					}
					else
						levels_.debug = log;
					if (ExternalInterface.call( "function(){ return typeof console.warn == 'function'}"))
					{
						levels_.warn = function(message:String):void {
							ExternalInterface.call("console.warn", message);
						}
					}
					else
						levels_.warn = log;
					if (ExternalInterface.call( "function(){ return typeof console.error == 'function'}"))
					{
						levels_.error = function(message:String):void {
							ExternalInterface.call("console.error", message);
						}
					}
					else
						levels_.error = log;
				}
			}
		}
		
		private static var contextProperties_:Array = ["id", "name"];
		private static function log_(context:*, level:String, message:String, objects:Array):void
		{
			var category:String = null;
			
			var categoryName:String = describeType(context).@name;
			if (categoryName == "String")
				category = context;
			else
			{
				for (var i:int =0; i < contextProperties_.length; ++i)
				{
					try
					{
						category = context[contextProperties_[i]];
					}
					catch(e:Error)
					{
						continue;
					}
					break;
				}
				
				if (category === null)
				{
					try
					{
						category = describeType(context).@name;
					}
					catch(e:Error)
					{
						category = (context as Object).toString();
					}
				}
			}
			
			if (!initialized_)
			{
				initialize_();
				initialized_ = true;
			}

			var levelf:Function = levels_[level];
			if (levelf != null)
			{
				write_(levelf, (new Date()).toLocaleTimeString()+" - ["+category+"] - ");
				
				var parts:Array = message.split(new RegExp("{(\\d+)}", "g"));
				for (i = 0;;)
				{
					if (parts[i])
						write_(levelf, parts[i]);
					if (++i >= parts.length)
						break;
					
					var index:Number = parseInt(parts[i]);
					if (!isNaN(index) && (index >= 0 && index < objects.length))
						output_(levelf, objects[index]);
					else
						write_(levelf, "# FORMAT ERROR #");
					
					if (++i >= parts.length)
						break;
				}
				flush_(levelf);
			}
		}
		
		private static var logLevels_:int = 0;
		public static function setLevels(levels:int):void
		{
			logLevels_ = levels;
		}
		
		public static const INFO:int = 1;
		public static const DEBUG:int = 2;
		public static const WARNING:int = 4;
		public static const ERROR:int = 8;
		
		public static function info(context:*, message:String = null, ...objects): void
		{
			if (logLevels_ & INFO)
				log_(context, "info", ""+message, objects);			
		}
		public static function debug(context:*, message:String = null, ...objects): void
		{
			if (logLevels_ & DEBUG)
				log_(context, "debug", ""+message, objects);			
		}
		public static function warning(context:*, message:String = null, ...objects): void
		{
			if (logLevels_ & WARNING)
				log_(context, "warn", ""+message, objects);			
		}
		public static function error(context:*, message:String = null, ...objects): void
		{
			if (logLevels_ & ERROR)
				log_(context, "error", ""+message, objects);			
		}
	}
}