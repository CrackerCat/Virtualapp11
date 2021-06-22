package com.lody.virtual.client.hook.proxies.view;

import android.annotation.SuppressLint;
import android.content.ComponentName;
import android.util.Log;

import com.lody.virtual.client.hook.base.BinderInvocationProxy;
import com.lody.virtual.client.hook.base.ReplaceLastPkgMethodProxy;
import com.lody.virtual.client.hook.base.ReplaceLastUserIdMethodProxy;
import com.lody.virtual.helper.compat.BuildCompat;
import com.lody.virtual.helper.utils.ArrayUtils;

import java.lang.reflect.Field;
import java.lang.reflect.Method;

import mirror.android.view.IAutoFillManager;

/**
 * @author 陈磊.
 */

public class AutoFillManagerStub extends BinderInvocationProxy {

    private static final String TAG = "AutoFillManagerStub";

    private static final String AUTO_FILL_NAME = "autofill";

    public AutoFillManagerStub() {
        super(IAutoFillManager.Stub.asInterface, AUTO_FILL_NAME);
    }

    @SuppressLint("WrongConstant")
    @Override
    public void inject() throws Throwable {
        super.inject();
        try {
            Object AutoFillManagerInstance = getContext().getSystemService(AUTO_FILL_NAME);
            if (AutoFillManagerInstance == null) {
                throw new NullPointerException("AutoFillManagerInstance is null.");
            }
            Object AutoFillManagerProxy = getInvocationStub().getProxyInterface();
            if (AutoFillManagerProxy == null) {
                throw new NullPointerException("AutoFillManagerProxy is null.");
            }
            Field AutoFillManagerServiceField = AutoFillManagerInstance.getClass().getDeclaredField("mService");
            AutoFillManagerServiceField.setAccessible(true);
            AutoFillManagerServiceField.set(AutoFillManagerInstance, AutoFillManagerProxy);
        } catch (Throwable tr) {
            Log.e(TAG, "AutoFillManagerStub inject error.", tr);
            return;
        }

        addMethodProxy(new ReplacePkgAndComponentProxy("startSession") {
            @Override
            public Object call(Object who, Method method, Object... args) throws Throwable {
                replaceFirstUserId(args);
                return super.call(who, method, args);
            }
        });
        addMethodProxy(new ReplacePkgAndComponentProxy("updateOrRestartSession"));
        addMethodProxy(new ReplaceLastPkgMethodProxy("isServiceEnabled"));
        addMethodProxy(new ReplaceLastUserIdMethodProxy("addClient"));
        addMethodProxy(new ReplaceLastUserIdMethodProxy("removeClient"));
        addMethodProxy(new ReplaceLastUserIdMethodProxy("updateSession"));
        addMethodProxy(new ReplaceLastUserIdMethodProxy("finishSession"));
        addMethodProxy(new ReplaceLastUserIdMethodProxy("cancelSession"));
        addMethodProxy(new ReplaceLastUserIdMethodProxy("setAuthenticationResult"));
        addMethodProxy(new ReplaceLastUserIdMethodProxy("setHasCallback"));
        addMethodProxy(new ReplaceLastUserIdMethodProxy("disableOwnedAutofillServices"));
        addMethodProxy(new ReplaceLastUserIdMethodProxy("isServiceSupported"));
        addMethodProxy(new ReplaceLastUserIdMethodProxy("isServiceEnabled"));

    }

    static class ReplacePkgAndComponentProxy extends ReplaceLastPkgMethodProxy {

        ReplacePkgAndComponentProxy(String name) {
            super(name);
        }

        @Override
        public boolean beforeCall(Object who, Method method, Object... args) {
            replaceLastAppComponent(args, getHostPkg());
            return super.beforeCall(who, method, args);
        }

        private void replaceLastAppComponent(Object[] args, String hostPkg) {
            int index = ArrayUtils.indexOfLast(args, ComponentName.class);
            if (index != -1) {
                ComponentName orig = (ComponentName) args[index];
                ComponentName newComponent = new ComponentName(hostPkg, orig.getClassName());
                args[index] = newComponent;
            }
        }
    }
}
