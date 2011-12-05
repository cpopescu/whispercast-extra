package whispercast.osd
{
import flash.events.*;
import flash.display.*;
import flash.utils.*;
import flash.geom.*;
import flash.filters.*;

import fl.motion.Color;

import whispercast.*;
import whispercast.osd.*;

public class Crawler extends Element {
	private var close_btn_:DisplayObject;
	private var close_btn_class_name_:String = 'Osd_Default_Close';
	
	private var parameters_:Parameters = new Parameters();
	public function get parameters():Parameters {
		return parameters_;
	}
	
	private var items_:Array = new Array();
	private var items_by_id_:Object = {};
	public function get items():Array {
		return items_;
	}
	
	private var items_height_:Number = 0;
	
	private var holder_:Sprite;
	
	private var cookie_:int = 0;
	private var frame_cookie_:int = -1;
	public function get cookie():int {
		return cookie_;
	}
	
	private var virgin_:Boolean = true;
	
	private var time_:Number = getTimer();
	
	private var mouse_over_:Boolean = false;
	
	private var ttl_timer_id_:uint = 0;
	
	public function Crawler(owner:Osd) {
		super(owner);
	}
	
	public override function get shown():Boolean {
		return parameters_.show_ && owner.show_crawlers;
	}
	
	public override function create(container:DisplayObjectContainer, w:Number, h:Number, sx:Number, sy:Number):void {
		super.create(container, w, h, sx, sy);

		mask = new Sprite();
		container.addChild(mask);
		
		holder_ = new Sprite();
		addChild(holder_);
		
		visible = false;
		
		stage.addEventListener(Event.ENTER_FRAME, _onEnterFrame);
		
		addEventListener(MouseEvent.MOUSE_OUT, _onMouseOut);
		addEventListener(MouseEvent.MOUSE_OVER, _onMouseOver);
		
		addEventListener(MouseEvent.CLICK, _onMouseClick);
	}
	public override function destroy():void {
		if (ttl_timer_id_ != 0) {
			flash.utils.clearTimeout(ttl_timer_id_);
			ttl_timer_id_ = 0;
		}
		super.destroy();
	}
	
	protected override function finalize():void {
		removeEventListener(MouseEvent.CLICK, _onMouseClick);
		
		removeEventListener(MouseEvent.MOUSE_OVER, _onMouseOver);
		removeEventListener(MouseEvent.MOUSE_OUT, _onMouseOut);
		
		stage.removeEventListener(Event.ENTER_FRAME, _onEnterFrame);
		
		super.finalize();
	}
	
	public override function update(p:Object):void {
		if (p) {
			var update_style:Boolean = false;
			if (p.fore_color != undefined) {
				parameters_.fore_color_ = new whispercast.osd.Color(p.fore_color);
				update_style = true;
			}
			if (p.back_color != undefined) {
				parameters_.back_color_ = new whispercast.osd.Color(p.back_color);
				update_style = true;
			}
			if (p.link_color != undefined) {
				parameters_.link_color_ = new whispercast.osd.Color(p.link_color);
				update_style = true;
			}
			if (p.hover_color != undefined) {
				parameters_.hover_color_ = new whispercast.osd.Color(p.hover_color);
				update_style = true;
			}
			
			if (update_style) {
				cookie_++;
			}
			
			if (p.y_location != undefined) {
				parameters_.y_location_ = p.y_location;
			}
				
			if (p.margins != undefined) {
				parameters_.margins_ = p.margins;
			}
			
			if (p.items != undefined) {
				var id:String;
				for (id in items_by_id_) {
					removeCrawlerItem(id);
				}
				for (id in p.items) {
					addCrawlerItem(id, p.items[id]);
				}
			}
			
			if (p.speed != undefined) {
				parameters_.speed_ = Math.max(0, p.speed);
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
				ttl_timer_id_ = flash.utils.setTimeout(
					function() {
						ttl_timer_id_ = 0;
						destroy();
					}, 1000*parameters_.ttl_);
			}
		}
		super.update(p);
	}
	
	public override function layout(w:Number, h:Number, sx:Number, sy:Number):void {
		var crawler_height:Number = 0; 
		
		if (close_btn_ != null) {
			removeChild(close_btn_);
			close_btn_ = null;
		}
			
		if (holder_.numChildren > 0) {
			var child_idx:int;
			var previous:Pane;
			
			var distances:Array = new Array();
			
			for (child_idx = 1; child_idx < holder_.numChildren; child_idx++) {
				previous = Pane(holder_.getChildAt(child_idx-1));
				distances.push(holder_.getChildAt(child_idx).x - (previous.x+previous.width));
			}
				
			for (var item_idx:int = 0; item_idx < items_.length; item_idx++) {
				Item(items_[item_idx]).layout(sx, sy);
			}
				
			previous = null;
			for (child_idx = 0; child_idx < holder_.numChildren; child_idx++) {
				var pane:Pane = Pane(holder_.getChildAt(child_idx));
				pane.layout(sx, sy);
				
				crawler_height = Math.max(crawler_height, pane.height);
				
				if (previous) {
					pane.x = (previous.x+previous.width) + distances[child_idx-1]*w/width_;
				}
				previous = pane;
			}
		} else {
			// TODO: Best guess..
			var item:Item = new Item();
			item.create(this);
			item.update({content:''});
			item.layout(sx, sy);
			crawler_height = item.createTextField(sx, sy).height;
		}
		
		width_ = w;
		height_ = h;
		
		sx_ = sx;
		sy_ = sy;
		
		items_height_ = crawler_height;
		
		var gm:Matrix = new Matrix();
		gm.createGradientBox(w, crawler_height, -Math.PI/2, 0, 0);
		
		graphics.clear();
		graphics.beginGradientFill(
			GradientType.LINEAR,
			[
			fl.motion.Color.interpolateColor(0x000000, parameters_.back_color_.color_, 0.7),
			fl.motion.Color.interpolateColor(0x000000, parameters_.back_color_.color_, 0.8),
			fl.motion.Color.interpolateColor(0x000000, parameters_.back_color_.color_, 1)
			],
			[parameters_.back_color_.alpha_, parameters_.back_color_.alpha_, parameters_.back_color_.alpha_],
			[0, 96, 255],
			gm
		);
		graphics.drawRect(0, 0, w, crawler_height);
		graphics.endFill();

		x = 0;
		y = owner_.scale*parameters_.margins_.top*h + parameters_.y_location_*(h*(1-owner_.scale*(parameters_.margins_.top+parameters_.margins_.bottom))-crawler_height);
		
		if (parameters_.hideable_) {
			var close_btn_class_ref:Class = Class(getDefinitionByName(close_btn_class_name_));
			close_btn_ = new close_btn_class_ref();
			
			MovieClip(close_btn_).buttonMode = true;
			close_btn_.addEventListener(MouseEvent.CLICK, _onCloseClick);
			
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
				
			close_btn_.scaleX =
			close_btn_.scaleY =
			Math.max(Math.max(1, sx*owner_.scale/2), Math.max(1, sy*owner_.scale/2));
		
			var filters_:Array = MovieClip(close_btn_).background_mc.filters;
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
			
			close_btn_.x = close_btn_.width/3;

			if (y > close_btn_.height/3) {
				close_btn_.y = -close_btn_.height/3;
			} else {
				close_btn_.y = crawler_height-close_btn_.height/3;
			}
			if (close_btn_.scaleY < 0) {
				close_btn_.y += close_btn_.height;
			}

			addChildAt(close_btn_, numChildren);
		}
		
		var m:Sprite = Sprite(mask);
		m.graphics.clear();
		m.graphics.beginFill(0, 1);
		m.graphics.drawRect(0, 0, w, h);
		m.graphics.endFill();
		
		mask.x = x;
		mask.y = 0;
	}
	
	public function addCrawlerItem(id:String, p:Object) {
		if (id) {
			var item:Item = items_by_id_[id];
			if (!item) {
				item = new Item();
				items_.push(item);
				items_by_id_[id] = item;
				
				item.create(this);
				item.update(p);
				
				item.layout(sx_, sy_);
				
				cookie_++;
			} else {
				removeCrawlerItem(id);
				// will recurse...
				addCrawlerItem(id, p);
			}
		}
	}
	public function removeCrawlerItem(id:String) {
		if (id) {
			var item:Item = items_by_id_[id];
			if (item) {
				item.destroy();
				
				items_.splice(items_.indexOf(item), 1);
				delete items_by_id_[id];
				
				virgin_ = (items_.length == 0);
				
				cookie_++;
			}
		}
	}
	
	private function createPane():Boolean {
		if (items_.length > 0) {
			var pane:Pane = new Pane();
			
			pane.create(this);
			pane.update(sx_, sy_);
	
			if (holder_.numChildren > 0) {
				var previous:DisplayObject = holder_.getChildAt(holder_.numChildren-1);
				if (virgin_) {
					pane.x = Math.max(width_-holder_.x, previous.x + previous.width);
				} else {
					pane.x = previous.x+previous.width;
				}
			} else {
				pane.x = width_-holder_.x;
			}
			
			holder_.addChild(pane);
			virgin_ = false;
			
			if (pane.height != items_height_)
				layout(width_, height_, sx_, sy_);
			
			return true;
		}
		return false;
	}
	
	private function checkPadding(offset:Number):void {
		if (items_.length == 0)
			return;

		while (true) {
			if (holder_.numChildren > 1)
			if ((holder_.x+holder_.width) > (2*width_+offset)) {
				break;
			}
			createPane();
		}
	}
	
	private function _onCloseClick(e:Event) {
		close(true, true);
		e.stopImmediatePropagation();
	}
	
	private function _onMouseOut(e:Event) {
		mouse_over_ = false;
	}
	private function _onMouseOver(e:Event) {
		mouse_over_ = true;
	}
	
	private function _onMouseClick(e:Event) {
		e.stopImmediatePropagation();
	}
	
	private function _onEnterFrame(e:Event) {
		var time:Number = getTimer();
		if (items_.length == 0)
		if (holder_.numChildren == 0) {
			time_ = time;
			return;
		}
	
		var child_idx:int;
		var pane:Pane;
		
		if (frame_cookie_ != cookie_) {
			var offset_x = 0;
			for (child_idx = 0; child_idx < holder_.numChildren; child_idx++) {
				pane = Pane(holder_.getChildAt(child_idx));
				if (virgin_) {
					if (pane.x > (width_-holder_.x)+pane.separator_width) {
						break;
					}
				}
				else
					if (pane.x > (width_-holder_.x)) {
						break;
					}
					
				offset_x = pane.x + pane.width;
			}
			while (child_idx < holder_.numChildren) {
				if (items_.length > 0) {
					pane = Pane(holder_.getChildAt(child_idx));
					pane.update(sx_, sy_)
					
					pane.x = offset_x;
					offset_x += pane.width;
					
					child_idx++;
				} else {
					holder_.removeChildAt(child_idx);
				}
			}
			
			if (virgin_)
			if (holder_.numChildren > 0) {
				Pane(holder_.getChildAt(holder_.numChildren-1)).removeSeparator();
			}
		}
		
		var offset:Number = -width_*(mouse_over_ ? 0.005 : parameters_.speed_)*(time-time_)/1000;
		checkPadding(-offset);
		
		holder_.x += offset;
		time_ = time;

		var holder_offset:Number = 0;

		while (holder_.numChildren > 0) {
			var head:DisplayObject = holder_.getChildAt(0);
			if (holder_.x + (head.x + head.width) + holder_offset < 0) {
				holder_offset += head.width;
				
				if (items_.length > 0) {
					if ((holder_.x+holder_.width-head.width) > (2*width_+offset)) {
						Pane(head).update(sx_, sy_);
						
						var tail:DisplayObject = holder_.getChildAt(holder_.numChildren-1);
						head.x = tail.x+tail.width;
						
						holder_.setChildIndex(head, holder_.numChildren-1);
					} else {
						holder_.removeChildAt(0);
					}
				} else {
					holder_.removeChildAt(0);
				}
				continue;
			}
			break;
		}
		
		for (child_idx = 0; child_idx < holder_.numChildren; child_idx++) {
			pane = Pane(holder_.getChildAt(child_idx));
			pane.x -= holder_offset;
		}
		
		holder_.x += holder_offset;
		frame_cookie_ = cookie_;
	}
}
}

import flash.events.*;
import flash.display.*;
import flash.text.*;

import whispercast.osd.*

class Parameters {
	// foreground color
	public var fore_color_:whispercast.osd.Color = new whispercast.osd.Color({color:'ffffff',alpha:1});
	// background color
	public var back_color_:whispercast.osd.Color = new whispercast.osd.Color({color:'000000',alpha:0.7});
	// link color
	public var link_color_:whispercast.osd.Color = new whispercast.osd.Color({color:'ffff00',alpha:1});
	// hover color
	public var hover_color_:whispercast.osd.Color = new whispercast.osd.Color({color:'ffff00',alpha:1});
	
	// y coordinate, relative to the container
	public var y_location_:Number = 0;
	
	// overlay's margins, relative to the container
	public var margins_:Object = {top: 0, right: 0, bottom: 0, left: 0};
	
	// speed, expressed as percent of the width scrolled in 1 second
	public var speed_:Number = 0.15;
	
	// show the crawler
	public var show_:Boolean = true;
	
	// the overlay TTL
	public var ttl_:Number = 0;
	
	// the 'user can hide this' flag
	public var hideable_:Boolean = false;
}

class Item {
	// content
	private var content_:String;
	
	// foreground color
	private var fore_color_:whispercast.osd.Color;
	// background color
	private var back_color_:whispercast.osd.Color;
	// link color
	private var link_color_:whispercast.osd.Color;
	// hover color
	private var hover_color_:whispercast.osd.Color;
	
	public function get fore_color():whispercast.osd.Color {
		return fore_color_ ? fore_color_ : crawler_.parameters.fore_color_;
	}
	public function get back_color():whispercast.osd.Color {
		return back_color_ ? back_color_ : crawler_.parameters.back_color_;
	}
	public function get link_color():whispercast.osd.Color {
		return link_color_ ? link_color_ : crawler_.parameters.link_color_;
	}
	public function get hover_color():whispercast.osd.Color {
		return hover_color_ ? hover_color_ : crawler_.parameters.hover_color_;
	}
	
	public function get content():String {
		return content_;
	}
	
	private var bitmap_:BitmapData;
	
	public function get bitmap():BitmapData {
		return bitmap_;
	}
	
	private var crawler_:Crawler;
	
	public function get crawler():Crawler {
		return crawler_;
	}
	
	static private var has_links_:RegExp = /<a(?!r)[^>]*>(.*?)<\/a[^>]*>/gi;
	
	public function Item() {
	}
	
	public function create(crawler:Crawler) {
		crawler_ = crawler;
	}
	public function destroy() {
	}
	
	public function update(p:Object) {
		if (p.fore_color != undefined) {
			fore_color_ = new whispercast.osd.Color(p.fore_color);
		}
		if (p.back_color != undefined) {
			back_color_ = new whispercast.osd.Color(p.back_color);
		}
		if (p.link_color != undefined) {
			link_color_ = new whispercast.osd.Color(p.link_color);
		}
		if (p.hover_color != undefined) {
			hover_color_ = new whispercast.osd.Color(p.hover_color);
		}
			
		if (p.content != undefined) {
			content_ = p.content;
		}
	}
	
	public function layout(sx:Number, sy:Number):void {
		has_links_.lastIndex = 0;
		if (!has_links_.test(content_))
			try {
				var text_field:TextField = createTextField(sx, sy);
				bitmap_ = new BitmapData(text_field.width, text_field.height, true, 0x00000000);
				var scale:flash.geom.Matrix = new flash.geom.Matrix();
				scale.scale(text_field.scaleX, text_field.scaleY);
				bitmap_.draw(text_field, scale);
			} catch(e) {
				bitmap_ = null;
			}
		else {
			bitmap_ = null;
		}
	}
	public function createTextField(sx:Number, sy:Number):TextField {
		var text_field:TextField = new TextField();
		text_field.selectable = false;
		text_field.multiline = false;
		text_field.wordWrap = false;
		text_field.blendMode = BlendMode.LAYER;
		text_field.autoSize = TextFieldAutoSize.LEFT;
		
		text_field.alpha = fore_color.alpha_;
		
		var ss:StyleSheet = crawler_.owner.style_sheet_clone();
		if (ss.styleNames.indexOf('body') < 0) {
			ss.setStyle('body', {color:fore_color.toHtml(), fontFamily:crawler_.owner.font_family, fontSize:crawler_.owner.font_size});
		}
		if (ss.styleNames.indexOf('a') < 0) {
			ss.setStyle('a', {color:link_color.toHtml()});
		}
		if (ss.styleNames.indexOf('a:hover') < 0) {
			ss.setStyle('a:hover', {color:hover_color.toHtml(), textDecoration:'underline'});
		}
		text_field.styleSheet = ss;
		
		text_field.width = 1;
		text_field.height = 1;
		text_field.htmlText = '<body class="crawler">'+Osd.convertHtml(content_)+'</body>';
		
		text_field.scaleX = text_field.scaleY = sy*crawler_.owner.scale;
		return text_field;
	}
}

class Pane extends Sprite {
	private var crawler_:Crawler;
	
	public function get crawler():Crawler {
		return crawler_;
	}
	
	private var cookie_:int = -1;
	
	private var separator_width_:Number = 0;
	
	public function get separator_width():Number {
		return separator_width_;
	}
	
	public function Pane() {
	}
	
	public function create(crawler:Crawler) {
		crawler_ = crawler;
	}
	public function destroy() {
	}
	
	public function update(sx:Number, sy:Number):Boolean {
		if (cookie_ != crawler_.cookie) {
			layout(sx, sy);
			cookie_ = crawler_.cookie;
			return true;
		}
		return false;
	}
	
	public function layout(sx:Number, sy:Number):void {
		while (numChildren > 0) {
			removeChildAt(0);
		}
		
		var separator:Shape;
		separator_width_ = 0;
		
		var offset_x:Number = 0;
		
		var items:Array = crawler_.items;
		for (var item_idx:int = 0; item_idx < items.length; item_idx++) {
			var item = Item(items[item_idx]);
			item.layout(sx, sy);

			if (item.bitmap) {
				var bitmap:Bitmap = new Bitmap(item.bitmap);
				bitmap.x = offset_x;
				addChild(bitmap);
				offset_x += bitmap.width;
			} else {
				var text_field:TextField = item.createTextField(sx, sy);
				text_field.x = offset_x;
				addChild(text_field);
				offset_x += text_field.width;
			}
			
			separator = createSeparator(sx, sy, getChildAt(numChildren-1).height);
			separator.x = offset_x;
			addChild(separator);
			offset_x += separator.width;
			
		}
		separator_width_ = separator ? separator.width : 0;
	}
	
	private function createSeparator(sx:Number, sy:Number, h:Number):Shape {
		var separator:Shape = new Shape();
		separator.graphics.clear();
		separator.graphics.beginFill(0, 0);
		separator.graphics.drawRect(0, 0, h, h);
		separator.graphics.endFill();
		separator.graphics.beginFill(crawler.parameters.fore_color_.color_, crawler.parameters.fore_color_.alpha_);
		separator.graphics.drawCircle(h/2-1, h/2, h/5);
		separator.graphics.endFill();
		
		return separator;
	}
	public function removeSeparator():void {
		if (numChildren > 1) {
			removeChildAt(numChildren-1);
		}
	}
}