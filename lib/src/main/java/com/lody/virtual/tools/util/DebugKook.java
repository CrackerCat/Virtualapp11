package com.lody.virtual.tools.util;

import android.os.Binder;
import android.os.Build;
import android.os.Process;
import android.util.Log;


import java.io.BufferedWriter;
import java.io.File;
import java.io.FileWriter;
import java.io.IOException;
import java.text.SimpleDateFormat;
import java.util.Calendar;
import java.util.Date;


/**
 * Created by kook on 16-8-9.
 * 修改此文件者,请写好备注或注释
 * 邮箱 329716228@qq.com
 */
public class DebugKook {
    public static Boolean LOG_SWITCH = true; // 日志文件总开关
    private static Boolean LOG_WRITE_TO_FILE = false;// 日志写入文件开关
    private static char LOG_TYPE = 'v';// 输入日志类型，w代表只输出告警信息等，v代表输出所有信息
    public static String LOG_PATH_SDCARD_DIR = "/sdcard/Theme";//BaseRequest.getLogPath();// 日志文件在sdcard中的路径
    private static int SDCARD_LOG_FILE_SAVE_DAYS = 0;// sd卡中日志文件的最多保存天数
    private static String LOG_FILE_NAME = "";// 本类输出的日志文件名称

    private static SimpleDateFormat myLogSdf = new SimpleDateFormat("yyyy-MM-dd HH:mm:ss");// 日志的输出格式
    private static SimpleDateFormat logfile = new SimpleDateFormat("yyyy-MM-dd");// 日志文件格式
    public static String TAG = "VA-";
    public static boolean DEBUG = true;

    public static void w(Object msg) { // 警告信息
        log(TAG, msg.toString(), 'w');
    }

    public static void w(String tag, Object msg) { // 警告信息
        log(tag, msg.toString(), 'w');
    }

    public static void e(Object msg) { // 错误信息
        log(TAG, msg.toString(), 'e');
    }


    public static void e(String tag, Object msg) { // 错误信息
        log(tag, msg.toString(), 'e');
    }

    public static void d(Object msg) {// 调试信息
        log(TAG, msg.toString(), 'd');
    }

    public static void d(String tag, Object msg) {// 调试信息
        log(tag, msg.toString(), 'd');
    }

    public static void i(Object msg) {
        log(TAG, msg.toString(), 'i');
    }

    public static void i(String tag, Object msg) {
        log(tag, msg.toString(), 'i');
    }

    public static void v(Object msg) {
        log(TAG, msg.toString(), 'v');
    }

    public static void v(String tag, Object msg) {
        log(tag, msg.toString(), 'v');
    }

    public static void w(String text) {
        log(TAG, text, 'w');
    }

    public static void w(String tag, String text) {
        log(tag, text, 'w');
    }

    public static void e(String text) {
        log(TAG, text, 'e');
    }

    public static void e(String tag, String text) {
        log(tag, text, 'e');
    }

    public static void d(String text) {
        log(TAG, text, 'd');
    }

    public static void d(String tag, String text) {
        log(tag, text, 'd');
    }

    public static void i(String text) {
        log(TAG, text, 'i');
    }

    public static void i(String tag, String text) {
        log(tag, text, 'i');
    }

    public static void v(String text) {
        log(TAG, text, 'v');
    }

    public static void v(String tag, String text) {
        log(tag, text, 'v');
    }

    /**
     * 根据tag, msg和等级，输出日志
     *
     * @param tag
     * @param msg
     * @param level
     * @return void
     * @since v 1.0
     */
    private static void log(String tag, String msg, char level) {
        if (LOG_SWITCH) {
            if (!TAG.equals(tag)){
                boolean bit = Process.is64Bit();
                if (bit) {
                    tag = TAG +"64-"+"-"+ tag;
                }else {
                    tag = TAG +"32-"+"-"+ tag;
                }
            }

            if ('e' == level && ('e' == LOG_TYPE || 'v' == LOG_TYPE)) { // 输出错误信息
                Log.e(tag, msg);
            } else if ('w' == level && ('w' == LOG_TYPE || 'v' == LOG_TYPE)) {
                Log.w(tag, msg);
            } else if ('d' == level && ('d' == LOG_TYPE || 'v' == LOG_TYPE)) {
                Log.d(tag, msg);
            } else if ('i' == level && ('d' == LOG_TYPE || 'v' == LOG_TYPE)) {
                Log.i(tag, msg);
            } else {
                Log.v(tag, msg);
            }
            if (LOG_WRITE_TO_FILE){
                writeLogtoFile(String.valueOf(level), tag, msg);
            }
        }
    }

    /**
     * 打开日志文件并写入日志
     *
     * @return
     **/
    private static void writeLogtoFile(String mylogtype, String tag, String text) {// 新建或打开日志文件
        Date nowtime = new Date();
        String needWriteFiel = logfile.format(nowtime);
        String needWriteMessage = myLogSdf.format(nowtime) + "    " + mylogtype + "    " + tag + "    " + text;

        File fileDir = new File(LOG_PATH_SDCARD_DIR);
        if(!fileDir.exists()){
            fileDir.mkdirs();
        }

        File file = new File(LOG_PATH_SDCARD_DIR, needWriteFiel + LOG_FILE_NAME);
        try {
            FileWriter filerWriter = new FileWriter(file, true);//后面这个参数代表是不是要接上文件中原来的数据，不进行覆盖
            BufferedWriter bufWriter = new BufferedWriter(filerWriter);
            bufWriter.write(needWriteMessage);
            bufWriter.newLine();
            bufWriter.close();
            filerWriter.close();
        } catch (IOException e) {
            e.printStackTrace();
            Log.e(TAG,"log 写入异常:"+e.toString());
        }
    }

    /**
     * 删除制定的日志文件
     */
    public static void delFile() {// 删除日志文件
        String needDelFiel = logfile.format(getDateBefore());
        File file = new File(LOG_PATH_SDCARD_DIR, needDelFiel + LOG_FILE_NAME);
        if (file.exists()) {
            file.delete();
        }
    }

    /**
     * 得到现在时间前的几天日期，用来得到需要删除的日志文件名
     */
    private static Date getDateBefore() {
        Date nowtime = new Date();
        Calendar now = Calendar.getInstance();
        now.setTime(nowtime);
        now.set(Calendar.DATE, now.get(Calendar.DATE) - SDCARD_LOG_FILE_SAVE_DAYS);
        return now.getTime();
    }


    public static void printException(String subtag,Exception e){
        StackTraceElement[] stackTrace = e.getStackTrace();
        Throwable cause = e.getCause();
        DebugKook.d(subtag,"Build.VERSION.SDK_INT = "+ Build.VERSION.SDK_INT);
        if (cause != null) {
            String stackTraceString = Log.getStackTraceString(cause);
            DebugKook.e(subtag,"异常 cause:" + stackTraceString);
        }else {
            Log.e(subtag, "异常:" + e.toString() + "     cause is null ?" + (cause == null));
            for (int i = 0; i < stackTrace.length; i++) {
                DebugKook.e(subtag, "Exception e:" + stackTrace[i].toString());
            }
        }
    }

    public static void printException(Exception e){
        StackTraceElement[] stackTrace = e.getStackTrace();
        Throwable cause = e.getCause();
        DebugKook.d("Build.VERSION.SDK_INT = "+ Build.VERSION.SDK_INT);
        if (cause != null) {
            String stackTraceString = Log.getStackTraceString(cause);
            DebugKook.e(TAG,"异常 cause(printException):" + stackTraceString);
        }else {
            DebugKook.e(TAG, "异常:" + e.toString() + "     cause is null ?" + (cause == null));
            for (int i = 0; i < stackTrace.length; i++) {
                DebugKook.e(TAG, "Exception e:" + stackTrace[i].toString());
            }
        }
    }

    public static void printThrowable(Throwable throwable){
        String stackTraceString = Log.getStackTraceString(throwable);
        DebugKook.d("Build.VERSION.SDK_INT = "+ Build.VERSION.SDK_INT);
        DebugKook.e(TAG,"printThrowable e:" + stackTraceString);
    }

    public static void printThrowable(String subtag,Throwable throwable){
        String stackTraceString = Log.getStackTraceString(throwable);
        DebugKook.d("Build.VERSION.SDK_INT = "+ Build.VERSION.SDK_INT);
        DebugKook.e(subtag,"printThrowable e:" + stackTraceString);
    }

    /*打印栈信息*/
    public static void printInfo() {
        Throwable ex = new Throwable();
        StackTraceElement[] stackElements = ex.getStackTrace();
        if (stackElements != null) {
            for (int i = 0; i < stackElements.length; i++) {
                StackTraceElement stackTraceElement = stackElements[i];
                String output = String.format("%s():%s, %s (%s)", stackTraceElement.getMethodName(), stackTraceElement.getLineNumber(), getSimpleName(stackTraceElement.getClassName()), getPackageName(stackTraceElement.getClassName()));
                Log.i(TAG, output);
            }
        }
    }

    /*打印栈信息*/
    public static void printInfo(String tag) {
        Throwable ex = new Throwable();
        StackTraceElement[] stackElements = ex.getStackTrace();
        if (stackElements != null) {
            for (int i = 0; i < stackElements.length; i++) {
                StackTraceElement stackTraceElement = stackElements[i];
                String output = String.format("%s():%s, %s (%s)", stackTraceElement.getMethodName(), stackTraceElement.getLineNumber(), getSimpleName(stackTraceElement.getClassName()), getPackageName(stackTraceElement.getClassName()));
                DebugKook.i(tag, output);
            }
        }
    }

    private static String getSimpleName(String className) {
        // TODO Auto-generated method stub
        return className.substring(className.lastIndexOf('.') + 1);
    }

    private static String getPackageName(String className) {
        // TODO Auto-generated method stub
        return className.substring(0, className.lastIndexOf('.'));
    }

}