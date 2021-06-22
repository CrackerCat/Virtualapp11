package com.lody.virtual.server.pm;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.pm.ApplicationInfo;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
import android.graphics.drawable.Drawable;
import android.net.Uri;
import android.os.Bundle;
import android.os.RemoteCallbackList;
import android.os.RemoteException;
import android.os.ResultReceiver;


import com.lody.virtual.R;
import com.lody.virtual.client.core.VirtualCore;
import com.lody.virtual.client.env.SpecialComponentList;
import com.lody.virtual.client.stub.StubManifest;
import com.lody.virtual.helper.DexOptimizer;
import com.lody.virtual.helper.InstalledInfoCache;
import com.lody.virtual.helper.collection.IntArray;
import com.lody.virtual.helper.compat.BuildCompat;
import com.lody.virtual.helper.compat.NativeLibraryHelperCompat;
import com.lody.virtual.helper.utils.ArrayUtils;
import com.lody.virtual.helper.utils.FileUtils;
import com.lody.virtual.helper.utils.Singleton;
import com.lody.virtual.helper.utils.VLog;
import com.lody.virtual.os.VEnvironment;
import com.lody.virtual.os.VUserHandle;
import com.lody.virtual.os.VUserInfo;
import com.lody.virtual.os.VUserManager;
import com.lody.virtual.remote.InstallOptions;
import com.lody.virtual.remote.InstallResult;
import com.lody.virtual.remote.InstalledAppInfo;
import com.lody.virtual.server.accounts.VAccountManagerService;
import com.lody.virtual.server.am.UidSystem;
import com.lody.virtual.server.am.VActivityManagerService;
import com.lody.virtual.server.bit64.V64BitHelper;
import com.lody.virtual.server.interfaces.IAppManager;
import com.lody.virtual.server.interfaces.IPackageObserver;
import com.lody.virtual.server.notification.VNotificationManagerService;
import com.lody.virtual.server.pm.parser.PackageParserEx;
import com.lody.virtual.server.pm.parser.VPackage;
import com.lody.virtual.tools.util.DebugKook;

import java.io.File;
import java.io.IOException;
import java.util.ArrayList;
import java.util.HashSet;
import java.util.List;
import java.util.Set;

import static com.lody.virtual.remote.InstalledAppInfo.MODE_APP_COPY_APK;
import static com.lody.virtual.remote.InstalledAppInfo.MODE_APP_USE_OUTSIDE_APK;


/**
 * @author Lody
 */
public class VAppManagerService extends IAppManager.Stub {

    private static final String TAG = VAppManagerService.class.getSimpleName();
    private static final Singleton<VAppManagerService> sService = new Singleton<VAppManagerService>() {
        @Override
        protected VAppManagerService create() {
            return new VAppManagerService();
        }
    };
    private final UidSystem mUidSystem = new UidSystem();
    private final PackagePersistenceLayer mPersistenceLayer = new PackagePersistenceLayer(this);
    private final Set<String> mVisibleOutsidePackages = new HashSet<>();
    private boolean mBooting;
    private RemoteCallbackList<IPackageObserver> mRemoteCallbackList = new RemoteCallbackList<>();
    private BroadcastReceiver appEventReciever = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            if (mBooting) {
                return;
            }
            PendingResult result = goAsync();
            String action = intent.getAction();
            if (action == null) {
                return;
            }
            Uri data = intent.getData();
            if (data == null) {
                return;
            }
            String pkg = data.getSchemeSpecificPart();
            if (pkg == null) {
                return;
            }
            PackageSetting ps = PackageCacheManager.getSetting(pkg);
            if (ps == null || ps.appMode != InstalledAppInfo.MODE_APP_USE_OUTSIDE_APK) {
                return;
            }
            VActivityManagerService.get().killAppByPkg(pkg, VUserHandle.USER_ALL);
            if (action.equals(Intent.ACTION_PACKAGE_REPLACED)) {
                ApplicationInfo outInfo = null;
                try {
                    outInfo = VirtualCore.getPM().getApplicationInfo(pkg, 0);
                } catch (PackageManager.NameNotFoundException e) {
                    e.printStackTrace();
                }
                if (outInfo == null) {
                    return;
                }
                InstallOptions options = InstallOptions.makeOptions(true, false, InstallOptions.UpdateStrategy.FORCE_UPDATE);
                InstallResult res = installPackageImpl(outInfo.publicSourceDir, options);
                VLog.e(TAG, "Update package %s %s", res.packageName, res.isSuccess ? "success" : "failed");
            } else if (action.equals(Intent.ACTION_PACKAGE_REMOVED)) {
                if (intent.getBooleanExtra(Intent.EXTRA_DATA_REMOVED, false)) {
                    VLog.e(TAG, "Removing package %s...", ps.packageName);
                    uninstallPackageFully(ps, true);
                }
            }
            result.finish();
        }
    };

    private final static String PKG_WE_WORK = "com.tencent.wework";


    public static VAppManagerService get() {
        return sService.get();
    }

    public static void systemReady() {
        VEnvironment.systemReady();
        if (!BuildCompat.isPie()) {
            get().extractRequiredFrameworks();
        }
        get().startup();
    }

    private void startup() {
        mVisibleOutsidePackages.add("com.android.providers.downloads");
        mUidSystem.initUidList();
        IntentFilter filter = new IntentFilter();
        filter.addAction(Intent.ACTION_PACKAGE_REPLACED);
        filter.addAction(Intent.ACTION_PACKAGE_REMOVED);
        filter.addDataScheme("package");
        VirtualCore.get().getContext().registerReceiver(appEventReciever, filter);
    }

    public boolean isBooting() {
        return mBooting;
    }

    private void extractRequiredFrameworks() {
        for (String framework : StubManifest.REQUIRED_FRAMEWORK) {
            File zipFile = VEnvironment.getFrameworkFile32(framework);
            File odexFile = VEnvironment.getOptimizedFrameworkFile32(framework);
            if (!odexFile.exists()) {
                OatHelper.extractFrameworkFor32Bit(framework, zipFile, odexFile);
            }
        }
    }


    @Override
    public void scanApps() {
        if (mBooting) {
            return;
        }
        synchronized (this) {
            mBooting = true;
            mPersistenceLayer.read();
            if (mPersistenceLayer.changed) {
                mPersistenceLayer.changed = false;
                DebugKook.d(" scanApps ");
                mPersistenceLayer.save();
                VLog.w(TAG, "Package PersistenceLayer updated.");
            }

            /**
             * 作者：kook
             * 时间：21-1-18:下午9:05
             * 邮箱： 329716228@qq.com
             * 说明： 这里应该是获取预装应用
             *******begin start*********/
            for (String preInstallPkg : SpecialComponentList.getPreInstallPackages()) {
                DebugKook.d("----------- VAppManagerService scanApps  preInstallPkg --------------   "+preInstallPkg);
                if (!isAppInstalled(preInstallPkg)) {
                    try {
                        ApplicationInfo outInfo = VirtualCore.get().getUnHookPackageManager().getApplicationInfo(preInstallPkg, 0);
                        InstallOptions options = InstallOptions.makeOptions(true, false, InstallOptions.UpdateStrategy.FORCE_UPDATE);
                        installPackageImpl(outInfo.publicSourceDir, options);
                    } catch (PackageManager.NameNotFoundException e) {
                        // ignore
                    }
                }
            }
            PrivilegeAppOptimizer.get().performOptimizeAllApps();
            mBooting = false;
        }
    }

    private void cleanUpResidualFiles(PackageSetting ps) {
        VLog.e(TAG, "cleanup residual files for : %s", ps.packageName);
        DebugKook.e("=================== cleanup residual files for : "+ ps.packageName+"");
        uninstallPackageFully(ps, false);
    }


    public void onUserCreated(VUserInfo userInfo) {
        VEnvironment.getUserDataDirectory(userInfo.id).mkdirs();
    }


    synchronized boolean loadPackage(PackageSetting setting) {
        if (!loadPackageInnerLocked(setting)) {
            cleanUpResidualFiles(setting);
            return false;
        }
        return true;
    }

    private boolean loadPackageInnerLocked(PackageSetting ps) {
        boolean modeUseOutsideApk = ps.appMode == InstalledAppInfo.MODE_APP_USE_OUTSIDE_APK;
        if (modeUseOutsideApk) {
            if (!VirtualCore.get().isOutsideInstalled(ps.packageName)) {
                return false;
            }
        }
        File cacheFile = VEnvironment.getPackageCacheFile(ps.packageName);
        VPackage pkg = null;
        try {
            pkg = PackageParserEx.readPackageCache(ps.packageName);
        } catch (Throwable e) {
            e.printStackTrace();
        }
        if (pkg == null || pkg.packageName == null) {
            return false;
        }
        VEnvironment.chmodPackageDictionary(cacheFile);
        PackageCacheManager.put(pkg, ps);
        if (modeUseOutsideApk) {
            try {
                PackageInfo outInfo = VirtualCore.get().getUnHookPackageManager().getPackageInfo(ps.packageName, 0);
                if (pkg.mVersionCode != outInfo.versionCode) {
                    VLog.d(TAG, "app (" + ps.packageName + ") has changed version, update it.");
                    InstallOptions options = InstallOptions.makeOptions(true, false, InstallOptions.UpdateStrategy.FORCE_UPDATE);
                    installPackageImpl(outInfo.applicationInfo.publicSourceDir, options);
                }
            } catch (PackageManager.NameNotFoundException e) {
                e.printStackTrace();
                return false;
            }

        }
        return true;
    }

    @Override
    public boolean isOutsidePackageVisible(String pkg) {
        return pkg != null && mVisibleOutsidePackages.contains(pkg);
    }

    @Override
    public int getUidForSharedUser(String sharedUserName) {
        if (sharedUserName == null) {
            return -1;
        }
        return mUidSystem.getUid(sharedUserName);
    }

    @Override
    public void addVisibleOutsidePackage(String pkg) {
        if (pkg != null) {
            mVisibleOutsidePackages.add(pkg);
        }
    }

    @Override
    public void removeVisibleOutsidePackage(String pkg) {
        if (pkg != null) {
            mVisibleOutsidePackages.remove(pkg);
        }
    }

    @Override
    public void installPackage(String path, InstallOptions options, ResultReceiver receiver) {
        InstallResult res;
        synchronized (this) {
            res = installPackageImpl(path, options);
        }
        if (receiver != null) {
            android.os.Bundle data = new Bundle();
            data.putParcelable("result", res);
            receiver.send(0, data);
        }
    }

    @Override
    public void requestCopyPackage64(String packageName) {
        /* Lock VAMS avoid two process invoke this method Simultaneously. */
        synchronized (VActivityManagerService.get()) {
            PackageSetting ps = PackageCacheManager.getSetting(packageName);
            if (ps != null && ps.appMode == MODE_APP_USE_OUTSIDE_APK) {
                V64BitHelper.copyPackage64(ps.getApkPath(false), packageName);
            }
        }
    }

    public InstallResult installPackage(String path, InstallOptions options) {
        synchronized (this) {
            return installPackageImpl(path, options);
        }
    }

    private InstallResult installPackageImpl(String path, InstallOptions options) {
        long installTime = System.currentTimeMillis();
        if (path == null) {
            return InstallResult.makeFailure(VirtualCore.get().getContext().getString(R.string.please_provide_apk_file_path));
        }
        File packageFile = new File(path);
        if (!packageFile.exists() || !packageFile.isFile()) {
            return InstallResult.makeFailure(VirtualCore.get().getContext().getString(R.string.apk_file_does_not_exists_please_provide_valid_file_path));
        }

        VPackage pkg = null;// package info parsed form apk file
        try {
            pkg = PackageParserEx.parsePackage(packageFile);
        } catch (Throwable e) {
            e.printStackTrace();
        }
        if (pkg == null || pkg.packageName == null) {
            return InstallResult.makeFailure(VirtualCore.get().getContext().getString(R.string.broken_apk_file_please_select_another_apk_file));
        }
        InstallResult res = new InstallResult();
        res.packageName = pkg.packageName;

        // PackageCache holds all packages, try to check if we need to update.
        VPackage existOne = PackageCacheManager.get(pkg.packageName);
        if (existOne != null) {
            if (options.updateStrategy == InstallOptions.UpdateStrategy.IGNORE_NEW_VERSION) {
                res.isUpdate = true;
                return res;
            }
            if (!isAllowedUpdate(existOne, pkg, options.updateStrategy)) {
                return InstallResult.makeFailure(VirtualCore.get().getContext().getString(R.string.not_allowed_to_update_package));
            }
            res.isUpdate = true;
            VActivityManagerService.get().killAppByPkg(res.packageName, VUserHandle.USER_ALL);
            PackageCacheManager.remove(pkg.packageName);
        }

        boolean support64bit = false, support32bit = false;
        boolean shouldLinkInstalledSo = false;
        NativeLibraryHelperCompat.SoLib installedSo = null;
        Set<String> abiList = NativeLibraryHelperCompat.getSupportAbiList(packageFile.getPath());
        if (abiList.isEmpty()) {
            if (options.useSourceLocationApk) {
                //check /data/data
                try {
                    NativeLibraryHelperCompat.SoLib soLib = NativeLibraryHelperCompat.getInstalledSo(VirtualCore.getPM().getApplicationInfo(pkg.packageName, 0), VirtualCore.get().is64BitEngineInstalled() && V64BitHelper.has64BitEngineStartPermission());
                    if (soLib != null) {
                        support32bit = !soLib.is64Bit();
                        support64bit = !support32bit;
                        installedSo = soLib;
                        shouldLinkInstalledSo = true;
                    }
                } catch (Throwable e) {
                    VLog.e(TAG, "get Installed so error!", e);
                }
            }
            if (!shouldLinkInstalledSo) {
                support64bit = true;
                support32bit = true;
            }
        } else {
            if (NativeLibraryHelperCompat.support64bitAbi(abiList)) {
                support64bit = true;
            }
            if (NativeLibraryHelperCompat.contain32bitAbi(abiList)) {
                support32bit = true;
            }
        }

        PackageSetting ps = existOne != null ? (PackageSetting) existOne.mExtras : new PackageSetting();

        if (support32bit) {
            if (support64bit) {
                ps.flag = PackageSetting.FLAG_RUN_BOTH_32BIT_64BIT;
            } else {
                ps.flag = PackageSetting.FLAG_RUN_32BIT;
            }
        } else {
            ps.flag = PackageSetting.FLAG_RUN_64BIT;
        }

        if (ps.isRunOn64BitProcess()) {
            if (!VirtualCore.get().is64BitEngineInstalled() || !V64BitHelper.has64BitEngineStartPermission()) {
                return InstallResult.makeFailure(VirtualCore.get().getContext().getString(R.string.virtual_core_x64_engine_not_installed));
            }
        }

        if (shouldLinkInstalledSo) {
            NativeLibraryHelperCompat.linkInstalledSo(installedSo.path, ps.isRunOn64BitProcess()
                    ? VEnvironment.getAppLibDirectory64(pkg.packageName).getAbsolutePath()
                    : VEnvironment.getAppLibDirectory(pkg.packageName).getAbsolutePath());
        } else {
            NativeLibraryHelperCompat.copyNativeBinaries(packageFile, VEnvironment.getAppLibDirectory(pkg.packageName));
        }

        boolean useSourceLocationApk = options.useSourceLocationApk;
        if (!useSourceLocationApk) {
            File privatePackageFile = VEnvironment.getPackageResourcePath(pkg.packageName);
            try {
                FileUtils.copyFile(packageFile, privatePackageFile);
            } catch (IOException e) {
                //noinspection ResultOfMethodCallIgnored
                privatePackageFile.delete();
                return InstallResult.makeFailure(VirtualCore.get().getContext().getString(R.string.unable_to_copy_package_file));
            }
            packageFile = privatePackageFile;
            VEnvironment.chmodPackageDictionary(packageFile);
        }

        if (support64bit && !useSourceLocationApk) {
            V64BitHelper.copyPackage64(packageFile.getPath(), pkg.packageName);
            PackageManager pm = VirtualCore.getPM();
            PackageInfo pkgInfo = pm.getPackageArchiveInfo(path, PackageManager.GET_ACTIVITIES);
            ApplicationInfo appInfo = pkgInfo.applicationInfo;
            appInfo.sourceDir = path;
            appInfo.publicSourceDir = path;
            String name = appInfo.loadLabel(VirtualCore.getPM()).toString();
            Drawable icon = appInfo.loadIcon(VirtualCore.getPM());
            InstalledInfoCache.save(new InstalledInfoCache.CacheItem(pkg.packageName, name, icon));
        }

        ps.appMode = useSourceLocationApk ? MODE_APP_USE_OUTSIDE_APK : MODE_APP_COPY_APK;

        ps.dependSystem = false;
        ps.apkPath = packageFile.getPath();
        File appDir = VEnvironment.getDataAppPackageDirectory(pkg.packageName);
        File libDir = new File(appDir, "lib");
        ps.libPath = libDir.getPath();
        ps.packageName = pkg.packageName;
        ps.appId = VUserHandle.getAppId(mUidSystem.getOrCreateUid(pkg));
        if (res.isUpdate) {
            ps.lastUpdateTime = installTime;
        } else {
            ps.firstInstallTime = installTime;
            ps.lastUpdateTime = installTime;
            for (int userId : VUserManagerService.get().getUserIds()) {
                boolean installed = userId == 0;
                ps.setUserState(userId, false/*launched*/, false/*hidden*/, installed);
            }
        }
        PackageParserEx.savePackageCache(pkg);
        PackageCacheManager.put(pkg, ps);

        DebugKook.d(" installPackageImpl "+path);
        DebugKook.printInfo();
        mPersistenceLayer.save();
        if (support32bit && !useSourceLocationApk) {
            try {
                DexOptimizer.optimizeDex(packageFile.getPath(), VEnvironment.getOdexFile(ps.packageName).getPath());
            } catch (IOException e) {
                e.printStackTrace();
            }
        }
        if (options.notify) {
            notifyAppInstalled(ps, -1);
        }
        res.isSuccess = true;
        return res;
    }

    /**
     * Install virtual app as an existing virtual user
     *
     * @param userId      the existing virtual user's id
     * @param packageName the installed virtual app's application id
     * @return true: install success or already installed as specified user.<br/>
     * false: virtual app not installed yet or virtual user not exists
     * @see #installPackage(String, InstallOptions, ResultReceiver)
     * @see VUserManager#createUser(String, int)
     * @see VUserManagerService#exists(int)
     */
    @Override
    public synchronized boolean installPackageAsUser(int userId, String packageName) {
        if (VUserManagerService.get().exists(userId)) {
            PackageSetting ps = PackageCacheManager.getSetting(packageName);
            if (ps != null) {
                if (!ps.isInstalled(userId)) {
                    ps.setInstalled(userId, true);
                    notifyAppInstalled(ps, userId);
                    DebugKook.d(" installPackageAsUser  ");
                    mPersistenceLayer.save();
                    return true;
                }// else virtual app already installed as specified virtual user
                return true;
            }// virtual app not installed yet
        }// virtual user not exists
        return false;
    }

    private boolean isAllowedUpdate(VPackage existOne, VPackage newOne, InstallOptions.UpdateStrategy strategy) {
        switch (strategy) {
            case FORCE_UPDATE:
                return true;
            case COMPARE_VERSION:
                return existOne.mVersionCode < newOne.mVersionCode;
            case TERMINATE_IF_EXIST:
                return false;
        }
        return true;
    }


    @Override
    public synchronized boolean uninstallPackage(String packageName) {
        DebugKook.d(" uninstallPackage :"+packageName+"     ");
        PackageSetting ps = PackageCacheManager.getSetting(packageName);
        if (ps != null) {
            uninstallPackageFully(ps, true);
            return true;
        }
        return false;
    }

    @Override
    public synchronized boolean uninstallPackageAsUser(String packageName, int userId) {
        DebugKook.d("卸载 应用:"+packageName+"     "+userId);
        if (!VUserManagerService.get().exists(userId)) {
            return false;
        }
        PackageSetting ps = PackageCacheManager.getSetting(packageName);
        if (ps != null) {
            int[] userIds = getPackageInstalledUsers(packageName);
            if (!ArrayUtils.contains(userIds, userId)) {
                return false;
            }
            if (userIds.length == 1) {
                uninstallPackageFully(ps, true);
            } else {
                // Just hidden it
                VActivityManagerService.get().killAppByPkg(packageName, userId);
                ps.setInstalled(userId, false);

                DebugKook.d(" uninstallPackageAsUser  ");
                mPersistenceLayer.save();
                deletePackageDataAsUser(userId, ps);
                notifyAppUninstalled(ps, userId);
            }
            return true;
        }
        return false;
    }

    private boolean isPackageSupport32Bit(PackageSetting ps) {
        return ps.flag == PackageSetting.FLAG_RUN_32BIT
                || ps.flag == PackageSetting.FLAG_RUN_BOTH_32BIT_64BIT;
    }

    private boolean isPackageSupport64Bit(PackageSetting ps) {
        return ps.flag == PackageSetting.FLAG_RUN_64BIT
                || ps.flag == PackageSetting.FLAG_RUN_BOTH_32BIT_64BIT;
    }

    private void deletePackageDataAsUser(int userId, PackageSetting ps) {
        if (isPackageSupport32Bit(ps)) {
            if (userId == -1) {
                List<VUserInfo> userInfos = VUserManager.get().getUsers();
                if (userInfos != null) {
                    for (VUserInfo info : userInfos) {
                        FileUtils.deleteDir(VEnvironment.getDataUserPackageDirectory(info.id, ps.packageName));
                    }
                }
            } else {
                FileUtils.deleteDir(VEnvironment.getDataUserPackageDirectory(userId, ps.packageName));
            }
        }
        if (isPackageSupport64Bit(ps)) {
            V64BitHelper.cleanPackageData64(userId, ps.packageName);
        }
        VNotificationManagerService.get().cancelAllNotification(ps.packageName, userId);
        ComponentStateManager.user(userId).clear(ps.packageName);
    }

    public boolean cleanPackageData(String pkg, int userId) {
        PackageSetting ps = PackageCacheManager.getSetting(pkg);
        if (ps == null) {
            return false;
        }
        VActivityManagerService.get().killAppByPkg(pkg, userId);
        deletePackageDataAsUser(userId, ps);
        //clear component state
        ComponentStateManager.user(userId).clear(pkg);
        return true;
    }

    private void uninstallPackageFully(PackageSetting ps, boolean notify) {
        DebugKook.d("卸载应用 "+ps.packageName+"    notify:"+notify);
        String packageName = ps.packageName;
        try {
            VActivityManagerService.get().killAppByPkg(packageName, VUserHandle.USER_ALL);
            if (isPackageSupport32Bit(ps)) {
                VEnvironment.getPackageResourcePath(packageName).delete();
                FileUtils.deleteDir(VEnvironment.getDataAppPackageDirectory(packageName));
                VEnvironment.getOdexFile(packageName).delete();
                for (int id : VUserManagerService.get().getUserIds()) {
                    deletePackageDataAsUser(id, ps);
                }
            }
            if (isPackageSupport64Bit(ps)) {
                V64BitHelper.uninstallPackage64(-1, packageName);
            }
            PackageCacheManager.remove(packageName);
            File cacheFile = VEnvironment.getPackageCacheFile(packageName);
            cacheFile.delete();
            File signatureFile = VEnvironment.getSignatureFile(packageName);
            signatureFile.delete();
        } catch (Exception e) {
            e.printStackTrace();
        } finally {
            if (notify) {
                notifyAppUninstalled(ps, -1);
            }
        }
    }

    @Override
    public int[] getPackageInstalledUsers(String packageName) {
        PackageSetting ps = PackageCacheManager.getSetting(packageName);
        if (ps != null) {
            IntArray installedUsers = new IntArray(5);
            int[] userIds = VUserManagerService.get().getUserIds();
            for (int userId : userIds) {
                if (ps.readUserState(userId).installed) {
                    installedUsers.add(userId);
                }
            }
            return installedUsers.getAll();
        }
        return new int[0];
    }

    /**
     * 获取安装的应用列表
     * @param flags
     * @return
     */
    @Override
    public List<InstalledAppInfo> getInstalledApps(int flags) {
        List<InstalledAppInfo> infoList = new ArrayList<>(getInstalledAppCount());
        //DebugKook.d("PackageCacheManager.PACKAGE_CACHE :"+PackageCacheManager.PACKAGE_CACHE.size());
        /*if (flags == InstalledAppInfo.MODE_APP_COPY_APK||flags == MODE_APP_USE_OUTSIDE_APK) {
            for (VPackage p : PackageCacheManager.PACKAGE_CACHE.values()) {
                PackageSetting setting = (PackageSetting) p.mExtras;
                infoList.add(setting.getAppInfo());
            }
        } else {
            boolean filterXposedModules = (flags & InstalledAppInfo.FLAG_XPOSED_MODULE) != 0;
            boolean excludeXposedModules = (flags & InstalledAppInfo.FLAG_EXCLUDE_XPOSED_MODULE) != 0;
            boolean enabledXposedModules = (flags & InstalledAppInfo.FLAG_ENABLED_XPOSED_MODULE) != 0;
            for (VPackage p : PackageCacheManager.PACKAGE_CACHE.values()) {
                if (excludeXposedModules) {
                    if (p.xposedModule == null) {
                        PackageSetting setting = (PackageSetting) p.mExtras;
                        infoList.add(setting.getAppInfo());
                    }
                } else if (!filterXposedModules || p.xposedModule != null) {
                    if (!enabledXposedModules || (XposedModuleProfile.isXposedEnable() && XposedModuleProfile.isModuleEnable(p.packageName))) {
                        PackageSetting setting = (PackageSetting) p.mExtras;
                        infoList.add(setting.getAppInfo());
                    }
                }
            }
        }*/
        for (VPackage p : PackageCacheManager.PACKAGE_CACHE.values()) {
            PackageSetting setting = (PackageSetting) p.mExtras;
            infoList.add(setting.getAppInfo());
        }
        return infoList;
    }

    @Override
    public List<InstalledAppInfo> getInstalledAppsAsUser(int userId, int flags) {
        List<InstalledAppInfo> infoList = new ArrayList<>(getInstalledAppCount());
        for (VPackage p : PackageCacheManager.PACKAGE_CACHE.values()) {
            PackageSetting setting = (PackageSetting) p.mExtras;
            boolean visible = setting.isInstalled(userId);
            if ((flags & VirtualCore.GET_HIDDEN_APP) == 0 && setting.isHidden(userId)) {
                visible = false;
            }
            if (visible) {
                infoList.add(setting.getAppInfo());
            }
        }
        return infoList;
    }

    @Override
    public int getInstalledAppCount() {
        return PackageCacheManager.PACKAGE_CACHE.size();
    }

    @Override
    public boolean isAppInstalled(String packageName) {
        return packageName != null && PackageCacheManager.PACKAGE_CACHE.containsKey(packageName);
    }

    @Override
    public boolean isAppInstalledAsUser(int userId, String packageName) {
        if (packageName == null || !VUserManagerService.get().exists(userId)) {
            return false;
        }
        PackageSetting setting = PackageCacheManager.getSetting(packageName);
        if (setting == null) {
            return false;
        }
        return setting.isInstalled(userId);
    }

    private void notifyAppInstalled(PackageSetting setting, int userId) {
        final String pkg = setting.packageName;
        int N = mRemoteCallbackList.beginBroadcast();
        while (N-- > 0) {
            try {
                if (userId == -1) {
                    mRemoteCallbackList.getBroadcastItem(N).onPackageInstalled(pkg);
                    mRemoteCallbackList.getBroadcastItem(N).onPackageInstalledAsUser(0, pkg);

                } else {
                    mRemoteCallbackList.getBroadcastItem(N).onPackageInstalledAsUser(userId, pkg);
                }
            } catch (RemoteException e) {
                e.printStackTrace();
            }
        }
        sendInstalledBroadcast(pkg, new VUserHandle(userId));
        mRemoteCallbackList.finishBroadcast();
        VAccountManagerService.get().refreshAuthenticatorCache(null);
    }

    private void notifyAppUninstalled(PackageSetting setting, int userId) {
        DebugKook.d("卸载  notifyAppUninstalled 应用:"+setting.packageName+"     "+userId);
        final String pkg = setting.packageName;
        int N = mRemoteCallbackList.beginBroadcast();
        while (N-- > 0) {
            try {
                if (userId == -1) {
                    mRemoteCallbackList.getBroadcastItem(N).onPackageUninstalled(pkg);
                    mRemoteCallbackList.getBroadcastItem(N).onPackageUninstalledAsUser(0, pkg);
                } else {
                    mRemoteCallbackList.getBroadcastItem(N).onPackageUninstalledAsUser(userId, pkg);
                }
            } catch (RemoteException e) {
                e.printStackTrace();
            }
        }
        sendUninstalledBroadcast(pkg, new VUserHandle(userId));
        mRemoteCallbackList.finishBroadcast();
        VAccountManagerService.get().refreshAuthenticatorCache(null);
    }


    private void sendInstalledBroadcast(String packageName, VUserHandle user) {
        Intent intent = new Intent(Intent.ACTION_PACKAGE_ADDED);
        intent.setData(Uri.parse("package:" + packageName));
        VActivityManagerService.get().sendBroadcastAsUser(intent, user);
    }

    private void sendUninstalledBroadcast(String packageName, VUserHandle user) {
        DebugKook.d(" sendUninstalledBroadcast :"+packageName+"     ");

        Intent intent = new Intent(Intent.ACTION_PACKAGE_REMOVED);
        intent.setData(Uri.parse("package:" + packageName));
        VActivityManagerService.get().sendBroadcastAsUser(intent, user);
    }

    @Override
    public void registerObserver(IPackageObserver observer) {
        try {
            mRemoteCallbackList.register(observer);
        } catch (Throwable e) {
            e.printStackTrace();
        }
    }

    @Override
    public void unregisterObserver(IPackageObserver observer) {
        try {
            mRemoteCallbackList.unregister(observer);
        } catch (Throwable e) {
            e.printStackTrace();
        }
    }

    @Override
    public InstalledAppInfo getInstalledAppInfo(String packageName, int flags) {
        synchronized (PackageCacheManager.class) {
            if (packageName != null) {
                PackageSetting setting = PackageCacheManager.getSetting(packageName);
                if (setting != null) {
                    return setting.getAppInfo();
                }
            }
            return null;
        }
    }

    @Override
    public boolean isRun64BitProcess(String packageName) {
        PackageSetting ps = PackageCacheManager.getSetting(packageName);
        return ps != null && ps.isRunOn64BitProcess();
    }

    @Override
    public synchronized boolean isIORelocateWork() {
        return true;
    }

    public boolean isPackageLaunched(int userId, String packageName) {
        PackageSetting ps = PackageCacheManager.getSetting(packageName);
        return ps != null && ps.isLaunched(userId);
    }

    public void setPackageHidden(int userId, String packageName, boolean hidden) {
        PackageSetting ps = PackageCacheManager.getSetting(packageName);
        if (ps != null && VUserManagerService.get().exists(userId)) {
            ps.setHidden(userId, hidden);
            DebugKook.d(" setPackageHidden ");
            mPersistenceLayer.save();
        }
    }

    public int getAppId(String packageName) {
        PackageSetting setting = PackageCacheManager.getSetting(packageName);
        return setting != null ? setting.appId : -1;
    }

    void restoreFactoryState() {
        VLog.w(TAG, "Warning: Restore the factory state...");
        VEnvironment.getDalvikCacheDirectory().delete();
        VEnvironment.getUserSystemDirectory().delete();
        VEnvironment.getUserDeSystemDirectory().delete();
        VEnvironment.getDataAppDirectory().delete();
    }

    public void savePersistenceData() {
        mPersistenceLayer.save();
    }

    public boolean is64BitUid(int uid) throws PackageManager.NameNotFoundException {
        int appId = VUserHandle.getAppId(uid);
        synchronized (PackageCacheManager.PACKAGE_CACHE) {
            for (VPackage p : PackageCacheManager.PACKAGE_CACHE.values()) {
                PackageSetting ps = (PackageSetting) p.mExtras;
                if (ps.appId == appId) {
                    return ps.isRunOn64BitProcess();
                }
            }
        }
        throw new PackageManager.NameNotFoundException();
    }
}
