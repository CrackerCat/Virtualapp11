package com.lody.virtual.sandxposed;

import android.content.Context;
import android.os.Build;
import android.text.TextUtils;
import android.util.Log;


import com.lody.virtual.client.core.VirtualCore;
import com.lody.virtual.helper.compat.BuildCompat;
import com.lody.virtual.remote.InstalledAppInfo;
import com.scorpion.IHook.XposedBridge;
import com.swift.sandhook.HookLog;
import com.swift.sandhook.PendingHookHandler;
import com.swift.sandhook.SandHook;
import com.swift.sandhook.SandHookConfig;
import com.swift.sandhook.utils.ReflectionUtils;
import com.swift.sandhook.xposedcompat.XposedCompat;

import java.io.File;
import java.util.List;

import mirror.dalvik.system.VMRuntime;

import static com.swift.sandhook.xposedcompat.utils.DexMakerUtils.MD5;

public class SandXposed {

    public static void initForXposed(Context context, String processName) {
        XposedCompat.cacheDir = new File(context.getCacheDir(), MD5(processName));
    }

    public static void init() {
        if (Build.VERSION.SDK_INT >= 28) {
            ReflectionUtils.passApiCheck();
        }
        SandHookConfig.DEBUG = VMRuntime.isJavaDebuggable == null ? false : VMRuntime.isJavaDebuggable.call(VMRuntime.getRuntime.call());
        HookLog.DEBUG = SandHookConfig.DEBUG;
        SandHookConfig.SDK_INT = BuildCompat.isQ() ? 29 : Build.VERSION.SDK_INT;
        SandHookConfig.compiler = SandHookConfig.SDK_INT < Build.VERSION_CODES.O;
        if (PendingHookHandler.canWork()) {
            Log.e("SandHook", "Pending Hook Mode!");
        }
        SandHook.disableVMInline();
        //XposedCompat.cacheDir = new File(VirtualCore.get().getContext().getCacheDir(), "sandhook_cache_general");
    }

    public static void injectXposedModule(Context context, String packageName, String processName) {
        //黑名单，不加载的XposedModule
        if (BlackList.canNotInject(packageName, processName))
            return;
        //获取安装APP中，过滤的xposed插件
        List<InstalledAppInfo> appInfos = VirtualCore.get().getInstalledApps(InstalledAppInfo.MODE_APP_USE_OUTSIDE_APK);
        ClassLoader classLoader = context.getClassLoader();

        for (InstalledAppInfo module : appInfos) {
            if (TextUtils.equals(packageName, module.packageName)) {
                Log.d("injectXposedModule", "injectSelf : " + processName);
            }
            XposedCompat.loadModule(module.apkPath, module.getOdexFile().getParent(), module.libPath, XposedBridge.class.getClassLoader());
        }

        XposedCompat.context = context;
        XposedCompat.packageName = packageName;
        XposedCompat.processName = processName;
        XposedCompat.cacheDir = new File(context.getCacheDir(), MD5(processName));
        XposedCompat.classLoader = XposedCompat.getSandHookXposedClassLoader(classLoader, XposedBridge.class.getClassLoader());
        XposedCompat.isFirstApplication = true;

        SandHookHelper.initHookPolicy();
        EnvironmentSetup.init(context, packageName, processName);

        try {
            XposedCompat.callXposedModuleInit();
        } catch (Throwable throwable) {
            throwable.printStackTrace();
        }
    }

}
