package io.virtualapp.utils;

import android.content.Context;
import android.content.SharedPreferences;

import java.util.ArrayList;
import java.util.List;

public class SPTools {
    private static final String FILE_NAME = "sharedPreferences";
    private static final String TAG = "ToolsSharedPreferences";
    private static SharedPreferences.OnSharedPreferenceChangeListener mOnSharedPreferenceChangeListener;
    private static SharedPreferences mSharedPreferences;

    public SPTools() {
    }

    public static void setOnSharedPreferenceChangeListener(SharedPreferences.OnSharedPreferenceChangeListener onSharedPreferenceChangeListener) {
        mOnSharedPreferenceChangeListener = onSharedPreferenceChangeListener;
        SharedPreferences sharedPreferences = mSharedPreferences;
        if (sharedPreferences != null) {
            sharedPreferences.registerOnSharedPreferenceChangeListener(mOnSharedPreferenceChangeListener);
        }
    }

    private static SharedPreferences getSharedPreferences(Context context) {
        if (mSharedPreferences == null) {
            mSharedPreferences = context.getSharedPreferences("sharedPreferences", 0);
        }
        return mSharedPreferences;
    }

    public static int getInt(Context context, String keyname, int def) {
        int v = getSharedPreferences(context).getInt(keyname, def);
        if (v == def) {
            return def;
        }
        return v;
    }

    public static int getInt(Context context, String keyname) {
        return getInt(context, keyname, -1);
    }

    public static String getString(Context context, String keyname, String defValues) {
        String str = getSharedPreferences(context).getString(keyname, (String) null);
        if (str == null) {
            return defValues;
        }
        return str;
    }

    public static String getString(Context context, String keyname) {
        String str = getSharedPreferences(context).getString(keyname, (String) null);
        if (str == null) {
            return null;
        }
        return str;
    }

    public static long getLong(Context context, String keyname) {
        return getSharedPreferences(context).getLong(keyname, -1);
    }

    public static boolean getBoolean(Context context, String keyname) {
        return getSharedPreferences(context).getBoolean(keyname, false);
    }

    public static boolean getBoolean(Context context, String keyname, boolean defaultBoolean) {
        return getSharedPreferences(context).getBoolean(keyname, defaultBoolean);
    }

    public static void putString(Context context, String keyname, String values) {
        SharedPreferences.Editor e = getSharedPreferences(context).edit();
        e.putString(keyname, values);
        boolean commit = e.commit();
    }

    public static void putIntList(Context context, String keyname, List<Integer> values) {
        SharedPreferences.Editor editor = getSharedPreferences(context).edit();
        editor.putInt(keyname, values.size());
        for (int i = 0; i < values.size(); i++) {
            editor.putInt(keyname + "_" + i, values.get(i).intValue());
        }
        boolean i2 = editor.commit();
    }

    public static List<Integer> getIntList(Context context, String keyname) {
        List<Integer> environmentList = new ArrayList<>();
        SharedPreferences shared = getSharedPreferences(context);
        int environNums = shared.getInt(keyname, 0);
        for (int i = 0; i < environNums; i++) {
            if (shared.contains(keyname + "_" + i)) {
                environmentList.add(Integer.valueOf(shared.getInt(keyname + "_" + i, 0)));
            }
        }
        return environmentList;
    }

    public static void putStringList(Context context, String keyname, List<String> values) {
        SharedPreferences.Editor editor = getSharedPreferences(context).edit();
        editor.putInt(keyname, values.size());
        for (int i = 0; i < values.size(); i++) {
            editor.putString(keyname + "_" + i, values.get(i));
        }
        boolean i2 = editor.commit();
    }

    public static List<String> getStringList(Context context, String keyname) {
        List<String> environmentList = new ArrayList<>();
        SharedPreferences shared = getSharedPreferences(context);
        int environNums = shared.getInt(keyname, 0);
        for (int i = 0; i < environNums; i++) {
            if (shared.contains(keyname + "_" + i)) {
                environmentList.add(shared.getString(keyname + "_" + i, (String) null));
            }
        }
        return environmentList;
    }

    public static void removeListAll(Context context, String keyname) {
        SharedPreferences shared = getSharedPreferences(context);
        SharedPreferences.Editor editor = shared.edit();
        int environNums = shared.getInt(keyname, 0);
        for (int i = 0; i < environNums; i++) {
            editor.remove(keyname + "" + i);
        }
        boolean i2 = editor.commit();
    }

    public static void removeListItem(Context context, String keyname, Object value) {
        if ((value instanceof String) || (value instanceof Integer)) {
            try {
                throw new Exception("value 类型必须是 int 或者 String");
            } catch (Exception e) {
                e.printStackTrace();
            }
        }
        SharedPreferences shared = getSharedPreferences(context);
        SharedPreferences.Editor editor = shared.edit();
        int environNums = shared.getInt(keyname, 0);
        for (int i = 0; i < environNums; i++) {
            if (value instanceof Integer) {
                if (shared.getInt(keyname + "_" + i, 0) == ((Integer) value).intValue()) {
                    editor.remove(keyname + "_" + i);
                    editor.commit();
                }
            } else {
                if (shared.getString(keyname + "_" + i, (String) null).equals((String) value)) {
                    editor.remove(keyname + "_" + i);
                    editor.commit();
                }
            }
        }
    }

    public static void putInt(Context context, String keyname, int values) {
        SharedPreferences.Editor e = getSharedPreferences(context).edit();
        e.putInt(keyname, values);
        boolean commit = e.commit();
    }

    public static void putLong(Context context, String keyname, long values) {
        SharedPreferences.Editor e = getSharedPreferences(context).edit();
        e.putLong(keyname, values);
        boolean commit = e.commit();
    }

    public static void putBoolean(Context context, String keyname, boolean values) {
        SharedPreferences.Editor e = getSharedPreferences(context).edit();
        e.putBoolean(keyname, values);
        boolean commit = e.commit();
    }
}
