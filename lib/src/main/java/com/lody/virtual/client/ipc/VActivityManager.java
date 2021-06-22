package com.lody.virtual.client.ipc;

import android.app.Activity;
import android.app.IServiceConnection;
import android.app.Notification;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.ServiceConnection;
import android.content.pm.ActivityInfo;
import android.content.pm.ProviderInfo;
import android.content.pm.ResolveInfo;
import android.content.pm.ServiceInfo;
import android.os.Bundle;
import android.os.Handler;
import android.os.IBinder;
import android.os.IInterface;
import android.os.Process;
import android.os.RemoteException;
import android.os.ResultReceiver;

import com.lody.virtual.client.VClient;
import com.lody.virtual.client.core.VirtualCore;
import com.lody.virtual.client.env.VirtualRuntime;
import com.lody.virtual.client.hook.secondary.ServiceConnectionDelegate;
import com.lody.virtual.client.stub.WindowPreviewActivity;
import com.lody.virtual.helper.compat.ActivityManagerCompat;
import com.lody.virtual.helper.utils.ComponentUtils;
import com.lody.virtual.helper.utils.IInterfaceUtils;
import com.lody.virtual.helper.utils.VLog;
import com.lody.virtual.os.VUserHandle;
import com.lody.virtual.remote.AppTaskInfo;
import com.lody.virtual.remote.BadgerInfo;
import com.lody.virtual.remote.ClientConfig;
import com.lody.virtual.remote.IntentSenderData;
import com.lody.virtual.remote.VParceledListSlice;
import com.lody.virtual.server.bit64.V64BitHelper;
import com.lody.virtual.server.interfaces.IActivityManager;

import java.util.List;

import mirror.android.app.ActivityThread;
import mirror.android.content.ContentProviderNative;

/**
 * @author Lody
 */
public class VActivityManager {

    private static final VActivityManager sAM = new VActivityManager();
    private IActivityManager mService;

    public IActivityManager getService() {
        if (!IInterfaceUtils.isAlive(mService)) {
            synchronized (VActivityManager.class) {
                final Object remote = getRemoteInterface();
                mService = LocalProxyUtils.genProxy(IActivityManager.class, remote);
            }
        }
        return mService;
    }

    private Object getRemoteInterface() {
        return IActivityManager.Stub
                .asInterface(ServiceManagerNative.getService(ServiceManagerNative.ACTIVITY));
    }

    public static VActivityManager get() {
        return sAM;
    }

    public int startActivity(Intent intent, ActivityInfo info, IBinder resultTo, Bundle options, String resultWho, int requestCode, int userId) {
        if (info == null) {
            info = VirtualCore.get().resolveActivityInfo(intent, userId);
            if (info == null) {
                return ActivityManagerCompat.START_INTENT_NOT_RESOLVED;
            }
        }
        try {
            return getService().startActivity(intent, info, resultTo, options, resultWho, requestCode, userId);
        } catch (RemoteException e) {
            return VirtualRuntime.crash(e);
        }
    }

    public int startActivities(Intent[] intents, String[] resolvedTypes, IBinder token, Bundle options, int userId) {
        try {
            return getService().startActivities(intents, resolvedTypes, token, options, userId);
        } catch (RemoteException e) {
            return VirtualRuntime.crash(e);
        }
    }

    public int startActivity(Intent intent, int userId) {
        if (userId < 0) {
            return ActivityManagerCompat.START_NOT_CURRENT_USER_ACTIVITY;
        }
        ActivityInfo info = VirtualCore.get().resolveActivityInfo(intent, userId);
        if (info == null) {
            return ActivityManagerCompat.START_INTENT_NOT_RESOLVED;
        }
        return startActivity(intent, info, null, null, null, 0, userId);
    }

    public void appDoneExecuting(String packageName) {
        try {
            getService().appDoneExecuting(packageName, VUserHandle.myUserId());
        } catch (RemoteException e) {
            VirtualRuntime.crash(e);
        }
    }

    public void onActivityCreate(IBinder record, IBinder token, int taskId) {
        try {
            getService().onActivityCreated(record, token, taskId);
        } catch (RemoteException e) {
            e.printStackTrace();
        }
    }

    public void onActivityResumed(IBinder token) {
        try {
            getService().onActivityResumed(VUserHandle.myUserId(), token);
        } catch (RemoteException e) {
            e.printStackTrace();
        }
    }

    public boolean onActivityDestroy(IBinder token) {
        try {
            return getService().onActivityDestroyed(VUserHandle.myUserId(), token);
        } catch (RemoteException e) {
            return VirtualRuntime.crash(e);
        }
    }

    public AppTaskInfo getTaskInfo(int taskId) {
        try {
            return getService().getTaskInfo(taskId);
        } catch (RemoteException e) {
            return VirtualRuntime.crash(e);
        }
    }

    public ComponentName getCallingActivity(IBinder token) {
        try {
            return getService().getCallingActivity(VUserHandle.myUserId(), token);
        } catch (RemoteException e) {
            return VirtualRuntime.crash(e);
        }
    }

    public String getCallingPackage(IBinder token) {
        try {
            return getService().getCallingPackage(VUserHandle.myUserId(), token);
        } catch (RemoteException e) {
            return VirtualRuntime.crash(e);
        }
    }

    public String getPackageForToken(IBinder token) {
        try {
            return getService().getPackageForToken(VUserHandle.myUserId(), token);
        } catch (RemoteException e) {
            return VirtualRuntime.crash(e);
        }
    }

    public ComponentName getActivityForToken(IBinder token) {
        try {
            return getService().getActivityClassForToken(VUserHandle.myUserId(), token);
        } catch (RemoteException e) {
            return VirtualRuntime.crash(e);
        }
    }

    public void processRestarted(String packageName, String processName, int userId) {
        try {
            getService().processRestarted(packageName, processName, userId);
        } catch (RemoteException e) {
            e.printStackTrace();
        }
    }

    public String getAppProcessName(int pid) {
        try {
            return getService().getAppProcessName(pid);
        } catch (RemoteException e) {
            return VirtualRuntime.crash(e);
        }
    }

    public String getInitialPackage(int pid) {
        try {
            return getService().getInitialPackage(pid);
        } catch (RemoteException e) {
            return VirtualRuntime.crash(e);
        }
    }

    public boolean isAppProcess(String processName) {
        try {
            return getService().isAppProcess(processName);
        } catch (RemoteException e) {
            return VirtualRuntime.crash(e);
        }
    }

    public void killAllApps() {
        try {
            getService().killAllApps();
        } catch (RemoteException e) {
            e.printStackTrace();
        }
    }

    public void killApplicationProcess(String procName, int uid) {
        try {
            getService().killApplicationProcess(procName, uid);
        } catch (RemoteException e) {
            e.printStackTrace();
        }
    }

    public void killAppByPkg(String pkg, int userId) {
        try {
            getService().killAppByPkg(pkg, userId);
        } catch (RemoteException e) {
            e.printStackTrace();
        }
    }

    public List<String> getProcessPkgList(int pid) {
        try {
            return getService().getProcessPkgList(pid);
        } catch (RemoteException e) {
            return VirtualRuntime.crash(e);
        }
    }

    public boolean isAppPid(int pid) {
        try {
            return getService().isAppPid(pid);
        } catch (RemoteException e) {
            return VirtualRuntime.crash(e);
        }
    }

    public int getUidByPid(int pid) {
        try {
            return getService().getUidByPid(pid);
        } catch (RemoteException e) {
            return VirtualRuntime.crash(e);
        }
    }

    public int getSystemPid() {
        try {
            return getService().getSystemPid();
        } catch (RemoteException e) {
            return VirtualRuntime.crash(e);
        }
    }

    public int getSystemUid() {
        try {
            return getService().getSystemUid();
        } catch (RemoteException e) {
            return VirtualRuntime.crash(e);
        }
    }

    public void sendCancelActivityResult(IBinder resultTo, String resultWho, int requestCode) {
        sendActivityResult(resultTo, resultWho, requestCode, null, 0);
    }

    public void sendActivityResult(IBinder resultTo, String resultWho, int requestCode, Intent data, int resultCode) {
        Activity activity = findActivityByToken(resultTo);
        if (activity != null) {
            Object mainThread = VirtualCore.mainThread();
            ActivityThread.sendActivityResult.call(mainThread, resultTo, resultWho, requestCode, data, resultCode);
        }
    }

    public IInterface acquireProviderClient(int userId, ProviderInfo info) throws RemoteException {
        IBinder binder = getService().acquireProviderClient(userId, info);
        if (binder != null) {
            return ContentProviderNative.asInterface.call(binder);
        }
        return null;
    }

    public void addOrUpdateIntentSender(IntentSenderData sender) throws RemoteException {
        getService().addOrUpdateIntentSender(sender, VUserHandle.myUserId());
    }

    public void removeIntentSender(IBinder token) throws RemoteException {
        getService().removeIntentSender(token);
    }

    public IntentSenderData getIntentSender(IBinder token) {
        try {
            return getService().getIntentSender(token);
        } catch (RemoteException e) {
            return VirtualRuntime.crash(e);
        }
    }

    public Activity findActivityByToken(IBinder token) {
        Object r = ActivityThread.mActivities.get(VirtualCore.mainThread()).get(token);
        if (r != null) {
            return ActivityThread.ActivityClientRecord.activity.get(r);
        }
        return null;
    }


    public void finishActivity(IBinder token) {
        Activity activity = findActivityByToken(token);
        if (activity == null) {
            VLog.e("VActivityManager", "finishActivity fail : activity = null");
            return;
        }
        while (true) {
            // We shouldn't use Activity.getParent(),
            // because It may be overwritten.
            Activity parent = mirror.android.app.Activity.mParent.get(activity);
            if (parent == null) {
                break;
            }
            activity = parent;
        }
        // We shouldn't use Activity.isFinishing(),
        // because it may be overwritten.
        int resultCode = mirror.android.app.Activity.mResultCode.get(activity);
        Intent resultData = mirror.android.app.Activity.mResultData.get(activity);
        ActivityManagerCompat.finishActivity(token, resultCode, resultData);
        mirror.android.app.Activity.mFinished.set(activity, true);
    }

    public boolean isAppRunning(String packageName, int userId, boolean foreground) {
        try {
            return getService().isAppRunning(packageName, userId, foreground);
        } catch (RemoteException e) {
            return VirtualRuntime.crash(e);
        }
    }

    public int getUid() {
        return VClient.get().getVUid();
    }

    public ClientConfig initProcess(String packageName, String processName, int userId) {
        try {
            return getService().initProcess(packageName, processName, userId);
        } catch (RemoteException e) {
            return VirtualRuntime.crash(e);
        }
    }

    public void sendBroadcast(Intent intent, int userId) {
        Intent newIntent = ComponentUtils.redirectBroadcastIntent(intent, userId);
        if (newIntent != null) {
            VirtualCore.get().getContext().sendBroadcast(newIntent);
        }
    }

    public void notifyBadgerChange(BadgerInfo info) {
        try {
            getService().notifyBadgerChange(info);
        } catch (RemoteException e) {
            VirtualRuntime.crash(e);
        }
    }

    public int getCallingUid() {
        try {
            int id = getService().getCallingUidByPid(Process.myPid());
            if (id <= 0) {
                return VClient.get().getVUid();
            }
            return id;
        } catch (RemoteException e) {
            VirtualRuntime.crash(e);
        }
        return VClient.get().getVUid();
    }

    public void setAppInactive(String packageName, boolean idle, int userId) {
        try {
            getService().setAppInactive(packageName, idle, userId);
        } catch (RemoteException e) {
            VirtualRuntime.crash(e);
        }
    }

    public boolean isAppInactive(String packageName, int userId) {
        try {
            return getService().isAppInactive(packageName, userId);
        } catch (RemoteException e) {
            return VirtualRuntime.crash(e);
        }
    }

    public boolean launchApp(final int userId, String packageName) {
        return launchApp(userId, packageName, true);
    }

    public boolean launchApp(final int userId, String packageName, boolean preview) {
        if (VirtualCore.get().isRun64BitProcess(packageName)) {
            if (!V64BitHelper.has64BitEngineStartPermission()) {
                return false;
            }
        }
        Context context = VirtualCore.get().getContext();
        VPackageManager pm = VPackageManager.get();
        Intent intentToResolve = new Intent(Intent.ACTION_MAIN);
        intentToResolve.addCategory(Intent.CATEGORY_INFO);
        intentToResolve.setPackage(packageName);
        List<ResolveInfo> ris = pm.queryIntentActivities(intentToResolve, intentToResolve.resolveType(context), 0, userId);

        // Otherwise, try to find a main launcher activity.
        if (ris == null || ris.size() <= 0) {
            // reuse the intent instance
            intentToResolve.removeCategory(Intent.CATEGORY_INFO);
            intentToResolve.addCategory(Intent.CATEGORY_LAUNCHER);
            intentToResolve.setPackage(packageName);
            ris = pm.queryIntentActivities(intentToResolve, intentToResolve.resolveType(context), 0, userId);
        }
        if (ris == null || ris.size() <= 0) {
            return false;
        }
        ActivityInfo info = ris.get(0).activityInfo;
        final Intent intent = new Intent(intentToResolve);
        intent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
        intent.setClassName(info.packageName, info.name);
        if (!preview || VActivityManager.get().isAppRunning(info.packageName, userId, true)) {
            VActivityManager.get().startActivity(intent, userId);
        } else {
            intent.putExtra("_VA_|no_animation", true);
            WindowPreviewActivity.previewActivity(userId, info);
            VirtualRuntime.getUIHandler().postDelayed(new Runnable() {
                @Override
                public void run() {
                    VActivityManager.get().startActivity(intent, userId);
                }
            }, 400L);
        }
        return true;
    }

    public void onFinishActivity(IBinder token) {
        try {
            getService().onActivityFinish(VUserHandle.myUserId(), token);
        } catch (RemoteException e) {
            VirtualRuntime.crash(e);
        }
    }

    public int checkPermission(String permission, int pid, int uid) {
        try {
            return getService().checkPermission(VirtualCore.get().is64BitEngine(), permission, pid, uid);
        } catch (RemoteException e) {
            return VirtualRuntime.crash(e);
        }
    }

    public void handleDownloadCompleteIntent(Intent intent) {
        try {
            getService().handleDownloadCompleteIntent(intent);
        } catch (RemoteException e) {
            e.printStackTrace();
        }
    }

    public boolean finishActivityAffinity(int userId, IBinder token) {
        try {
            return getService().finishActivityAffinity(userId, token);
        } catch (RemoteException e) {
            return VirtualRuntime.crash(e);
        }
    }

    public ComponentName startService(Intent service, String resolvedType, int userId) {
        try {
            return getService().startService(service, resolvedType, userId);
        } catch (RemoteException e) {
            return VirtualRuntime.crash(e);
        }
    }

    public int stopService(IInterface caller, Intent service, String resolvedType) {
        try {
            return getService().stopService(caller != null ? caller.asBinder() : null, service, resolvedType, VUserHandle.myUserId());
        } catch (RemoteException e) {
            return VirtualRuntime.crash(e);
        }
    }

    public boolean stopServiceToken(ComponentName className, IBinder token, int startId) {
        try {
            return getService().stopServiceToken(className, token, startId, VUserHandle.myUserId());
        } catch (RemoteException e) {
            return VirtualRuntime.crash(e);
        }
    }

    public boolean isVAServiceToken(IBinder token) {
        try {
            return getService().isVAServiceToken(token);
        } catch (RemoteException e) {
            return VirtualRuntime.crash(e);
        }
    }

    public void setServiceForeground(ComponentName className, IBinder token, int id, Notification notification, boolean removeNotification) {
        try {
            getService().setServiceForeground(className, token, id, notification, removeNotification, VUserHandle.myUserId());
        } catch (RemoteException e) {
            e.printStackTrace();
        }
    }

    public boolean bindService(Context context, Intent service, ServiceConnection connection, int flags) {
        try {
            IServiceConnection conn = ServiceConnectionDelegate.getDelegate(context, connection, flags);
            return getService().bindService(null, null, service, null, conn, flags, 0) > 0;
        } catch (RemoteException e) {
            return VirtualRuntime.crash(e);
        }
    }

    public boolean unbindService(Context context, ServiceConnection connection) {
        try {
            IServiceConnection conn = ServiceConnectionDelegate.removeDelegate(context, connection);
            return getService().unbindService(conn, VUserHandle.myUserId());
        } catch (RemoteException e) {
            return VirtualRuntime.crash(e);
        }
    }

    public int bindService(IBinder caller, IBinder token, Intent service, String resolvedType, IServiceConnection connection, int flags, int userId) {
        try {
            return getService().bindService(caller, token, service, resolvedType, connection, flags, userId);
        } catch (RemoteException e) {
            return VirtualRuntime.crash(e);
        }
    }

    public boolean unbindService(IServiceConnection connection) {
        try {
            return getService().unbindService(connection, VUserHandle.myUserId());
        } catch (RemoteException e) {
            return VirtualRuntime.crash(e);
        }
    }

    public void unbindFinished(IBinder token, Intent service, boolean doRebind) {
        try {
            getService().unbindFinished(token, service, doRebind, VUserHandle.myUserId());
        } catch (RemoteException e) {
            e.printStackTrace();
        }
    }

    public void serviceDoneExecuting(IBinder token, int type, int startId, int res) {
        try {
            getService().serviceDoneExecuting(token, type, startId, res, VUserHandle.myUserId());
        } catch (RemoteException e) {
            //ignore
        }
    }

    public IBinder peekService(Intent service, String resolvedType) {
        try {
            return getService().peekService(service, resolvedType, VUserHandle.myUserId());
        } catch (RemoteException e) {
            return VirtualRuntime.crash(e);
        }
    }

    public void publishService(IBinder token, Intent intent, IBinder service) {
        try {
            getService().publishService(token, intent, service, VUserHandle.myUserId());
        } catch (RemoteException e) {
            e.printStackTrace();
        }
    }

    public VParceledListSlice getServices(int maxNum, int flags) {
        try {
            return getService().getServices(maxNum, flags, VUserHandle.myUserId());
        } catch (RemoteException e) {
            return VirtualRuntime.crash(e);
        }
    }

    public boolean broadcastFinish(IBinder token) {
        try {
            return getService().broadcastFinish(token);
        } catch (RemoteException e) {
            return VirtualRuntime.crash(e);
        }
    }
}
