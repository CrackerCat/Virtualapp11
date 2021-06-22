package com.lody.virtual.client.hook.base;

import java.lang.reflect.Method;

public class ReplaceLastUserIdMethodProxy extends StaticMethodProxy {

    public ReplaceLastUserIdMethodProxy(String name) {
        super(name);
    }

    @Override
    public boolean beforeCall(Object who, Method method, Object... args) {
        replaceLastUserId(args);
        return super.beforeCall(who, method, args);
    }
}