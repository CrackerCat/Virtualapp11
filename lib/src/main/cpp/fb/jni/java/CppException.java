package fb.jni.java;
import com.facebook.proguard.annotations.DoNotStrip;
@DoNotStrip
public class CppException extends RuntimeException {
  @DoNotStrip
  public CppException(String message) {
    super(message);
  }
}
