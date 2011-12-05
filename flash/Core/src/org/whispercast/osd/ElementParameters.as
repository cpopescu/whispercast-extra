package org.whispercast.osd
{
	internal class ElementParameters
	{
		// element's margins, relative to the container
		public var margins:Object = {top: 0, right: 0, bottom: 0, left: 0};
		
		// element's coordinates, relative to the container - margins
		public var x:Number = 0;
		public var y:Number = 0;
		
		// element's relative width, relative to the container - margins, 0 means 'auto'
		public var width:Number = 0;
		// element's relative height, relative to the container - margins, 0 means 'auto'
		public var height:Number = 0;
		
		// font
		public function get fontFamily():String
		{
			return osd_.fontFamily;
		}
		// font
		public function get fontSize():Number
		{
			return osd_.fontSize;
		}
		
		// foreground color
		public var foreColor:OSDColor = new OSDColor({color:'ffffff',alpha:1});
		// background color
		public var backColor:OSDColor = new OSDColor({color:'000000',alpha:0.7});
		// link color
		public var linkColor:OSDColor = new OSDColor({color:'ffff00',alpha:1});
		// hover color
		public var hoverColor:OSDColor = new OSDColor({color:'ffff00',alpha:1});
		
		// show the element
		public var show:Boolean = true;
		// the element time-to-live, in seconds
		public var ttl:Number = 0;
		
		// don't render any skin for the element
		public var noSkin:Boolean = false;
		
		private var osd_:OSD = null;
		
		public function ElementParameters(osd:OSD)
		{
			osd_ = osd;
		}
	}
}