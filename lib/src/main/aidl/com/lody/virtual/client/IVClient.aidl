// IVClient.aidl
package com.lody.virtual.client;

import android.content.ComponentName;
import android.content.pm.ActivityInfo;
import android.content.pm.ServiceInfo;
import android.content.pm.ApplicationInfo;
import android.content.pm.ProviderInfo;
import android.content.Intent;

import com.lody.virtual.remote.PendingResultData;

interface IVClient {
    void scheduleNewIntent(in String creator, in IBinder token, in Intent intent);
    void finishActivity(in IBinder token);
    IBinder createProxyService(in ComponentName component, in IBinder binder);
    IBinder acquireProviderClient(in ProviderInfo info);
    IBinder getAppThread();
    IBinder getToken();
    boolean isAppRunning();
    String getDebugInfo();
    boolean finishReceiver(in IBinder token);
    void scheduleCreateService(in IBinder token, in ServiceInfo info);
    void scheduleBindService(in IBinder token, in Intent intent, in boolean rebind);
    void scheduleUnbindService(in IBinder token, in Intent intent);
    void scheduleServiceArgs(in IBinder token, in int startId, in Intent args);
    void scheduleStopService(in IBinder token);
}