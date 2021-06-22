package com.lody.virtual.controller;

import android.app.Activity;
import android.app.Application;
import android.os.Bundle;

import java.lang.reflect.Method;

public interface InstrumentationDelegateCallback {

    InstrumentationDelegateCallback EMPTY = new InstrumentationDelegateCallback() {

        @Override
        public Object callBeforeApplicationOnCreate(Application app, Method method) {
            return null;
        }

        @Override
        public Object callBeforeActivityChanged(String sMethodName, Activity activity) {
            return null;
        }

        @Override
        public Object callBeforeActivityChanged(String sMethodName, Activity activity, Object... extra) {
            return null;
        }

        @Override
        public void callAfterApplicationOnCreate(Object object) {

        }

        @Override
        public void callAfterActivityChanged(String methodName, Object object) {

        }
    };
    Object callBeforeApplicationOnCreate(Application app, Method method);

    Object callBeforeActivityChanged(String sMethodName, Activity activity);
    Object callBeforeActivityChanged(String sMethodName, Activity activity, Object... extra);

    void callAfterApplicationOnCreate(Object object);
    void callAfterActivityChanged(String methodName ,Object object);
}
