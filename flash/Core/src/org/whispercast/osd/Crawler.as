package org.whispercast.osd
{
	import flash.display.*;
	import flash.events.*;
	import flash.filters.*;
	import flash.geom.*;
	import flash.utils.*;
	
	import mx.logging.*;
	
	internal class Crawler extends Element
	{
		private var borderSize_:int = 5;
		
		private var paneFactory_:CrawlerPaneFactory = null; 
		private var paneFactoryPending_:CrawlerPaneFactory = null; 
		
		private var holder_:Sprite = null;
		private var holderWidth_:Number = 0;
		
		private var advanceTimerId_:uint = 0;
		
		private var time_:Number = 0;
		private var delta_:Number = 0;
		
		private var virgin_:Boolean = true;
		
		public function Crawler()
		{
		}
		
		private var parameters_:CrawlerParameters = null;
		override public function get parameters():ElementParameters
		{
			return parameters_;
		}
		
		override public function get isReady():Boolean
		{
			return (paneFactory_ != null);
		}
		
		override public function get isShown():Boolean
		{
			return !destroyed_ && parameters_.show && osd_.showCrawlers;
		}
		
		override public function create(id:String, osd:OSD):void
		{
			parameters_ = new CrawlerParameters(osd);
			super.create(id, osd);
	
			holder_ = new Sprite();
			addChild(holder_);
			
			addEventListener(MouseEvent.CLICK, onMouseClick_);
			if (advanceTimerId_ == 0)
				advanceTimerId_ = setInterval(onAdvance_, 10); 
		}
		
		override protected function finalize():void
		{
			if (advanceTimerId_)
				clearInterval(advanceTimerId_);
			super.finalize();
		}
		
		override public function update(p:Object, layout:Boolean = false):void
		{
			var initializing:Boolean = false;
			if (!bounds_)
			{
				bounds_ = new Rectangle(50, 50, 100, 100);
				initializing = true;
			}
			super.update(p);
			
			var styleChanged:Boolean = false;
			if (p.fore_color != undefined)
				if (!parameters_.foreColor.equals(p.fore_color))
				{
					parameters_.foreColor = new OSDColor(p.fore_color);
					styleChanged = true;
				}
			if (p.back_color != undefined)
				if (!parameters_.backColor.equals(p.back_color))
				{
					parameters_.backColor = new OSDColor(p.back_color);
					styleChanged = true;
				}
			if (p.link_color != undefined)
				if (!parameters_.linkColor.equals(p.link_color))
				{
					parameters_.linkColor = new OSDColor(p.link_color);
					styleChanged = true;
				}
			if (p.hover_color != undefined)
				if (!parameters_.hoverColor.equals(p.hover_color))
				{
					parameters_.hoverColor = new OSDColor(p.hover_color);
					styleChanged = true;
				}
			
			var locationChanged:Boolean = false;
			if (p.y_location != undefined)
				if (p.y_location != parameters_.y)
				{
					parameters_.y = p.y_location;
					locationChanged = true;
				}
				
			var sizeChanged:Boolean = false;
			if (p.margins != undefined)
			{
				if (p.margins.left != parameters_.margins.left || p.margins.top != parameters_.margins.top || p.margins.right != parameters_.margins.right || p.margins.bottom != parameters_.margins.bottom)
				{
					parameters_.margins = p.margins;
					sizeChanged = true;
				}
			}
			
			var speedChanged:Boolean = false;
			if (p.speed != undefined)
				if (p.speed != parameters_.speed)
				{
					parameters_.speed = Math.max(0, p.speed);
					speedChanged = true;
				}
			
			var showChanged:Boolean = false;
			if (p.show != undefined)
				if (p.show != parameters_.show)
				{
					parameters_.show = p.show;
					showChanged = true;
				}
	
			
			var noSkinChanged:Boolean = false;
			if (p.no_skin != undefined)
			{
				if (p.no_skin != parameters_.noSkin)
				{
					parameters_.noSkin = p.no_skin;
					noSkinChanged = true;
				}
			}
			
			var contentChanged:Boolean = false;
			if (p.items != undefined)
			{
				parameters_.items = p.items;
				contentChanged = true;
			}
			
			if (styleChanged)
				contentChanged = true;
			
			if (contentChanged)
				updateContent_();
			else
				if (sizeChanged || locationChanged || noSkinChanged || layout)			
					osd_.layoutElement(this, NaN, NaN, locationChanged || noSkinChanged || layout); 
			
			if (showChanged)
				show(parameters_.show);
		}
		override public function layout(extent:Rectangle, scale:Number, force:Boolean):void
		{
			if (!paneFactory_)
				return;
			
			var skinMargins:Rectangle = new Rectangle();
			if (!parameters_.noSkin)
			{
				skinMargins.top = borderSize_;
				skinMargins.bottom = borderSize_;
			}
			
			var margins:Rectangle = new Rectangle();
			margins.left = parameters_.margins.left*extent.width+skinMargins.left;
			margins.top = parameters_.margins.top*extent.height+skinMargins.top;
			margins.right = parameters_.margins.right*extent.width+skinMargins.right;
			margins.bottom = parameters_.margins.bottom*extent.height+skinMargins.bottom;
			
			var bounds:Rectangle = new Rectangle(
				extent.left+margins.left,
				extent.top+margins.top,
				Math.max(0, extent.width - (margins.left+margins.right)),
				Math.max(0, extent.height - (margins.top+margins.bottom))
			);
			
			if (!force)
				if (scale == scale_)
					if (margins.equals(margins_))
						if (bounds.equals(bounds_)) return;
			
			var ratio:Number = holder_.x/bounds_.width; 
			
			scale_ = scale;
			
			margins_ = margins;
			bounds_ = bounds; 
			
			holder_.scaleX = holder_.scaleY = scale_;
			
			if (virgin_)
			{
				holder_.x = bounds_.width;
				virgin_ = false;
			}
			else
				if (!isNaN(ratio))
					holder_.x = ratio*bounds_.width;
			
			render_(Math.min(paneFactory_ ? scale_*(paneFactory_.height+skinMargins.top+skinMargins.bottom) : 0, bounds_.height));
			
			// this will also call show_(shown_)...
			super.layout(extent, scale, force);
		}
		
		private function render_(height:Number):void
		{
			var newMask:Sprite = new Sprite();
			addChild(newMask);
			newMask.graphics.clear();
			newMask.graphics.beginFill(0, 1);
			newMask.graphics.drawRect(0, 0, Math.floor(bounds_.width), height);
			newMask.graphics.endFill();
			
			if (mask)
				removeChild(mask);
			mask = newMask;
			
			var skinMargins:Rectangle = new Rectangle();
			if (!parameters_.noSkin)
			{
				graphics.clear();
				graphics.beginFill(0xff0000, 1);
				graphics.beginFill(parameters_.backColor.color, parameters_.backColor.alpha);
				graphics.drawRect(0, 0, Math.floor(bounds_.width), height);
				graphics.endFill();
				
				skinMargins.top = borderSize_;
				skinMargins.bottom = borderSize_;
			}
			
			holder_.y = skinMargins.top*scale_;
			
			x = bounds_.left;
			y = bounds_.top - skinMargins.top + (bounds_.height - height + (skinMargins.top+skinMargins.bottom))*parameters_.y;
		}
		
		protected function updateContent_():void
		{
			if (paneFactoryPending_)
			{
				paneFactoryPending_.removeEventListener(Event.COMPLETE, onPendingFactoryComplete_);
				paneFactoryPending_.destroy();
			}
			
			paneFactoryPending_ = new CrawlerPaneFactory();
			paneFactoryPending_.addEventListener(Event.COMPLETE, onPendingFactoryComplete_);
			
			paneFactoryPending_.create(this);
		}
		
		public function addCrawlerItem(id:String, item:Object):void
		{
			parameters_.items[id] = item;
			updateContent_();
		}
		public function removeCrawlerItem(id:String):void
		{
			if (parameters_.items[id])
			{
				delete parameters_.items[id];
				updateContent_();
			}
		}
		
		private function advanceNormal_(time:Number):void
		{
			var delta:Number = ((time-time_)*0.8+1.2*delta_)/2;
			
			var offset:Number = Math.ceil(-osd_.referenceWidth*(mouseOver_ ? parameters_.speed/10 : parameters_.speed)*delta/1000/scaleX);
			if (offset == 0)
				return;
			
			offset *= scaleX;
			if (!createPaneIfNeeded_(offset))
				return;
			
			if (holder_.numChildren > 0)
			{
				delta_ = delta;
				time_ = time;
				
				holder_.x += offset*scale_;
				if ((holder_.x+holderWidth_*holder_.scaleX)-offset < 0)
					if ((holder_.getChildAt(0) as CrawlerPane).factory == paneFactory_)
						holder_.x = bounds_.width+offset;
					else
					{
						holderWidth_ -= holder_.getChildAt(0).width; 
						holder_.removeChildAt(0);
					}
			}
		}
		private function advanceContinuous_(time:Number):void
		{
			var delta:Number = ((time-time_)*0.8+1.2*delta_)/2;
			
			var offset:Number = -osd_.referenceWidth*(mouseOver_ ? parameters_.speed/10 : parameters_.speed)*delta/1000;
			if (!createPaneIfNeeded_(offset))
				return;
			
			delta_ = delta;
			time_ = time;
			
			holder_.x += offset*scale_;
			
			var holderOffset:Number = 0;
			while (holder_.numChildren > 0)
			{
				var head:CrawlerPane = holder_.getChildAt(0) as CrawlerPane;
				
				var headBounds:Rectangle = head.getBounds(this);
				if (headBounds.right-offset < 0)
				{
					if (head.factory == paneFactory_)
					{
						if ((holder_.x+holderWidth_*holder_.scaleX) - bounds_.width < head.width*holder_.scaleX)
						{
							var tail:DisplayObject = holder_.getChildAt(holder_.numChildren-1);
							head.x = tail.x+tail.width;
							holder_.setChildIndex(head, holder_.numChildren-1);
							holderOffset += head.width*holder_.scaleX; 
						}
						else
						{
							holderOffset += head.width*holder_.scaleX;
							holderWidth_ -= head.width; 
							holder_.removeChildAt(0);
						}
					}
					else
					{
						holderOffset += head.width*holder_.scaleX;
						holderWidth_ -= head.width; 
						holder_.removeChildAt(0);
					}
				}
				else
					break;
			}
			
			for (var i:int = 0; i < holder_.numChildren; i++)
				holder_.getChildAt(i).x -= holderOffset/holder_.scaleX;
			
			holder_.x += holderOffset;
		}
		
		private function createPane_():Boolean
		{
			var pane:CrawlerPane = paneFactory_.createPane();
			if (pane)
			{
				pane.layout(paneFactory_.height);
				
				var previous:DisplayObject = (holder_.numChildren > 0) ? holder_.getChildAt(holder_.numChildren-1) : null;
				holder_.addChild(pane);
				
				if (previous)
					pane.x = previous.x+previous.width;
				else
					pane.x = 0;
				
				holderWidth_ += pane.width; 
				return true;
			}
			return false;
		}
		private function createPaneIfNeeded_(offset:Number):Boolean
		{
			if (paneFactory_)
			{
				if (holder_.numChildren == 0)
					return createPane_();
				else
					if (!parameters_.continuous) return true;
				
				var availableWidth:Number = bounds_.width - (parameters_.margins.left+parameters_.margins.right)*bounds_.width;
				if ((holder_.x +holderWidth_*holder_.scaleX) < (availableWidth+offset))
					return createPane_();
				else
					return true;
			}
			return false;
		}
		
		private function onAdvance_():void
		{
			if (visible)
				if (paneFactory_)
					if (parameters_.continuous)
						advanceContinuous_(getTimer());
					else
						advanceNormal_(getTimer());
		}
		
		private function onPendingFactoryComplete_(e:Event):void
		{
			paneFactoryPending_.removeEventListener(Event.COMPLETE, onPendingFactoryComplete_);
			
			if (paneFactory_)
				paneFactory_.destroy();
			else
			{
				virgin_ = true;
				time_ = getTimer();
			}

			paneFactory_ = paneFactoryPending_;

			osd_.layoutElement(this, NaN, NaN, true);
		}
		
		private function onMouseClick_(e:Event):void
		{
			e.stopImmediatePropagation();
		}
	}
}
