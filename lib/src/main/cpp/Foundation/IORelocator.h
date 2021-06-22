//
// VirtualApp Native Project
//

#ifndef NDK_HOOK_H
#define NDK_HOOK_H


#include <string>
#include <map>
#include <list>
#include <jni.h>
#include <dlfcn.h>
#include <stddef.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/syscall.h>

#include "Jni/Helper.h"

#define ANDROID_K 19
#define ANDROID_L 21
#define ANDROID_L2 22
#define ANDROID_M 23
#define ANDROID_N 24
#define ANDROID_N2 25
#define ANDROID_O 26
#define ANDROID_O2 27
#define ANDROID_P 28
//could not 29
#define ANDROID_Q 29
typedef struct Segment{
    void *startAddress;
    void *endAddress;
    off64_t offset;
    int permission;

    Segment(void *startAddress, void *endAddress, off64_t offset, int permission)
            : startAddress(startAddress), endAddress(endAddress), offset(offset),
              permission(permission) {}
} Segment;
#define HOOK_SYMBOL(handle, func) hook_function(handle, #func, (void*) new_##func, (void**) &orig_##func)
#define HOOK_TENCENT(handle, func) hook_function(handle, #func, (void*) new_##func, (void**) &orig_##func)
#define HOOK_REGISTER(handle, func) hookByHandle(handle, #func, (void*) new_##func, (void**) &old_##func)
#undef PTR_DEF(ret, func, ...)
#define PTR_DEF(ret, func, ...) static ret (*func)(__VA_ARGS__)
#define HOOK_DEF(ret, func, ...) \
  ret (*orig_##func)(__VA_ARGS__); \
  ret new_##func(__VA_ARGS__)
#define HOOK_SYSCALL(syscall_name) \
case __NR_##syscall_name: \
MSHookFunction(func, (void *) new_##syscall_name, (void **) &orig_##syscall_name); \
pass++; \
break;

#define HOOK_SYSCALL_(syscall_name) \
case __NR_##syscall_name: \
MSHookFunction(func, (void *) new___##syscall_name, (void **) &orig___##syscall_name); \
pass++; \
break;

#define HOOK_DEF(ret, func, ...) \
  ret (*orig_##func)(__VA_ARGS__); \
  ret new_##func(__VA_ARGS__)

#define CHECK_NE(a, b) \
  if ((a) == (b)) abort();
void MSHookFunctionSafe(void *symbol, void *new_func, void **old_func);
PTR_DEF(void,hookByAddress,void *symbol, void *replace, void **result);
#define FIND_PTR(handle,name,func) findPtr(handle,name,(void**)&func)
/*#define HOOK_ADDR(base,address, func) hookByAddress((void *)((char*)(base)+(address)), (void *) new_##func,(void **) &orig_##func)
static void hookByAddress(void *symbol, void *replace, void **result,const char* name){
  MSHookFunctionSafe(symbol,replace,result);
}*/
namespace IOUniformer {

    void init_env_before_all();

    void startUniformer(JNIEnv *env, const char *so_path, const char *so_path_64, const char *native_path,
                        int api_level, int preview_api_level, bool hook_dlopen, bool skip_kill);

    void relocate(const char *orig_path, const char *new_path);

    void whitelist(const char *path);

    const char *query(const char *orig_path, char *const buffer, const size_t size);

    const char *reverse(const char *redirected_path, char *const buffer, const size_t size);

    void forbid(const char *path);

    void readOnly(const char *path);
}

#endif //NDK_HOOK_H
