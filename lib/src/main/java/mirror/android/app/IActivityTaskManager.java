package mirror.android.app;

import android.os.IBinder;
import android.os.IInterface;

import mirror.MethodParams;
import mirror.RefClass;
import mirror.RefStaticMethod;

//
// Created by Swift Gan on 2019/3/18.
//
public class IActivityTaskManager {
    public static Class<?> TYPE = RefClass.load(IActivityTaskManager.class, "android.app.IActivityTaskManager");

    public static class Stub {
        public static Class<?> TYPE = RefClass.load(IActivityTaskManager.Stub.class, "android.app.IActivityTaskManager$Stub");
        @MethodParams({IBinder.class})
        public static RefStaticMethod<IInterface> asInterface;
    }
}
