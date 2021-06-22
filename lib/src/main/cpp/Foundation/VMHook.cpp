//
// VirtualApp Native Project
//
#include <Jni/JniHook.h>
#include <ostream>
#include "VMHook.h"

PatchEnv patchEnv;





jint dvm_getCallingUid(JNIEnv *env, jclass clazz) {
    jint uid = patchEnv.native_getCallingUid(patchEnv.IPCThreadState_self());
    env = ensureEnvCreated();
    uid = env->CallStaticIntMethod(nativeEngineClass, patchEnv.method_onGetCallingUid, uid);
    return uid;
}

jint new_getCallingUid(JNIEnv *env, jclass clazz) {
    int uid = patchEnv.orig_getCallingUid(env, clazz);
    env = ensureEnvCreated();
    uid = env->CallStaticIntMethod(nativeEngineClass, patchEnv.method_onGetCallingUid, uid);
    return uid;
}



jstring new_nativeLoad(JNIEnv *env, jclass clazz, jstring _file, jobject classLoader, jobject _ld) {
    ScopeUtfString orig_path(_file);
    char buffer[PATH_MAX];
    const char *redirected_path = IOUniformer::query(orig_path.c_str(), buffer, sizeof(buffer));
    if (redirected_path != NULL) {
        env = ensureEnvCreated();
        _file = env->NewStringUTF(redirected_path);
    }
    return patchEnv.orig_nativeLoad(env, clazz, _file, classLoader, _ld);
}

static void
new_bridge_openDexNativeFunc(const void **args, void *pResult, const void *method, void *self) {

    JNIEnv *env = ensureEnvCreated();

    const char *source = args[0] == NULL ? NULL : patchEnv.GetCstrFromString((void *) args[0]);
    const char *output = args[1] == NULL ? NULL : patchEnv.GetCstrFromString((void *) args[1]);

    jstring orgSource = source == NULL ? NULL : env->NewStringUTF(source);
    jstring orgOutput = output == NULL ? NULL : env->NewStringUTF(output);

    jclass stringClass = env->FindClass("java/lang/String");
    jobjectArray array = env->NewObjectArray(2, stringClass, NULL);
    if (orgSource) {
        env->SetObjectArrayElement(array, 0, orgSource);
    }
    if (orgOutput) {
        env->SetObjectArrayElement(array, 1, orgOutput);
    }
    env->CallStaticVoidMethod(nativeEngineClass, patchEnv.method_onOpenDexFileNative, array);

    jstring newSource = (jstring) env->GetObjectArrayElement(array, 0);
    jstring newOutput = (jstring) env->GetObjectArrayElement(array, 1);

    const char *_newSource = newSource == NULL ? NULL : env->GetStringUTFChars(newSource, NULL);
    const char *_newOutput = newOutput == NULL ? NULL : env->GetStringUTFChars(newOutput, NULL);

    args[0] = _newSource == NULL ? NULL : patchEnv.GetStringFromCstr(_newSource);
    args[1] = _newOutput == NULL ? NULL : patchEnv.GetStringFromCstr(_newOutput);

    if (source && orgSource) {
        env->ReleaseStringUTFChars(orgSource, source);
    }
    if (output && orgOutput) {
        env->ReleaseStringUTFChars(orgOutput, output);
    }

    patchEnv.orig_openDexFile_dvm(args, pResult, method, self);
}

static jobject new_native_openDexNativeFunc(JNIEnv *env, jclass jclazz, jstring javaSourceName,
                                            jstring javaOutputName, jint options) {
    jclass stringClass = env->FindClass("java/lang/String");
    jobjectArray array = env->NewObjectArray(2, stringClass, NULL);

    if (javaSourceName) {
        env->SetObjectArrayElement(array, 0, javaSourceName);
    }
    if (javaOutputName) {
        env->SetObjectArrayElement(array, 1, javaOutputName);
    }
    env->CallStaticVoidMethod(nativeEngineClass, patchEnv.method_onOpenDexFileNative, array);

    jstring newSource = (jstring) env->GetObjectArrayElement(array, 0);
    jstring newOutput = (jstring) env->GetObjectArrayElement(array, 1);

    return patchEnv.orig_openDexNativeFunc_art.beforeN(env, jclazz, newSource, newOutput,
                                                       options);
}

static jobject new_native_openDexNativeFunc_N(JNIEnv *env, jclass jclazz, jstring javaSourceName,
                                              jstring javaOutputName, jint options, jobject loader,
                                              jobject elements) {

    env = ensureEnvCreated();

    jclass stringClass = env->FindClass("java/lang/String");
    jobjectArray array = env->NewObjectArray(2, stringClass, NULL);

    if (javaSourceName) {
        env->SetObjectArrayElement(array, 0, javaSourceName);
    }
    if (javaOutputName) {
        env->SetObjectArrayElement(array, 1, javaOutputName);
    }
    env->CallStaticVoidMethod(nativeEngineClass, patchEnv.method_onOpenDexFileNative, array);

    jstring newSource = (jstring) env->GetObjectArrayElement(array, 0);
    jstring newOutput = (jstring) env->GetObjectArrayElement(array, 1);

    return patchEnv.orig_openDexNativeFunc_art.afterN(env, jclazz, newSource, newOutput, options,
                                                      loader, elements);
}

static void
new_bridge_cameraNativeSetupFunc(const void **args, void *pResult, const void *method, void *self) {
    jint index = patchEnv.cameraMethodPkgIndex + 1;
    args[index] = patchEnv.GetStringFromCstr(patchEnv.host_packageName);
    patchEnv.orig_cameraNativeSetup_dvm(args, pResult, method, self);
}

static jint new_native_cameraNativeSetupFunc_T(JNIEnv *env, jobject thiz,
                                               jobject o1, jobject o2, jobject o3, jobject o4,
                                               jobject o5,
                                               jobject o6, jobject o7, jobject o8) {

    env = ensureEnvCreated();

    jint index = patchEnv.cameraMethodPkgIndex;
    if (index >= 0) {
        jstring host = env->NewStringUTF(patchEnv.host_packageName);
        switch (index) {
            case 0:
                return patchEnv.orig_cameraNativeSetupFunc(env, thiz, host, o2, o3, o4, o5, o6, o7,
                                                           o8);
            case 1:
                return patchEnv.orig_cameraNativeSetupFunc(env, thiz, o1, host, o3, o4, o5, o6, o7,
                                                           o8);
            case 2:
                return patchEnv.orig_cameraNativeSetupFunc(env, thiz, o1, o2, host, o4, o5, o6, o7,
                                                           o8);
            case 3:
                return patchEnv.orig_cameraNativeSetupFunc(env, thiz, o1, o2, o3, host, o5, o6, o7,
                                                           o8);
            case 4:
                return patchEnv.orig_cameraNativeSetupFunc(env, thiz, o1, o2, o3, o4, host, o6, o7,
                                                           o8);
            case 5:
                return patchEnv.orig_cameraNativeSetupFunc(env, thiz, o1, o2, o3, o4, o5, host, o7,
                                                           o8);
            case 6:
                return patchEnv.orig_cameraNativeSetupFunc(env, thiz, o1, o2, o3, o4, o5, o6, host,
                                                           o8);
            case 7:
                return patchEnv.orig_cameraNativeSetupFunc(env, thiz, o1, o2, o3, o4, o5, o6, o7,
                                                           host);
        }
    }
    return patchEnv.orig_cameraNativeSetupFunc(env, thiz, o1, o2, o3, o4, o5, o6, o7, o8);
}

static jint
new_native_audioRecordNativeCheckPermission(JNIEnv *env, jobject thiz, jstring _packagename) {
    env = ensureEnvCreated();
    jstring host = env->NewStringUTF(patchEnv.host_packageName);
    return patchEnv.orig_audioRecordNativeCheckPermission(env, thiz, host);
}

static void new_native_mediaRecorderNativeSetupFunc(JNIEnv *env, jobject thiz,
                                                    jobject o1, jstring o2, jstring o3) {
    jstring host = env->NewStringUTF(patchEnv.host_packageName);
    patchEnv.orig_mediaRecorderNativeSetupFunc(env, thiz, o1, host, host);
}

static void
new_bridge_mediaRecorderNativeSetupFunc(const void **args, void *pResult, const void *method,
                                        void *self) {
    jint index = 1 + 1;
    args[index] = patchEnv.GetStringFromCstr(patchEnv.host_packageName);
    patchEnv.org_mediaRecorderNativeSetup_dvm(args, pResult, method, self);
}

//hookAudioRecordNativeSetup
static jint new_native_audioRecordNativeSetupFunc_M(JNIEnv *env, jobject thiz,
                                                    jobject o1, jobject o2, jint o3, jint o4,
                                                    jint o5,
                                                    jint o6, jint o7, jintArray o8, jobject o9) {
    env = ensureEnvCreated();
    jstring host = env->NewStringUTF(patchEnv.host_packageName);
    return patchEnv.orig_audioRecordNativeSetupFunc_M(env, thiz, o1, o2, o3, o4, o5, o6, o7, o8,
                                                      host);
}

static jint new_native_audioRecordNativeSetupFunc_N(JNIEnv *env, jobject thiz,
                                                    jobject o1, jobject o2, jintArray o3, jint o4,
                                                    jint o5,
                                                    jint o6, jint o7, jintArray o8, jobject o9,
                                                    jlong o10) {
    env = ensureEnvCreated();
    jstring host = env->NewStringUTF(patchEnv.host_packageName);
    return patchEnv.orig_audioRecordNativeSetupFunc_N(env, thiz, o1, o2, o3, o4, o5, o6, o7, o8,
                                                      host, o10);
}

void mark() {
    // Do nothing
};


void measureNativeOffset(JNIEnv *env, bool isArt) {

    jmethodID markMethod = env->GetStaticMethodID(nativeEngineClass, "nativeMark", "()V");

    size_t start = (size_t) markMethod;
    size_t target = (size_t) mark;

    if (isArt && patchEnv.art_work_around_app_jni_bugs) {
        target = (size_t) patchEnv.art_work_around_app_jni_bugs;
    }

    int offset = 0;
    bool found = false;
    while (true) {
        if (*((size_t *) (start + offset)) == target) {
            found = true;
            break;
        }
        offset += 4;
        if (offset >= 100) {
            ALOGE("Error: Cannot find the jni function offset.");
            break;
        }
    }
    if (found) {
        patchEnv.native_offset = offset;
        if (!isArt) {
            patchEnv.native_offset += (sizeof(int) + sizeof(void *));
        }

        ALOGE("计算 native 地址偏移量 offset = %d,native_offset = %d",offset,patchEnv.native_offset);
    }
}

/**
 * 作者：kook
 * 时间：21-2-18:下午11:34
 * 邮箱： hangjunhe@ecarx.com.cn
 * 说明：

 *******begin start*********/
static void *getArtMethod(JNIEnv *env, jobject jmethod) {
    void *artMethod = NULL;

    if(jmethod == NULL) {
        return artMethod;
    }
    jclass classExecutable;
    classExecutable = (*env).FindClass("java/lang/reflect/Executable");
    jfieldID fieldArtMethod = (*env).GetFieldID(classExecutable, "artMethod", "J");
    artMethod = (void *) (*env).GetLongField(jmethod, fieldArtMethod);
    return artMethod;
}

static jobject getMethodObj(JNIEnv* env, const char* jclazz, const char* jme)
{
    jclass js = env->FindClass("com/lody/virtual/cpuabi/RUtils");
    jmethodID jmid = env->GetStaticMethodID(js,"getMethod", "(Ljava/lang/String;Ljava/lang/String;)Ljava/lang/Object;");
    return env->CallStaticObjectMethod(js,jmid,env->NewStringUTF(jclazz),env->NewStringUTF(jme));
}

void measureNativeOffset(JNIEnv *env, bool isArt, int apiLevel) {
    jmethodID markMethod = env->GetStaticMethodID(nativeEngineClass, "nativeMark", "()V");

    auto start = (unsigned long) markMethod;

    if(apiLevel>=30) {
        jobject jobj = getMethodObj(env,JNI_CLASS_NAME,"nativeMark");
        start = (unsigned long)getArtMethod(env,jobj);
    }
    auto target = (unsigned long) mark;
    ALOGE("native start offset start = %d,target = %d",start,target);
    if (isArt && patchEnv.art_work_around_app_jni_bugs) {
        target = (size_t) patchEnv.art_work_around_app_jni_bugs;
        ALOGE("native start target = %d",target);
    }

    int offset = 0;
    bool found = false;
    while (true) {
        if (*((unsigned long *) (start + offset)) == target) {
            found = true;
            break;
        }
        offset += 4;
        if (offset >= 100) {
            ALOGE("Error: Cannot find the jni function offset.");
            break;
        }
    }
    if (found) {
        patchEnv.native_offset = offset;
        if (!isArt) {
            patchEnv.native_offset += (sizeof(int) + sizeof(void *));
        }
        ALOGE("计算 native 地址偏移量 offset = %d,native_offset = %d",offset,patchEnv.native_offset);
    }
}



/** kook modify end*/
void vmUseJNIFunction(jmethodID method, void *jniFunction) {
    void **funPtr = (void **) (reinterpret_cast<size_t>(method) + patchEnv.native_offset);
    *funPtr = jniFunction;
}

void *vmGetJNIFunction(jmethodID method) {
    void **funPtr = (void **) (reinterpret_cast<size_t>(method) + patchEnv.native_offset);
    return *funPtr;
}


void hookJNIMethod(jmethodID method, void *new_jni_func, void **orig_jni_func) {
    *orig_jni_func = vmGetJNIFunction(method);
    vmUseJNIFunction(method, new_jni_func);
}


void hookGetCallingUid(JNIEnv *env, jboolean isArt) {
    if (isArt) {
        jclass binderClass = env->FindClass("android/os/Binder");
        jmethodID getCallingUid = env->GetStaticMethodID(binderClass, "getCallingUid", "()I");
        hookJNIMethod(getCallingUid,
                      (void *) new_getCallingUid,
                      (void **) &patchEnv.orig_getCallingUid
        );
    } else {
        static JNINativeMethod methods[] = {
                {"getCallingUid", "()I", (void *) dvm_getCallingUid},
        };
        jclass binderClass = env->FindClass("android/os/Binder");
        env->RegisterNatives(binderClass, methods, 1);
    }
}

/**
 * 作者：
 * 时间：:
 * 邮箱： hangjunhe@ecarx.com.cn
 * 说明：

 ***********begin start************/
void *vmGetJNIFunctionR(JNIEnv* env, jobject method) {
    void **funPtr = (void **) (reinterpret_cast<size_t>(getArtMethod(env,method)) + patchEnv.native_offset);
    return *funPtr;
}

void vmUseJNIFunctionR(JNIEnv* env, jobject method, void *jniFunction) {
    void **funPtr = (void **) (reinterpret_cast<size_t>(getArtMethod(env,method)) + patchEnv.native_offset);
    *funPtr = jniFunction;
}
void hookJNIMethodR(JNIEnv* env, jobject method, void *new_jni_func, void **orig_jni_func) {
    *orig_jni_func = vmGetJNIFunctionR(env, method);
    vmUseJNIFunctionR(env, method, new_jni_func);
}
//static void new_sendSignal(JNIEnv* env, jclass clazz, jint stub0, jint stub1)
//{
//    if(banProcessFromExit)
//    {
//        // Kill.
//        // ALOGE("Process killed with called sendSignal! Get uid -> %d and signal %d.", stub0, stub1);
//        env->CallStaticVoidMethod(
//                nativeEngineClass,
//                env->GetStaticMethodID(nativeEngineClass,
//                                       "printStackTraceNative","()V")
//        );
//        return;
//    }
//    patchEnv.orig_sendSignal(env, clazz, stub0, stub1);
//}
//HOOK sendsignal

static void new_sendSignal(JNIEnv* env, jclass clazz, jint stub0, jint stub1)
{
    ALOGE("send -> %d and signal %d.", stub0, stub1);
    orig_sendSignal(env,clazz, stub0, stub1);
}
void HookSendSing(JNIEnv *env){
    jclass runtimeClass = env->FindClass("android/os/Process");
    jmethodID killProcess = env->GetStaticMethodID(runtimeClass, "sendSignal",
                                                   "(II)V");
    env->ExceptionClear();
    if (killProcess) {

        hookJNIMethod(killProcess, (void *) new_sendSignal,
                      (void **) &orig_sendSignal);
    } else {
        ALOGE("没hook到killProcess");
    }
}



/*HOOK Process kill*/
HOOK_JNI(void, killProcess, JNIEnv *env, jint pid) {
//    orig_sendSignal(env, pid,signal);
    ALOGE("HOOK到了");
//    return orig_killProcess(env, pid);
}

void HookkillProcess(JNIEnv *env){
    jclass runtimeClass = env->FindClass("android/os/Process");
    jmethodID killProcess = env->GetStaticMethodID(runtimeClass, "killProcess",
                                                  "(I)V");
    env->ExceptionClear();
    if (killProcess) {
        hookJNIMethod(killProcess, (void *) new_killProcess,
                      (void **) &orig_killProcess);
    } else {
        ALOGE("没hook到killProcess");
    }
}
/*HOOK Process kill*/
HOOK_JNI(void, killProcessGroup, JNIEnv *env,jint uid ,jint pid) {
//    orig_sendSignal(env, pid,signal);
    ALOGE("HOOK到了");
//    return orig_killProcessGroup(env, uid,pid);
}

void HookkillProcessGroup(JNIEnv *env){
    jclass runtimeClass = env->FindClass("android/os/Process");
    jmethodID killProcessGroup = env->GetStaticMethodID(runtimeClass, "killProcessGroup",
                                                   "(II)I");
    env->ExceptionClear();
    if (killProcessGroup) {
        hookJNIMethod(killProcessGroup, (void *) new_killProcessGroup,
                      (void **) &orig_killProcess);
    } else {
        ALOGE("没hook到killProcess");
    }
}


void hookRuntimeNativeLoad(JNIEnv *env, int apilevel) {
    if (patchEnv.is_art&&apilevel<30) {
        jclass runtimeClass = env->FindClass("java/lang/Runtime");
        jmethodID nativeLoad = env->GetStaticMethodID(runtimeClass, "nativeLoad",
                                                      "(Ljava/lang/String;Ljava/lang/ClassLoader;Ljava/lang/String;)Ljava/lang/String;");
        env->ExceptionClear();
        if (!nativeLoad) {
            nativeLoad = env->GetStaticMethodID(runtimeClass, "nativeLoad",
                                                "(Ljava/lang/String;Ljava/lang/ClassLoader;)Ljava/lang/String;");
            env->ExceptionClear();
        }
        if (nativeLoad) {
            hookJNIMethod(nativeLoad, (void *) new_nativeLoad,
                          (void **) &patchEnv.orig_nativeLoad);
        } else {
            ALOGE("Error: cannot find nativeLoad method.");
        }
    }
    else if(apilevel>=30)
    {
        jobject jobj = getMethodObj(env,"java/lang/Runtime","nativeLoad");
        if(jobj!= nullptr)
        {
            hookJNIMethodR(env,jobj,(void *) new_nativeLoad,
                           (void **) patchEnv.orig_nativeLoad);
        } else
        {
            ALOGE("Warning -> nativeLoad is null!!!");
        }
    }
}
/**  modify end*/


void hookOpenDexFileNative(JNIEnv *env, jobject javaMethod, jboolean isArt, int apiLevel) {
    ALOGD("hookOpenDexFileNative apiLevel = %d, isArt = %d ",apiLevel, isArt);
    if (!isArt) {
        size_t mtd_openDexNative = (size_t) env->FromReflectedMethod(javaMethod);
        int nativeFuncOffset = patchEnv.native_offset;
        void **jniFuncPtr = (void **) (mtd_openDexNative + nativeFuncOffset);
        patchEnv.orig_openDexFile_dvm = (Function_DalvikBridgeFunc) (*jniFuncPtr);
        *jniFuncPtr = (void *) new_bridge_openDexNativeFunc;
    } else if(apiLevel<30){
        jmethodID method = env->FromReflectedMethod(javaMethod);
        void *jniFunc = vmGetJNIFunction(method);
        if (apiLevel < 24) {
            patchEnv.orig_openDexNativeFunc_art.beforeN = (JNI_openDexNativeFunc) (jniFunc);
            vmUseJNIFunction(method, (void *) new_native_openDexNativeFunc);
        } else {
            patchEnv.orig_openDexNativeFunc_art.afterN = (JNI_openDexNativeFunc_N) (jniFunc);
            vmUseJNIFunction(method, (void *) new_native_openDexNativeFunc_N);
        }
    } else{
        void *jniFunc = vmGetJNIFunctionR(env,javaMethod);
        patchEnv.orig_openDexNativeFunc_art.afterN = (JNI_openDexNativeFunc_N) (jniFunc);
        vmUseJNIFunctionR(env, javaMethod, (void *) new_native_openDexNativeFunc_N);
    }
    /*if (!isArt) {
        size_t mtd_openDexNative = (size_t) env->FromReflectedMethod(javaMethod);
        int nativeFuncOffset = patchEnv.native_offset;
        void **jniFuncPtr = (void **) (mtd_openDexNative + nativeFuncOffset);
        patchEnv.orig_openDexFile_dvm = (Function_DalvikBridgeFunc) (*jniFuncPtr);
        *jniFuncPtr = (void *) new_bridge_openDexNativeFunc;
    } else {
        jmethodID method = env->FromReflectedMethod(javaMethod);
        void *jniFunc = vmGetJNIFunction(method);
        if (apiLevel < 24) {
            patchEnv.orig_openDexNativeFunc_art.beforeN = (JNI_openDexNativeFunc) (jniFunc);
            vmUseJNIFunction(method, (void *) new_native_openDexNativeFunc);
        } else {
            patchEnv.orig_openDexNativeFunc_art.afterN = (JNI_openDexNativeFunc_N) (jniFunc);
            vmUseJNIFunction(method, (void *) new_native_openDexNativeFunc_N);
        }
    }*/
}

void hookRuntimeNativeLoad(JNIEnv *env) {
    if (patchEnv.is_art) {
        jclass runtimeClass = env->FindClass("java/lang/Runtime");
        jmethodID nativeLoad = env->GetStaticMethodID(runtimeClass, "nativeLoad",
                                                      "(Ljava/lang/String;Ljava/lang/ClassLoader;Ljava/lang/String;)Ljava/lang/String;");
        env->ExceptionClear();
        if (!nativeLoad) {
            //for Android Q
            nativeLoad = env->GetStaticMethodID(runtimeClass, "nativeLoad",
                                                "(Ljava/lang/String;Ljava/lang/ClassLoader;Ljava/lang/Class;)Ljava/lang/String;");
            env->ExceptionClear();
        }
        if (!nativeLoad) {
            nativeLoad = env->GetStaticMethodID(runtimeClass, "nativeLoad",
                                                "(Ljava/lang/String;Ljava/lang/ClassLoader;)Ljava/lang/String;");
            env->ExceptionClear();
        }
        if (nativeLoad) {
            hookJNIMethod(nativeLoad, (void *) new_nativeLoad,
                          (void **) &patchEnv.orig_nativeLoad);
        } else {
            ALOGE("Error: cannot find nativeLoad method.");
        }
    }
}

inline void
hookCameraNativeSetup(JNIEnv *env, jobject javaMethod, jboolean isArt, int apiLevel) {
    if (!javaMethod) {
        return;
    }
    if (!isArt) {
        size_t mtd_cameraNativeSetup = (size_t) env->FromReflectedMethod(
                javaMethod);
        int nativeFuncOffset = patchEnv.native_offset;
        void **jniFuncPtr = (void **) (mtd_cameraNativeSetup + nativeFuncOffset);

        patchEnv.orig_cameraNativeSetup_dvm = (Function_DalvikBridgeFunc) (*jniFuncPtr);
        *jniFuncPtr = (void *) new_bridge_cameraNativeSetupFunc;
    } else {
        jmethodID method = env->FromReflectedMethod(javaMethod);
        hookJNIMethod(method,
                      (void *) new_native_cameraNativeSetupFunc_T,
                      (void **) &patchEnv.orig_cameraNativeSetupFunc
        );
    }
}

void
hookAudioRecordNativeCheckPermission(JNIEnv *env, jobject javaMethod, jboolean isArt, int api) {
    if (!javaMethod || !isArt) {
        return;
    }
    jmethodID method = env->FromReflectedMethod(javaMethod);
    hookJNIMethod(method,
                  (void *) new_native_audioRecordNativeCheckPermission,
                  (void **) &patchEnv.orig_audioRecordNativeCheckPermission
    );
}


inline void
hookMediaRecorderNativeSetup(JNIEnv *env, jobject javaMethod, jboolean isArt, int apiLevel) {
    if (!javaMethod) {
        return;
    }
    if (!isArt) {
        size_t mtd_NativeSetup = (size_t) env->FromReflectedMethod(javaMethod);
        int nativeFuncOffset = patchEnv.native_offset;
        void **jniFuncPtr = (void **) (mtd_NativeSetup + nativeFuncOffset);

        patchEnv.org_mediaRecorderNativeSetup_dvm = (Function_DalvikBridgeFunc) (*jniFuncPtr);
        *jniFuncPtr = (void *) new_bridge_mediaRecorderNativeSetupFunc;
    } else {
        jmethodID method = env->FromReflectedMethod(javaMethod);
        hookJNIMethod(method,
                      (void *) new_native_mediaRecorderNativeSetupFunc,
                      (void **) &patchEnv.orig_mediaRecorderNativeSetupFunc
        );
    }
}

inline void
hookAudioRecordNativeSetup(JNIEnv *env, jobject javaMethod, jboolean isArt, jint apiLevel,
                           jint audioRecordMethodType) {
    if (!javaMethod || !isArt) {
        return;
    }
    jmethodID method = env->FromReflectedMethod(javaMethod);
    if (audioRecordMethodType == 2) {
        hookJNIMethod(method,
                      (void *) new_native_audioRecordNativeSetupFunc_N,
                      (void **) &patchEnv.orig_audioRecordNativeSetupFunc_N
        );
    } else {
        hookJNIMethod(method,
                      (void *) new_native_audioRecordNativeSetupFunc_M,
                      (void **) &patchEnv.orig_audioRecordNativeSetupFunc_M
        );
    }
}

void *getDalvikSOHandle() {
    char so_name[25] = {0};
    __system_property_get("persist.sys.dalvik.vm.lib.2", so_name);
    if (strlen(so_name) == 0) {
        __system_property_get("persist.sys.dalvik.vm.lib", so_name);
    }
    void *soInfo = dlopen(so_name, 0);
    if (!soInfo) {
        soInfo = RTLD_DEFAULT;
    }
    return soInfo;
}


/**
 * Only called once.
 * @param javaMethod Method from Java
 * @param isArt Dalvik or Art
 * @param apiLevel Api level from Java
 */
void hookAndroidVM(JNIEnv *env, jobjectArray javaMethods,
                   jstring hostPackageName, jstring appPackageName, jboolean isArt, jint apiLevel,
                   jint cameraMethodType, jint audioRecordMethodType) {
    JNINativeMethod methods[] = {
            {"nativeMark", "()V", (void *) mark},
    };
    if (env->RegisterNatives(nativeEngineClass, methods, 1) < 0) {
        return;
    }
    patchEnv.is_art = isArt;
    patchEnv.cameraMethodType = cameraMethodType;
    if (cameraMethodType >= 0x10) {
        patchEnv.cameraMethodPkgIndex = cameraMethodType - 0x10;
    } else {
        if (patchEnv.cameraMethodType == 2 || patchEnv.cameraMethodType == 3) {
            patchEnv.cameraMethodPkgIndex = 3;
        } else {
            patchEnv.cameraMethodPkgIndex = 2;
        }
    }



    patchEnv.host_packageName = (char *) env->GetStringUTFChars(hostPackageName,
                                                                NULL);
    patchEnv.app_packageName = (char *) env->GetStringUTFChars(appPackageName,
                                                                NULL);

    patchEnv.api_level = apiLevel;
    patchEnv.method_onGetCallingUid = env->GetStaticMethodID(nativeEngineClass,
                                                             "onGetCallingUid",
                                                             "(I)I");
    patchEnv.method_onOpenDexFileNative = env->GetStaticMethodID(nativeEngineClass,
                                                                 "onOpenDexFileNative",
                                                                 "([Ljava/lang/String;)V");

    if (!isArt) {
        // workaround for dlsym returns null when system has libhoudini
        void *h = dlopen("/system/lib/libandroid_runtime.so", RTLD_LAZY);
        {
            patchEnv.IPCThreadState_self = (int (*)(void)) dlsym(RTLD_DEFAULT,
                                                                 "_ZN7android14IPCThreadState4selfEv");
            patchEnv.native_getCallingUid = (int (*)(int)) dlsym(RTLD_DEFAULT,
                                                                 "_ZNK7android14IPCThreadState13getCallingUidEv");
            if (patchEnv.native_getCallingUid == nullptr) {
                patchEnv.native_getCallingUid = (int (*)(int)) dlsym(RTLD_DEFAULT,
                                                                     "_ZN7android14IPCThreadState13getCallingUidEv");
            }
        }
        if (h != nullptr) {
            dlclose(h);
        }
        void *soInfo = getDalvikSOHandle();
        patchEnv.GetCstrFromString = (char *(*)(void *)) dlsym(soInfo,
                                                               "_Z23dvmCreateCstrFromStringPK12StringObject");
        if (!patchEnv.GetCstrFromString) {
            patchEnv.GetCstrFromString = (char *(*)(void *)) dlsym(soInfo,
                                                                   "dvmCreateCstrFromString");
        }
        patchEnv.GetStringFromCstr = (void *(*)(const char *)) dlsym(soInfo,
                                                                     "_Z23dvmCreateStringFromCstrPKc");
        if (!patchEnv.GetStringFromCstr) {
            patchEnv.GetStringFromCstr = (void *(*)(const char *)) dlsym(soInfo,
                                                                         "dvmCreateStringFromCstr");
        }
        patchEnv.dvmUseJNIBridge = (void (*)(void *, void *)) (dlsym(soInfo,
                                                                     "_Z15dvmUseJNIBridgeP6MethodPv"));
    }

    ALOGD(" VMHOOK  measureNativeOffset ");
    if (apiLevel >= 30){
        measureNativeOffset(env, isArt, apiLevel);
    } else {
        measureNativeOffset(env, isArt);
    }

    ALOGD(" VMHOOK  hookGetCallingUid ");
    hookGetCallingUid(env, isArt);
    hookOpenDexFileNative(env, env->GetObjectArrayElement(javaMethods, OPEN_DEX), isArt,
                          apiLevel);
    hookCameraNativeSetup(env, env->GetObjectArrayElement(javaMethods, CAMERA_SETUP), isArt,
                          apiLevel);
    hookAudioRecordNativeCheckPermission(
            env, env->GetObjectArrayElement(javaMethods, AUDIO_NATIVE_CHECK_PERMISSION), isArt,
            apiLevel);
    hookMediaRecorderNativeSetup(env,
                                 env->GetObjectArrayElement(javaMethods, MEDIA_RECORDER_SETUP),
                                 isArt,
                                 apiLevel);
    hookAudioRecordNativeSetup(env, env->GetObjectArrayElement(javaMethods, AUDIO_RECORD_SETUP),
                               isArt, apiLevel,
                               audioRecordMethodType);
    ALOGD(" VMHOOK  hookRuntimeNativeLoad ");
    if (apiLevel >= 30){
        hookRuntimeNativeLoad(env,apiLevel);
    } else {
/*        hookRuntimeNativeLoad(env);
//        HookSendSing(env);
        HookkillProcess(env);
        HookkillProcessGroup(env);*/
    }

}
