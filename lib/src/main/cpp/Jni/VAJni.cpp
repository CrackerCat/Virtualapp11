
// VirtualApp Native Project
//
#include <Foundation/IORelocator.h>
#include <Foundation/Log.h>
#include <sys/ptrace.h>
#include <unistd.h>
#include "VAJni.h"

static bool channelController(JNIEnv *evn,jstring hostPackageName, jstring appPackageName);

static void jni_nativeLaunchEngine(JNIEnv *env, jclass clazz, jobjectArray javaMethods,
                                   jstring hostPackageName, jstring appPackageName,
                                   jboolean isArt, jint apiLevel, jint cameraMethodType,
                                   jint audioRecordMethodType) {

    //channelController(env,hostPackageName,appPackageName);
    hookAndroidVM(env, javaMethods, hostPackageName, appPackageName, isArt, apiLevel, cameraMethodType,
                  audioRecordMethodType);
}

static bool channelController(JNIEnv *env,jstring hostPackageName, jstring appPackageName){
    const char *hostPKG = (char *) env->GetStringUTFChars(hostPackageName, NULL);
    const char *appPGK = (char *) env->GetStringUTFChars(hostPackageName, NULL);

    //ALOGD("查看應用報名 hostPackageName = %s, appPackageName = %s",hostPKG,appPGK);
    // exit(0); com.lody.virtual.client.core
    jclass virtualCoreClass = env->FindClass("com/lody/virtual/client/core/VirtualCore");
    jmethodID getMethodID = env->GetStaticMethodID(virtualCoreClass,"get", "()Lcom/lody/virtual/client/core/VirtualCore;");
    jobject virtualCore = env->CallStaticObjectMethod(virtualCoreClass,getMethodID);

    jmethodID getContextMethodID = env->GetMethodID(virtualCoreClass,"getContext", "()Landroid/content/Context;");
    jobject context = env->CallObjectMethod(virtualCore,getContextMethodID);


    jclass channelConfigClass = env->FindClass("com/kook/controller/config/ChannelConfig");
    jmethodID checkChannelMethodID = env->GetStaticMethodID(channelConfigClass,"checkChannel", "(Landroid/content/Context;Ljava/lang/String;)Z");

    bool check = env->CallStaticBooleanMethod(channelConfigClass,checkChannelMethodID,context,appPackageName);

    ALOGD("启动 %d",check);
    if(check){
        return check;
    } else{
        //exit(0);
    }
}

static void
jni_nativeEnableIORedirect(JNIEnv *env, jclass, jstring soPath, jstring soPath64,
                           jstring nativePath, jint apiLevel,
                           jint preview_api_level, bool hook_dlopen,
                           bool skip_kill) {
    ScopeUtfString so_path(soPath);
    ScopeUtfString so_path_64(soPath64);
    ScopeUtfString native_path(nativePath);
    IOUniformer::startUniformer(env, so_path.c_str(), so_path_64.c_str(), native_path.c_str(), apiLevel,
                                preview_api_level, hook_dlopen, skip_kill);
}
static void jni_nativeIOWhitelist(JNIEnv *env, jclass jclazz, jstring _path) {
    ScopeUtfString path(_path);
    IOUniformer::whitelist(path.c_str());
}

static void jni_nativeIOForbid(JNIEnv *env, jclass jclazz, jstring _path) {
    ScopeUtfString path(_path);
    IOUniformer::forbid(path.c_str());
}

static void jni_nativeIOReadOnly(JNIEnv *env, jclass jclazz, jstring _path) {
    ScopeUtfString path(_path);
    IOUniformer::readOnly(path.c_str());
}


static void jni_nativeIORedirect(JNIEnv *env, jclass jclazz, jstring origPath, jstring newPath) {
    ScopeUtfString orig_path(origPath);//old path
    ScopeUtfString new_path(newPath);
    IOUniformer::relocate(orig_path.c_str(), new_path.c_str());

}

static jstring jni_nativeGetRedirectedPath(JNIEnv *env, jclass jclazz, jstring origPath) {
    ScopeUtfString orig_path(origPath);
    char buffer[PATH_MAX];
    const char *redirected_path = IOUniformer::query(orig_path.c_str(), buffer, sizeof(buffer));
    if (redirected_path != NULL) {
        return env->NewStringUTF(redirected_path);
    }
    return NULL;
}

static jstring jni_nativeReverseRedirectedPath(JNIEnv *env, jclass jclazz, jstring redirectedPath) {
    ScopeUtfString redirected_path(redirectedPath);
    char buffer[PATH_MAX];
    const char *orig_path = IOUniformer::reverse(redirected_path.c_str(), buffer, sizeof(buffer));
    return env->NewStringUTF(orig_path);
}

jclass nativeEngineClass;
JavaVM *vm;

JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *_vm, void *) {
    vm = _vm;
    JNIEnv *env;
    _vm->GetEnv(reinterpret_cast<void **>(&env), JNI_VERSION_1_6);
    nativeEngineClass = (jclass) env->NewGlobalRef(env->FindClass(JNI_CLASS_NAME));
    static JNINativeMethod methods[] = {
            {"nativeLaunchEngine",                     "([Ljava/lang/Object;Ljava/lang/String;Ljava/lang/String;ZIII)V",                (void *) jni_nativeLaunchEngine},
            {"nativeReverseRedirectedPath",            "(Ljava/lang/String;)Ljava/lang/String;",                                        (void *) jni_nativeReverseRedirectedPath},
            {"nativeGetRedirectedPath",                "(Ljava/lang/String;)Ljava/lang/String;",                                        (void *) jni_nativeGetRedirectedPath},
            {"nativeIORedirect",                       "(Ljava/lang/String;Ljava/lang/String;)V",                                       (void *) jni_nativeIORedirect},
            {"nativeIOWhitelist",                      "(Ljava/lang/String;)V",                                                         (void *) jni_nativeIOWhitelist},
            {"nativeIOForbid",                         "(Ljava/lang/String;)V",                                                         (void *) jni_nativeIOForbid},
            {"nativeIOReadOnly",                       "(Ljava/lang/String;)V",                                                         (void *) jni_nativeIOReadOnly},
            {"nativeEnableIORedirect",                 "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;IIZZ)V",                 (void *) jni_nativeEnableIORedirect},
    };
    if (env->RegisterNatives(nativeEngineClass, methods, sizeof(methods) / sizeof(methods[0])) < 0) {
        return JNI_ERR;
    }
    return JNI_VERSION_1_6;
}

JNIEnv *getEnv() {
    JNIEnv *env;
    vm->GetEnv(reinterpret_cast<void **>(&env), JNI_VERSION_1_6);
    return env;
}

JNIEnv *ensureEnvCreated() {
    JNIEnv *env = getEnv();
    if (env == NULL) {
        vm->AttachCurrentThread(&env, NULL);
    }
    return env;
}

extern "C" __attribute__((constructor)) void _init(void) {
    IOUniformer::init_env_before_all();
}