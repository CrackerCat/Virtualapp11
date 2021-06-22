package fb.jni.java;
import com.facebook.proguard.annotations.DoNotStrip;
@DoNotStrip
public class CppSystemErrorException extends CppException {
  int errorCode;
  @DoNotStrip
  public CppSystemErrorException(String message, int errorCode) {
    super(message);
    this.errorCode = errorCode;
  }
  public int getErrorCode() {
    return errorCode;
  }
}
