//
// VirtualApp Native Project
//

#ifndef NDK_CORE_H
#define NDK_CORE_H

#include <jni.h>
#include <stdlib.h>


#include "Helper.h"
#include "Foundation/IORelocator.h"
#include "Foundation/VMHook.h"

#define JNI_CLASS_NAME "com/lody/virtual/client/NativeEngine"

extern jclass nativeEngineClass;
extern JavaVM * vm;

JNIEnv *getEnv();

JNIEnv *ensureEnvCreated();


JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *vm, void *reserved);
JNIEXPORT void JNICALL JNI_OnUnload(JavaVM* vm, void* reserved);


#endif //NDK_CORE_H
