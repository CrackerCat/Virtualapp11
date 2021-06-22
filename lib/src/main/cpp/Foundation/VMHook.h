//
// VirtualApp Native Project
//

#ifndef FOUNDATION_VM_HOOK
#define FOUNDATION_VM_HOOK


#include <Jni/VAJni.h>
#include <unistd.h>
#include <Substrate/CydiaSubstrate.h>
#include <cstring>
#include <jni.h>
#include <dlfcn.h>
#include <stddef.h>
#include <fcntl.h>
#include <sys/system_properties.h>
#include "Jni/Helper.h"
#include "Log.h"
#include "SandboxFs.h"
#include "Symbol.h"


enum METHODS {
    OPEN_DEX = 0,
    CAMERA_SETUP,
    AUDIO_NATIVE_CHECK_PERMISSION,
    MEDIA_RECORDER_SETUP,
    AUDIO_RECORD_SETUP
};

namespace FunctionDef {
    typedef void (*Function_DalvikBridgeFunc)(const void **, void *, const void *, void *);

    typedef jobject (*JNI_openDexNativeFunc)(JNIEnv *, jclass, jstring, jstring, jint);

    typedef jobject (*JNI_openDexNativeFunc_N)(JNIEnv *, jclass, jstring, jstring, jint, jobject,
                                               jobject);

    typedef jint (*JNI_cameraNativeSetupFunc)(JNIEnv *, jobject,
                                              jobject, jobject, jobject, jobject, jobject,
                                              jobject, jobject, jobject);

    typedef jint (*JNI_getCallingUid)(JNIEnv *, jclass);

    typedef jint (*JNI_audioRecordNativeCheckPermission)(JNIEnv *, jobject, jstring);

    typedef jstring (*JNI_nativeLoad)(JNIEnv *env, jclass, jstring, jobject, jobject);

    typedef void (*JNI_mediaRecorderNativeSetupFunc)(JNIEnv *, jobject,
                                                     jobject, jstring, jstring);

    typedef jint (*JNI_audioRecordNativeSetupFunc_M)(JNIEnv *, jobject,
                                                     jobject, jobject, jint, jint, jint,
                                                     jint, jint, jintArray, jstring);

    typedef jint (*JNI_audioRecordNativeSetupFunc_N)(JNIEnv *, jobject,
                                                     jobject, jobject, jintArray, jint, jint,
                                                     jint, jint, jintArray, jstring, jlong);

}
static void (*orig_sendSignal)(JNIEnv *pEnv,
                            jclass pJclass,
                            jint stub0,
                            jint stub1);


using namespace FunctionDef;

struct PatchEnv {
    bool is_art;
    int native_offset;
    char *host_packageName;
    char *app_packageName;
    jint api_level;
    jmethodID method_onGetCallingUid;
    jmethodID method_onOpenDexFileNative;

    void *art_work_around_app_jni_bugs;

    char *(*GetCstrFromString)(void *);

    void *(*GetStringFromCstr)(const char *);

    int (*native_getCallingUid)(int);

    int (*IPCThreadState_self)(void);


    JNI_getCallingUid orig_getCallingUid;

    int cameraMethodType;
    int cameraMethodPkgIndex;
    Function_DalvikBridgeFunc orig_cameraNativeSetup_dvm;
    JNI_cameraNativeSetupFunc orig_cameraNativeSetupFunc;

    union {
        JNI_openDexNativeFunc beforeN;
        JNI_openDexNativeFunc_N afterN;
    } orig_openDexNativeFunc_art;

    Function_DalvikBridgeFunc orig_openDexFile_dvm;
    JNI_audioRecordNativeCheckPermission orig_audioRecordNativeCheckPermission;
    JNI_nativeLoad orig_nativeLoad;

    void (*dvmUseJNIBridge)(void *method, void *func);

    Function_DalvikBridgeFunc org_mediaRecorderNativeSetup_dvm;
    JNI_mediaRecorderNativeSetupFunc orig_mediaRecorderNativeSetupFunc;

    JNI_audioRecordNativeSetupFunc_M orig_audioRecordNativeSetupFunc_M;
    JNI_audioRecordNativeSetupFunc_N orig_audioRecordNativeSetupFunc_N;

};

extern PatchEnv patchEnv;

void hookAndroidVM(JNIEnv *env, jobjectArray javaMethods,
                   jstring hostPackageName, jstring appPackageName, jboolean isArt, jint apiLevel, jint cameraMethodType,
                   jint audioRecordMethodType);


#endif //NDK_HOOK_NATIVE_H
