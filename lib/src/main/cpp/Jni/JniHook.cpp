//
// Created by 86151 on 2021/5/18.
//

#include <Foundation/Log.h>
#include "JniHook.h"
#include "ArtMethod.h"


static struct {
    int api_level;
    unsigned int art_method_size;
    int art_flags_offset;
    int art_native_offset;

    jclass method_utils_class;
    jmethodID get_method_desc_id;
    jmethodID get_method_declaring_class_id;
    jmethodID get_method_name_id;

} HookEnv;

static const char *GetMethodDesc(JNIEnv *env, jobject javaMethod) {
    auto desc = reinterpret_cast<jstring>(env->CallStaticObjectMethod(HookEnv.method_utils_class,
                                                                      HookEnv.get_method_desc_id,
                                                                      javaMethod));
    return env->GetStringUTFChars(desc, JNI_FALSE);
}

static const char *GetMethodDeclaringClass(JNIEnv *env, jobject javaMethod) {
    auto desc = reinterpret_cast<jstring>(env->CallStaticObjectMethod(HookEnv.method_utils_class,
                                                                      HookEnv.get_method_declaring_class_id,
                                                                      javaMethod));
    return env->GetStringUTFChars(desc, JNI_FALSE);
}

static const char *GetMethodName(JNIEnv *env, jobject javaMethod) {
    auto desc = reinterpret_cast<jstring>(env->CallStaticObjectMethod(HookEnv.method_utils_class,
                                                                      HookEnv.get_method_name_id,
                                                                      javaMethod));
    return env->GetStringUTFChars(desc, JNI_FALSE);
}

inline static uint32_t GetAccessFlags(const char *art_method) {
    return *reinterpret_cast<const uint32_t *>(art_method + HookEnv.art_flags_offset);
}

inline static bool SetAccessFlags(char *art_method, uint32_t flags) {
    *reinterpret_cast<uint32_t *>(art_method + HookEnv.art_flags_offset) = flags;
    return true;
}

inline static bool AddAccessFlag(char *art_method, uint32_t flag) {
    uint32_t old_flag = GetAccessFlags(art_method);
    uint32_t new_flag = old_flag | flag;
    return new_flag != old_flag && SetAccessFlags(art_method, new_flag);
}

inline static bool ClearAccessFlag(char *art_method, uint32_t flag) {
    uint32_t old_flag = GetAccessFlags(art_method);
    uint32_t new_flag = old_flag & ~flag;
    return new_flag != old_flag && SetAccessFlags(art_method, new_flag);
}

inline static bool HasAccessFlag(char *art_method, uint32_t flag) {
    uint32_t flags = GetAccessFlags(art_method);
    return (flags & flag) == flag;
}

inline static bool ClearFastNativeFlag(char *art_method) {
    // FastNative
    return HookEnv.api_level < __ANDROID_API_P__ && ClearAccessFlag(art_method, kAccFastNative);
}

static void *GetArtMethod(JNIEnv *env, jclass clazz, jmethodID methodId) {
    if (HookEnv.api_level >= __ANDROID_API_Q__) {
        jclass executable = env->FindClass("java/lang/reflect/Executable");
        jfieldID artId = env->GetFieldID(executable, "artMethod", "J");
        jobject method = env->ToReflectedMethod(clazz, methodId, true);
        return reinterpret_cast<void *>(env->GetLongField(method, artId));
    } else {
        return methodId;
    }
}

bool CheckFlags(void *artMethod) {
    char *method = static_cast<char *>(artMethod);
    if (!HasAccessFlag(method, kAccNative)) {
        ALOGE("not native method");
        return false;
    }
    ClearFastNativeFlag(method);
    return true;
}

void JniHook::HookJniFun(JNIEnv *env, jobject java_method, void *new_fun,
                         void **orig_fun, bool is_static) {
    const char *class_name = GetMethodDeclaringClass(env, java_method);
    const char *method_name = GetMethodName(env, java_method);
    const char *sign = GetMethodDesc(env, java_method);
    HookJniFun(env, class_name, method_name, sign, new_fun, orig_fun, is_static);
}

void
JniHook::HookJniFun(JNIEnv *env, const char *class_name, const char *method_name, const char *sign,
                    void *new_fun, void **orig_fun, bool is_static) {
    if (HookEnv.art_native_offset == 0) {
        return;
    }
    jclass clazz = env->FindClass(class_name);
    if (!clazz) {
        ALOGD("findClass fail: %s %s", class_name, method_name);
        env->ExceptionClear();
        return;
    }
    jmethodID method = nullptr;
    if (is_static) {
        method = env->GetStaticMethodID(clazz, method_name, sign);
    } else {
        method = env->GetMethodID(clazz, method_name, sign);
    }
    if (!method) {
        env->ExceptionClear();
        ALOGD("get method id fail: %s %s", class_name, method_name);
        return;
    }
    JNINativeMethod gMethods[] = {
            {method_name, sign, (void *) new_fun},
    };

    auto artMethod = reinterpret_cast<uintptr_t *>(GetArtMethod(env, clazz, method));
    if (!CheckFlags(artMethod)) {
        ALOGE("check flags error. class：%s, method：%s", class_name, method_name);
        return;
    }
    *orig_fun = reinterpret_cast<void *>(artMethod[HookEnv.art_native_offset]);
    if (env->RegisterNatives(clazz, gMethods, 1) < 0) {
        ALOGE("jni hook error. class：%s, method：%s", class_name, method_name);
        return;
    }
    // FastNative
    if (HookEnv.api_level == __ANDROID_API_O__ || HookEnv.api_level == __ANDROID_API_O_MR1__) {
        AddAccessFlag((char *) artMethod, kAccFastNative);
    }
    ALOGD("register class：%s, method：%s success!", class_name, method_name);
}

__attribute__((section (".mytext")))  JNICALL void native_offset
        (JNIEnv *env, jclass obj) {
}

__attribute__((section (".mytext")))  JNICALL void native_offset2
        (JNIEnv *env, jclass obj) {
}

void registerNative(JNIEnv *env) {
    jclass clazz = env->FindClass("com/lody/virtual/jniHook/jniHook");
    JNINativeMethod gMethods[] = {
            {"nativeOffset",  "()V", (void *) native_offset},
            {"nativeOffset2", "()V", (void *) native_offset2},
    };
    if (env->RegisterNatives(clazz, gMethods, sizeof(gMethods) / sizeof(gMethods[0])) < 0) {
        ALOGE("jni register error.");
    }
}

void JniHook::InitJniHook(JNIEnv *env, int api_level) {
    registerNative(env);

    HookEnv.api_level = api_level;

    jclass clazz = env->FindClass("com/lody/virtual/jniHook/jniHook");
    jmethodID nativeOffsetId = env->GetStaticMethodID(clazz, "nativeOffset", "()V");
    jmethodID nativeOffset2Id = env->GetStaticMethodID(clazz, "nativeOffset2", "()V");

    void *nativeOffset = GetArtMethod(env, clazz, nativeOffsetId);
    void *nativeOffset2 = GetArtMethod(env, clazz, nativeOffset2Id);

    HookEnv.art_method_size = (size_t) nativeOffset2 - (size_t) nativeOffset;

    // calc native offset
    auto artMethod = reinterpret_cast<uintptr_t *>(nativeOffset);
    for (int i = 0; i < HookEnv.art_method_size; ++i) {
        if (reinterpret_cast<void *>(artMethod[i]) == native_offset) {
            HookEnv.art_native_offset = i;
            break;
        }
    }

    uint32_t flags = 0x0;
    flags = flags | kAccPublic;
    flags = flags | kAccStatic;
    flags = flags | kAccNative;
    if (api_level >= __ANDROID_API_Q__) {
        flags = flags | kAccPublicApi;
    }

    char *start = reinterpret_cast<char *>(artMethod);
    for (int i = 1; i < HookEnv.art_method_size; ++i) {
        auto value = *(uint32_t *) (start + i * sizeof(uint32_t));
        if (value == flags) {
            HookEnv.art_flags_offset = i * sizeof(uint32_t);
            break;
        }
    }

    HookEnv.method_utils_class = env->FindClass("com/lody/virtual/jniHook/MethodUtils");
    HookEnv.get_method_desc_id = env->GetStaticMethodID(HookEnv.method_utils_class, "getDesc",
                                                        "(Ljava/lang/reflect/Method;)Ljava/lang/String;");
    HookEnv.get_method_declaring_class_id = env->GetStaticMethodID(HookEnv.method_utils_class,
                                                                   "getDeclaringClass",
                                                                   "(Ljava/lang/reflect/Method;)Ljava/lang/String;");
    HookEnv.get_method_name_id = env->GetStaticMethodID(HookEnv.method_utils_class, "getMethodName",
                                                        "(Ljava/lang/reflect/Method;)Ljava/lang/String;");
}
