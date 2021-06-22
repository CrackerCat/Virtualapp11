package com.lody.virtual.sandxposed;

import android.content.Context;
import android.os.Process;
import android.text.TextUtils;


import com.scorpion.IHook.XC_MethodHook;
import com.scorpion.IHook.XposedBridge;
import com.scorpion.IHook.XposedHelpers;
import com.swift.sandhook.xposedcompat.utils.FileUtils;
import java.io.File;
import java.util.Arrays;


public class EnvironmentSetup {

    public static void init(Context context, String packageName, String processName) {
        initSystemProp(context);
        initForWeChat(context, processName);
    }

    private static void initSystemProp(Context context) {
        //inject vxp name
        System.setProperty("vxp", "1");
        System.setProperty("vxp_user_dir", new File(context.getApplicationInfo().dataDir).getParent());
        //sandvxp
        System.setProperty("sandvxp", "1");
    }

    private static void initForWeChat(Context context, String processName) {
        if (!TextUtils.equals("com.tencent.mm", processName))
            return;
        //delete tinker patches
        File dataDir = new File(context.getApplicationInfo().dataDir);
        File tinker = new File(dataDir, "tinker");
        File tinker_temp = new File(dataDir, "tinker_temp");
        File tinker_server = new File(dataDir, "tinker_server");

        try {
            FileUtils.delete(tinker);
            FileUtils.delete(tinker_temp);
            FileUtils.delete(tinker_server);
        } catch (Exception e) {
        }
        //avoid mm kill self
        final int mainProcessId = Process.myPid();
        XposedHelpers.findAndHookMethod(Process.class, "killProcess", int.class, new XC_MethodHook() {
            @Override
            protected void beforeHookedMethod(MethodHookParam param) throws Throwable {
                super.beforeHookedMethod(param);
                int pid = (int) param.args[0];
                if (pid != mainProcessId) {
                    return;
                }
                // try kill main process, find stack
                StackTraceElement[] stackTrace = Thread.currentThread().getStackTrace();
                if (stackTrace == null) {
                    return;
                }

                for (StackTraceElement stackTraceElement : stackTrace) {
                    if (stackTraceElement.getClassName().contains("com.tencent.mm.app")) {
                        XposedBridge.log("do not suicide..." + Arrays.toString(stackTrace));
                        param.setResult(null);
                        break;
                    }
                }
            }
        });
    }

}
