package org.whispercast.osd
{
	import flash.display.*;
	import flash.events.*;
	import flash.external.*;
	import flash.geom.*;
	import flash.net.*;
	import flash.system.*;
	import flash.text.*;
	import flash.utils.*;
	
	import mx.flash.UIMovieClip;
	
	import org.whispercast.*;
	
	public class OSD extends UIMovieClip
	{
		public var Skin:Object;
		
		private var elements_:Object = {};
		
		private var showOverlays_:Boolean = true;
		public function get showOverlays():Boolean
		{
			return showOverlays_;
		}
		
		private var showCrawlers_:Boolean = true;
		public function get showCrawlers():Boolean
		{
			return showCrawlers_;
		}
		
		private var fontSize_:Number = 12;
		public function get fontSize():Number
		{
			return fontSize_;
		}
		
		private var fontFamily_:String = 'Verdana,Arial,Helvetica';
		public function get fontFamily():String
		{
			return fontFamily_;
		}
		
		private var scale_:Number = 1;
		[Bindable]
		public function get scale():Number
		{
			return scale_;
		}
		public function set scale(value:Number):void
		{
			scale_ = value;
		}
		
		private var referenceWidth_:Number = 640;
		[Bindable]
		public function get referenceWidth():Number
		{
			return referenceWidth_;
		}
		public function set referenceWidth(value:Number):void
		{
			referenceWidth_ = value;
		}
		
		private var referenceHeight_:Number = 480;
		[Bindable]
		public function get referenceHeight():Number
		{
			return referenceHeight_;
		}
		public function set referenceHeight(value:Number):void
		{
			referenceHeight_ = value;
		}
		
		public override function initialize():void
		{
			super.initialize();
		}
	
		protected override function findFocusCandidates(obj:DisplayObjectContainer):void
		{
			if (obj is Element)
				return;
			
			super.findFocusCandidates(obj);
		}
		
		public function reset():void
		{
			for (var id:String in elements_)
			{
				elements_[id].destroy();
			}
			elements_ = {};
		}
		
		override public function setActualSize(newWidth:Number, newHeight:Number):void
		{
			var oldScale:Number = scale_;
			if (newWidth > newHeight)
				scale_ = newWidth/referenceWidth_;
			else
				scale_ = newHeight/referenceHeight_;
			
			for (var id:String in elements_)
			{
				var element:Element = elements_[id];
				layoutElement(element, newWidth, newHeight, scale_ != oldScale); 
			}
			
			var oldWidth:Number = _width;
			_width = newWidth;
			var oldHeight:Number = _height;
			_height = newHeight;
			
			if (sizeChanged(_width, oldWidth) || sizeChanged(_height, oldHeight))
				dispatchResizeEvent();
		}

		private var elementExtents_:Object = {};
		public function setElementExtent(id:String, type:String, extent:Rectangle, scale:Number):void
		{
			var lid:String = type+id;
			if (extent)
			{
				elementExtents_[lid] = {e:extent,s:scale};
				if (elements_[lid])
					elements_[lid].update({no_skin:true}, true);
			}
			else
			{
				if (elements_[lid])
					elements_[lid].update({no_skin:false}, true);
				delete elementExtents_[lid];
			}
		}
		
		public function layoutElement(element:Element, unscaledWidth:Number, unscaledHeight:Number, force:Boolean):void
		{
			var e:Object = elementExtents_[element.elementID];
			if (e)
				element.layout(e.e, e.s, force);
			else
			{
				var extent:Rectangle = new Rectangle(0, 0, isNaN(unscaledWidth) ? width : unscaledWidth, isNaN(unscaledHeight) ? height : unscaledHeight);
				element.layout(extent, scale_, force);
			}
		}
		
		public function closeElement(element:Element):void
		{
			var element:Element = elements_[element.elementID];
			if (element)
			{
				element.destroy();
				delete elements_[id];
			}
		}
		
		protected function createElement_(p:Object, type:String, c:Class):void
		{
			try
			{
				if (p.id)
				{
					var id:String = type+p.id;
					if (elementExtents_[id])
						p.no_skin = true;
					
					if (elements_[id])
					{
						elements_[id].update(p);
						Logger.debug(this, type+'[{0}] was updated to {1}', p.id, p);
					}
					else
					{
						var element:Element = new c();
						elements_[id] = element;
						
						element.create(id, this);
						addChild(element);
						
						element.update(p);
						Logger.debug(this, type+'[{0}] was created with {1}', p.id, p);
					}
				}
				else
				{
					Logger.error(this, 'No id specified in createElement("{1}"), parameters: {0}', p, type);
				}
			}
			catch(e:Error)
			{
				Logger.error(this, 'createElement({2}) failed with {0}, parameters: {1}', e, p, type);
			}
		}
		protected function destroyElement_(p:Object, type:String):void
		{
			try
			{
				if (p.id)
				{
					var id:String = type+p.id;
					if (elements_[id])
					{
						elements_[id].destroy();
						delete elements_[id];
						
						Logger.debug(this, type+'[{0}] was destroyed', p.id);
					}
					else
					{
						Logger.error(this, type+'[{0}] does not exist in destroyElement({2}), parameters: {1}', p.id, p, type);
					}
				}
				else
				{
					Logger.error(this, 'No id specified in destroyElement({1}), parameters: {0}', p, type);
				}
			}
			catch(e:Error)
			{
				Logger.error(this, 'destroyElement({2}) failed with {0}, parameters: {1}', e, p, type);
			}
		}
		
		/* Overlays */
		public function _osd_CreateOverlay(p:Object):void
		{
			createElement_(p, "overlay", Overlay);
		}
		public function _osd_DestroyOverlay(p:Object):void
		{
			destroyElement_(p, "overlay");
		}
		public function _osd_ShowOverlays(p:Object):void
		{
			if (p.show != showOverlays_)
			{
				showOverlays_ = p.show;
				for (var id:String in elements_)
				{
					var element:Element = elements_[id]; 
					if (element is Overlay)
						element.show(element.isShown);
				}
				Logger.debug(this, 'showOverlays was set to (0}', showOverlays_);
			}
		}
		
		/* Crawlers */
		public function _osd_CreateCrawler(p:Object):void
		{
			createElement_(p, "crawler", Crawler);
		}
		public function _osd_DestroyCrawler(p:Object):void
		{
			destroyElement_(p, "crawler");
		}
		public function _osd_ShowCrawlers(p:Object):void
		{
			if (p.show != showCrawlers_)
			{
				showCrawlers_ = p.show;
				for (var id:String in elements_)
				{
					var element:Element = elements_[id]; 
					if (element is Crawler)
						element.show(element.isShown);
				}
				Logger.debug(this, 'showCrawlers was set to (0}', showCrawlers_);
			}
		}
		
		protected function findCrawler_(id:String):Crawler
		{
			if (elements_["crawler"+id])
				return elements_["crawler"+id] as Crawler;
			return null;			
		}
		public function _osd_AddCrawlerItem(p:Object):void
		{
			try
			{
				if (p.crawler_id)
				{
					var crawler:Crawler = findCrawler_(p.crawler_id);
					if (crawler)
					{
						crawler.addCrawlerItem(p.item_id, p.item);
						Logger.debug(this, 'Item[{0}] in crawler[{1}] was created/updated with {2}', p.item_id, p.crawler_id, p);
					}
					else
					{
						Logger.error(this, 'Crawler[{0}] does not exist in AddCrawlerItem(), parameters: {1}', p.crawler_id, p);
					}
				}
				else
				{
					Logger.error(this, 'No crawler id specified in AddCrawlerCrawler(), parameters: {0}', p);
				}
			}
			catch(e:Error)
			{
				Logger.error(this, 'AddCrawlerItem() failed with {0}, parameters: {1}', e, p);
			}
		}
		public function _osd_RemoveCrawlerItem(p:Object):void
		{
			try
			{
				if (p.crawler_id)
				{
					var crawler:Crawler = findCrawler_(p.crawler_id);
					if (crawler)
					{
						crawler.removeCrawlerItem(p.item_id);
						Logger.debug(this, 'Item[{0}] was removed from crawler[{1}]', p.item_id, p.crawler_id);
					}
					else
					{
						Logger.error(this, 'Crawler[{0}] does not exist in RemoveCrawlerItem(), parameters: {1}', p.crawler_id, p);
					}
				}
				else
				{
					Logger.error(this, 'No crawler id specified in RemoveCrawlerCrawler(), parameters: {0}', p);
				}
			}
			catch(e:Error)
			{
				Logger.error(this, 'RemoveCrawlerItem() failed with {0}, parameters: {1}', e, p);
			}
		}
	}
}