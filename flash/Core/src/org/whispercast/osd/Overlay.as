package org.whispercast.osd
{
	import flash.display.*;
	import flash.events.*;
	import flash.filters.*;
	import flash.geom.*;
	import flash.system.ApplicationDomain;
	import flash.text.*;
	import flash.utils.*;
	
	import mx.core.*;
	import mx.graphics.*;
	import mx.utils.ColorUtil;
	
	internal class Overlay extends Element
	{
		private var textPart_:TextPart = null;
		private var textPartPending_:TextPart = null;
		
		private var pending_:Boolean = false;
		private var invalid_:Boolean = false;
		
		private var wordWrapping_:int = 0;
		
		private var background_:SpriteAsset;
		private var closeButton_:SimpleButton;
		
		public function Overlay()
		{
		}
		
		private var parameters_:OverlayParameters = null;
		override public function get parameters():ElementParameters
		{
			return parameters_;
		}
		
		override public function get isShown():Boolean
		{
			return !destroyed_ && parameters_.show && osd_.showOverlays;
		}
		override public function get isReady():Boolean
		{
			return (textPart_ != null) && !invalid_;
		}
		
		override public function create(id:String, osd:OSD):void
		{
			parameters_ = new OverlayParameters(osd);
			super.create(id, osd);
			
			filters = [new DropShadowFilter(0, 0, 0, 1, 4, 4, 1, BitmapFilterQuality.HIGH)];
			
			addEventListener(MouseEvent.CLICK, onClick_);
		}
		
		override public function update(p:Object, layout:Boolean = false):void
		{
			var initializing:Boolean = false;
			if (!bounds_)
			{
				bounds_ = new Rectangle(0, 0, 0, 0);
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
			if (p.x_location != undefined)
				if (p.x_location != parameters_.x)
				{
					parameters_.x = p.x_location;
					locationChanged = true;
				}
			if (p.y_location != undefined)
				if (p.y_location != parameters_.y)
				{
					parameters_.y = p.y_location;
					locationChanged = true;
				}

			var sizeChanged:Boolean = false;
			if (p.width != undefined)
				if (p.width != parameters_.width)
				{
					parameters_.width = p.width;
					sizeChanged = true;
				}
			if (p.margins != undefined)
			{
				if (p.margins.left != parameters_.margins.left || p.margins.top != parameters_.margins.top || p.margins.right != parameters_.margins.right || p.margins.bottom != parameters_.margins.bottom)
				{
					parameters_.margins = p.margins;
					sizeChanged = true;
				}
			}
			
			var showChanged:Boolean = false;
			if (p.show != undefined)
			{
				if (p.show != parameters_.show)
				{
					parameters_.show = p.show;
					showChanged = true;
				}
			}
			var closableChanged:Boolean = false;
			if (p.closable != undefined)
			{
				if (p.closable != parameters_.closable)
				{
					parameters_.closable = p.closable;
					closableChanged = true;
				}
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
				
			var contentChanged:Boolean = initializing;
			if (p.content != undefined)
				if (p.content != parameters_.content)
				{
					parameters_.content = p.content;
					contentChanged = true;
				}
			
			if (styleChanged)
				contentChanged = true;
			
			if (contentChanged)
				updateContent_();
			else
			if (contentChanged || sizeChanged || locationChanged || closableChanged || noSkinChanged || layout)
				osd_.layoutElement(this, NaN, NaN, sizeChanged || locationChanged || closableChanged || noSkinChanged || layout); 
			
			if (showChanged)
				show(parameters_.show);
		}
		override public function layout(extent:Rectangle, scale:Number, force:Boolean):void
		{
			var hasBackground:Boolean = !parameters_.noSkin;
			if (hasBackground)
			{
				if (!background_)
				{
					background_ = new osd_.Skin.OverlayBackground();
					addChildAt(background_, 0);
				}
			}
			else
				if (background_)
				{
					if (background_.parent)
						background_.parent.removeChild(background_);
					background_ = null;
				}
			
			var hasCloseButton:Boolean = !parameters_.noSkin && parameters_.closable;
			if (hasCloseButton)
			{
				if (!closeButton_)
				{
					closeButton_ = new osd_.Skin.OverlayCloseButton();
					addChildAt(closeButton_, textPart_ ? getChildIndex(textPart_)+1 : getChildIndex(background_)+1);
					
					closeButton_.addEventListener(MouseEvent.CLICK, onClose_);
				}
			}
			else
				if (closeButton_)
				{
					if (closeButton_.parent)
						closeButton_.parent.removeChild(background_);
					closeButton_ = null;
				}
			
				
			var skinMargins:Rectangle = new Rectangle();
			skinMargins.left = background_ ? background_.scale9Grid.left : 0;
			skinMargins.top = background_ ? background_.scale9Grid.top : 0;
			//skinMargins.right = (background_ ? (background_.measuredWidth-background_.scale9Grid.right) : 0) + (closeButton_ ? closeButton_.width+2 : 0);
			skinMargins.right = background_ ? (background_.measuredWidth-background_.scale9Grid.right) : 0;
			skinMargins.bottom = background_ ? (background_.measuredHeight-background_.scale9Grid.bottom) : 0;
			
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
			
			scale_ = scale;
			
			margins_ = margins;
			bounds_ = bounds; 
			
			if (textPart_)
			{
				textPart_.scaleX = textPart_.scaleY = scale_;
				
				wordWrapping_ = 0;
				textPart_.setExtent((parameters_.width == 0) ? NaN : bounds_.width*parameters_.width/scale_, bounds_.height/scale_);
			}
			
			updateSkinState_();
			
			// rendering will take place in onTextPartComplete_...
			
			// this will also call show_(shown_)...
			super.layout(extent, scale, force);
		}
		
		private function render_():void
		{
			var skinMargins:Rectangle = new Rectangle();
			skinMargins.left = background_ ? background_.scale9Grid.left : 0;
			skinMargins.top = background_ ? background_.scale9Grid.top : 0;
			//skinMargins.right = (background_ ? (background_.measuredWidth-background_.scale9Grid.right) : 0) + (closeButton_ ? closeButton_.width+2 : 0);
			skinMargins.right = background_ ? (background_.measuredWidth-background_.scale9Grid.right) : 0;
			skinMargins.bottom = background_ ? (background_.measuredHeight-background_.scale9Grid.bottom) : 0;
			
			var bounds:Rectangle = textPart_.getExtent();
			if (parameters_.width != 0)
				bounds.width = bounds_.width*parameters_.width;

			bounds.offset(bounds_.left-bounds.left, bounds_.top-bounds.top);
			if (bounds.right > bounds_.right)
				bounds.right = bounds_.right;
			if (bounds.bottom > bounds_.bottom)
				bounds.bottom = bounds_.bottom;
			
			var newMask:Sprite = new Sprite();
			textPart_.addChild(newMask);
			newMask.graphics.clear();
			newMask.graphics.beginFill(0, 1);
			newMask.graphics.drawRect(-1, -1, Math.ceil(bounds.width/textPart_.scaleX+2), Math.ceil(bounds.height/textPart_.scaleY+2));
			newMask.graphics.endFill();
			
			if (textPart_.mask)
				textPart_.removeChild(textPart_.mask);
			textPart_.mask = newMask;
			
			if (closeButton_)
			{
				closeButton_.x = (skinMargins.left+skinMargins.right) + bounds.width - closeButton_.width-4; 
				closeButton_.y = 4;
				
				var r:Number = ((parameters.backColor.color >> 16) & 0xff)/0xff;
				var g:Number = ((parameters.backColor.color >> 8) & 0xff)/0xff;
				var b:Number = ((parameters.backColor.color) & 0xff)/0xff;
				
				var br:Number = (0.2126*r) + (0.7152*g) + (0.0722*b);
				
				var cButtonCross:ColorTransform = new ColorTransform();
				cButtonCross.color = parameters.foreColor.color;//ColorUtil.adjustBrightness2(parameters.backColor.color, (br < 0.6) ? 90 : -90);
				cButtonCross.alphaMultiplier = 1;

				var cButtonBackground:ColorTransform = new ColorTransform();
				cButtonBackground.color = parameters.backColor.color;
				cButtonBackground.alphaMultiplier = 1;
				
				(closeButton_.downState as DisplayObjectContainer).getChildAt(0).transform.colorTransform = cButtonBackground;
				(closeButton_.upState as DisplayObjectContainer).getChildAt(0).transform.colorTransform = cButtonBackground;
				(closeButton_.overState as DisplayObjectContainer).getChildAt(0).transform.colorTransform = cButtonBackground;
				
				(closeButton_.downState as DisplayObjectContainer).getChildAt(1).transform.colorTransform = cButtonCross;
				(closeButton_.upState as DisplayObjectContainer).getChildAt(1).transform.colorTransform = cButtonCross;
				(closeButton_.overState as DisplayObjectContainer).getChildAt(1).transform.colorTransform = cButtonCross;
				
				closeButton_.filters = [
					new DropShadowFilter(0, 0, (br < 0.6) ? 0 : 0xffffff, 0.7, 4, 4, 1, BitmapFilterQuality.HIGH)
				];
			}
			
			if (background_)
			{
				background_.width = bounds.width+(skinMargins.left+skinMargins.right);
				background_.height = bounds.height+(skinMargins.top+skinMargins.bottom);
				
				var cBackround:ColorTransform = new ColorTransform();
				cBackround.color = parameters.backColor.color;
				cBackround.alphaMultiplier = parameters.backColor.alpha;
				background_.transform.colorTransform = cBackround;
			}
			
			x = bounds_.left - skinMargins.left + parameters_.x*Math.max(0, bounds_.width-bounds.width);
			y = bounds_.top - skinMargins.top + parameters_.y*Math.max(0, bounds_.height-bounds.height);
			
			textPart_.x = skinMargins.left;
			textPart_.y = skinMargins.top;
		}
		
		private function updateContent_():void
		{
			if (textPartPending_)
			{
				textPartPending_.removeEventListener(Event.COMPLETE, onPendingTextPartComplete_);
				textPartPending_.setContent(null);
			}
			else
				textPartPending_ = new TextPart();

			pending_ = true;
			
			textPartPending_.addEventListener(Event.COMPLETE, onPendingTextPartComplete_);
			textPartPending_.setContent(parameters_.content, "overlay", NaN, NaN, parameters_);
		}
		
		private function updateSkinState_():void
		{
			if (background_)
				background_.alpha = mouseOver_ ? 6*parameters_.backColor.alpha/5 : parameters_.backColor.alpha;
			if (closeButton_)
				closeButton_.visible = mouseOver_;
		}
		
		override protected function onShowComplete_():void
		{
			if (!destroyed_)
				if ((textPartPending_ != null) && !pending_)
				{
					if (textPart_)
					{
						if (textPart_.parent)
							textPart_.parent.removeChild(textPart_);
						textPart_.setContent(null);
					}
					
					textPartPending_.removeEventListener(Event.COMPLETE, onPendingTextPartComplete_);
					
					textPart_ = textPartPending_;
					textPartPending_ = null;
					
					textPart_.addEventListener(Event.COMPLETE, onTextPartComplete_);
					addChildAt(textPart_, background_ ? 1 : 0);
					
					osd_.layoutElement(this, NaN, NaN, true); 
				}
			
			super.onShowComplete_();
		}
		
		override protected function onMouseOver_(e:Event):void
		{
			super.onMouseOver_(e);
			updateSkinState_();
		}
		override protected function onMouseOut_(e:Event):void
		{
			super.onMouseOut_(e);
			updateSkinState_();
		}
		
		private function onPendingTextPartComplete_(e:Event):void
		{
			pending_ = false;
			invalid_ = true;
			
			show(isShown);
		}
		private function onTextPartComplete_(e:Event):void
		{
			var extent:Rectangle = textPart_.getExtent();
			if (wordWrapping_ == 0)
				if (parameters_.width == 0)
					if (extent.width > bounds_.width)
					{
						wordWrapping_ = 1;
						textPart_.setExtent(bounds_.width*parameters_.width/scale_, bounds_.height/scale_);
						return;
					}
			
			if (wordWrapping_ != 2)
				if (extent.width == 0 && extent.height == 0)
				{
					wordWrapping_ = 2;
					textPart_.setExtent(NaN, NaN);
					return;
				}

			invalid_ = false;
			render_();
			
			show(isShown);
		}
		
		private function onClick_(e:Event):void
		{
			e.stopImmediatePropagation();
		}
		
		private function onClose_(e:Event):void
		{
			osd_.closeElement(this);
		};
	}
}
