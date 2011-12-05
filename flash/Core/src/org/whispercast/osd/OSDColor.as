package org.whispercast.osd
{
	internal class OSDColor extends Object
	{
		private var color_:uint;
		public function get color():uint
		{
			return color_;
		}
		private var alpha_:Number;
		public function get alpha():Number
		{
			return alpha_;
		}
		
		public function OSDColor(p:Object)
		{
			color_ = parseInt(p.color, 16);
			alpha_ = p.alpha;
		}
		public function toHtml():String
		{
			var c:String = '000000'+color_.toString(16);
			return '#'+c.substring(c.length - 6, c.length);
		}
		
		public function equals(p:Object):Boolean
		{
			var c:OSDColor = new OSDColor(p);
			return c.color_ == color_ && c.alpha_ == alpha_;
		}
	}
}