package mirror.com.android.internal.view.inputmethod;

import android.os.IInterface;
import android.view.View;


import mirror.RefBoolean;
import mirror.RefClass;
import mirror.RefMethod;
import mirror.RefObject;
import mirror.RefStaticMethod;
import mirror.RefStaticObject;

public class InputMethodManager {
    public static Class<?> TYPE = RefClass.load(InputMethodManager.class, android.view.inputmethod.InputMethodManager.class);
    public static RefObject<IInterface> mService;

    public static RefStaticMethod<android.view.inputmethod.InputMethodManager> getInstance;
    public static RefStaticObject<android.view.inputmethod.InputMethodManager> sInstance;
    public static RefObject<View> mCurRootView;
    public static RefObject<View> mServedView;

    public static RefMethod finishInputLocked;
    public static RefBoolean mServedConnecting;

    public static RefMethod closeCurrentInput;
    public static RefMethod<Boolean> checkFocusNoStartInput;
    public static RefMethod<Boolean> isAcceptingText;
}
