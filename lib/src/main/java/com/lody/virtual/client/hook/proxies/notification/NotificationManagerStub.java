package com.lody.virtual.client.hook.proxies.notification;

import android.os.Build;
import android.os.IInterface;

import com.lody.virtual.client.hook.annotations.Inject;
import com.lody.virtual.client.hook.base.MethodInvocationProxy;
import com.lody.virtual.client.hook.base.MethodInvocationStub;
import com.lody.virtual.client.hook.base.ReplaceCallingPkgAndLastUserIdMethodProxy;
import com.lody.virtual.client.hook.base.ReplaceCallingPkgMethodProxy;
import com.lody.virtual.client.hook.base.ReplaceLastUserIdMethodProxy;
import com.lody.virtual.helper.compat.BuildCompat;

import mirror.android.app.NotificationManager;
import mirror.android.widget.Toast;

/**
 * @author Lody
 * @see android.app.NotificationManager
 * @see android.widget.Toast
 */
@Inject(MethodProxies.class)
public class NotificationManagerStub extends MethodInvocationProxy<MethodInvocationStub<IInterface>> {

    public NotificationManagerStub() {
        super(new MethodInvocationStub<IInterface>(NotificationManager.getService.call()));
    }

    @Override
    protected void onBindMethods() {
        super.onBindMethods();
        addMethodProxy(new ReplaceCallingPkgMethodProxy("enqueueToast"));
        addMethodProxy(new ReplaceCallingPkgMethodProxy("enqueueToastEx"));
        addMethodProxy(new ReplaceCallingPkgMethodProxy("cancelToast"));
        addMethodProxy(new ReplaceLastUserIdMethodProxy("cancelAllNotifications"));
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.N) {
            addMethodProxy(new ReplaceCallingPkgMethodProxy("removeAutomaticZenRules"));
            addMethodProxy(new ReplaceCallingPkgMethodProxy("getImportance"));
            addMethodProxy(new ReplaceCallingPkgMethodProxy("areNotificationsEnabled"));
            addMethodProxy(new ReplaceCallingPkgMethodProxy("setNotificationPolicy"));
            addMethodProxy(new ReplaceCallingPkgMethodProxy("getNotificationPolicy"));
            addMethodProxy(new ReplaceCallingPkgMethodProxy("setNotificationPolicyAccessGranted"));
            addMethodProxy(new ReplaceCallingPkgMethodProxy("isNotificationPolicyAccessGranted"));
            addMethodProxy(new ReplaceCallingPkgMethodProxy("isNotificationPolicyAccessGrantedForPackage"));
        }
        if ("samsung".equalsIgnoreCase(Build.BRAND) || "samsung".equalsIgnoreCase(Build.MANUFACTURER)) {
            addMethodProxy(new ReplaceCallingPkgMethodProxy("removeEdgeNotification"));
        }

        if(BuildCompat.isOreo()) {
            addMethodProxy(new ReplaceCallingPkgMethodProxy("createNotificationChannelGroups"));
            addMethodProxy(new ReplaceCallingPkgMethodProxy("getNotificationChannelGroups"));
            addMethodProxy(new ReplaceCallingPkgMethodProxy("deleteNotificationChannelGroup"));

            addMethodProxy(new ReplaceCallingPkgMethodProxy("createNotificationChannels"));
            addMethodProxy(new ReplaceCallingPkgAndLastUserIdMethodProxy("getNotificationChannels"));
            addMethodProxy(new ReplaceCallingPkgAndLastUserIdMethodProxy("getNotificationChannel"));
            addMethodProxy(new ReplaceCallingPkgMethodProxy("deleteNotificationChannel"));
        }

        if (BuildCompat.isPie()) {
            addMethodProxy(new ReplaceCallingPkgMethodProxy("getNotificationChannelGroup"));
        }

        addMethodProxy(new ReplaceCallingPkgMethodProxy("setInterruptionFilter"));
        addMethodProxy(new ReplaceCallingPkgMethodProxy("getPackageImportance"));
    }

    @Override
    public void inject() throws Throwable {
        NotificationManager.sService.set(getInvocationStub().getProxyInterface());
        Toast.sService.set(getInvocationStub().getProxyInterface());
    }

    @Override
    public boolean isEnvBad() {
        return NotificationManager.getService.call() != getInvocationStub().getProxyInterface();
    }
}
