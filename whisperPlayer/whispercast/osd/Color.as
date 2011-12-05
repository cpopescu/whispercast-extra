package whispercast.osd
{
public class Color extends Object {
	public var color_:uint;
	public var alpha_:Number;
	
	public function Color(p:Object) {
		color_ = parseInt(p.color, 16);
		alpha_ = p.alpha;
	}
	public function toHtml() {
		var c:String = '000000'+color_.toString(16);
		return '#'+c.substring(c.length - 6, c.length);
	}
}
}