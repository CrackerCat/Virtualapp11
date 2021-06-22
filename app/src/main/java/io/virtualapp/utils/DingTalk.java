package io.virtualapp.utils;

import android.util.Log;

import com.lody.virtual.helper.utils.VLog;
import com.scorpion.IHook.XposedHelpers;

public class DingTalk {
    private static final String TAG = "DingTalk";

    public DingTalk() {
    }

    public static void hook(ClassLoader classLoader) {
        try {
            VLog.d("VA-", "DingTalk hook", new Object[0]);
            Class<?> aClass = XposedHelpers.findClass("com.alibaba.lightapp.runtime.ActionRequest", classLoader);
        /*    XposedHelpers.findAndHookMethod("com.alibaba.lightapp.runtime.plugin.internal.Util", classLoader, "getWua", new Object[]{aClass, new 1()});
            XposedHelpers.findAndHookMethod("com.alibaba.lightapp.runtime.plugin.internal.Util", classLoader, "getLBSWua", new Object[]{aClass, new 2()});*/
        } catch (Throwable th) {
            Log.e("DingTalk", "sunchao-va:hook json error ");
        }
    }

}
