package org.whispercast.osd
{
	import flash.display.*;
	import flash.events.*;
	
	import flashx.textLayout.formats.TextLayoutFormat;
	
	public class CrawlerPaneFactory extends EventDispatcher
	{
		private var crawler_:Crawler = null;
		public function get crawler():Crawler
		{
			return crawler_; 
		}
		
		private var items_:Array = [];
		public function get items():Array
		{
			return items_; 
		}
		
		private var height_:Number = 0;
		public function get height():Number
		{
			return height_;
		}
		
		private var next_:CrawlerPane = null;
		private var nextInvalidCount_:int = 0;
		
		public function create(crawler:Crawler):void
		{
			crawler_ = crawler;
			var crawlerItems:Object = (crawler.parameters as CrawlerParameters).items;
			
			var items:Array = [];
			for (var id:String in crawlerItems)
			{
				items.push({n:id,o:crawlerItems[id]});
			}
			items.sortOn("n");
			
			for (var i:int = 0; i < items.length; ++i)
			{
				var item:CrawlerItem = new CrawlerItem();
				item.create(crawler, items[i].o);
				items_.push(item);
			}
			
			createPaneInternal_((crawler.parameters as CrawlerParameters).continuous);
		}
		public function destroy():void
		{
			while (items_.length > 0)
				items_.shift().destroy();
			
			next_ = null;
			nextInvalidCount_ = 0;
		}
		
		public function createPane():CrawlerPane
		{
			if (next_ && nextInvalidCount_ == 0)
			{
				var pane:CrawlerPane = next_;
				createPaneInternal_((crawler.parameters as CrawlerParameters).continuous);
				return pane;
			}
			return null;				
		}
		private function createPaneInternal_(continuous:Boolean):void
		{
			next_ = new CrawlerPane();
			next_.create(this);
			
			nextInvalidCount_ = items_.length;
			
			for (var i:int = 0; i < items_.length; ++i)
			{
				var part:TextPart = new TextPart();
				part.addEventListener(Event.COMPLETE, onTextPartComplete_);
				
				next_.addChild(part);
				if (continuous || i < (items_.length-1))
					next_.addChild(createSeparator_());
				
				part.setContent(items[i].content, "crawler", NaN, NaN, items[i]);
			}
		}
		
		private function createSeparator_():Shape
		{
			var separator:Shape = new Shape();
			separator.graphics.clear();
			separator.graphics.beginFill(0, 0);
			separator.graphics.drawRect(0, 0, 16, 16);
			separator.graphics.endFill();
			separator.graphics.beginFill(crawler_.parameters.foreColor.color, crawler_.parameters.foreColor.alpha);
			separator.graphics.drawCircle(8, 8, 3);
			separator.graphics.endFill();
			return separator;
		}
		
		private function onTextPartComplete_(e:Event):void
		{
			e.target.removeEventListener(Event.COMPLETE, onTextPartComplete_);
			
			if (--nextInvalidCount_ == 0)
			{
				height_ = 0;
				for (var i:int = 0; i < next_.numChildren; ++i)
				{
					var child:DisplayObject = next_.getChildAt(i);
					if (child is TextPart)
						height_ = Math.max(height, (child as TextPart).getExtent().height);
					else
						height_ = Math.max(height, child.height);
				}
				next_.layout(height_);
				dispatchEvent(new Event(Event.COMPLETE));
			}
		}
	}
}