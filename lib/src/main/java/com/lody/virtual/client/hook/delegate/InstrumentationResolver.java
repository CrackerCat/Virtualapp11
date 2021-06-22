package com.lody.virtual.client.hook.delegate;

import android.app.Instrumentation;

import com.lody.virtual.helper.utils.Reflect;

import java.lang.reflect.Field;
import java.util.HashMap;
import java.util.Map;

/**
 * @author Swift Gan
 * Create: 2019/4/23
 * Desc: for resolve base Instrumantation
 */

public class InstrumentationResolver {

    private static Map<String,IResolver> resolvers = new HashMap<>();

    private static IResolver defaultResolver = new IResolver() {
        @Override
        public boolean resolve(Instrumentation root, Instrumentation base) {
            Field field = checkInstrumentation(base);
            // avoid recycle ref
            if (field != null) {
                try {
                    field.setAccessible(true);
                    field.set(base, root);
                    return true;
                } catch (IllegalAccessException e) {
                    e.printStackTrace();
                }
            }
            return false;
        }
    };

    static {
        //YY
        //Small plugin framework
        //AppInstrumentation -> InstrumentationWrapper(Small) -> RootInstrumentation
        resolvers.put("com.duowan.mobile", new IResolver() {
            @Override
            public boolean resolve(Instrumentation root, Instrumentation base) {
                if (root == null || base == null || base == root)
                    return false;
                Class instrumentationWrapper = base.getClass();
                if (instrumentationWrapper.getName().equals("com.yy.android.small.launcher.ApkPluginLauncher$InstrumentationWrapper")) {
                    Class apkPluginLauncher = Reflect.on("com.yy.android.small.launcher.ApkPluginLauncher", instrumentationWrapper.getClassLoader()).type();
                    if (apkPluginLauncher != null) {
                        try {
                            Reflect.on(apkPluginLauncher).set("sHostInstrumentation", root);
                            return true;
                        } catch (Throwable throwable) {
                            throwable.printStackTrace();
                        }
                    }
                }
                return false;
            }
        });
    }

    public static boolean resolveInstrumentation(String packageName) {
        Instrumentation root = AppInstrumentation.getDefault().root;
        Instrumentation base = AppInstrumentation.getDefault().base;
        IResolver resolver = resolvers.get(packageName);
        if (resolver != null && resolver.resolve(root, base)) {
            return true;
        } else {
            return defaultResolver.resolve(root, base);
        }
    }

    private static Field checkInstrumentation(Instrumentation instrumentation) {
        if (instrumentation instanceof AppInstrumentation) {
            return null;
        }
        Class<?> clazz = instrumentation.getClass();
        if (Instrumentation.class.equals(clazz)) {
            return null;
        }
        do {
            Field[] fields = clazz.getDeclaredFields();
            if (fields != null) {
                for (Field field : fields) {
                    if (Instrumentation.class.isAssignableFrom(field.getType())) {
                        field.setAccessible(true);
                        Object obj;
                        try {
                            obj = field.get(instrumentation);
                        } catch (IllegalAccessException e) {
                            return null;
                        }
                        if ((obj instanceof AppInstrumentation)) {
                            return field;
                        }
                    }
                }
            }
            clazz = clazz.getSuperclass();
        } while (!Instrumentation.class.equals(clazz));
        return null;
    }

    @FunctionalInterface
    public interface IResolver {
        boolean resolve(Instrumentation root, Instrumentation base);
    }

}
