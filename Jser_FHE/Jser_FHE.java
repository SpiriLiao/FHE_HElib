public class Jser_FHE{
	static {System.loadLibrary("ser_FHE_J") ;}
	public native int service() ;
	public static void main(String[] args){
		Jser_FHE jsf = new Jser_FHE() ;
		jsf.service() ;
	}
}
