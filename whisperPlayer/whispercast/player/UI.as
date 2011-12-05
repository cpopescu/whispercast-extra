// Copyright (c) 2009, Whispersoft s.r.l.
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
// * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
// * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
// * Neither the name of Whispersoft s.r.l. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

package whispercast.player
{
import flash.display.*;
import flash.events.*;
import flash.utils.*;
import flash.geom.*;
import flash.text.*;

import fl.transitions.*;
import fl.transitions.easing.*;

import whispercast.*;
import whispercast.player.*;

public class UI
{
	/*
	The button states.
	*/
	public static var BTN_UP:String = 'up';
	public static var BTN_OVER:String = 'over';
	public static var BTN_DOWN:String = 'down';
	public static var BTN_DISABLED:String = 'disabled';
	
	/*
	The tooltip element.
	*/
	private static var tooltip_:MovieClip = null;
	private static var tooltip_visible_:Boolean = false;
	/*
	The element that's asociated with the current tooltip.
	*/
	private static var tooltip_ctrl_:MovieClip = null;
	
	/*
	The tooltip timer id.
	*/
	private static var tooltip_timer_id_:uint = 0;
	/*
	The tooltip timer duration.
	*/
	private static var tooltip_timer_ms_:Number = 500;
	
	/*
	The element that's currently capturing the mouse.
	*/
	private static var capture_ctrl_:MovieClip = null;
	/*
	Capture mouse event handler.
	*/
	private static function _onCaptureMouseEvent(e:MouseEvent):void {
		e.stopPropagation();
	}
	/*
	Capture mouse.
	*/
	private static function _captureMouse(ctrl:MovieClip, callback:Function):void {
		capture_ctrl_ = ctrl;
		
		var top_level:DisplayObject = ctrl.stage;
		try {
			top_level.addEventListener(MouseEvent.MOUSE_DOWN, _onCaptureMouseEvent, true);
		} catch (se:SecurityError) {
			top_level = ctrl.root;
			top_level.addEventListener(MouseEvent.MOUSE_DOWN, _onCaptureMouseEvent, true);
		}
		top_level.addEventListener(MouseEvent.MOUSE_OUT, _onCaptureMouseEvent, true);
		top_level.addEventListener(MouseEvent.MOUSE_OVER, _onCaptureMouseEvent, true);
		top_level.addEventListener(MouseEvent.MOUSE_UP, function(e:Event) {
			top_level.removeEventListener(MouseEvent.MOUSE_DOWN, _onCaptureMouseEvent, true);
			top_level.removeEventListener(MouseEvent.MOUSE_OUT, _onCaptureMouseEvent, true);
			top_level.removeEventListener(MouseEvent.MOUSE_OVER, _onCaptureMouseEvent, true);
			top_level.removeEventListener(MouseEvent.MOUSE_UP, arguments.callee);
			top_level.removeEventListener(MouseEvent.ROLL_OUT, _onCaptureMouseEvent, true);
			top_level.removeEventListener(MouseEvent.ROLL_OVER, _onCaptureMouseEvent, true);
		
			callback(capture_ctrl_, e);
			capture_ctrl_ = null;
		});
		top_level.addEventListener(MouseEvent.ROLL_OUT, _onCaptureMouseEvent, true);
		top_level.addEventListener(MouseEvent.ROLL_OVER, _onCaptureMouseEvent, true);
	}
	public static function capturedControl():MovieClip {
		return capture_ctrl_;
	}
	
	/*
	Loads the given skin into the given button.
	*/
	private static function _loadButtonSkin(ctrl:MovieClip, state:String, linkage:String, def:String) {
		var ctrl = (ctrl.current_mc_ != undefined) ? ctrl.current_mc_ : ctrl;
		if (ctrl[state] == undefined) {
			if (ctrl[linkage] == undefined) {
				ctrl[state] = ctrl.getChildByName(def);
			} else {
				var mc_class:Class = Class(getDefinitionByName(ctrl[linkage]));
				ctrl[state] = new mc_class();
				ctrl[state].name = state;
				ctrl.addChildAt(ctrl[state], ctrl.numChildren);
			}
		}
	}
	/*
	Sets the state of the given button.
	*/
	private static function _setButtonState(ctrl:MovieClip, state:String, force:Boolean = false):void {
		if (force || (state != ctrl.state_)) {
			_loadButtonSkin(ctrl, state+'_mc', state+'LinkageID', 'up_mc');
			ctrl.state_ = state;
			
			var ctrl = (ctrl.current_mc_ != undefined) ? ctrl.current_mc_ : ctrl;
			if (ctrl.state_mc) {
				ctrl.state_mc.visible = false;
			}
			ctrl.state_mc = ctrl[state+'_mc'];
			ctrl.state_mc.visible = true;
		}
	}
	
	/*
	Starts the tooltip for the given control.
	*/
	private static function _startTooltip(ctrl:MovieClip, timer:Number = NaN):void {
		if (!tooltip_)
			return;
			
		if (ctrl.tooltip_) {
			if (tooltip_ctrl_ != ctrl) {
				_stopTooltip();
				
				tooltip_ctrl_ = ctrl;
				if (tooltip_timer_id_) {
					flash.utils.clearTimeout(tooltip_timer_id_);
				}
				tooltip_timer_id_ = flash.utils.setTimeout(function() {tooltip_timer_id_ = 0; _showTooltip()}, isNaN(timer) ? tooltip_timer_ms_ : timer);
			}
		}
	}
	/*
	Stops any running tooltip.
	*/
	private static function _stopTooltip():void {
		if (!tooltip_)
			return;

		if (tooltip_visible_) {
			tooltip_visible_ = false;
			TransitionManager.start(tooltip_, {type:Fade, direction:Transition.OUT, duration:0.5, easing:Strong.easeOut, startPoint:8})
		}
		if (tooltip_timer_id_) {
			flash.utils.clearTimeout(tooltip_timer_id_);
			tooltip_timer_id_ = 0;
		}
		tooltip_ctrl_ = null;
	}
	
	/*
	Forces a tooltip update.
	*/
	private static function _updateTooltip():void {
		if (tooltip_ctrl_) {
			var p:Object = {};
			if (typeof(tooltip_ctrl_.tooltip_) == 'function') {
				tooltip_ctrl_.tooltip_.call(tooltip_ctrl_, p);
			} else {
				var pos:Point = new Point(tooltip_ctrl_.x, tooltip_ctrl_.y);
				pos = tooltip_ctrl_.parent.localToGlobal(pos);
				
				p.x = pos.x;
				p.y = pos.y;
				
				p.text = tooltip_ctrl_.tooltip_;
			}
			
			var text_field:TextField = TextField(tooltip_.getChildByName('text_mc'));
			tooltip_.removeChild(text_field);
			
			text_field.width = 1;
			text_field.height = 1;
			text_field.text = p.text;
			
			tooltip_.background_mc.width = text_field.width+9;
			tooltip_.background_mc.height = text_field.height+1;
			
			text_field.x = 4;
			text_field.y = -1;
			
			tooltip_.addChild(text_field);
			
			if (p.x < 0) {
				p.x = 2;
			} else
			if ((p.x+tooltip_.width) > tooltip_.stage.stageWidth) {
				p.x = tooltip_.stage.stageWidth - tooltip_.width - 2;
			}
			
			p.y -= tooltip_.height+2;
			if (p.y < 0) {
				p.y += tooltip_ctrl_.height + 2*(tooltip_.height);
			}
		
			tooltip_.x = p.x;
			tooltip_.y = p.y;
		}
	}
	/*
	Shows the tooltip.
	*/
	private static function _showTooltip():void {
		_updateTooltip();
		
		tooltip_.parent.setChildIndex(tooltip_, tooltip_.parent.numChildren-1);
		TransitionManager.start(tooltip_, {type:Fade, direction:Transition.IN, duration:0.5, easing:Strong.easeOut, startPoint:8});
		tooltip_visible_ = true;
	}
	
	/*
	Initialize tooltip support.
	*/
	public static function initializeTooltipSupport(tooltip:MovieClip, timer_ms:Number = 500):void {
		tooltip_ = tooltip;
		if (tooltip_) {
			var text_field:TextField = TextField(tooltip_.getChildByName('text_mc'));
			text_field.autoSize = TextFieldAutoSize.LEFT;
			text_field.blendMode = BlendMode.LAYER;
		}
		tooltip_timer_ms_ = timer_ms;
	}
		
	/*
	Normal mouse event handler for buttons.
	*/
	private static function _onButtonMouseEvent(e:MouseEvent) {
		var ctrl:MovieClip = MovieClip(e.currentTarget);
		switch (e.type) {
		case MouseEvent.ROLL_OVER:
			_startTooltip(ctrl);
			if (!ctrl.is_disabled_) {
				ctrl.is_rolled_over_ = true;
				_setButtonState(ctrl, BTN_OVER);
				if (ctrl.on_changed_ != undefined) {
					ctrl.on_changed_(ctrl, 'roll_over');
				}
			}
			break;
		case MouseEvent.ROLL_OUT:
			_stopTooltip();
			if (ctrl.state_ != BTN_DISABLED) {
				ctrl.is_rolled_over_ = false;
				_setButtonState(ctrl, BTN_UP);
				if (ctrl.on_changed_ != undefined) {
					ctrl.on_changed_(ctrl, 'roll_out');
				}
			}
			break;
		case MouseEvent.MOUSE_DOWN:
			_stopTooltip();
			if (ctrl.state_ != BTN_DISABLED) {
				ctrl.is_down_ = true;
				_setButtonState(ctrl, BTN_DOWN);
				_captureMouse(ctrl, function(ctrl:MovieClip, e:MouseEvent) {
					if (ctrl.state_ != BTN_DISABLED) {
						if (ctrl.hitTestPoint(e.stageX, e.stageY, true)) {
							ctrl.is_down_ = false;
							ctrl.is_rolled_over_ = true;
							_setButtonState(ctrl, BTN_OVER);
							if (ctrl.on_changed_ != undefined) {
								ctrl.on_changed_(ctrl, 'click');
							}
						} else {
							ctrl.is_down_ = false;
							ctrl.is_rolled_over_ = false;
							_setButtonState(ctrl, BTN_UP);
						}
					}
				});
			}
			break;
		}
	}

	/*
	Adds a new button.
	*/
	public static function addButton(ctrl:MovieClip, on_changed:Function, appearance:String, enabled:Boolean, tooltip:String = null) {
		ctrl.on_changed_ = on_changed;
		
		ctrl.addEventListener(MouseEvent.ROLL_OVER, _onButtonMouseEvent);
		ctrl.addEventListener(MouseEvent.ROLL_OUT, _onButtonMouseEvent);
		ctrl.addEventListener(MouseEvent.MOUSE_DOWN, _onButtonMouseEvent);
																	 
		if (appearance != null) {
			appearance = appearance +'_mc';
			for (var i = 0; i < ctrl.numChildren; i++) {
				var child:MovieClip = MovieClip(ctrl.getChildAt(i));
				child.placeholder_mc.visible = false;
				
				if (child.name == appearance) {
					child.visible = true;
					ctrl.current_mc_ = child;
				} else {
					child.visible = false;
				}
			}
		}
		
		ctrl.tooltip_ = tooltip;
		ctrl.is_rolled_over_ = false;
		
		enableButton(ctrl, enabled);
	}
	
	/*
	Enables/disables the given button.
	*/
	public static function enableButton(ctrl:MovieClip, enabled:Boolean) {
		ctrl.is_disabled_ = !enabled;
		ctrl.buttonMode = enabled;

		_setButtonState(ctrl, enabled ? (ctrl.is_down_ ? BTN_DOWN : (ctrl.is_rolled_over_ ? BTN_OVER : BTN_UP)) : BTN_DISABLED);
	}
	
	/*
	Sets the appearance of the given button to the given appearance,
	for multi-state buttons (play/pause, etc).
	*/
	public static function setButtonAppearance(ctrl:MovieClip, appearance:String):void {
		var appearance_mc:DisplayObject =
		ctrl.getChildByName(appearance+'_mc');
		
		ctrl.current_mc_.visible = false;
		ctrl.current_mc_ = appearance_mc;
		ctrl.current_mc_.visible = true;
		
		_setButtonState(ctrl, ctrl.state_, true);
	}
	
	/*
	Bar handle pressed callback.
	*/
	private static function _onStartDragging(ctrl, e:MouseEvent) {
		ctrl.is_dragging_ = true;
		if (ctrl.barVertical) {
			ctrl.handle_mc.startDrag(false,
				new flash.geom.Rectangle(
					ctrl.handle_mc.x,
					ctrl.background_mc.y - ctrl.background_mc.height+1,
					0,
					ctrl.background_mc.height
				)
			);
		}
		else {
			ctrl.handle_mc.startDrag(false,
				new flash.geom.Rectangle(
					ctrl.background_mc.x,
					ctrl.handle_mc.y,
					ctrl.background_mc.width,
					0
				)
			);
		}
		
		_onBarChanged(ctrl, e);
		
		ctrl.mouse_move_cb_ = function(e:MouseEvent) {
			_onBarChanged(ctrl, e);
		}
		ctrl.stage.addEventListener(MouseEvent.MOUSE_MOVE, ctrl.mouse_move_cb_);
		
		_captureMouse(ctrl, function(ctrl:MovieClip, e:MouseEvent) {
			_onBarRelease(ctrl, e, ctrl.handle_mc.hitTestPoint(e.stageX, e.stageY, true));
		});
	}
	private static function _onBarMouseEvent(e:MouseEvent) {
		var ctrl:MovieClip = MovieClip(e.currentTarget);
		switch (e.type) {
			case MouseEvent.ROLL_OVER:
				_startTooltip(ctrl, 50);
				break;
			case MouseEvent.ROLL_OUT:
				_stopTooltip();
				break;
			case MouseEvent.MOUSE_MOVE:
				_updateTooltip();
				break;
		}
	}
	private static function _onBarDown(e:MouseEvent):void {
		var ctrl:MovieClip = MovieClip(e.currentTarget);
		switch (e.target.name) {
			case 'handle_mc':
				_onStartDragging(ctrl, e);
				break;
			case 'mask_mc':
			case 'value_mc':
			case 'background_mc':
				var p:Point = ctrl.background_mc.globalToLocal(new Point(e.stageX, e.stageY));
				
				var percent:Number;
				if (ctrl.barVertical) {
					percent = -p.y/ctrl.background_mc.height*ctrl.background_mc.scaleY;
				} else {
					percent = e.localX/ctrl.background_mc.width*ctrl.background_mc.scaleX;
				}
				percent = Math.max(0, Math.min(1, percent));
	
				_setBarPosition(ctrl, percent, true);
				_onStartDragging(ctrl, e);
				break;
		}
	}
	
	/*
	Called whenever the bar's position has changed.
	*/
	private static function _onBarChanged(ctrl:MovieClip, e:MouseEvent):void {
		var percent:Number;
		if (ctrl.barVertical) {
			percent = (ctrl.background_mc.y - ctrl.handle_mc.y)/ctrl.background_mc.height;
			percent = Math.max(0, Math.min(1, percent));
		} else {
			percent = (ctrl.handle_mc.x - ctrl.background_mc.x)/ctrl.background_mc.width;
			percent = Math.max(0, Math.min(1, percent));
		}

		if ((ctrl.is_dragging_ && ctrl.barSyncOnDrag) || (!ctrl.is_dragging_ && ctrl.barSyncOnRelease)) {
			_setBarPosition(ctrl, percent);
		}

		if (ctrl.on_changed_ != undefined) {
			ctrl.on_changed_(ctrl, Math.max(0, Math.min(1, percent)), ctrl.is_dragging_);
		}
	}
	/*
	Bar handle released callback.
	*/
	private static function _onBarRelease(ctrl:MovieClip, e:MouseEvent, over:Boolean):void {
		ctrl.handle_mc.stopDrag();
		
		ctrl.is_dragging_ = false;
		_onBarChanged(ctrl, e);
		
		ctrl.stage.removeEventListener(MouseEvent.MOUSE_MOVE, ctrl.mouse_move_cb_);
		delete ctrl.mouse_move_cb_;
	}
	
	/*
	Adds a new bar control.
	*/
	public static function addBar(ctrl:MovieClip, on_changed:Function, enabled:Boolean, percent:Number, offset:Number = 0):void {
		ctrl.is_dragging_ = false;
		ctrl.on_changed_ = on_changed;
		
		enableBar(ctrl, enabled);
		_setBarOffset(ctrl, percent);
		_setBarPosition(ctrl, percent);
	}
	
	/*
	Enables/disables the given bar.
	*/
	public static function enableBar(ctrl:MovieClip, enabled:Boolean) {
		if (enabled) {
			ctrl.buttonMode = true;
			if (ctrl.handle_mc) {
				ctrl.handle_mc.visible = true;
			
				ctrl.addEventListener(MouseEvent.MOUSE_DOWN, _onBarDown);
				if (ctrl.tooltip_) {
					ctrl.addEventListener(MouseEvent.MOUSE_MOVE, _onBarMouseEvent);
					ctrl.addEventListener(MouseEvent.ROLL_OVER, _onBarMouseEvent);
					ctrl.addEventListener(MouseEvent.ROLL_OUT, _onBarMouseEvent);
				}
			}
		}
		else {
			ctrl.buttonMode = false;
			if (ctrl.handle_mc) {
				ctrl.handle_mc.visible = false;
			
				if (ctrl.tooltip_) {
					ctrl.removeEventListener(MouseEvent.ROLL_OUT, _onBarMouseEvent);
					ctrl.removeEventListener(MouseEvent.ROLL_OVER, _onBarMouseEvent);
					ctrl.removeEventListener(MouseEvent.MOUSE_MOVE, _onBarMouseEvent);
				}
				ctrl.removeEventListener(MouseEvent.MOUSE_DOWN, _onBarDown);
			}
		}
	}
	/*
	Sets the current position on a bar control.
	*/
	private static function _setBarOffset(ctrl:MovieClip, percent:Number):void {
		percent = Math.max(0, Math.min(1, percent));
		
		if (ctrl.barVertical) {
			ctrl.mask_mc.y = -percent*ctrl.background_mc.height;
			ctrl.mask_mc.height = ctrl.background_mc.height - percent*ctrl.background_mc.height;
		}
		else {
			ctrl.mask_mc.x = percent*ctrl.background_mc.width;
			ctrl.mask_mc.width = ctrl.background_mc.width - percent*ctrl.background_mc.width;
		}
	}
	private static function _setBarPosition(ctrl:MovieClip, percent:Number, handleOnly:Boolean = false):void {
		percent = Math.max(0, Math.min(1, percent));
		
		if (handleOnly) {
			if (ctrl.barVertical) {
				if (ctrl.handle_mc) {
					ctrl.handle_mc.y = - percent*ctrl.background_mc.height;
				}
			}
			else {
				if (ctrl.handle_mc) {
					ctrl.handle_mc.x = percent*ctrl.background_mc.width;
				}
			}
		} else
			if (ctrl.barVertical) {
				ctrl.mask_mc.height = percent*(ctrl.background_mc.height + ctrl.mask_mc.y);
				if (ctrl.handle_mc) {
					ctrl.handle_mc.y = ctrl.mask_mc.y - percent*(ctrl.background_mc.height+ctrl.mask_mc.y);
				}
			}
			else {
				ctrl.mask_mc.width = percent*(ctrl.background_mc.width - ctrl.mask_mc.x);
				if (ctrl.handle_mc) {
					ctrl.handle_mc.x = ctrl.mask_mc.x + percent*(ctrl.background_mc.width-ctrl.mask_mc.x);
				}
			}
	}
	public static function setBarPosition(ctrl:MovieClip, percent:Number, offset:Number = 0):void {
		_setBarOffset(ctrl, offset);
		_setBarPosition(ctrl, percent);
	}
	public static function setBarState(ctrl:MovieClip, state:Object):void {
		setBarPosition(ctrl, state.position, state.offset);
	}
	/*
	Returns the current bar position.
	*/
	public static function getBarOffset(ctrl:MovieClip):Number {
		var percent:Number;
		if (ctrl.vertical) {
			percent = -ctrl.mask_mc.y/ctrl.background_mc.height;
			return Math.max(0, Math.min(1, percent));
		} else {
			percent = ctrl.mask_mc.x/ctrl.background_mc.width;
			return Math.max(0, Math.min(1, percent));
		}
	}
	public static function getBarPosition(ctrl:MovieClip):Number {
		var percent:Number;
		if (ctrl.vertical) {
			percent = ctrl.mask_mc.height/(ctrl.background_mc.height - ctrl.mask_mc.y);
			return Math.max(0, Math.min(1, percent));
		} else {
			percent = ctrl.mask_mc.width/(ctrl.background_mc.width - ctrl.mask_mc.x); 
			return Math.max(0, Math.min(1, percent));
		}
	}
	public static function getBarState(ctrl:MovieClip):Object {
		return {offset:getBarOffset(ctrl), position:getBarPosition(ctrl)};
	}
}
}