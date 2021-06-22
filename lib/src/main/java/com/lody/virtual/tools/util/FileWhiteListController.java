package com.lody.virtual.tools.util;

import android.os.Environment;


import com.lody.virtual.helper.compat.BuildCompat;

import java.io.File;
import java.util.Collection;
import java.util.LinkedList;
import java.util.List;

public class FileWhiteListController {
    private static List<String> pendingList = null;

    public static void addToList(String szPath) {
        if (pendingList == null) {
            pendingList = new LinkedList<>();
        }
        pendingList.add(szPath);
    }

    public static void addAllToList(Collection<? extends String> cPath) {
        if (pendingList == null) {
            pendingList = new LinkedList<>();
        }
        pendingList.addAll(cPath);
    }

    public static void removeFromList(String szPath) {
        if (pendingList != null) {
            pendingList.remove(szPath);
        }
    }

    private static List<String> getWhListQ() {
        LinkedList<String> mountPoints = new LinkedList<>();
        mountPoints.add(Environment.DIRECTORY_PODCASTS + File.separatorChar);
        mountPoints.add(Environment.DIRECTORY_PODCASTS + File.separatorChar);
        mountPoints.add(Environment.DIRECTORY_RINGTONES + File.separatorChar);
        mountPoints.add(Environment.DIRECTORY_ALARMS + File.separatorChar);
        mountPoints.add(Environment.DIRECTORY_NOTIFICATIONS + File.separatorChar);
        mountPoints.add(Environment.DIRECTORY_PICTURES + File.separatorChar);
        mountPoints.add(Environment.DIRECTORY_MOVIES + File.separatorChar);
        mountPoints.add(Environment.DIRECTORY_DOWNLOADS + File.separatorChar);
        mountPoints.add(Environment.DIRECTORY_DCIM + File.separatorChar);
        if (pendingList != null) {
            mountPoints.addAll(pendingList);
            pendingList = null;
        }
        return mountPoints;
    }

    private static List<String> getWhList() {
        LinkedList<String> mountPoints = new LinkedList<>();
        mountPoints.add("DCIM/");
        mountPoints.add("Pictures/");
        mountPoints.add("Movies/");
        mountPoints.add("tencent/QQ_Images/");
        mountPoints.add("Tencent/QQ_Images/");
        mountPoints.add("tencent/QQfile_recv/");
        mountPoints.add("Tencent/QQfile_recv/");
        mountPoints.add("tencent/MicroMsg/WeiXin/");
        mountPoints.add("Tencent/MicroMsg/WeiXin/");
        mountPoints.add("Quark/Download/");
        mountPoints.add("Download/");
        mountPoints.add("UCDownloads/");
        mountPoints.add("QQBrowser/");
        mountPoints.add("tieba/");
        mountPoints.add("AppProjects/");
        mountPoints.add("Music/");
        mountPoints.add("netease/");
        mountPoints.add("ksweb/");
        if (pendingList != null) {
            mountPoints.addAll(pendingList);
            pendingList = null;
        }
        return mountPoints;
    }

    public static List<String> getAllWhiteList() {
        if (BuildCompat.isQ()) return getWhListQ();
        return getWhList();
    }
}
