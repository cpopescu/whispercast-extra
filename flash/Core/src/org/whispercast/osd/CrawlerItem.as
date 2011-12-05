package org.whispercast.osd
{
	import flash.display.*;
	import flash.geom.*;
	import flash.text.*;
	
	internal class CrawlerItem
	{
		// font
		public function get fontFamily():String
		{
			return crawler_.parameters.fontFamily;
		}
		public function get fontSize():Number
		{
			return crawler_.parameters.fontSize;
		}
		
		// content
		private var content_:String = '';
		public function get content():String
		{
			return content_;
		}
		
		// foreground color
		private var foreColor_:OSDColor;
		public function get foreColor():OSDColor
		{
			return foreColor_ ? foreColor_ : crawler_.parameters.foreColor;
		}
		// background color
		private var backColor_:OSDColor;
		public function get backColor():OSDColor
		{
			return backColor_ ? backColor_ : crawler_.parameters.backColor;
		}
		// link color
		private var linkColor_:OSDColor;
		public function get linkColor():OSDColor
		{
			return linkColor_ ? linkColor_ : crawler_.parameters.linkColor;
		}
		// hover color
		private var hoverColor_:OSDColor;
		public function get hoverColor():OSDColor
		{
			return hoverColor_ ? hoverColor_ : crawler_.parameters.hoverColor;
		}
		
		private var crawler_:Crawler;
		public function get crawler():Crawler
		{
			return crawler_;
		}
		
		public function CrawlerItem()
		{
		}
		
		public function create(crawler:Crawler, p:Object):void
		{
			crawler_ = crawler;
			
			if (p.fore_color != undefined)
				foreColor_ = new OSDColor(p.fore_color);
			if (p.back_color != undefined)
				backColor_ = new OSDColor(p.back_color);
			if (p.link_color != undefined)
				linkColor_ = new OSDColor(p.link_color);
			if (p.hover_color != undefined)
				hoverColor_ = new OSDColor(p.hover_color);
			
			if (p.content != undefined)
				content_ = p.content;
		}
		public function destroy():void
		{
		}
	}
}