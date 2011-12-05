package org.whispercast.player
{
	import flash.events.Event;
	
	public class VideoPlayerEvent extends Event
	{
		public static const CLICK:String = "org.whispercast.player.VideoPlayer.Click";
		public static const PICTURE_IN_PICTURE:String = "org.whispercast.player.VideoPlayer.PIP";
		
		private var info_:Object = null;
		public function get info():Object
		{
			return info_;
		}
		
		public function VideoPlayerEvent(type:String, info:Object = null, bubbles:Boolean=false, cancelable:Boolean=false)
		{
			info_ = info;
			super(type, bubbles, cancelable);
		}
	}
}