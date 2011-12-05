package
{
	import flash.geom.Rectangle;
	
	import mx.core.UIComponent;
	
	import org.whispercast.*;
	import org.whispercast.player.*;
	
	import spark.layouts.supportClasses.LayoutBase;
	
	public class Layout extends LayoutBase
	{
		public function Layout()
		{
			super();
		}
		
		public override function updateDisplayList(unscaledWidth:Number, unscaledHeight:Number):void
		{
			var r:Player = target.document as Player;
			if (!r)
				return;
			
			var mp:VideoPlayer = r.isSwitched ? r.player1 : r.player0;
			var sp:VideoPlayer = r.isSwitched ? r.player0 : r.player1;
			
			mp.setLayoutBoundsPosition(0, 0, true);
			mp.setLayoutBoundsSize(unscaledWidth, unscaledHeight, true);
			
			var ow:Number = mp.output.width+2;
			var oh:Number = mp.output.height+2;
			
			var w:Number = r.secondaryWidth*ow;
			var h:Number = r.secondaryHeight*oh;
			
			sp.setLayoutBoundsPosition((ow-w)*r.secondaryX, (oh-h)*r.secondaryY);
			sp.setLayoutBoundsSize(w, h, true);
		}
	}
}