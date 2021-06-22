package com.lody.virtual.client.hook.proxies.bluetooth;

import android.os.Build;
import android.os.IInterface;
import android.text.TextUtils;

import com.lody.virtual.client.hook.base.BinderInvocationProxy;
import com.lody.virtual.client.hook.base.ReplaceLastPkgMethodProxy;
import com.lody.virtual.client.hook.base.ResultBinderMethodProxy;
import com.lody.virtual.helper.utils.marks.FakeDeviceMark;
import com.lody.virtual.remote.VDeviceConfig;

import java.lang.reflect.InvocationHandler;
import java.lang.reflect.Method;

import mirror.android.bluetooth.IBluetooth;

import static android.os.Build.VERSION_CODES.JELLY_BEAN_MR1;

/**
 * @see android.bluetooth.BluetoothManager
 */
public class BluetoothStub extends BinderInvocationProxy {
    private final static String SERVER_NAME = Build.VERSION.SDK_INT >= JELLY_BEAN_MR1 ?
            "bluetooth_manager" : "bluetooth";

    public BluetoothStub() {
        super(IBluetooth.Stub.asInterface, SERVER_NAME);
    }

    @Override
    protected void onBindMethods() {
        super.onBindMethods();
        addMethodProxy(new GetAddress());
        if (Build.VERSION.SDK_INT >= JELLY_BEAN_MR1) {
            addMethodProxy(new ResultBinderMethodProxy("registerAdapter") {
                @Override
                public InvocationHandler createProxy(final IInterface base) {
                    return new InvocationHandler() {
                        @Override
                        public Object invoke(Object proxy, Method method, Object[] args) throws Throwable {
                            if ("getAddress".equals(method.getName())) {
                                VDeviceConfig config = getDeviceConfig();
                                if (config.enable) {
                                    String mac = getDeviceConfig().bluetoothMac;
                                    if (!TextUtils.isEmpty(mac)) {
                                        return mac;
                                    }
                                }
                            }
                            return method.invoke(base, args);
                        }
                    };
                }
            });
        }
    }

    @FakeDeviceMark("fake MAC")
    private static class GetAddress extends ReplaceLastPkgMethodProxy {
        public GetAddress() {
            super("getAddress");
        }

        @Override
        public Object call(Object who, Method method, Object... args) throws Throwable {
            VDeviceConfig config = getDeviceConfig();
            if (config.enable) {
                String mac = getDeviceConfig().bluetoothMac;
                if (!TextUtils.isEmpty(mac)) {
                    return mac;
                }
            }
            return super.call(who, method, args);
        }
    }
}
