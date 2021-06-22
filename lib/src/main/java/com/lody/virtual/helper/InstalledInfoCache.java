package com.lody.virtual.helper;

import android.graphics.drawable.Drawable;

import com.lody.virtual.client.core.VirtualCore;
import com.lody.virtual.helper.utils.ACache;

import java.io.Serializable;

/**
 * @author Swift Gan
 */

public class InstalledInfoCache {

    private static ACache diskCache;

    static {
        diskCache = ACache.get(VirtualCore.get().getContext(), "AppInfoCache");
    }

    public static void save(CacheItem cacheItem) {
        diskCache.put(CacheItem.INFO_CACHE_PREFIX + cacheItem.packageName, cacheItem);
        cacheItem.saveIcon();
    }

    public static CacheItem get(String packageName) {
        return (CacheItem) diskCache.getAsObject(CacheItem.INFO_CACHE_PREFIX + packageName);
    }

    @Keep
    public static class CacheItem implements Serializable {

        private static final long serialVersionUID = 1L;

        public final static transient String INFO_CACHE_PREFIX = "va_installed_info_cache@";
        public final static transient String ICON_CACHE_PREFIX = "va_installed_icon_cache@";

        public String packageName;
        public String label;

        public transient Drawable icon;

        public CacheItem(String packageName, String label, Drawable icon) {
            this.packageName = packageName;
            this.label = label;
            this.icon = icon;
        }

        public String getPackageName() {
            return packageName;
        }

        public String getLabel() {
            return label;
        }

        public synchronized void saveIcon() {
            if (icon != null) {
                diskCache.put(ICON_CACHE_PREFIX + packageName, icon);
            }
        }

        public synchronized Drawable getIcon() {
            if (icon == null) {
                icon = diskCache.getAsDrawable(ICON_CACHE_PREFIX + packageName);
            }
            return icon;
        }
    }

}
