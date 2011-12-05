package whispercast.osd
{
import flash.utils.*;
import flash.events.*;
import flash.display.*;
import flash.text.*;
import flash.geom.*;
import flash.external.*;

import fl.transitions.*;
import fl.transitions.easing.*;

import whispercast.*;
import whispercast.osd.*;
import whispercast.player.*;

dynamic public class Osd extends MovieClip {
	private var show_overlays_:Boolean = true;
	public function get show_overlays():Boolean {
		return show_overlays_;
	}
	
	private var show_crawlers_:Boolean = true;
	public function get show_crawlers():Boolean {
		return show_crawlers_;
	}
	
	private var font_size_:Number = 12;
	public function get font_size():Number {
		return font_size_;
	}
	
	private var font_family_:String = 'Verdana,Arial,Helvetica';
	public function get font_family():String {
		return font_family_;
	}
	
	private var scale_:Number = 1;
	public function get scale():Number {
		return scale_;
	}
	
	private var style_sheet_:StyleSheet = null;
	public function get style_sheet():StyleSheet {
		return style_sheet_;
	}
	
	public function style_sheet_clone():StyleSheet {
		var ss:StyleSheet = new StyleSheet();
		var ss_s:Array = style_sheet_.styleNames;
		
		var i:uint;
		for (i = 0; i < ss_s.length; i++) {
			ss.setStyle(ss_s[i], style_sheet_.getStyle(ss_s[i]));
		}
		
		return ss;
	}
	
	private var clip_width_:Number = 0;
	private var clip_height_:Number = 0;
	
	private var clip_sx_:Number = 1;
	private var clip_sy_:Number = 1;
	
	private var restore_btn_:MovieClip;
	private var restore_btn_class_name_:String = 'Osd_Default_Restore';
	private var restore_btn_closed_:Boolean = false;
	private var restore_btn_tween_:Tween;
	private var restore_btn_x_:Number = 0;
	private var restore_btn_y_:Number = 0;
	private var restore_btn_size_:Point = new Point();
	
	static private var pattern_:RegExp = /((<\s*a[^>]*)(href)\s*=\s*"([^"]*)"([^>]*)target\s*=\s*"([^"]*)")|((<\s*a[^>]*)target\s*=\s*"([^"]*)"([^>]*)(href)\s*=\s*"([^"]*)")|((<\s*a[^>]*)(href)\s*=\s*"([^"]*)")/gsi;
		
	static public function convertHtml(src:String):String {
		var result:String =
		src.replace(
			Osd.pattern_, 
			function(match, s1, s2, s3, s4, s5, s6, s7, s8, s9, s10, s11, s12, s13, s14, s15, s16, s17) {
				if (s1) {
					return s2+s5+s3+'="event:'+s4+','+s6+'"';
				} else
				if (s7) {
					return s8+s10+s11+'="event:'+s12+','+s9+'"';
				}
				return s14+s15+'="event:'+s16+','+s17+',_self"'
			}
		);
		return result;
	}
	
	public function Osd(p:Object) {
		overlays_ = new Object();
		crawlers_ = new Object();
		
		if (p.font_size != undefined) {
			scale_ = p.font_size/font_size;
		}
		if (p.font_family != undefined) {
			font_family_ = p.font_family;
		}
		
		if (p.style_sheet != undefined) {
			style_sheet_ = p.style_sheet;
		} else {
			style_sheet_ = new StyleSheet();
		}
		if (p.restore_btn_x != undefined) {
			restore_btn_x_ = Math.max(0, Math.min(1, p.restore_btn_x));
		}
		if (p.restore_btn_y != undefined) {
			restore_btn_y_ = Math.max(0, Math.min(1, p.restore_btn_y));
		}
		
		var restore_btn_class_ref:Class = Class(getDefinitionByName(restore_btn_class_name_));
		restore_btn_ = new restore_btn_class_ref();
		addChild(restore_btn_);
		
		restore_btn_.buttonMode = true;
		
		restore_btn_.stop();
		restore_btn_.visible = false;
		restore_btn_.alpha = 0;
		
		restore_btn_size_.x = restore_btn_.width;
		restore_btn_size_.y = restore_btn_.height;
		
		restore_btn_.addEventListener(MouseEvent.CLICK, _onRestoreClick);
		
		addEventListener(TextEvent.LINK, function(e:TextEvent) {
			var parts = e.text.split(',');
			ExternalInterface.call('window.open', parts[0], parts[1]);
		});
	}
	
	public function layout(ax:Number, ay:Number, w:Number, h:Number, sx:Number, sy:Number) {
		// we use a reference height of 240, in order to keep the scale
		// even if the streamed video size changes...
		sy = (h>0) ? h/240 : 1;
		
		for (var overlay in overlays_) {
			overlays_[overlay].layout(w, h, sx, sy);
		}
		for (var crawler in crawlers_) {
			crawlers_[crawler].layout(w, h, sx, sy);
		}
		
		graphics.clear();
		graphics.beginFill(0, 0);
		graphics.drawRect(0, 0, w, h);
		graphics.endFill();
		
		x = ax;
		y = ay;
		
		clip_width_ = w;
		clip_height_ = h;
		
		clip_sx_ = sx;
		clip_sy_ = sy;
		
		restore_btn_.scaleX = 
		restore_btn_.scaleY = Math.max(1, Math.max(sx, sy)/2);

		var rbX:Number = restore_btn_x_*clip_width_;
		var rbY:Number = restore_btn_y_*clip_height_;
		var rboX:Number = 2*restore_btn_.scaleX*restore_btn_size_.x/3;
		var rboY:Number = restore_btn_.scaleY*restore_btn_size_.y/3;
		restore_btn_.x = rbX + rboX*(1-2*restore_btn_x_) - (1-restore_btn_x_)*restore_btn_.scaleX*restore_btn_size_.x;
		restore_btn_.y = rbY + rboY*(1-2*restore_btn_y_) - (1-restore_btn_y_)*restore_btn_.scaleY*restore_btn_size_.y;
		
		setChildIndex(restore_btn_, numChildren-1);
	}

	public function reset() {
		var id:String;
		
		for (id in overlays_) {
			overlays_[id].destroy();
			delete overlays_[id];
		}
		for (id in crawlers_) {
			crawlers_[id].destroy();
			delete crawlers_[id];
		}
	}
	
	/* Overlays */
	private var overlays_:Object;
	
	public function _osd_CreateOverlay(p:Object) {
		try {
			if (p.id) {
				if (overlays_[p.id]) {
					overlays_[p.id].update(p);
					whispercast.Logger.log('osd:main','Osd::_osd_CreateOverlay() - Overlay['+p.id+'] was updated.');
				} else {
					var overlay:Overlay = new Overlay(this);
					overlays_[p.id] = overlay;
					
					addChild(overlay);
					
					overlay.create(this, clip_width_, clip_height_, clip_sx_, clip_sy_);
					overlay.update(p);
					
					whispercast.Logger.log('osd:main','Osd::_osd_CreateOverlay() - Overlay['+p.id+'] was created.');
				}
			} else {
					whispercast.Logger.log('osd:main','Osd::_osd_CreateOverlay() - No overlay id.');
			}
		} catch(e) {
			whispercast.Logger.log('osd:main','Osd::_osd_CreateOverlay() - Exception: '+e.message);
		}
	}
	public function _osd_DestroyOverlay(p:Object) {
		try {
			if (p.id) {
				if (overlays_[p.id]) {
					overlays_[p.id].destroy();
					delete overlays_[p.id];
					whispercast.Logger.log('osd:main','Osd::_osd_DestroyOverlay() - Overlay['+p.id+'] was destroyed.');
				} else {
					whispercast.Logger.log('osd:main','Osd::_osd_DestroyOverlay() - Overlay['+p.id+'] does not exist.');
				}
			} else {
				whispercast.Logger.log('osd:main','Osd::_osd_DestroyOverlay() - No overlay id');
			}
		} catch(e) {
			whispercast.Logger.log('osd:main','DestroyOverlay() - Exception: '+e.message);
		}
	}
	public function _osd_ShowOverlays(p:Object) {
		whispercast.Logger.log('osd:main','Osd::_osd_ShowOverlays('+p.show+')');
		show_overlays_ = p.show;
		for (var id:String in overlays_) {
			overlays_[id].update(null);
		}
	}
	
	/* Crawlers */
	private var crawlers_:Object;
	
	public function _osd_CreateCrawler(p:Object) {
		try {
			if (p.id) {
				if (crawlers_[p.id]) {
					crawlers_[p.id].update(p);
					whispercast.Logger.log('osd:main','Osd::_osd_CreateCrawler() - Crawler['+p.id+'] was updated.');
				} else {
					var crawler:Crawler = new Crawler(this);
					crawlers_[p.id] = crawler;
					
					addChild(crawler);
					
					crawler.create(this, clip_width_, clip_height_, clip_sx_, clip_sy_);
					crawler.update(p);
					
					whispercast.Logger.log('osd:main','Osd::_osd_CreateCrawler() - Crawler['+p.id+'] was created.');
				}
			} else {
				whispercast.Logger.log('osd:main','Osd::_osd_CreateCrawler() - No crawler id.');
			}
		} catch(e) {
			whispercast.Logger.log('osd:main','Osd::_osd_CreateCrawler() - Exception: ' + e.message);
		}
	}
	public function _osd_DestroyCrawler(p:Object) {
		try {
			if (p.id) {
				if (crawlers_[p.id]) {
					crawlers_[p.id].destroy();
					delete crawlers_[p.id];
					whispercast.Logger.log('osd:main','Osd::_osd_DestroyCrawler() - Crawler['+p.id+'] was destroyed.');
				} else {
					whispercast.Logger.log('osd:main','Osd::_osd_DestroyCrawler() - Crawler['+p.id+'] does not exist.');
				}
			} else {
					whispercast.Logger.log('osd:main','Osd::_osd_DestroyCrawler() - No crawler id.');
			}
		} catch(e) {
			whispercast.Logger.log('osd:main','Osd::_osd_DestroyCrawler() - Exception: '+e.message);
		}
	}
	public function _osd_ShowCrawlers(p:Object) {
		whispercast.Logger.log('osd:main','Osd::_osd_ShowCrawlers('+p.show+')');
		show_crawlers_ = p.show;
		for (var id:String in overlays_) {
			overlays_[id].update({});
		}
	}
	public function _osd_AddCrawlerItem(p:Object) {
		try {
			if (p.crawler_id) {
				if (crawlers_[p.crawler_id]) {
					crawlers_[p.crawler_id].addCrawlerItem(p.item_id, p.item);
					whispercast.Logger.log('osd:main','Osd::_osd_AddCrawlerItem() - Item['+p.item_id+'] in crawler['+p.crawler_id+'] was created/updated.');
				} else {
					whispercast.Logger.log('osd:main','Osd::_osd_AddCrawlerItem() - Crawler['+p.crawler_id+'] doesn\'t exist.');
				}
			} else {
				whispercast.Logger.log('osd:main','Osd::_osd_AddCrawlerItem() - Crawler id is empty.');
			}
		} catch(e) {
			whispercast.Logger.log('osd:main','Osd::_osd_AddCrawlerItem() - Exception: '+e.message);
		}
	}
	public function _osd_RemoveCrawlerItem(p:Object) {
		try {
			if (p.crawler_id) {
				if (crawlers_[p.crawler_id]) {
					crawlers_[p.crawler_id].removeCrawlerItem(p.item_id);
					whispercast.Logger.log('osd:main','Osd::_osd_RemoveCrawlerItem() - Item['+p.item_id+'] in crawler['+p.crawler_id+'] was removed.');
				} else {
					whispercast.Logger.log('osd:main','Osd::_osd_RemoveCrawlerItem() - Crawler['+p.crawler_id+'] doesn\'t exist.');
				}
			} else {
				whispercast.Logger.log('osd:main','Osd::_osd_RemoveCrawlerItem() - Crawler id is empty.');
			}
		} catch(e) {
			whispercast.Logger.log('osd:main','Osd::_osd_RemoveCrawlerItem() - Exception: '+e.message);
		}
	}
	
	public function update() {
		var closed:Boolean = false;
		
		if (!closed) {
			for (var overlay in overlays_) {
				if (overlays_[overlay].closed) {
					closed = true;
					break;
				}
			}
		}
		if (!closed) {
			for (var crawler in crawlers_) {
				if (crawlers_[crawler].closed) {
					closed = true;
					break;
				}
			}
		}
		
		if (closed) {
			if (!restore_btn_closed_) {
				restore_btn_closed_ = true;
				
				if (restore_btn_tween_) {
					restore_btn_tween_.stop();
					restore_btn_tween_.removeEventListener(TweenEvent.MOTION_FINISH, _onRestoreHidden);
				}
				restore_btn_tween_ = new Tween(restore_btn_, 'alpha', Strong.easeOut, restore_btn_.alpha, 1, 1, true);
				restore_btn_tween_.start();
				
				restore_btn_.visible = true;
				restore_btn_.gotoAndPlay(1);
			}
		} else {
			restore_btn_closed_ = false;
			
			if (restore_btn_tween_) {
				restore_btn_tween_.stop();
				restore_btn_tween_.removeEventListener(TweenEvent.MOTION_FINISH, _onRestoreHidden);
			}
			restore_btn_tween_ = new Tween(restore_btn_, 'alpha', Strong.easeOut, this.alpha, 0, 1, true);
			
			restore_btn_tween_.addEventListener(TweenEvent.MOTION_FINISH, _onRestoreHidden);
			restore_btn_tween_.start();
		}
	}
	
	private function _onRestoreHidden(e:Event) {
		restore_btn_.visible = false;
		restore_btn_.stop();
	}
	private function _onRestoreClick(e:Event) {
		for (var overlay in overlays_) {
			if (overlays_[overlay].closed) {
				overlays_[overlay].close(false);
			}
		}
		for (var crawler in crawlers_) {
			if (crawlers_[crawler].closed) {
				crawlers_[crawler].close(false);
			}
		}
		update();
	}
}
}