package com.lody.virtual.helper.compat;

import android.annotation.TargetApi;
import android.content.pm.ApplicationInfo;
import android.os.Build;
import android.text.TextUtils;
import android.util.Log;

import com.lody.virtual.client.env.VirtualRuntime;
import com.lody.virtual.helper.utils.ArrayUtils;
import com.lody.virtual.helper.utils.Reflect;
import com.lody.virtual.helper.utils.VLog;

import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;
import java.util.Collections;
import java.util.Enumeration;
import java.util.HashSet;
import java.util.Set;
import java.util.zip.ZipEntry;
import java.util.zip.ZipFile;

import mirror.com.android.internal.content.NativeLibraryHelper;

public class NativeLibraryHelperCompat {

    private static String TAG = NativeLibraryHelperCompat.class.getSimpleName();

    public static int copyNativeBinaries(File apkFile, File sharedLibraryDir) {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP) {
            return copyNativeBinariesAfterL(apkFile, sharedLibraryDir);
        } else {
            return copyNativeBinariesBeforeL(apkFile, sharedLibraryDir);
        }
    }

    private static int copyNativeBinariesBeforeL(File apkFile, File sharedLibraryDir) {
        try {
            return Reflect.on(NativeLibraryHelper.TYPE).call("copyNativeBinariesIfNeededLI", apkFile, sharedLibraryDir)
                    .get();
        } catch (Throwable e) {
            e.printStackTrace();
        }
        return -1;
    }

    @TargetApi(Build.VERSION_CODES.LOLLIPOP)
    private static int copyNativeBinariesAfterL(File apkFile, File sharedLibraryDir) {
        try {
            Object handle = NativeLibraryHelper.Handle.create.call(apkFile);
            if (handle == null) {
                return -1;
            }

            String abi = null;
            Set<String> abiSet = getSupportAbiList(apkFile.getAbsolutePath());
            if (abiSet == null || abiSet.isEmpty()) {
                return 0;
            }
            boolean is64Bit = VirtualRuntime.is64bit();
            if (is64Bit && support64bitAbi(abiSet)) {
                if (Build.SUPPORTED_64_BIT_ABIS.length > 0) {
                    int abiIndex = NativeLibraryHelper.findSupportedAbi.call(handle, Build.SUPPORTED_64_BIT_ABIS);
                    if (abiIndex >= 0) {
                        abi = Build.SUPPORTED_64_BIT_ABIS[abiIndex];
                    }
                }
            } else {
                if (Build.SUPPORTED_32_BIT_ABIS.length > 0) {
                    int abiIndex = NativeLibraryHelper.findSupportedAbi.call(handle, Build.SUPPORTED_32_BIT_ABIS);
                    if (abiIndex >= 0) {
                        abi = Build.SUPPORTED_32_BIT_ABIS[abiIndex];
                    }
                }
            }
            if (abi == null) {
                VLog.e(TAG, "Not match any abi [%s].", apkFile.getAbsolutePath());
                return -1;
            }
            return NativeLibraryHelper.copyNativeBinaries.call(handle, sharedLibraryDir, abi);
        } catch (Throwable e) {
            VLog.d(TAG, "copyNativeBinaries with error : %s", e.getLocalizedMessage());
            e.printStackTrace();
        }

        return -1;
    }

    // check has Installed So
    public static SoLib getInstalledSo(ApplicationInfo applicationInfo, boolean has64Support) {
        if (applicationInfo == null)
            return null;
        File nativeLibraryDir = new File(applicationInfo.nativeLibraryDir);
        File[] soFiles = nativeLibraryDir.listFiles();
        //64nit env
        if (has64Support) {
            File dir64Bits = new File(applicationInfo.nativeLibraryDir + "64");
            //use 64bit so
            if (dir64Bits.exists()) {
                nativeLibraryDir = dir64Bits;
                soFiles = dir64Bits.listFiles();
            }
            if (!ArrayUtils.isEmpty(soFiles)) {
                String abi = getAbiFromElf(soFiles[0]);
                if (abi != null) {
                    return new SoLib(abi, nativeLibraryDir.getAbsolutePath());
                }
            }
        // 32bit env
        } else if (!ArrayUtils.isEmpty(soFiles)) {
            String abi = getAbiFromElf(soFiles[0]);
            if ((applicationInfo.flags & ApplicationInfo.FLAG_SYSTEM) == 0 && is32bitAbi(abi)) {
                return new SoLib(abi, nativeLibraryDir.getAbsolutePath());
            }
        }
        return null;
    }

    public static void linkInstalledSo(String installedSoPath, String vaSoPath) {
        File dirInstalledSo = new File(installedSoPath);
        File dirVaSo = new File(vaSoPath);

        if (!dirVaSo.exists())
            dirVaSo.mkdirs();

        File[] sos = dirInstalledSo.listFiles();

        if (!ArrayUtils.isEmpty(sos)) {
            for (File so:sos) {
                try {
                    Runtime.getRuntime().exec(String.format("ln -s %s %s", so.getAbsoluteFile(), new File(dirVaSo, so.getName()).getAbsolutePath())).waitFor();
                } catch (Exception e) {
                    e.printStackTrace();
                }
            }
        }
    }

    public static String getAbiFromElf(File elfFile) {
        byte[] header = new byte[20];
        FileInputStream fileInputStream = null;
        try {
            fileInputStream = new FileInputStream(elfFile);
            if (fileInputStream.read(header) == header.length) {
                switch (header[5] == 1 ? header[0x12] : header[0x13]) {
                    case 3:
                        return "x86";
                    case 0x28:
                        return "armeabi-v7a";
                    case -73:
                        return "arm64-v8a";
                    case 62:
                        return "x86_64";
                }
                return null;
            }
            return null;
        } catch (Exception e) {
            Log.e(TAG, "getAbiFromElf error", e);
            return null;
        } finally {
            if (fileInputStream != null) {
                try {
                    fileInputStream.close();
                } catch (IOException e) {
                    Log.e(TAG, "getAbiFromElf close error", e);
                }
            }
        }
    }

    @TargetApi(Build.VERSION_CODES.LOLLIPOP)
    public static boolean is64bitAbi(String abi) {
        return "arm64-v8a".equals(abi)
                || "x86_64".equals(abi)
                || "mips64".equals(abi);
    }

    public static boolean is32bitAbi(String abi) {
        return "armeabi".equals(abi)
                || "armeabi-v7a".equals(abi)
                || "mips".equals(abi)
                || "x86".equals(abi);
    }

    @TargetApi(Build.VERSION_CODES.LOLLIPOP)
    public static boolean contain64bitAbi(Set<String> supportedABIs) {
        for (String supportedAbi : supportedABIs) {
            if (is64bitAbi(supportedAbi)) {
                return true;
            }
        }
        return false;
    }

    @TargetApi(Build.VERSION_CODES.LOLLIPOP)
    public static boolean support64bitAbi(Set<String> supportedABIs) {
        String[] cpuABIs = Build.SUPPORTED_64_BIT_ABIS;
        if (ArrayUtils.isEmpty(cpuABIs) || supportedABIs == null || supportedABIs.isEmpty())
            return false;
        for (String cpuABI : cpuABIs) {
            for (String supportedABI:supportedABIs) {
                if (TextUtils.equals(cpuABI, supportedABI))
                    return true;
            }
        }
        return false;
    }

    public static boolean contain32bitAbi(Set<String> abiList) {
        for (String supportedAbi : abiList) {
            if (is32bitAbi(supportedAbi)) {
                return true;
            }
        }
        return false;
    }


    public static Set<String> getSupportAbiList(String apk) {
        try {
            ZipFile apkFile = new ZipFile(apk);
            Enumeration<? extends ZipEntry> entries = apkFile.entries();
            Set<String> supportedABIs = new HashSet<String>();
            while (entries.hasMoreElements()) {
                ZipEntry entry = entries.nextElement();
                String name = entry.getName();
                if (name.contains("../")) {
                    continue;
                }
                if (name.startsWith("lib/") && !entry.isDirectory() && name.endsWith(".so")) {
                    String supportedAbi = name.substring(name.indexOf("/") + 1, name.lastIndexOf("/"));
                    supportedABIs.add(supportedAbi);
                }
            }
            return supportedABIs;
        } catch (Exception e) {
            e.printStackTrace();
        }
        return Collections.emptySet();
    }

    public static class SoLib {

        public String ABI;
        public String path;

        public SoLib() {
        }

        public SoLib(String ABI, String path) {
            this.ABI = ABI;
            this.path = path;
        }

        public boolean is64Bit() {
            return ABI != null && is64bitAbi(ABI);
        }

    }
}
