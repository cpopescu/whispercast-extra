package org.whispercast.osd
{
	internal class OverlayParameters extends ElementParameters
	{
		// content
		public var content:String = '';
		// the 'user can close this' flag
		public var closable:Boolean = false;
		
		public function OverlayParameters(osd:OSD)
		{
			super(osd);
		}
	}
}