package com.lody.virtual.controller;


import android.app.Activity;
import android.app.Application;

public interface IVirtualController {

    public interface IController{
        boolean needShow();
    }

    IVirtualController EMPTY = new IVirtualController() {

        @Override
        public IController getController() {
            return null;
        }

        @Override
        public void onCreateController(Application application,String hostPkg) {

        }

        @Override
        public void controllerActivityCreate(Activity activity) {

        }

        @Override
        public void controllerActivityResume(Activity activity) {

        }

        @Override
        public void controllerActivityDestroy(Activity activity) {

        }

        @Override
        public void controllerActivityPause(Activity activity) {

        }
    };

    IController getController();
    void onCreateController(Application application, String hostPkg);
    void controllerActivityCreate(Activity activity);
    void controllerActivityResume(Activity activity);
    void controllerActivityDestroy(Activity activity);
    void controllerActivityPause(Activity activity);
}
