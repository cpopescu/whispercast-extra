package whispercast.osd
{
import flash.events.*;
import flash.display.*;
import flash.utils.*;
import flash.geom.*;
import flash.text.*;
import flash.filters.*;

import fl.transitions.*;
import fl.transitions.easing.*;

import whispercast.*;
import whispercast.osd.*;

public class Overlay extends Element{
	private var parameters_:Parameters = new Parameters();
	
	private var frame_:DisplayObject;
	private var frame_class_name_:String = 'Osd_Overlay_Default_Frame';
	
	private var close_btn_:DisplayObject;
	private var close_btn_class_name_:String = 'Osd_Default_Close';
	
	private var text_field_:TextField;
	
	private var ttl_timer_id_:uint = 0;
	
	public function Overlay(owner:Osd) {
		super(owner);
	}
	
	public override function get shown():Boolean {
		return !destroyed_ && parameters_.show_ && owner.show_overlays;
	}
	
	public override function create(container:DisplayObjectContainer, w:Number, h:Number, sx:Number, sy:Number):void {
		text_field_ = new TextField();
		text_field_.selectable = false;
		text_field_.multiline = true;
		text_field_.blendMode = BlendMode.LAYER;
		text_field_.autoSize = TextFieldAutoSize.NONE;
		
		super.create(container, w, h, sx, sy);
		addEventListener(MouseEvent.CLICK, _onMainClick);
	}
	
	public override function destroy():void {
		if (ttl_timer_id_ != 0) {
			flash.utils.clearTimeout(ttl_timer_id_);
			ttl_timer_id_ = 0;
		}
		super.destroy();
	}
	
	protected override function finalize():void {
		super.finalize();
	}
	
	public override function update(p:Object):void {
		if (p) {
			if (p.fore_color != undefined) {
				parameters_.fore_color_ = new whispercast.osd.Color(p.fore_color);
			}
			if (p.back_color != undefined) {
				parameters_.back_color_ = new whispercast.osd.Color(p.back_color);
			}
			if (p.link_color != undefined) {
				parameters_.link_color_ = new whispercast.osd.Color(p.link_color);
			}
			if (p.hover_color != undefined) {
				parameters_.hover_color_ = new whispercast.osd.Color(p.hover_color);
			}
			
			if (p.width != undefined)
				parameters_.width_ = Math.max(0, Math.min(1, p.width));
				
			if (p.x_location != undefined)
				parameters_.x_location_ = Math.max(0, Math.min(1, p.x_location));
			if (p.y_location != undefined)
				parameters_.y_location_ = Math.max(0, Math.min(1, p.y_location));
	
			if (p.margins != undefined)
				parameters_.margins_ = p.margins;
				
			if (p.content != undefined) {
				parameters_.content_ = p.content;
			}
	
			if (p.show != undefined) {
				parameters_.show_ = p.show;
			}
				
			if (p.hideable != undefined) {
				parameters_.hideable_ = p.hideable;
			}
				
			var update_ttl_:Boolean = false;
			if (p.ttl != undefined) {
				update_ttl_ = true;
				parameters_.ttl_ = p.ttl;
			}
				
			if (update_ttl_) {
				if (ttl_timer_id_ != 0) {
					flash.utils.clearTimeout(ttl_timer_id_);
					ttl_timer_id_ = 0;
				}
			}
			if ((parameters_.ttl_ > 0) && (ttl_timer_id_ == 0)) {
				ttl_timer_id_ = flash.utils.setTimeout(function() {ttl_timer_id_ = 0;destroy()}, 1000*parameters_.ttl_);
			}
		}
		
		super.update(p);
	}
	
	public override function layout(w:Number, h:Number, sx:Number, sy:Number):void {
		if (frame_ != null) {
			removeChild(frame_);
			frame_ = null;
			
			if (close_btn_ != null) {
				removeChild(close_btn_);
				close_btn_ = null;
			}
			
			removeChild(text_field_);
		}
		
		var frame_class_ref:Class = Class(getDefinitionByName(frame_class_name_));
		frame_ = new frame_class_ref();
		
		if (parameters_.hideable_) {
			var close_btn_class_ref:Class = Class(getDefinitionByName(close_btn_class_name_));
			close_btn_ = new close_btn_class_ref();
			
			MovieClip(close_btn_).buttonMode = true;
			close_btn_.addEventListener(MouseEvent.CLICK, _onCloseClick);
		}
		
		var dr:Number = uint(parameters_.back_color_.color_/(256*256))&0xff;
		var dg:Number = uint(parameters_.back_color_.color_/256)&0xff;
		var db:Number = uint(parameters_.back_color_.color_)&0xff;
		
		var cm:Array = new Array(
			0.3*dr/255, 0.59*dr/255, 0.11*dr/255, 0, 0,
			0.3*dg/255, 0.59*dg/255, 0.11*dg/255, 0, 0,
			0.3*db/255, 0.59*db/255, 0.11*db/255, 0, 0,
			0         , 0          , 0          , parameters_.back_color_.alpha_, 0
		);
		var cmf:ColorMatrixFilter = new ColorMatrixFilter(cm);
		
		var sf:DropShadowFilter = new DropShadowFilter();
		sf.distance = 0;
		sf.blurX = 5*sx;
		sf.blurY = 5*sy;
		sf.quality = 5;
			
		MovieClip(frame_).filters = [
			cmf,
			sf
		];
		if (close_btn_) {
			close_btn_.scaleX =
			close_btn_.scaleY =
			Math.max(Math.max(1, sx*owner_.scale/2), Math.max(1, sy*owner_.scale/2));
		
			var  filters_:Array = MovieClip(close_btn_).background_mc.filters;
			filters_.push(cmf);
			filters_.push(sf);
			MovieClip(close_btn_).background_mc.filters = filters_;
			
			dr = uint(parameters_.fore_color_.color_/(256*256))&0xff;
			dg = uint(parameters_.fore_color_.color_/256)&0xff;
			db = uint(parameters_.fore_color_.color_)&0xff;
		
			var cmc:ColorMatrixFilter = new ColorMatrixFilter([
  				  0.3*dr/255, 0.59*dr/255, 0.11*dr/255, 0, 0,
				  0.3*dg/255, 0.59*dg/255, 0.11*dg/255, 0, 0,
				  0.3*db/255, 0.59*db/255, 0.11*db/255, 0, 0,
				  0, 0, 0, 1, 0
				]);
			
			MovieClip(close_btn_).cross_mc.filters = [
				cmc
			];
		}
		
		// the slice margins
		var slice_w:Number = 0;
		var slice_h:Number = 0;
		if (frame_.scale9Grid) {
			slice_w = frame_.scale9Grid.left+(frame_.width-frame_.scale9Grid.right);
			slice_h = frame_.scale9Grid.top+(frame_.height-frame_.scale9Grid.bottom);
		}
		
		text_field_.alpha = parameters_.fore_color_.alpha_;
		updateTextField();
		// the layout extent (container extent - margins)
		var layout_w:Number = w*(1 - (parameters_.margins_.left+parameters_.margins_.right));
		var layout_h:Number = h*(1 - (parameters_.margins_.top+parameters_.margins_.bottom));
		whispercast.Logger.log('osd:layout','Overlay::layout() - LAYOUT ['+layout_w+','+layout_h+']');

		var available_w:Number = (parameters_.width_ > 0) ? layout_w*parameters_.width_ : layout_w;
		var available_h:Number = layout_h;
		whispercast.Logger.log('osd:layout','Overlay::layout() - AVAILABLE ['+available_w+','+available_h+']');
		
		available_w -= slice_w;
		available_h -= slice_h;
		
		text_field_.scaleX = sx*owner_.scale;
		text_field_.scaleY = sy*owner_.scale;
		
		for (var step:int = (parameters_.width_ == 0) ? 0 : 1; step < 2; step++) {
			text_field_.autoSize = TextFieldAutoSize.LEFT;
			if (step == 0) {
				text_field_.wordWrap = false;
			} else {
				text_field_.wordWrap = true;
			}
			
			text_field_.height = 1;
			text_field_.width = (step == 0) ? 1 : available_w/text_field_.scaleX;
			whispercast.Logger.log('osd:layout','Overlay::layout() - TEXTFIELD STEP '+step+' ['+text_field_.width+','+text_field_.height+']');
			
			if (step > 0) {
				var th:Number = text_field_.height + text_field_.maxScrollV;
				text_field_.autoSize = TextFieldAutoSize.NONE;
				text_field_.height = th/text_field_.scaleY;
			}
			
			if ((text_field_.width+slice_w) <= available_w) {
				break;
			}
		}
		
		text_field_.x = (frame_.scale9Grid ? frame_.scale9Grid.left : 0);
		text_field_.y = (frame_.scale9Grid ? frame_.scale9Grid.top : 0);
		whispercast.Logger.log('osd:layout','Overlay::layout() - TEXTFIELD POSITION ['+text_field_.x+','+text_field_.y+']');
		
		var f_width = text_field_.width+slice_w;
		f_width = Math.min(2048, f_width);
		var f_height = text_field_.height+slice_h-1;
		f_height = Math.min(2048, f_height);
		
		frame_.x = 0;
		frame_.y = 0;
		frame_.width = f_width;
		frame_.height = f_height;
		
		addChild(frame_);
		addChild(text_field_);

		var newX:Number = w*owner_.scale*parameters_.margins_.left + parameters_.x_location_*(layout_w - f_width);
		var newY:Number = h*owner_.scale*parameters_.margins_.top + parameters_.y_location_*(layout_h - f_height);
		
		if (close_btn_) {
			close_btn_.x = f_width - close_btn_.width;
			if (newY > close_btn_.height/3) {
				close_btn_.y = -close_btn_.height/3;
			} else {
				close_btn_.y = f_height-close_btn_.height/3;
			}
			if (close_btn_.scaleY < 0) {
				close_btn_.y += close_btn_.height;
			}
			addChild(close_btn_);
		}
		
		x = newX;
		y = newY;
		
		super.layout(w, h, sx, sy);
	}
	
	private function updateTextField():void {
		var ss:StyleSheet = owner_.style_sheet_clone();
		if (ss.styleNames.indexOf('body') < 0) {
			ss.setStyle('body', {color:parameters_.fore_color_.toHtml(), fontFamily:owner_.font_family, fontSize:owner_.font_size});
		}
		if (ss.styleNames.indexOf('a') < 0) {
			ss.setStyle('a', {color:parameters_.link_color_.toHtml()});
		}
		if (ss.styleNames.indexOf('a:hover') < 0) {
			ss.setStyle('a:hover', {color:parameters_.hover_color_.toHtml(), textDecoration:'underline'});
		}
		text_field_.styleSheet = ss;
		
		text_field_.htmlText = '<body class="overlay">'+Osd.convertHtml(parameters_.content_)+'</body>';
	}
	
	private function _onCloseClick(e:Event) {
		close(true, true);
		e.stopImmediatePropagation();
	}
	private function _onMainClick(e:Event) {
		e.stopImmediatePropagation();
	}
}
}

class Parameters {
	// content
	public var content_:String = '';
	
	// foreground color
	public var fore_color_:whispercast.osd.Color = new whispercast.osd.Color({color:'ffffff',alpha:1});
	// background color
	public var back_color_:whispercast.osd.Color = new whispercast.osd.Color({color:'000000',alpha:0.7});
	// link color
	public var link_color_:whispercast.osd.Color = new whispercast.osd.Color({color:'ffff00',alpha:1});
	// hover color
	public var hover_color_:whispercast.osd.Color = new whispercast.osd.Color({color:'ffff00',alpha:1});
	
	// overlay's coordinates, relative to the container
	public var x_location_:Number = 0;
	public var y_location_:Number = 0;
	
	// overlay's margins, relative to the container
	public var margins_:Object = {top: 0, right: 0, bottom: 0, left: 0};
	
	// overlay's relative width
	public var width_:Number = 0;
	
	// show the overlay
	public var show_:Boolean = true;
	
	// the time-to-live
	public var ttl_:Number = 0;
	
	// the 'user can hide this' flag
	public var hideable_:Boolean = false;
};