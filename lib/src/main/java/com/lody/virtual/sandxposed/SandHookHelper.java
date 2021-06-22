package com.lody.virtual.sandxposed;

import com.lody.virtual.helper.compat.BuildCompat;
import com.swift.sandhook.SandHook;
import com.swift.sandhook.xposedcompat.XposedCompat;

import java.math.BigInteger;
import java.security.MessageDigest;
import java.security.NoSuchAlgorithmException;

public class SandHookHelper {


    public static void initHookPolicy() {
        SandHook.disableVMInline();
        //android Q
        if (BuildCompat.isQ()) {
            XposedCompat.useInternalStub = false;
            XposedCompat.cacheDir = XposedCompat.context.getCacheDir();
        }
    }

    //hash to avoid sandbox io redirect
    public static String MD5(String source) {
        try {
            MessageDigest messageDigest = MessageDigest.getInstance("MD5");
            messageDigest.update(source.getBytes());
            return new BigInteger(1, messageDigest.digest()).toString(32);
        } catch (NoSuchAlgorithmException e) {
            e.printStackTrace();
        }
        return source;
    }

}
