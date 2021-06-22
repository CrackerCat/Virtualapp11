package com.lody.virtual.client.hook.proxies.wifi;

import android.annotation.SuppressLint;
import android.content.Context;
import android.net.DhcpInfo;
import android.net.wifi.ScanResult;
import android.net.wifi.SupplicantState;
import android.net.wifi.WifiConfiguration;
import android.net.wifi.WifiInfo;
import android.net.wifi.WifiManager;
import android.os.Build;
import android.os.Parcel;
import android.os.Parcelable;
import android.os.WorkSource;
import android.text.TextUtils;

import com.lody.virtual.client.core.SettingConfig;
import com.lody.virtual.client.core.VirtualCore;
import com.lody.virtual.client.hook.base.BinderInvocationProxy;
import com.lody.virtual.client.hook.base.MethodProxy;
import com.lody.virtual.client.hook.base.ReplaceCallingPkgMethodProxy;
import com.lody.virtual.client.hook.base.ResultStaticMethodProxy;
import com.lody.virtual.client.hook.base.StaticMethodProxy;
import com.lody.virtual.client.hook.utils.MethodParameterUtils;
import com.lody.virtual.helper.compat.BuildCompat;
import com.lody.virtual.helper.utils.ArrayUtils;
import com.lody.virtual.helper.utils.Reflect;
import com.lody.virtual.helper.utils.marks.FakeDeviceMark;
import com.lody.virtual.helper.utils.marks.FakeLocMark;
import com.lody.virtual.remote.VDeviceConfig;

import java.lang.reflect.Method;
import java.net.InetAddress;
import java.net.NetworkInterface;
import java.net.SocketException;
import java.util.ArrayList;
import java.util.Collections;
import java.util.List;
import java.util.Random;
import java.util.UUID;
import java.util.regex.Pattern;

import mirror.android.net.wifi.IWifiManager;
import mirror.android.net.wifi.WifiSsid;

/**
 * @author Lody
 * @see android.net.wifi.WifiManager
 */
public class WifiManagerStub extends BinderInvocationProxy {

    @Override
    public void inject() throws Throwable {
        super.inject();
        @SuppressLint("WifiManagerLeak")
        WifiManager wifiManager = (WifiManager) VirtualCore.get().getContext().getSystemService(Context.WIFI_SERVICE);
        if (mirror.android.net.wifi.WifiManager.mService != null) {
            try {
                mirror.android.net.wifi.WifiManager.mService.set(wifiManager, getInvocationStub().getProxyInterface());
            } catch (Exception e) {
                e.printStackTrace();
            }
        } else if (mirror.android.net.wifi.WifiManager.sService != null) {
            try {
                mirror.android.net.wifi.WifiManager.sService.set(getInvocationStub().getProxyInterface());
            } catch (Exception e) {
                e.printStackTrace();
            }
        }
    }

    private class RemoveWorkSourceMethodProxy extends StaticMethodProxy {

        RemoveWorkSourceMethodProxy(String name) {
            super(name);
        }

        @Override
        public Object call(Object who, Method method, Object... args) throws Throwable {
            int index = ArrayUtils.indexOfFirst(args, WorkSource.class);
            if (index >= 0) {
                args[index] = null;
            }
            return super.call(who, method, args);
        }
    }


    public WifiManagerStub() {
        super(IWifiManager.Stub.asInterface, Context.WIFI_SERVICE);
    }

    @Override
    protected void onBindMethods() {
        super.onBindMethods();
        addMethodProxy(new MethodProxy() {
            @Override
            public String getMethodName() {
                return "isWifiEnabled";
            }

            @Override
            public Object call(Object who, Method method, Object... args) throws Throwable {
                SettingConfig.FakeWifiStatus fakeWifiStatus = getConfig().getFakeWifiStatus();
                if (fakeWifiStatus != null) {
                    return true;
                }
                return super.call(who, method, args);
            }
        });
        addMethodProxy(new MethodProxy() {
            @Override
            public String getMethodName() {
                return "getWifiEnabledState";
            }

            @Override
            public Object call(Object who, Method method, Object... args) throws Throwable {
                SettingConfig.FakeWifiStatus fakeWifiStatus = getConfig().getFakeWifiStatus();
                if (fakeWifiStatus != null) {
                    return WifiManager.WIFI_STATE_ENABLED;
                }
                return super.call(who, method, args);
            }
        });
        addMethodProxy(new MethodProxy() {
            @Override
            public String getMethodName() {
                return "createDhcpInfo";
            }

            @Override
            public Object call(Object who, Method method, Object... args) throws Throwable {
                SettingConfig.FakeWifiStatus fakeWifiStatus = getConfig().getFakeWifiStatus();
                if (fakeWifiStatus != null) {
                    IPInfo ipInfo = getIPInfo();
                    if (ipInfo != null) {
                        return createDhcpInfo(ipInfo);
                    }
                }
                return super.call(who, method, args);
            }
        });
        addMethodProxy(new GetConnectionInfo());
        addMethodProxy(new GetScanResults());
        addMethodProxy(new ReplaceCallingPkgMethodProxy("getBatchedScanResults"));
        addMethodProxy(new RemoveWorkSourceMethodProxy("acquireWifiLock"));
        addMethodProxy(new RemoveWorkSourceMethodProxy("updateWifiLockWorkSource"));
        if (Build.VERSION.SDK_INT > Build.VERSION_CODES.LOLLIPOP) {
            addMethodProxy(new RemoveWorkSourceMethodProxy("startLocationRestrictedScan"));
        }
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.KITKAT) {
            addMethodProxy(new RemoveWorkSourceMethodProxy("requestBatchedScan"));
        }
        addMethodProxy(new ReplaceCallingPkgMethodProxy("setWifiEnabled"));
        addMethodProxy(new StaticMethodProxy("getWifiApConfiguration") {

            @Override
            public Object call(Object who, Method method, Object... args) throws Throwable {
                List<WifiConfiguration> configurations = ((WifiManager) getContext().getApplicationContext().getSystemService(Context.WIFI_SERVICE)).getConfiguredNetworks();
                if (!configurations.isEmpty()) {
                    return configurations.get(0);
                } else {
                    WifiConfiguration wifiConfiguration = new WifiConfiguration();
                    wifiConfiguration.SSID = "AndroidAP_" + new Random().nextInt(9000) + 1000;
                    wifiConfiguration.allowedKeyManagement.set(4);
                    String uuid = UUID.randomUUID().toString();
                    wifiConfiguration.preSharedKey = uuid.substring(0, 8) + uuid.substring(9, 13);
                    return wifiConfiguration;
                }
            }
        });
        addMethodProxy(new ResultStaticMethodProxy("setWifiApConfiguration", 0));
        addMethodProxy(new ReplaceCallingPkgMethodProxy("startLocalOnlyHotspot"));
        if (BuildCompat.isOreo()) {
            addMethodProxy(new RemoveWorkSourceMethodProxy("startScan") {
                @Override
                public Object call(Object who, Method method, Object... args) throws Throwable {
                    MethodParameterUtils.replaceFirstAppPkg(args);
                    return super.call(who, method, args);
                }
            });
        } else if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.KITKAT) {
            addMethodProxy(new RemoveWorkSourceMethodProxy("startScan"));
        }
    }

    @FakeLocMark("Fake wifi bssid")
    @FakeDeviceMark("fake wifi MAC")
    private final class GetConnectionInfo extends MethodProxy {
        @Override
        public String getMethodName() {
            return "getConnectionInfo";
        }

        @Override
        public Object call(Object who, Method method, Object... args) throws Throwable {
            MethodParameterUtils.replaceFirstAppPkg(args);
            SettingConfig.FakeWifiStatus status = getConfig().getFakeWifiStatus();
            if (status != null) {
                return createWifiInfo(status);
            }
            WifiInfo wifiInfo = (WifiInfo) method.invoke(who, args);
            if (wifiInfo != null) {
                if (isFakeLocationEnable()) {
                    mirror.android.net.wifi.WifiInfo.mBSSID.set(wifiInfo, "00:00:00:00:00:00");
                    mirror.android.net.wifi.WifiInfo.mMacAddress.set(wifiInfo, "00:00:00:00:00:00");
                } else {
                    VDeviceConfig config = getDeviceConfig();
                    if (config.enable) {
                        String mac = getDeviceConfig().wifiMac;
                        if (!TextUtils.isEmpty(mac)) {
                            mirror.android.net.wifi.WifiInfo.mMacAddress.set(wifiInfo, mac);
                        }
                    }
                }
            }
            return wifiInfo;
        }
    }

    @FakeLocMark("fake scan result")
    private final class GetScanResults extends ReplaceCallingPkgMethodProxy {

        public GetScanResults() {
            super("getScanResults");
        }

        @Override
        public Object call(Object who, Method method, Object... args) throws Throwable {
//            noinspection unchecked
            if (isFakeLocationEnable()) {
                return new ArrayList<ScanResult>();
            }
            return super.call(who, method, args);
        }
    }

    private static ScanResult cloneScanResult(Parcelable scanResult) {
        Parcel p = Parcel.obtain();
        scanResult.writeToParcel(p, 0);
        p.setDataPosition(0);
        ScanResult newScanResult = Reflect.on(scanResult).field("CREATOR").call("createFromParcel", p).get();
        p.recycle();
        return newScanResult;
    }

    public static class IPInfo {
        NetworkInterface intf;
        InetAddress addr;
        String ip;
        int ip_hex;
        int netmask_hex;
    }


    private static IPInfo getIPInfo() {
        try {
            List<NetworkInterface> interfaces = Collections.list(NetworkInterface.getNetworkInterfaces());
            for (NetworkInterface intf : interfaces) {
                List<InetAddress> addrs = Collections.list(intf.getInetAddresses());
                for (InetAddress addr : addrs) {
                    if (!addr.isLoopbackAddress()) {
                        String sAddr = addr.getHostAddress().toUpperCase();
                        boolean isIPv4 = isIPv4Address(sAddr);
                        if (isIPv4) {
                            IPInfo info = new IPInfo();
                            info.addr = addr;
                            info.intf = intf;
                            info.ip = sAddr;
                            info.ip_hex = InetAddress_to_hex(addr);
                            info.netmask_hex = netmask_to_hex(intf.getInterfaceAddresses().get(0).getNetworkPrefixLength());
                            return info;
                        }
                    }
                }
            }
        } catch (SocketException e) {
            e.printStackTrace();
        }
        return null;
    }

    private static boolean isIPv4Address(String input) {
        Pattern IPV4_PATTERN = Pattern.compile("^(25[0-5]|2[0-4]\\d|[0-1]?\\d?\\d)(\\.(25[0-5]|2[0-4]\\d|[0-1]?\\d?\\d)){3}$");
        return IPV4_PATTERN.matcher(input).matches();
    }

    private static int netmask_to_hex(int netmask_slash) {
        int r = 0;
        int b = 1;
        for (int i = 0; i < netmask_slash; i++, b = b << 1)
            r |= b;
        return r;
    }

    private static int InetAddress_to_hex(InetAddress a) {
        int result = 0;
        byte b[] = a.getAddress();
        for (int i = 0; i < 4; i++)
            result |= (b[i] & 0xff) << (8 * i);
        return result;
    }

    private DhcpInfo createDhcpInfo(IPInfo ip) {
        DhcpInfo i = new DhcpInfo();
        i.ipAddress = ip.ip_hex;
        i.netmask = ip.netmask_hex;
        i.dns1 = 0x04040404;
        i.dns2 = 0x08080808;
        return i;
    }

    private static WifiInfo createWifiInfo(SettingConfig.FakeWifiStatus status) {
        WifiInfo info = mirror.android.net.wifi.WifiInfo.ctor.newInstance();
        IPInfo ip = getIPInfo();
        InetAddress address = (ip != null ? ip.addr : null);
        mirror.android.net.wifi.WifiInfo.mNetworkId.set(info, 1);
        mirror.android.net.wifi.WifiInfo.mSupplicantState.set(info, SupplicantState.COMPLETED);
        mirror.android.net.wifi.WifiInfo.mBSSID.set(info, status.getBSSID());
        mirror.android.net.wifi.WifiInfo.mMacAddress.set(info, status.getMAC());
        mirror.android.net.wifi.WifiInfo.mIpAddress.set(info, address);
        mirror.android.net.wifi.WifiInfo.mLinkSpeed.set(info, 65);
        if (Build.VERSION.SDK_INT >= 21) {
            mirror.android.net.wifi.WifiInfo.mFrequency.set(info, 5000); // MHz
        }
        mirror.android.net.wifi.WifiInfo.mRssi.set(info, 200); // MAX_RSSI
        if (mirror.android.net.wifi.WifiInfo.mWifiSsid != null) {
            mirror.android.net.wifi.WifiInfo.mWifiSsid.set(info, WifiSsid.createFromAsciiEncoded.call(status.getSSID()));
        } else {
            mirror.android.net.wifi.WifiInfo.mSSID.set(info, status.getSSID());
        }
        return info;
    }

}
