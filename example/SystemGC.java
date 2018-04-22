public class SystemGC{

  private static boolean isTrace = false;

  public static void main(String[] args){
    isTrace = true;  // jnativetrace ON
    {
      System.gc();
    }
    isTrace = false; // jnativetrace OFF
  }
}
