package org.whispercast.osd
{
	import flash.display.*;
	import flash.geom.Rectangle;
	import flash.text.*;
	
	internal class CrawlerPane extends Sprite
	{
		private var factory_:CrawlerPaneFactory;
		public function get factory():CrawlerPaneFactory
		{
			return factory_;
		}
		
		public function CrawlerPane() {
		}
		
		public function create(factory:CrawlerPaneFactory):void
		{
			factory_ = factory;
		}
		public function destroy():void
		{
		}
		
		public function layout(height:Number):void
		{
			var offsetX:Number = 0;
			for (var i:int = 0; i < numChildren; ++i)
			{
				var child:DisplayObject = getChildAt(i);
				child.x = offsetX;
				
				if (child is TextPart)
				{
					child.y = (height - (child as TextPart).getExtent().height)/2
					offsetX += (child as TextPart).getExtent().width;
				}
				else
				{
					child.y = (height - child.height)/2;
					offsetX += child.width;
				}
			}
		}
	}
}