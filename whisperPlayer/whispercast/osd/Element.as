package whispercast.osd
{
import flash.events.*;
import flash.display.*;

import fl.transitions.*;
import fl.transitions.easing.*;

public class Element extends Sprite {
	protected var owner_:Osd;
	public function get owner():Osd {
		return owner_;
	}
	
	protected var tween_:Tween;
	
	protected var visible_:Boolean = false;
	
	protected var width_:Number = 0;
	protected var height_:Number = 0;
	
	protected var sx_:Number = 1;
	protected var sy_:Number = 1;
	
	protected var closed_:Boolean = false;
	protected var destroyed_:Boolean = false;
	
	public function Element(owner:Osd) {
		owner_ = owner;
	}
	
	public function get shown():Boolean {
		return !destroyed_;
	}
	public function get closed():Boolean {
		return shown && closed_;
	}
	
	public function create(container:DisplayObjectContainer, w:Number, h:Number, sx:Number, sy:Number):void {
		width_ = w;
		height_ = h;
		
		sx_ = sx;
		sy_ = sy;
		
		visible = false;
		this.alpha = 0;
	}
	public function destroy():void {
		destroyed_ = true;
		
		if (visible_) {
			show(false);
		}
		else {
			finalize();
		}
			
		owner_.update();
	}
	
	protected function finalize():void {
		if (parent)
			parent.removeChild(this);
	}
	
	public function update(p:Object):void {
		show(shown && !closed_);
		layout(width_, height_, sx_, sy_);
	}
	
	public function show(show:Boolean):void {
		if (show && !visible_) {
			visible_ = true;
			
			if (tween_) {
				tween_.stop();
				tween_.removeEventListener(TweenEvent.MOTION_FINISH, _onHidden);
			}
			tween_ = new Tween(this, 'alpha', Strong.easeOut, this.alpha, 1, 1, true);
			tween_.start();
			
			visible = true;
		}
		else
		if (!show && visible_) {
			visible_ = false;
			
			if (tween_) {
				tween_.stop();
				tween_.removeEventListener(TweenEvent.MOTION_FINISH, _onHidden);
			}
			tween_ = new Tween(this, 'alpha', Strong.easeOut, this.alpha, 0, 1, true);
			
			tween_.addEventListener(TweenEvent.MOTION_FINISH, _onHidden);
			tween_.start();
		}
	}
	public function close(close:Boolean, notify:Boolean = false):void {
		closed_ = close;
		update(null);
		
		if (notify) {
			owner_.update();
		}
	}
	
	public function layout(w:Number, h:Number, sx:Number, sy:Number):void {
		width_ = w;
		height_ = h;
		
		sx_ = sx;
		sy_ = sy;
	}
	
	private function _onHidden(e:Event) {
		if (destroyed_) {
			finalize();
		}
		tween_.removeEventListener(TweenEvent.MOTION_FINISH, _onHidden);
		
		visible = false;
	}
}
}