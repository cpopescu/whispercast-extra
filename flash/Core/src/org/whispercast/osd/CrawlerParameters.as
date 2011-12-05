package org.whispercast.osd
{
	internal class CrawlerParameters extends ElementParameters
	{
		// items
		public var items:Object = {};
		// if true, the items are flowing continuously
		public var continuous:Boolean = true;
		// speed, expressed as percent of the width scrolled in 1 second
		public var speed:Number = 0.15;
		
		public function CrawlerParameters(osd:OSD):void
		{
			super(osd);
		}
	}
}