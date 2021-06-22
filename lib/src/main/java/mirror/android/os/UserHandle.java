package mirror.android.os;

import mirror.MethodParams;
import mirror.RefClass;
import mirror.RefStaticMethod;

public class UserHandle {
    public static Class<?> TYPE = RefClass.load(UserHandle.class, "android.os.UserHandle");
    @MethodParams({int.class})
    public static RefStaticMethod<Integer> getUserId;
}
