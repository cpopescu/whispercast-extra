package org.whispercast.osd
{
	import flash.display.*;
	import flash.events.*;
	import flash.geom.*;
	import flash.text.Font;
	import flash.text.TextFormat;
	import flash.text.engine.*;
	
	import flashx.textLayout.container.*;
	import flashx.textLayout.conversion.*;
	import flashx.textLayout.elements.*;
	import flashx.textLayout.events.*;
	import flashx.textLayout.formats.*;
	
	import spark.utils.TextFlowUtil;
	
	public class TextPart extends Sprite
	{
		private var flow_:TextFlow;
		private var controller_:ContainerController;
		
		private var width_:Number = NaN;
		private var height_:Number = NaN;
		
		private var style_:Object = {};
		
		private var pending_:Object = {};
		
		public function TextPart()
		{
		}
		
		public function setContent(content:String, bodyClass:String = null, width:Number = Infinity, height:Number = Infinity, style:Object = null):void
		{
			if (flow_)
			{
				flow_.removeEventListener(DamageEvent.DAMAGE, onDamage_);
				flow_.removeEventListener(StatusChangeEvent.INLINE_GRAPHIC_STATUS_CHANGE, onStatusChange_);
				flow_.removeEventListener(CompositionCompleteEvent.COMPOSITION_COMPLETE, onCompositionComplete_);
				flow_.removeEventListener(UpdateCompleteEvent.UPDATE_COMPLETE, onUpdateComplete_);
				
				flow_.flowComposer.removeAllControllers();
				flow_.flowComposer = null;
				flow_ = null;
			}
			
			if (content)
			{
				content = content.replace(/\<TEXTFORMAT LEADING="[0-9]*"/gi, '<P').replace(/TEXTFORMAT\>/gi, 'P>');
				
				if (width != Infinity)
					width_ = width;
				if (height != Infinity)
					height_ = height;
				
				if (!controller_)
					controller_ = new ContainerController(this, width_, height_);

				var f:TextLayoutFormat = new TextLayoutFormat();
				f.kerning = Kerning.OFF;
				f.trackingLeft = f.trackingRight = 0;
				var c:Configuration = new Configuration();
				c.textFlowInitialFormat = f;
				
				flow_ = TextConverter.importToFlow((bodyClass ? '<body class="overlay">' : '')+content+(bodyClass ? '</body>' : ''), TextConverter.TEXT_FIELD_HTML_FORMAT, c);
				
				flow_.addEventListener(DamageEvent.DAMAGE, onDamage_);
				flow_.addEventListener(StatusChangeEvent.INLINE_GRAPHIC_STATUS_CHANGE, onStatusChange_);
				flow_.addEventListener(CompositionCompleteEvent.COMPOSITION_COMPLETE, onCompositionComplete_);
				
				flow_.addEventListener(UpdateCompleteEvent.UPDATE_COMPLETE, onUpdateComplete_);
				
				flow_.flowComposer.addController(controller_);
				
				if (style)
					setStyle(style);
				else
					updateStyle_();
			}
		}
		
		public function setStyle(style:Object):void
		{
			style_ = {};
			style_.foreColor = style.foreColor;  
			style_.linkColor = style.linkColor;  
			style_.hoverColor = style.hoverColor;
			
			style_.fontFamily = style.fontFamily;
			style_.fontSize = style.fontSize;

			updateStyle_();
		}
		
		public function setExtent(width:Number, height:Number):void
		{
			var nWidth:Number = (width != Infinity) ? width : width_; 
			var nHeight:Number = (height != Infinity) ? height : height_;
			
			if (isNaN(nWidth) && isNaN(width_) || nWidth == width_)
				if (isNaN(nHeight) && isNaN(height_) || nHeight == height_)
				{
					if (checkIfComplete_())
						dispatchComplete_();
					
					return;
				}
			
			width_ = nWidth;
			height_ = nHeight;
			
			controller_.setCompositionSize(width_ = nWidth, NaN);
			update_(true);
		}
		public function getExtent():Rectangle
		{
			if (controller_)
			{
				var extent:Rectangle = controller_.getContentBounds();
				extent.left *= scaleX;
				extent.top *= scaleY;
				extent.right *= scaleX;
				extent.bottom *= scaleY;
				return extent;
			}
			return null;
		}
		
		private function update_(dispatch:Boolean = false):Boolean
		{
			if (flow_)
				if (flow_.flowComposer.updateAllControllers())
					return true;
			
			if (dispatch)
				dispatchComplete_()
			return false;
		}
		private function updateStyle_():void
		{
			if (flow_)
			{
				if (style_.foreColor)
					flow_.color = style_.foreColor.color;
				
				if (style_.fontFamily)
					flow_.fontFamily = style_.fontFamily; 
				if (style_.fontSize)
					flow_.fontSize = style_.fontSize;
				
				if (style_.linkColor)
				{
					flow_.linkNormalFormat = new TextLayoutFormat(); 
					flow_.linkNormalFormat.color = style_.linkColor.color;
				}
				if (style_.hoverColor)
				{
					flow_.linkHoverFormat = new TextLayoutFormat(); 
					flow_.linkHoverFormat.color = style_.hoverColor.color;
				}
				
				update_(true);
			}
		}
		
		private function checkIfComplete_():Boolean
		{
			var stageMin:int = 100;
			var stageMax:int = -100;
			
			for (var id:String in pending_)
			{
				stageMin = Math.min(stageMin, pending_[id]); 
				stageMax = Math.max(stageMax, pending_[id]);
			}
			
			//trace('checkIfComplete: '+stageMin+','+stageMax);
			if (stageMin == stageMax)
				if (stageMin < 2)
					return update_(false);
				else
					return true;
			else
				if (stageMin == 1)
					update_(false);
			
			return stageMin == 100;
		}
		private function dispatchComplete_():void
		{
			//trace('dispatch ('+getExtent().width+','+getExtent().height+')');
			dispatchEvent(new Event(Event.COMPLETE));
		}
		
		private function onStatusChange_(e:StatusChangeEvent):void
		{
			var g:InlineGraphicElement = e.element as InlineGraphicElement;
			
			var id:String = e.element.getAbsoluteStart().toString();
			//trace('statusChange: '+e.status+': '+g.source+' '+id);
			switch (e.status)
			{
				case InlineGraphicElementStatus.LOADING:
					pending_[id] = 0;
					break;
				case InlineGraphicElementStatus.SIZE_PENDING:
					pending_[id] = 1;
					checkIfComplete_();
					break;
				case InlineGraphicElementStatus.READY:
				case InlineGraphicElementStatus.ERROR:
					var pid:int = pending_[id];
					pending_[id] = 2;
					if (checkIfComplete_())
						if (pid == 0) dispatchComplete_();
					break;
			}
		}
		private function onUpdateComplete_(e:UpdateCompleteEvent):void
		{
			//trace('complete');
			addEventListener(Event.ENTER_FRAME, function(e:Event):void {
				if (checkIfComplete_())
					dispatchComplete_();
			});
		}
		private function onCompositionComplete_(e:CompositionCompleteEvent):void
		{
			//trace('compositionComplete');
		}
		
		private function onDamage_(e:DamageEvent):void
		{
		}
	}
}