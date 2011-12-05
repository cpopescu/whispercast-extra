package org.whispercast.osd
{
	import flash.display.*;
	import flash.events.*;
	import flash.geom.*;
	import flash.system.*;
	import flash.utils.*;
	
	import mx.core.IVisualElementContainer;
	import mx.core.UIComponent;
	import mx.effects.Tween;
	import mx.effects.easing.Cubic;
	import mx.effects.easing.Elastic;
	import mx.effects.easing.Quadratic;
	import mx.effects.easing.Quintic;
	import mx.effects.easing.Sine;
	import mx.flash.ContainerMovieClip;
	import mx.flash.UIMovieClip;
	import mx.managers.ILayoutManagerClient;
	
	internal class Element extends MovieClip
	{
		protected var elementID_:String;
		public function get elementID():String
		{
			return elementID_;
		}
		
		protected var osd_:OSD;
		public function get osd():OSD
		{
			return osd_;
		}
		
		public function get parameters():ElementParameters
		{
			return null;
		}
		
		protected var visible_:Boolean = false;
		protected var destroyed_:Boolean = false;
		
		protected var scale_:Number = 1;
		
		protected var margins_:Rectangle = new Rectangle();
		protected var bounds_:Rectangle = new Rectangle();
		
		protected var mouseOver_:Boolean = false;
		
		private var alphaTween_:Tween = null;
		private var ttlTimerId_:uint = 0;
		
		public function Element()
		{
		}
		
		override public function getBounds(targetCoordinateSpace:DisplayObject):Rectangle
		{
			if (targetCoordinateSpace != parent)
			{
				var tl:Point = parent.localToGlobal(bounds_ ? bounds_.topLeft : new Point(0, 0));
				tl = targetCoordinateSpace.globalToLocal(tl);
				var br:Point = parent.localToGlobal(bounds_ ? bounds_.bottomRight : new Point(0, 0));
				br = targetCoordinateSpace.globalToLocal(br);
				
				return new Rectangle(tl.x, tl.y, br.x-tl.x, br.y-tl.y);
			}
			trace(bounds_.left+","+bounds_.top+","+bounds_.width+","+ bounds_.height);
			return bounds_;
		}

		public function get isShown():Boolean
		{
			return !destroyed_;
		}
		public function get isReady():Boolean
		{
			return true;
		}
		
		
		public function create(id:String, osd:OSD):void
		{
			elementID_ = id;
			osd_ = osd;
			
			visible_ = visible = false;
			alpha = 0;
			
			addEventListener(MouseEvent.MOUSE_OVER, onMouseOver_);
			addEventListener(MouseEvent.MOUSE_OUT, onMouseOut_);
		}
		public function destroy():Boolean
		{
			if (ttlTimerId_ != 0)
			{
				flash.utils.clearTimeout(ttlTimerId_);
				ttlTimerId_ = 0;
			}
			destroyed_ = true;
			
			if (visible_)
			{
				show(false);
				return false;
			}
			
			finalize();
			return true;
		}
		
		protected function finalize():void
		{
			if (parent)
				parent.removeChild(this);
		}
		
		public function adjustMargins(margins:Rectangle):void
		{
		}
		
		public function update(p:Object, layout:Boolean = false):void
		{
			var ttlChanged:Boolean = false;
			if (p.ttl != undefined)
			{
				ttlChanged = true;
				parameters.ttl = p.ttl;
			}
			
			if (ttlChanged)
			{
				if (ttlTimerId_ != 0)
				{
					flash.utils.clearTimeout(ttlTimerId_);
					ttlTimerId_ = 0;
				}
				if ((parameters.ttl > 0) && (ttlTimerId_ == 0))
				{
					ttlTimerId_ = flash.utils.setTimeout(function():void {flash.utils.clearTimeout(ttlTimerId_); ttlTimerId_ = 0; destroy()}, 1000*parameters.ttl);
				}
			}
		}
		public function layout(extent:Rectangle, scale:Number, force:Boolean):void
		{
			show(isShown);
		}
		
		public function show(show:Boolean):Boolean
		{
			show = show && isReady;
			if (show && !visible_)
			{
				visible_ = true;
				if (alphaTween_)
					return false;
				
				visible = true;
				
				alphaTween_ = new Tween(this, 0, 1, 1000, -1, onAlphaTweenUpdate_, onAlphaTweenFinish_);
				alphaTween_.easingFunction = mx.effects.easing.Cubic.easeInOut;
			}
			else
				if (!show && visible_)
				{
					visible_ = false;
					if (alphaTween_)
						return false;
					
					alphaTween_ = new Tween(this, 1, 0, 1000, -1, onAlphaTweenUpdate_, onAlphaTweenFinish_);
					alphaTween_.easingFunction = mx.effects.easing.Cubic.easeInOut;
				}
				else
					if (!alphaTween_)
					{
						onShowComplete_();
						return true;
					}
			
			return false;
		}
		protected function onShowComplete_():void
		{
			if (destroyed_)
				finalize();
		}
		
		protected function onMouseOver_(e:Event):void
		{
			mouseOver_ = true;
		}
		protected function onMouseOut_(e:Event):void
		{
			mouseOver_ = false;
		}
		
		private function onAlphaTweenUpdate_(value:Number):void
		{
			alpha = value;
		}
		private function onAlphaTweenFinish_(value:Number):void
		{
			alpha = value;
			alphaTween_ = null;
			
			visible = (value == 1);
			if (visible_ != visible)
			{
				visible_ = !visible_;
				show(!visible_);
				
				return;
			}
			onShowComplete_();
		}
	}
}