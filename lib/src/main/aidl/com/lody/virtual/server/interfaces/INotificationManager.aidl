package com.lody.virtual.server.interfaces;

import android.os.RemoteException;

/**
 * @author Lody
 */
interface INotificationManager {

    int dealNotificationId(int id, String packageName, String tag, int userId);

    String dealNotificationTag(int id, String packageName, String tag, int userId);

    boolean areNotificationsEnabledForPackage(String packageName, int userId);

    void setNotificationsEnabledForPackage(String packageName, boolean enable, int userId);

    void addNotification(int id, String tag, String packageName, int userId);

    void cancelAllNotification(String packageName, int userId);
}