//
// VirtualApp Native Project
//
#include <unistd.h>
#include <sys/ptrace.h>
#include <Substrate/CydiaSubstrate.h>
#include <Jni/VAJni.h>
#include <sys/stat.h>
#include <syscall.h>
#include <Foundation/syscall/BinarySyscallFinder.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <sys/user.h>
#include <pthread.h>
#include <vector>
#include <sys/socket.h>
#include <unordered_set>
#include <asm/mman.h>
#include <sys/mman.h>
#include <asm/ioctl.h>
#include <syscall.h>
#include <dlfcn.h>
#include <dirent.h>
#include <android/fdsan.h>
#include<stdio.h>
#include<inttypes.h>

#ifdef __i386__
#include <unistd.h>

#elif defined(__ILP32__)
#include <unistd.h>
#else
//#include <unistd_64.h>
#include "unistd_32.h"
#endif

#include "IORelocator.h"
#include "SandboxFs.h"
#include "canonicalize_md.h"
#include "Symbol.h"
#include "Log.h"
#include "VMHook.h"
#include "MapsRedirector.h"

#define ulong unsigned long
typedef unsigned char u1;
typedef unsigned short u2;
typedef unsigned int u4;
typedef unsigned long u8;


// 内存偏移
template <class T>
inline T MemoryOff(void *addr, ulong off) {
    return (T)((char *)addr + off);
}

// 提取B指令的偏移
inline ulong BxxExtract(void* symbol)
{
#if defined(__arm__)
    return static_cast<ulong>((*static_cast<int32_t*>(symbol) << 0x8 >> 0x6));
#elif defined(__arm64__) || defined(__aarch64__)
    return static_cast<ulong>((*static_cast<int32_t*>(symbol) << 0x6 >> 0x4));
#else
#endif
}

// 修正B指令转跳
template<typename R>
inline R Amend_Bxx(R symbol, ulong off = 0ul)
{
    symbol = MemoryOff<R>(reinterpret_cast<void*>(symbol), off);
#if defined(__arm__)
    return MemoryOff<R>(reinterpret_cast<void*>(symbol),
                BxxExtract(reinterpret_cast<void*>(symbol)) + 0x8);
#elif defined(__arm64__) || defined(__aarch64__)
    return MemoryOff<R>(reinterpret_cast<void*>(symbol),
                        BxxExtract(reinterpret_cast<void*>(symbol)));
#else
#endif
}


void startIOHook(JNIEnv *env, int api_level, bool hook_dlopen);

bool need_load_env = true;
bool skip_kill = false;
bool debug_kill = false;
bool execve_process = false;

int g_preview_api_level = 0;
int g_api_level = 0;

int inline getArrayItemCount(char *const array[]) {
    int i;
    for (i = 0; array[i]; ++i);
    return i;
}

std::vector<std::string> Split(const std::string& s,
                               const std::string& delimiters) {
    CHECK_NE(delimiters.size(), 0U);

    std::vector<std::string> result;

    size_t base = 0;
    size_t found;
    while (true) {
        found = s.find_first_of(delimiters, base);
        result.push_back(s.substr(base, found - base));
        if (found == s.npos) break;
        base = found + 1;
    }

    return std::move(result);
}


char *get_process_name() {
    char *cmdline = (char *) calloc(0x400u, 1u);
    if (cmdline) {
        FILE *file = fopen("/proc/self/cmdline", "r");
        if (file) {
            int count = fread(cmdline, 1u, 0x400u, file);
            if (count) {
                if (cmdline[count - 1] == '\n') {
                    cmdline[count - 1] = '\0';
                }
            }
            fclose(file);
        } else {
            ALOGE("fail open cmdline.");
        }
    }
    return cmdline;
}

void IOUniformer::init_env_before_all() {
    if (!need_load_env) {
        return;
    }
    need_load_env = false;
    char *ld_preload = getenv("LD_PRELOAD");
    if (!ld_preload || !strstr(ld_preload, CORE_SO_NAME)) {
        return;
    }
    execve_process = true;
    char *process_name = get_process_name();
    ALOGE("Start init env : %s", process_name);
    free(process_name);
    char src_key[KEY_MAX];
    char dst_key[KEY_MAX];
    int i = 0;
    while (true) {
        memset(src_key, 0, sizeof(src_key));
        memset(dst_key, 0, sizeof(dst_key));
        sprintf(src_key, "V_REPLACE_ITEM_SRC_%d", i);
        sprintf(dst_key, "V_REPLACE_ITEM_DST_%d", i);
        char *src_value = getenv(src_key);
        if (!src_value) {
            break;
        }
        char *dst_value = getenv(dst_key);
        add_replace_item(src_value, dst_value);
        i++;
    }
    i = 0;
    while (true) {
        memset(src_key, 0, sizeof(src_key));
        sprintf(src_key, "V_KEEP_ITEM_%d", i);
        char *keep_value = getenv(src_key);
        if (!keep_value) {
            break;
        }
        add_keep_item(keep_value);
        i++;
    }
    i = 0;
    while (true) {
        memset(src_key, 0, sizeof(src_key));
        sprintf(src_key, "V_FORBID_ITEM_%d", i);
        char *forbid_value = getenv(src_key);
        if (!forbid_value) {
            break;
        }
        add_forbidden_item(forbid_value);
        i++;
    }
    char *api_level_char = getenv("V_API_LEVEL");
    char *preview_api_level_chars = getenv("V_PREVIEW_API_LEVEL");
    if (api_level_char != NULL) {
        int api_level = atoi(api_level_char);
        g_api_level = api_level;
        int preview_api_level;
        preview_api_level = atoi(preview_api_level_chars);
        g_preview_api_level = preview_api_level;
        startIOHook(nullptr, api_level, true);
    }
}

static inline void
hook_function(void *handle, const char *symbol, void *new_func, void **old_func);

void MSHookFunctionSafe(void *symbol, void *new_func, void **old_func) {
    if (symbol == nullptr or symbol == reinterpret_cast<void *>(0xFFFFFFFF)) {
        ALOGE("hook error!");
        return;
    }
    MSHookFunction(symbol, new_func, old_func);
}




void onSoLoaded(const char *name, void *handle);

void IOUniformer::relocate(const char *orig_path, const char *new_path) {
    add_replace_item(orig_path, new_path);
}

const char *IOUniformer::query(const char *orig_path, char *const buffer, const size_t size) {
    return relocate_path(orig_path, buffer, size);
}

void IOUniformer::whitelist(const char *_path) {
    add_keep_item(_path);
}

void IOUniformer::forbid(const char *_path) {
    add_forbidden_item(_path);
}

void IOUniformer::readOnly(const char *_path) {
    add_readonly_item(_path);
}

const char *IOUniformer::reverse(const char *_path, char *const buffer, const size_t size) {
    return reverse_relocate_path(_path, buffer, size);
}


__BEGIN_DECLS

// int faccessat(int dirfd, const char *pathname, int mode, int flags);
HOOK_DEF(int, faccessat, int dirfd, const char *pathname, int mode, int flags) {
    char temp[PATH_MAX];
    const char *relocated_path = relocate_path(pathname, temp, sizeof(temp));
    if (relocated_path && !(mode & W_OK && isReadOnly(relocated_path))) {
        return syscall(__NR_faccessat, dirfd, relocated_path, mode, flags);
    }
    errno = EACCES;
    return -1;
}

// int fchmodat(int dirfd, const char *pathname, mode_t mode, int flags);
HOOK_DEF(int, fchmodat, int dirfd, const char *pathname, mode_t mode, int flags) {
    char temp[PATH_MAX];
    const char *relocated_path = relocate_path(pathname, temp, sizeof(temp));
    if (__predict_true(relocated_path)) {
        return syscall(__NR_fchmodat, dirfd, relocated_path, mode, flags);
    }
    errno = EACCES;
    return -1;
}

// int fstatat64(int dirfd, const char *pathname, struct stat *buf, int flags);
HOOK_DEF(int, fstatat64, int dirfd, const char *pathname, struct stat *buf, int flags) {
    char temp[PATH_MAX];
    const char *relocated_path = relocate_path(pathname, temp, sizeof(temp));
    if (__predict_true(relocated_path)) {
        long ret;
#if defined(__arm__) || defined(__i386__)
        ret = syscall(__NR_fstatat64, dirfd, relocated_path, buf, flags);
#else
        ret = syscall(__NR_newfstatat, dirfd, relocated_path, buf, flags);
#endif
        return ret;
    }
    errno = EACCES;
    return -1;
}

// int kill(pid_t pid, int sig);
HOOK_DEF(int, kill, pid_t pid, int sig) {
    ALOGE("kill >>> pid : %d, sig : %d", pid, sig);
    if (debug_kill && sig == 9) {
        abort();
    }
    if (skip_kill)
        return 1;
    return syscall(__NR_kill, pid, sig);
}


// int exit(__status);
HOOK_DEF(int, exit, int __status) {
    ALOGE("exit >>> __status : %d", __status);
    return syscall(__NR_exit, __status);
}
/*//ssize_t send(int sockfd, const void *buff, size_t nbytes, int flags);
HOOK_DEF(ssize_t, send, int sockfd, const void *buff, size_t nbytes, int flags) {
    ALOGE("send >>> sockfd : %d, buff : %d, nbytes : %d, flags : %d",sockfd,buff,nbytes,flags);
    return syscall(__NR_sendto, sockfd,buff,nbytes,flags);
}*/

//ssize_t recv(int sockfd, const void *buff, size_t nbytes, int flags);
HOOK_DEF(ssize_t, recv, int sockfd, const void *buff, size_t nbytes, int flags) {
    ALOGE("recv >>> sockfd : %d, buff : %d, nbytes : %d, flags : %d",sockfd,buff,nbytes,flags);
    if (flags == 16384){
        return 0;
    }
    return orig_recv(sockfd,buff,nbytes,flags);
}

#ifndef __LP64__

// int __statfs64(const char *path, size_t size, struct statfs *stat);
HOOK_DEF(int, __statfs64, const char *pathname, size_t size, struct statfs *stat) {
    char temp[PATH_MAX];
    const char *relocated_path = relocate_path(pathname, temp, sizeof(temp));
    if (__predict_true(relocated_path)) {
        return syscall(__NR_statfs64, relocated_path, size, stat);
    }
    errno = EACCES;
    return -1;
}

// int __open(const char *pathname, int flags, int mode);
HOOK_DEF(int, __open, const char *pathname, int flags, int mode) {
    char temp[PATH_MAX];
    const char *relocated_path = relocate_path(pathname, temp, sizeof(temp));
    if (relocated_path && !((flags & O_WRONLY || flags & O_RDWR) && isReadOnly(relocated_path))) {
        int fake_fd = redirect_proc_maps(relocated_path, flags, mode);
        if (fake_fd != 0) {
            return fake_fd;
        }
        return syscall(__NR_open, relocated_path, flags, mode);
    }
    errno = EACCES;
    return -1;
}




// ssize_t readlink(const char *path, char *buf, size_t bufsiz);
HOOK_DEF(ssize_t, readlink, const char *pathname, char *buf, size_t bufsiz) {
    char temp[PATH_MAX];
    const char *relocated_path = relocate_path(pathname, temp, sizeof(temp));
    if (__predict_true(relocated_path)) {
        long ret = syscall(__NR_readlink, relocated_path, buf, bufsiz);
        if (ret < 0) {
            return ret;
        } else {
            // relocate link content
            if (reverse_relocate_path_inplace(buf, bufsiz) != -1) {
                return ret;
            }
        }
    }
    errno = EACCES;
    return -1;
}

// int mkdir(const char *pathname, mode_t mode);
HOOK_DEF(int, mkdir, const char *pathname, mode_t mode) {
    char temp[PATH_MAX];
    const char *relocated_path = relocate_path(pathname, temp, sizeof(temp));
    if (__predict_true(relocated_path)) {
        return syscall(__NR_mkdir, relocated_path, mode);
    }
    errno = EACCES;
    return -1;
}

// int rmdir(const char *pathname);
HOOK_DEF(int, rmdir, const char *pathname) {
    char temp[PATH_MAX];
    const char *relocated_path = relocate_path(pathname, temp, sizeof(temp));
    if (__predict_true(relocated_path)) {
        return syscall(__NR_rmdir, relocated_path);
    }
    errno = EACCES;
    return -1;
}

// int lchown(const char *pathname, uid_t owner, gid_t group);
HOOK_DEF(int, lchown, const char *pathname, uid_t owner, gid_t group) {
    char temp[PATH_MAX];
    const char *relocated_path = relocate_path(pathname, temp, sizeof(temp));
    if (__predict_true(relocated_path)) {
        return syscall(__NR_lchown, relocated_path, owner, group);
    }
    errno = EACCES;
    return -1;
}

// int utimes(const char *filename, const struct timeval *tvp);
HOOK_DEF(int, utimes, const char *pathname, const struct timeval *tvp) {
    char temp[PATH_MAX];
    const char *relocated_path = relocate_path(pathname, temp, sizeof(temp));
    if (__predict_true(relocated_path)) {
        return syscall(__NR_utimes, relocated_path, tvp);
    }
    errno = EACCES;
    return -1;
}

// int link(const char *oldpath, const char *newpath);
HOOK_DEF(int, link, const char *oldpath, const char *newpath) {
    char temp[PATH_MAX];
    const char *relocated_path_old = relocate_path(oldpath, temp, sizeof(temp));
    if (relocated_path_old) {
        return syscall(__NR_link, relocated_path_old, newpath);
    }
    errno = EACCES;
    return -1;
}

// int access(const char *pathname, int mode);
HOOK_DEF(int, access, const char *pathname, int mode) {
    char temp[PATH_MAX];
    const char *relocated_path = relocate_path(pathname, temp, sizeof(temp));
    if (relocated_path && !(mode & W_OK && isReadOnly(relocated_path))) {
        return syscall(__NR_access, relocated_path, mode);
    }
    errno = EACCES;
    return -1;
}

// int chmod(const char *path, mode_t mode);
HOOK_DEF(int, chmod, const char *pathname, mode_t mode) {
    char temp[PATH_MAX];
    const char *relocated_path = relocate_path(pathname, temp, sizeof(temp));
    if (__predict_true(relocated_path)) {
        return syscall(__NR_chmod, relocated_path, mode);
    }
    errno = EACCES;
    return -1;
}

// int chown(const char *path, uid_t owner, gid_t group);
HOOK_DEF(int, chown, const char *pathname, uid_t owner, gid_t group) {
    char temp[PATH_MAX];
    const char *relocated_path = relocate_path(pathname, temp, sizeof(temp));
    if (__predict_true(relocated_path)) {
        return syscall(__NR_chown, relocated_path, owner, group);
    }
    errno = EACCES;
    return -1;
}

// int lstat(const char *path, struct stat *buf);
HOOK_DEF(int, lstat, const char *pathname, struct stat *buf) {
    char temp[PATH_MAX];
    const char *relocated_path = relocate_path(pathname, temp, sizeof(temp));
    if (__predict_true(relocated_path)) {
        return syscall(__NR_lstat64, relocated_path, buf);
    }
    errno = EACCES;
    return -1;
}

// int stat(const char *path, struct stat *buf);
HOOK_DEF(int, stat, const char *pathname, struct stat *buf) {
    char temp[PATH_MAX];
    const char *relocated_path = relocate_path(pathname, temp, sizeof(temp));
    if (__predict_true(relocated_path)) {
        long ret = syscall(__NR_stat64, relocated_path, buf);
        if (isReadOnly(relocated_path)) {
            buf->st_mode &= ~S_IWGRP;
        }
        return ret;
    }
    errno = EACCES;
    return -1;
}

// int symlink(const char *oldpath, const char *newpath);
HOOK_DEF(int, symlink, const char *oldpath, const char *newpath) {
    char temp[PATH_MAX];
    const char *relocated_path_old = relocate_path(oldpath, temp, sizeof(temp));
    if (relocated_path_old) {
        return syscall(__NR_symlink, relocated_path_old, newpath);
    }
    errno = EACCES;
    return -1;
}

// int unlink(const char *pathname);
HOOK_DEF(int, unlink, const char *pathname) {
    char temp[PATH_MAX];
    const char *relocated_path = relocate_path(pathname, temp, sizeof(temp));
    if (relocated_path && !isReadOnly(relocated_path)) {
        return syscall(__NR_unlink, relocated_path);
    }
    errno = EACCES;
    return -1;
}

// int fchmod(const char *pathname, mode_t mode);
HOOK_DEF(int, fchmod, const char *pathname, mode_t mode) {
    char temp[PATH_MAX];
    const char *relocated_path = relocate_path(pathname, temp, sizeof(temp));
    if (__predict_true(relocated_path)) {
        return syscall(__NR_fchmod, relocated_path, mode);
    }
    errno = EACCES;
    return -1;
}


// int fstatat(int dirfd, const char *pathname, struct stat *buf, int flags);
HOOK_DEF(int, fstatat, int dirfd, const char *pathname, struct stat *buf, int flags) {
    char temp[PATH_MAX];
    const char *relocated_path = relocate_path(pathname, temp, sizeof(temp));
    if (__predict_true(relocated_path)) {
        return syscall(__NR_fstatat64, dirfd, relocated_path, buf, flags);
    }
    errno = EACCES;
    return -1;
}

// int fstat(const char *pathname, struct stat *buf, int flags);
HOOK_DEF(int, fstat, const char *pathname, struct stat *buf) {
    char temp[PATH_MAX];
    const char *relocated_path = relocate_path(pathname, temp, sizeof(temp));
    if (__predict_true(relocated_path)) {
        return syscall(__NR_fstat64, relocated_path, buf);
    }
    errno = EACCES;
    return -1;
}

// int mknod(const char *pathname, mode_t mode, dev_t dev);
HOOK_DEF(int, mknod, const char *pathname, mode_t mode, dev_t dev) {
    char temp[PATH_MAX];
    const char *relocated_path = relocate_path(pathname, temp, sizeof(temp));
    if (__predict_true(relocated_path)) {
        return syscall(__NR_mknod, relocated_path, mode, dev);
    }
    errno = EACCES;
    return -1;
}

// int rename(const char *oldpath, const char *newpath);
HOOK_DEF(int, rename, const char *oldpath, const char *newpath) {
    char temp_old[PATH_MAX], temp_new[PATH_MAX];
    const char *relocated_path_old = relocate_path(oldpath, temp_old, sizeof(temp_old));
    const char *relocated_path_new = relocate_path(newpath, temp_new, sizeof(temp_new));
    if (relocated_path_old && relocated_path_new) {
        return syscall(__NR_rename, relocated_path_old, relocated_path_new);
    }
    errno = EACCES;
    return -1;
}

#endif


// int mknodat(int dirfd, const char *pathname, mode_t mode, dev_t dev);
HOOK_DEF(int, mknodat, int dirfd, const char *pathname, mode_t mode, dev_t dev) {
    char temp[PATH_MAX];
    const char *relocated_path = relocate_path(pathname, temp, sizeof(temp));
    if (__predict_true(relocated_path)) {
        return syscall(__NR_mknodat, dirfd, relocated_path, mode, dev);
    }
    errno = EACCES;
    return -1;
}

// int utimensat(int dirfd, const char *pathname, const struct timespec times[2], int flags);
HOOK_DEF(int, utimensat, int dirfd, const char *pathname, const struct timespec times[2],
         int flags) {
    char temp[PATH_MAX];
    const char *relocated_path = relocate_path(pathname, temp, sizeof(temp));
    if (__predict_true(relocated_path)) {
        return syscall(__NR_utimensat, dirfd, relocated_path, times, flags);
    }
    errno = EACCES;
    return -1;
}

// int fchownat(int dirfd, const char *pathname, uid_t owner, gid_t group, int flags);
HOOK_DEF(int, fchownat, int dirfd, const char *pathname, uid_t owner, gid_t group, int flags) {
    char temp[PATH_MAX];
    const char *relocated_path = relocate_path(pathname, temp, sizeof(temp));
    if (__predict_true(relocated_path)) {
        return syscall(__NR_fchownat, dirfd, relocated_path, owner, group, flags);
    }
    errno = EACCES;
    return -1;
}

// int chroot(const char *pathname);
HOOK_DEF(int, chroot, const char *pathname) {
    char temp[PATH_MAX];
    const char *relocated_path = relocate_path(pathname, temp, sizeof(temp));
    if (__predict_true(relocated_path)) {
        return syscall(__NR_chroot, relocated_path);
    }
    errno = EACCES;
    return -1;
}

// int renameat(int olddirfd, const char *oldpath, int newdirfd, const char *newpath);
HOOK_DEF(int, renameat, int olddirfd, const char *oldpath, int newdirfd, const char *newpath) {
    char temp_old[PATH_MAX], temp_new[PATH_MAX];
    const char *relocated_path_old = relocate_path(oldpath, temp_old, sizeof(temp_old));
    const char *relocated_path_new = relocate_path(newpath, temp_new, sizeof(temp_new));
    if (relocated_path_old && relocated_path_new) {
        return syscall(__NR_renameat, olddirfd, relocated_path_old, newdirfd,
                       relocated_path_new);
    }
    errno = EACCES;
    return -1;
}

// int statfs64(const char *__path, struct statfs64 *__buf) __INTRODUCED_IN(21);
HOOK_DEF(int, statfs64, const char *filename, struct statfs64 *buf) {
    char temp[PATH_MAX];
    const char *relocated_path = relocate_path(filename, temp, sizeof(temp));
    if (__predict_true(relocated_path)) {
        return syscall(__NR_statfs, relocated_path, buf);
    }
    errno = EACCES;
    return -1;
}

// int unlinkat(int dirfd, const char *pathname, int flags);
HOOK_DEF(int, unlinkat, int dirfd, const char *pathname, int flags) {
    char temp[PATH_MAX];
    const char *relocated_path = relocate_path(pathname, temp, sizeof(temp));
    if (relocated_path && !isReadOnly(relocated_path)) {
        return syscall(__NR_unlinkat, dirfd, relocated_path, flags);
    }
    errno = EACCES;
    return -1;
}

// int symlinkat(const char *oldpath, int newdirfd, const char *newpath);
HOOK_DEF(int, symlinkat, const char *oldpath, int newdirfd, const char *newpath) {
    char temp[PATH_MAX];
    const char *relocated_path_old = relocate_path(oldpath, temp, sizeof(temp));
    if (relocated_path_old) {
        return syscall(__NR_symlinkat, relocated_path_old, newdirfd, newpath);
    }
    errno = EACCES;
    return -1;
}

// int linkat(int olddirfd, const char *oldpath, int newdirfd, const char *newpath, int flags);
HOOK_DEF(int, linkat, int olddirfd, const char *oldpath, int newdirfd, const char *newpath,
         int flags) {
    char temp[PATH_MAX];
    const char *relocated_path_old = relocate_path(oldpath, temp, sizeof(temp));
    if (relocated_path_old) {
        return syscall(__NR_linkat, olddirfd, relocated_path_old, newdirfd, newpath,
                       flags);
    }
    errno = EACCES;
    return -1;
}

// int mkdirat(int dirfd, const char *pathname, mode_t mode);
HOOK_DEF(int, mkdirat, int dirfd, const char *pathname, mode_t mode) {
    char temp[PATH_MAX];
    const char *relocated_path = relocate_path(pathname, temp, sizeof(temp));
    if (__predict_true(relocated_path)) {
        return syscall(__NR_mkdirat, dirfd, relocated_path, mode);
    }
    errno = EACCES;
    return -1;
}

// int readlinkat(int dirfd, const char *pathname, char *buf, size_t bufsiz);
HOOK_DEF(int, readlinkat, int dirfd, const char *pathname, char *buf, size_t bufsiz) {
    char temp[PATH_MAX];
    const char *relocated_path = relocate_path(pathname, temp, sizeof(temp));
    if (__predict_true(relocated_path)) {
        long ret = syscall(__NR_readlinkat, dirfd, relocated_path, buf, bufsiz);
        if (ret < 0) {
            return ret;
        } else {
            // relocate link content
            if (reverse_relocate_path_inplace(buf, bufsiz) != -1) {
                return ret;
            }
        }
    }
    errno = EACCES;
    return -1;
}


// int truncate(const char *path, off_t length);
HOOK_DEF(int, truncate, const char *pathname, off_t length) {
    char temp[PATH_MAX];
    const char *relocated_path = relocate_path(pathname, temp, sizeof(temp));
    if (__predict_true(relocated_path)) {
        return syscall(__NR_truncate, relocated_path, length);
    }
    errno = EACCES;
    return -1;
}

// int chdir(const char *path);
HOOK_DEF(int, chdir, const char *pathname) {
    char temp[PATH_MAX];
    const char *relocated_path = relocate_path(pathname, temp, sizeof(temp));
    if (__predict_true(relocated_path)) {
        return syscall(__NR_chdir, relocated_path);
    }
    errno = EACCES;
    return -1;
}

// int __getcwd(char *buf, size_t size);
HOOK_DEF(int, __getcwd, char *buf, size_t size) {
    long ret = syscall(__NR_getcwd, buf, size);
    if (!ret) {
        if (reverse_relocate_path_inplace(buf, size) < 0) {
            errno = EACCES;
            return -1;
        }
    }
    return ret;
}

// int __openat(int fd, const char *pathname, int flags, int mode);
HOOK_DEF(int, __openat, int fd, const char *pathname, int flags, int mode) {
    char temp[PATH_MAX];
    const char *relocated_path = relocate_path(pathname, temp, sizeof(temp));
    if (__predict_true(relocated_path)) {
//        int fake_fd = redirect_proc_maps(relocated_path, flags, mode);
//        if (fake_fd != 0) {
//            return fake_fd;
//        }
        return syscall(__NR_openat, fd, relocated_path, flags, mode);
    }
    errno = EACCES;
    return -1;
}

// int __statfs (__const char *__file, struct statfs *__buf);
HOOK_DEF(int, __statfs, __const char *__file, struct statfs *__buf) {
    char temp[PATH_MAX];
    const char *relocated_path = relocate_path(__file, temp, sizeof(temp));
    if (__predict_true(relocated_path)) {
        return syscall(__NR_statfs, relocated_path, __buf);
    }
    errno = EACCES;
    return -1;
}

static struct sigaction old_sig_act{};
HOOK_DEF(int, sigaction, int sig, struct sigaction *new_act, struct sigaction *old_act) {
    if (sig != SIGABRT) {
        return orig_sigaction(sig, new_act, old_act);
    } else {
        if (old_act) {
            *old_act = old_sig_act;
        }
//        if (new_act) {
//            old_sig_act = *new_act;
//        }
        return 0;
    }
}

HOOK_DEF(long,syscall,int number,...){
    long long stack[8];
    va_list args;
    va_start(args, number);
    if (number == __NR_open) {
        auto pathname = va_arg(args, const char *);
        auto flags = va_arg(args, int);
        auto mode = va_arg(args, int);
        va_end(args);
        return open(pathname, flags, mode);
    } else {
        if (number == __NR_openat) {
            auto fd = va_arg(args, int);
            auto pathname = va_arg(args, const char *);
            auto flags = va_arg(args, int);
            auto mode = va_arg(args, int);
            va_end(args);
            return openat(fd, pathname, flags, mode);
        }
    }
    memcpy(stack, (void*)*(int64_t*)&args, 8 * 8);
    va_end(args);
    return syscall(number, stack[0], stack[1], stack[2], stack[3], stack[4],
                   stack[5], stack[6], stack[7]);
}

static char **relocate_envp(const char *pathname, char *const envp[]) {
    if (strstr(pathname, "libweexjsb.so")) {
        return const_cast<char **>(envp);
    }
    char *soPath = getenv("V_SO_PATH");
    char *soPath64 = getenv("V_SO_PATH_64");

    char *env_so_path = NULL;
    FILE *fd = fopen(pathname, "r");
    if (!fd) {
        return const_cast<char **>(envp);
    }
    for (int i = 0; i < 4; ++i) {
        fgetc(fd);
    }
    int type = fgetc(fd);
    if (type == ELFCLASS32) {
        env_so_path = soPath;
    } else if (type == ELFCLASS64) {
        env_so_path = soPath64;
    }
    fclose(fd);
    if (env_so_path == NULL) {
        return const_cast<char **>(envp);
    }
    int len = 0;
    int ld_preload_index = -1;
    int self_so_index = -1;
    while (envp[len]) {
        /* find LD_PRELOAD element */
        if (ld_preload_index == -1 && !strncmp(envp[len], "LD_PRELOAD=", 11)) {
            ld_preload_index = len;
        }
        if (self_so_index == -1 && !strncmp(envp[len], "V_SO_PATH=", 10)) {
            self_so_index = len;
        }
        ++len;
    }
    /* append LD_PRELOAD element */
    if (ld_preload_index == -1) {
        ++len;
    }
    /* append V_env element */
    if (self_so_index == -1) {
        // V_SO_PATH
        // V_API_LEVEL
        // V_PREVIEW_API_LEVEL
        // V_NATIVE_PATH
        len += 4;
        if (soPath64) {
            // V_SO_PATH_64
            len++;
        }
        len += get_keep_item_count();
        len += get_forbidden_item_count();
        len += get_replace_item_count() * 2;
    }

    /* append NULL element */
    ++len;

    char **relocated_envp = (char **) malloc(len * sizeof(char *));
    memset(relocated_envp, 0, len * sizeof(char *));
    for (int i = 0; envp[i]; ++i) {
        if (i != ld_preload_index) {
            relocated_envp[i] = strdup(envp[i]);
        }
    }
    char LD_PRELOAD_VARIABLE[PATH_MAX];
    if (ld_preload_index == -1) {
        ld_preload_index = len - 2;
        sprintf(LD_PRELOAD_VARIABLE, "LD_PRELOAD=%s", env_so_path);
    } else {
        const char *orig_ld_preload = envp[ld_preload_index] + 11;
        // remove old preload va
        std::vector<std::string> paths;
        paths = Split(std::string(orig_ld_preload), ":");
        orig_ld_preload = nullptr;
        if (paths.size() > 0) {
            std::string new_ld_path_str;
            for (auto path : paths) {
                if (path.compare(soPath) != 0 && path.compare(soPath64) != 0) {
                    new_ld_path_str += path;
                    new_ld_path_str += ":";
                }
            }
            if (!new_ld_path_str.empty()) {
                orig_ld_preload = strdup(new_ld_path_str.c_str());
            }
        }
        if (orig_ld_preload) {
            sprintf(LD_PRELOAD_VARIABLE, "LD_PRELOAD=%s:%s", env_so_path, orig_ld_preload);
        } else {
            sprintf(LD_PRELOAD_VARIABLE, "LD_PRELOAD=%s", env_so_path);
        }
    }
    relocated_envp[ld_preload_index] = strdup(LD_PRELOAD_VARIABLE);
    int index = 0;
    while (relocated_envp[index]) index++;
    if (self_so_index == -1) {
        char element[PATH_MAX] = {0};
        sprintf(element, "V_SO_PATH=%s", soPath);
        relocated_envp[index++] = strdup(element);
        if (soPath64) {
            sprintf(element, "V_SO_PATH_64=%s", soPath64);
            relocated_envp[index++] = strdup(element);
        }
        sprintf(element, "V_API_LEVEL=%s", getenv("V_API_LEVEL"));
        relocated_envp[index++] = strdup(element);
        sprintf(element, "V_PREVIEW_API_LEVEL=%s", getenv("V_PREVIEW_API_LEVEL"));
        relocated_envp[index++] = strdup(element);
        sprintf(element, "V_NATIVE_PATH=%s", getenv("V_NATIVE_PATH"));
        relocated_envp[index++] = strdup(element);

        for (int i = 0; i < get_keep_item_count(); ++i) {
            PathItem &item = get_keep_items()[i];
            char env[PATH_MAX] = {0};
            sprintf(env, "V_KEEP_ITEM_%d=%s", i, item.path);
            relocated_envp[index++] = strdup(env);
        }

        for (int i = 0; i < get_forbidden_item_count(); ++i) {
            PathItem &item = get_forbidden_items()[i];
            char env[PATH_MAX] = {0};
            sprintf(env, "V_FORBID_ITEM_%d=%s", i, item.path);
            relocated_envp[index++] = strdup(env);
        }

        for (int i = 0; i < get_replace_item_count(); ++i) {
            ReplaceItem &item = get_replace_items()[i];
            char src[PATH_MAX] = {0};
            char dst[PATH_MAX] = {0};
            sprintf(src, "V_REPLACE_ITEM_SRC_%d=%s", i, item.orig_path);
            sprintf(dst, "V_REPLACE_ITEM_DST_%d=%s", i, item.new_path);
            relocated_envp[index++] = strdup(src);
            relocated_envp[index++] = strdup(dst);
        }
    }
    return relocated_envp;
}

void hook_function(void *handle, const char *symbol, void *new_func, void **old_func) {
    void *addr = dlsym(handle, symbol);
    if (addr == NULL) {
        ALOGE("Not found symbol : %s", symbol);
        return;
    }
    MSHookFunction(addr, new_func, old_func);
}

static inline void hookByHandle(void *handle, const char *symbol, void *new_func, void **old_func) {
    void *addr = dlsym(handle, symbol);
    if (addr == nullptr) {
        ALOGE("dlsym %s failed",symbol);
        return;
    }
    MSHookFunction(addr, new_func, old_func);
}

//skip dex2oat hooker
bool isSandHooker(char *const args[]) {
    int orig_arg_count = getArrayItemCount(args);

    for (int i = 0; i < orig_arg_count; i++) {
        if (strstr(args[i], "SandHooker")) {
            if (g_api_level >= ANDROID_N) {
                ALOGE("skip dex2oat hooker!");
                return true;
            } else {
                return false;
            }
        }
    }
    return false;
}

//disable inline
char **build_new_argv(char *const argv[]) {

    int orig_argv_count = getArrayItemCount(argv);

    int new_argv_count = orig_argv_count + 2;
    char **new_argv = (char **) malloc(new_argv_count * sizeof(char *));
    int cur = 0;
    for (int i = 0; i < orig_argv_count; ++i) {
        new_argv[cur++] = argv[i];
    }

    //(api_level == 28 && g_preview_api_level > 0) = Android Q Preview
    if (g_api_level >= ANDROID_L2 && g_api_level < ANDROID_Q) {
        new_argv[cur++] = (char *) "--compile-pic";
    }
    if (g_api_level >= ANDROID_M) {
        new_argv[cur++] = (char *) (g_api_level > ANDROID_N2 ? "--inline-max-code-units=0" : "--inline-depth-limit=0");
    }

    new_argv[cur] = NULL;

    return new_argv;
}


// int (*origin_execve)(const char *pathname, char *const argv[], char *const envp[]);
HOOK_DEF(int, execve, const char *pathname, char *argv[], char *const envp[]) {
    char temp[PATH_MAX];
    const char *relocated_path = relocate_path(pathname, temp, sizeof(temp));
    if (!relocated_path) {
        errno = EACCES;
        return -1;
    }

    char **new_argv = nullptr;

    if (strstr(pathname, "dex2oat")) {
        if (isSandHooker(argv)) {
            return -1;
        }
        new_argv = build_new_argv(argv);
    }

    char **relocated_envp = relocate_envp(relocated_path, envp);
    long ret = syscall(__NR_execve, relocated_path, new_argv != nullptr ? new_argv : argv, relocated_envp);
    if (relocated_envp != envp) {
        int i = 0;
        while (relocated_envp[i] != NULL) {
            free(relocated_envp[i]);
            ++i;
        }
        free(relocated_envp);
    }
    if (new_argv != nullptr) {
        free(new_argv);
    }
    return ret;
}

#include <string>
HOOK_DEF(FILE*,popen,const char *command , const char *type)
{
    std::string chkStr = std::string(type);
    if(chkStr==std::string("w"))
    {
        return orig_popen(command,type);
    }
    else if(chkStr==std::string("r"))
    {
        std::string theCommand = std::string(command);
        std::string buffer;
        std::string::size_type beginReplace = 0;
        bool slashBegin = false;
        for(char & itorStr : theCommand)
        {
            if((itorStr=='/')&&(!slashBegin))
            {
                slashBegin = true;
            }
            if(slashBegin)
            {
                if(itorStr==' ')break;
                buffer.push_back(itorStr);
            }
            else beginReplace++;
        }
        if(!buffer.empty())
        {
            char temp[PATH_MAX];
            const char *relocated_path = relocate_path(buffer.c_str(), temp, sizeof(temp));
            if (__predict_true(relocated_path)) {
                theCommand.replace(beginReplace,buffer.size(),relocated_path);
                return orig_popen(theCommand.c_str(),type);
            }
        }
    }
    return orig_popen(command,type);
}

HOOK_DEF(void *, dlopen_CI, const char *filename, int flag) {
    char temp[PATH_MAX];
    const char *redirect_path = relocate_path(filename, temp, sizeof(temp));
    void *ret = orig_dlopen_CI(redirect_path, flag);
    onSoLoaded(filename, ret);
    return ret;
}

HOOK_DEF(void*, do_dlopen_CIV, const char *filename, int flag, const void *extinfo) {
    char temp[PATH_MAX];
    const char *redirect_path = relocate_path(filename, temp, sizeof(temp));
    void *ret = orig_do_dlopen_CIV(redirect_path, flag, extinfo);
    onSoLoaded(filename, ret);
    return ret;
}

HOOK_DEF(void*, do_dlopen_CIVV, const char *name, int flags, const void *extinfo,
         void *caller_addr) {
    char temp[PATH_MAX];
    const char *redirect_path = relocate_path(name, temp, sizeof(temp));
    void *ret = orig_do_dlopen_CIVV(redirect_path, flags, extinfo, caller_addr);
    onSoLoaded(name, ret);
    return ret;
}

//void *dlsym(void *handle, const char *symbol)
HOOK_DEF(void*, dlsym, void *handle, char *symbol) {
    return orig_dlsym(handle, symbol);
}

HOOK_DEF(pid_t, vfork) {
    return fork();
}

/*HOOK_DEF(int, fstat64, int __fd, struct stat64* __buf) {
    char temp[BUFSIZ];
    bzero(temp,BUFSIZ);
    ct::hooks::get_fd_path(__fd, temp, BUFSIZ);
    if(strstr(temp,"/.native/")) {
        return syscall(__NR_stat64, ct::hooks::get_real_path(temp).c_str(), __buf);
    }else
        return orig_fstat64(__fd,__buf);
}*/

HOOK_DEF(enum android_fdsan_error_level, android_fdsan_set_error_level, enum android_fdsan_error_level new_level) {
    ALOGD("HITSS: suppressed android_fdsan_set_error_level: %d", new_level);
//    orig_android_fdsan_set_error_level(ANDROID_FDSAN_ERROR_LEVEL_DISABLED);
    static auto last_level = ANDROID_FDSAN_ERROR_LEVEL_WARN_ONCE;
    auto res = last_level;
    last_level = new_level;
    return res;
}

HOOK_DEF(bool, SetCheckJniEnabled, void* vm, bool enbaled) {
    return orig_SetCheckJniEnabled(vm, false);
}

HOOK_DEF(bool, is_accessible, void* thiz, const std::string& file) {
    return true;
}

/*ssize_t (*orig_send)(int, const void *, size_t, int);
ssize_t fake_send(int __fd, const void *__buf, size_t __n, int __flags) {
    ALOGD("called send fd=%d,buf=%d,n=%d,flags=%d",__fd,__buf,__n,__flags);
    ssize_t x = orig_send(__fd, __buf, __n, __flags);
    return x;
}*/
HOOK_DEF(ssize_t, send, int __fd, const void *__buf, size_t __n, int __flags) {
    ALOGD("called send fd=%d,buf=%d,n=%d,flags=%d",__fd,__buf,__n,__flags);
    return orig_send(__fd, __buf, __n,__flags);
}




__END_DECLS
// end IO DEF
static int return0(...){
    ALOGD("return0");
    return 0;
}

HOOK_DEF(int, return0,...){
    ALOGD("return0");
    return 0;
}

HOOK_DEF(int, return1,...){
    ALOGD("return1");
    return 1;
}
HOOK_DEF(void,sub_AAC90,__int64_t a1){
    ALOGD("sub_AAC90 %p %d");
    return orig_sub_AAC90(a1);
}
HOOK_DEF(__int64_t, sub_4E5B4, double *a1,__int64_t a2,char a3){
    ALOGD("sub_4E5B4 %p %d");
    return 0;
}
HOOK_DEF(__int64_t, sub_4EDA0, __int64_t a1,__int64_t a2){
    ALOGD("sub_4E5B4 %p %d");
    return 0;
}

HOOK_DEF(__int64_t, sub_534F8, __int64_t a1){
    ALOGD("sub_534F8 %p %d");
    return 0;
}

HOOK_DEF(__int64_t, sub_70428, __int64_t a1){
    ALOGD("sub_70428 %p %d");
    return 0;
}
HOOK_DEF(__int64_t, sub_3A4C8, __int64_t result){
    ALOGD("sub_3A4C8");
    return 0;
}
//tencent hook
class TssInfoReceiver {
public:
    virtual ~TssInfoReceiver() {}

    virtual int getInterfaceVersionCode() { return 1; }

    virtual void onReceive(int tssInfoType, const char *info) = 0;
};

struct TssSdkUserInfoEx {
    unsigned int size_;      // struct size
    unsigned int entry_id_;  // entrance id, wechat/open platform and so on.
    struct {
        unsigned int type_;  // type of uin, refer to TssSdkUinType
        union {
            unsigned int uin_int_;   // for integer format uin
            char uin_str_[64];       // for string format uin
        };
    } uin_;

    struct {
        unsigned int type_;
        union {
            unsigned int app_id_int_;   // for integer format appid
            char app_id_str_[64];       // for string format appid
        };
    } app_id_;

    unsigned int world_id_;
    char role_id_[64];
};


struct TssSdkAntiDataInfo {
    unsigned short anti_data_len_;   /* [in] length of anti data */
    const unsigned char *anti_data_; /* [in] anti data buffer */
};

typedef bool (*tss_sdk_send_data_to_svr)(const struct TssSdkAntiDataInfo *anti_data);

struct TssSdkInitInfo {
    unsigned int size_;                           // struct size
    unsigned int game_id_;                        // game id
    tss_sdk_send_data_to_svr send_data_to_svr_;   // callback interface,implement by game
};

struct TssSdkEncryptPkgInfo {
    unsigned int cmd_id_;                     /* [in] game pkg cmd */
    const unsigned char *game_pkg_;           /* [in] game pkg */
    unsigned int game_pkg_len;                /* [in] the length of game data packets, maximum length less than 65,000 */
    unsigned char *encrypt_data_;             /* [in/out] assembling encrypted game data package into anti data, memory allocated by the caller, 64k at the maximum */
    unsigned int encrypt_data_len_;           /* [in/out] length of anti_data when input, actual length of anti_data when output */
};

struct TssSdkDecryptPkgInfo {
    const unsigned char *encrypt_data_;     /* [in] anti data received by game client */
    unsigned int encrypt_data_len;          /* [in] length of anti data received by game client */
    unsigned char *game_pkg_;               /* [out] buffer used to store the decrypted game package, space allocated by the caller */
    unsigned int game_pkg_len_;             /* [out] input is size of game_pkg_, output is the actual length of decrypted game package */
};


/*int new_tss_sdk_ioctl(int action, const char *unknown, char *buf, size_t bufSize, int *p) {
    ALOGD("tss_sdk_ioctl");
    return 0;
}*/

/*int new_tp2_free_anti_data(struct TssSdkAntiDataInfo *data) {
    //log("free_anti_data");
    return 0;
}*/

void new_tp2_regist_tss_info_receiver(TssInfoReceiver *receiver) {
    //log("regist_info_receiver");
    return;
}

int new_tss_sdk_encryptpacket(unsigned char *a1){
    return 0;
}

/*int *new_tss_sdk_init(int *param_1, unsigned long param_2,unsigned long param_3,unsigned longparam_4,
                      unsigned long param_5,unsigned long param_6,unsigned long param_7,unsigned long param_8){
    ALOGE("tss_sdk_init");
    return 0;
}*/

int new_tss_sdk_decryptpacket(unsigned char *a1){

    return 0;
}


void disMemoryProtect(unsigned long addr){
    int pageSize = sysconf(_SC_PAGESIZE);
    unsigned baseAddr = addr - (addr % pageSize);
    mprotect((void*) baseAddr, pageSize, PROT_READ | PROT_WRITE | PROT_EXEC);
}


void mMSHookFunction(void* symbol, void* newSymbol, void** oldSymbol){
    if ((unsigned long) symbol > 0){
        disMemoryProtect((unsigned long) symbol);
        MSHookFunction(symbol, newSymbol, oldSymbol);
    }else{
        //log("symbol == NULL");
    }
}




int (*old_tp2_sdk_init)(int app_id);
int (*old_tp2_sdk_init_ex)(int app_id, const char *app_key);
int (*old_tp2_setuserinfo)(int account_type, int world_id, const char *open_id,
                           const char *role_id);
int (*old_tp2_getver)(char *ver_buf, size_t buf_size);
int (*old_tp2_setoptions)(int options);
int (*old_tp2_setgamestatus)(/*TP2GameStatus*/int status);
uintptr_t (*old_tp2_sdk_ioctl)(int request, const char *param_str);
int (*old_tp2_free_anti_data)(struct TssSdkAntiDataInfo *data);
//void (*old_tp2_regist_tss_info_receiver)(TssInfoReceiver *receiver);
int (*old_tp2_dec_tss_info)(const char *src, char *out, size_t len);
int (*old_tss_sdk_ioctl)(int action, const char *unknown, char *buf, size_t bufSize, int *p);
int (*old_TssSDKGetReportData)(void *ptr);
/*int (*old_tss_sdk_init)(int *param_1, unsigned long param_2,unsigned long param_3,unsigned longparam_4,
                        unsigned long param_5,unsigned long param_6,unsigned long param_7,unsigned long param_8);*/
int (*old_tss_sdk_encryptpacket)(unsigned char *a1);
int (*old_tss_sdk_decryptpacket)(unsigned char *a1);
void (*old_tp2_regist_tss_info_receiver) (TssInfoReceiver* receiver);
void (*old_TssSDKInit)(uint p1,u8 p2,u8 p3,u8 p4, u8 p5,u8 p6,u8 p7,u8 p8);
void (*old_TssSDKSetUserInfo)(uint p1,char *p2,u8 p3,u8 p4, uint *p5,u8 p6,u8 p7,u8 p8);
void (*old_TssSDKOnPause)(u8 p1,u8 p2,u8 p3,u8 p4, u8 p5,u8 p6,u8 p7,u8 p8);
//void (*old_TssSDKGetReportData)(u8 p1,u8 p2,u8 p3,u8 p4 ,u8 p5,u8 p6,u8 p7,u8 p8);
void (*old_TssSDKOnResume)(u8 p1,u8 p2,u8 p3,u8 p4, u8 p5,u8 p6,u8 p7,u8 p8);
void (*old_TssSDKDelReportData)(void *p1,u1 *p2,u8 p3,u8 p4, u8 p5,u8 p6,u8 p7,u8 p8);
void (*old_TssSDKOnRecvData)(long p1,uint p2,u8 p3,ulong p4,u8 p5, u8 p6,ulong p7,u8 p8);
void (*old_TssSDKForExport)(void);
int (*old_TssSDKRegistInfoListener)(int a1);
int (*old_tss_unity_is_enable)(int a1,int a2);
int (*old_tss_sdk_setuserinfo)(int result);
int (*old_tss_sdk_setuserinfo_ex)(char *a1);
int (*old_tss_sdk_setgamestatus)(char *a1);

int new_tss_sdk_setgamestatus(char *a1){
    ALOGD("tss_sdk_setgamestatus");
    return 0;
}
int new_tss_sdk_setuserinfo_ex(char *a1){
    ALOGD("tss_sdk_setuserinfo_ex");
    return 0;
}
int new_tss_sdk_setuserinfo(int result){
    ALOGD("new_tss_sdk_setuserinfo");
    return 0;
}
int new_tss_unity_is_enable(int a1,int a2){
    ALOGD("tss_unity_is_enable",a1,a2);
    return 0;
}
int new_TssSDKRegistInfoListener(int a1){
    ALOGD("TssSDKRegistInfoListener");
    return 0;
}

void new_TssSDKInit(uint p1,u8 p2,u8 p3,u8 p4, u8 p5,u8 p6,u8 p7,u8 p8){
    ALOGD("TssSDKInit");
    return;
}

void new_TssSDKSetUserInfo(uint p1,char *p2,u8 p3,u8 p4, uint *p5,u8 p6,u8 p7,u8 p8){
    ALOGD("TssSDKSetUserInfo");
    return;
}


void new_TssSDKOnPause(u8 p1,u8 p2,u8 p3,u8 p4, u8 p5,u8 p6,u8 p7,u8 p8){
    ALOGD("TssSDKOnPause");
    return;
}


void new_TssSDKGetReportData(u8 p1,u8 p2,u8 p3,u8 p4 ,u8 p5,u8 p6,u8 p7,u8 p8){
    ALOGD("TssSDKGetReportData");
    return;
}

void new_TssSDKOnResume(u8 p1,u8 p2,u8 p3,u8 p4, u8 p5,u8 p6,u8 p7,u8 p8){
    ALOGD("TssSDKOnResume");
    return;
}

void new_TssSDKDelReportData(void *p1,u1 *p2,u8 p3,u8 p4, u8 p5,u8 p6,u8 p7,u8 p8){
    ALOGD("TssSDKDelReportData");
    return;
}

void new_TssSDKOnRecvData(long p1,uint p2,u8 p3,ulong p4,u8 p5, u8 p6,ulong p7,u8 p8){
    ALOGD("TssSDKOnRecvData");
    return;
}

/*void new_TssSDKForExport(void){
    ALOGD("TssSDKForExport");
    return;
}*/

/*
 * int tss_sdk_setuserinfo(int result)
 *
 * */





HOOK_DEF(void,Java_com_tencent_tp_TssSdk_onruntimeinfo,JNIEnv *env, jclass clazz, jbyteArray bytes, jint length){
    jbyte *str=env->GetByteArrayElements(bytes, nullptr);
    str[length]=0;
    if(strstr((char*)str,"Monster")){
        env->CallObjectMethod(nullptr,0);
        ALOGD("Java_com_tencent_nop1tp_TssSdk_onruntimeinfo >>>> %s",str);
        env->ReleaseByteArrayElements(bytes,str,0);
        return;
    }
    orig_Java_com_tencent_tp_TssSdk_onruntimeinfo(env,clazz,bytes,length);
}
HOOK_DEF(int,checkFrom_9081,char *path, int **a2, int buf, unsigned int bufsize, int a5){
    int ret = orig_checkFrom_9081(path,a2,buf,bufsize,a5);
    ALOGD("checkFrom_9081 %s 0x%lx %s",path,ret,buf);
    return ret;
}

HOOK_DEF(void,report9081,unsigned int a1, int a2, unsigned int a3, char* message){
    ALOGD("report9081 %s",message);
}

HOOK_DEF(void,tp2_free_anti_data,struct TssSdkAntiDataInfo *data){
    ALOGD("tp2_free_anti_data %s",data);
}
HOOK_DEF(signed int ,report9005,char* name1, char* name2, char* buf, int bufSiz){
    ALOGD("report9005  %s %s",name1,name2);
    return 0;
}
HOOK_DEF(JNIEXPORT jint JNICALL ,JNI_OnLoad,JavaVM* vm, void* reserved){
    ALOGD("JNI_OnLoad  %s %s");
    return 0;
}


typedef struct ReportMessage{
    char pad1[4]; //0~4
    int32_t messageId; //4~8
    int32_t unknownNumber1; //8~12
    int32_t unknownNumber2; //12~16
    int32_t unknownNumber3; //16~20
    int32_t unknownNumber4; //20~24
    int32_t unknownNumber5; //24~28
    char pad2[508]; //28~536
    int32_t unknownNumber6; //536-540
    char message1[64]; //540-604
    char message2[76]; //604-680
    char message3[BUFSIZ];

    void printMessages(){
        ALOGD("message1 %d %s",messageId,message1);
        if(*message2)
            ALOGD("message2 %d %s",messageId,message2);
        if(*message3)
            ALOGD("message3 %d %s",messageId,message3);
    }
} ReportMessage;

HOOK_DEF(long long,tss_report_to_qos_ex,ReportMessage *message){
    if(message) {
        message->printMessages();
        switch (message->messageId){
            case 9001: //JailBroken
            case 9003:
            case 9006:
            case 9010: //SuspiciousModuleScaner
            case 9014: //CertScaner
            case 9015: //ReportAppCrc
            case 9019: //ReportFileCrc
            case 9021: //ReportCSChannelStat
            case 9026: //ReportInfoCollectStr
            case 9030:
            case 9047: //ReportAppName
            case 9054: //CC2name,buf
            case 9055:
            case 9060:
            case 9071:
            case 9082: //TssSDK::ScheduleEngine
            case 9088: //TssSDK::GP4
            case 9095:
            case 9091: //AntiEmulator
            case 9096:
                break;

            case 9020: //OpcodeScaner
                ALOGD("OpcodeScaner!");
                break;

            case 9005: //TssSDK::BlackProcessScaner
            case 9041: //ReportLuaResult
            case 9081: //F9AB1B4A03B9FADAF971C87A79E437E95AFA4068
                return 1;

            default:
                ALOGE("tss_report_to_qos_ex %d !!!!!!!!!!",message->messageId);
                return 1;
        }
    }
    return orig_tss_report_to_qos_ex(message);
}

void findPtr(void *handle,const char *name,void**ptr);
void findPtr(void *handle,const char *name,void**ptr){
    void *func=dlsym(handle,name);
    if(func== nullptr){
        ALOGE("func %s find error!",name);
        return;
    }
    *ptr=func;
}






//
//PTR_DEF(int32_t, tss_sdk_ioctl, int action, const char *unknown, char *buf, size_t bufSize, int *p);


static bool has_code(const char *perm) {
    bool r = false, x = false;
    for (int i = 0; i < 5; ++i) {
        if (perm[i] == 'r') {
            r = true;
        }
        if (perm[i] == 'x') {
            x = true;
        }
    }
    return r && x;
}

static bool has_code(int prot) {
    return prot&PROT_EXEC && prot&PROT_READ;
}

static void search_memory_svc(long long begin, long long  end, void (*callback)(void *)) {
    long long start = begin;
    long long limit = end - sizeof(int32_t) * 15;
    do {
        int32_t *insn = reinterpret_cast<int32_t *>(start);
#ifdef __arm__
        #define MOV_R12_SP 0xE1A0C00D
#define SVC_0 0xEF000000
            if (insn[0] == MOV_R12_SP  && (insn[7] == SVC_0 or insn[14] == SVC_0)) {
                (*callback)(insn);
            }
#else
#endif
        start += 1;
    } while (start < limit);
}


namespace maps{

    static void findSegments(const char *path, void (*callback_findSVC)(void *),
                             void (*callback_fileStart)(void *)) {
        FILE *f;
        if ((f = fopen("/proc/self/maps", "r")) == NULL) {
            return;
        }
        char buf[PATH_MAX + 100], perm[5], dev[6], mapname[PATH_MAX];
        int64_t begin, end, inode, foo;

        while (!feof(f)) {
            if (fgets(buf, sizeof(buf), f) == 0)
                break;
            mapname[0] = '\0';
            sscanf(buf, "%llx-%llx %4s %llx %5s %lld %s", &begin, &end, perm,
                   &foo, dev, &inode, mapname);
            if (strstr(buf, path)) {
                if(callback_findSVC and has_code(perm)) {
                    search_memory_svc(begin, end, callback_findSVC);
                }
                if(callback_fileStart and foo==0){
                    callback_fileStart((void*)begin);
                }
            }
        }
        fclose(f);
    }

}

static void callBack_findSVC(void* func){
    ALOGD("findSVC >>>> %p",func);
    hookByAddress(func, (void *) new_syscall, nullptr);
}

static int8_t* tersafeValues;
static int8_t* tersafeFuncs;
static void callBack_fileStart(void *start){
    tersafeValues=(int8_t*)start;
    tersafeFuncs=tersafeValues-1;
}


static inline __always_inline void handleLibc(){
    static bool hooked=false;
    if(!hooked){
        hooked = true;
        void* handle = dlopen("libc.so",RTLD_NOW);
        HOOK_SYMBOL(handle,execve);
        dlclose(handle);
    }
}

/*static inline __always_inline bool judgeMtpVersion(void *handle,const char* version){
    static char mtpVersion[BUFSIZ];
    if(tss_sdk_ioctl== nullptr) {
        FIND_PTR(handle, "tss_sdk_ioctl", tss_sdk_ioctl);
        tss_sdk_ioctl(14, nullptr, mtpVersion, BUFSIZ, nullptr);
    }

    if( strcmp(mtpVersion,version)!=0){
        ALOGE("fault mtp version >>>> %s , which should be %s",mtpVersion,version);
        return false;
    }
    ALOGD("mtp version >>>> %s", mtpVersion);
    return true;
}*/


/*
static void new_sendSignal(JNIEnv* env, jclass clazz, jint stub0, jint stub1)
{
    if(banProcessFromExit)
    {
        // Kill.
        // ALOGE("Process killed with called sendSignal! Get uid -> %d and signal %d.", stub0, stub1);
        env->CallStaticVoidMethod(
                nativeEngineClass,
                env->GetStaticMethodID(nativeEngineClass,
                                       "printStackTraceNative","()V")
        );
        return;
    }
    patchEnv.orig_sendSignal(env, clazz, stub0, stub1);
}

*/



void onSoLoaded(const char *name, void *handle) {
    ALOGD("so loaded: %s", name);
}

//E/libc: Access denied finding property "vendor.audio.game4D.switch"
bool relocate_art(JNIEnv *env, const char *art_path) {
    intptr_t art_addr, art_off, symbol;
    if ((art_addr = get_addr(art_path)) == 0) {
        ALOGE("Cannot found art addr.");
        return false;
    }
    //disable jni check
    if (g_api_level >= ANDROID_L && env && resolve_symbol(art_path, "_ZN3art9JavaVMExt18SetCheckJniEnabledEb",
                                                          &art_off) == 0) {
        symbol = art_addr + art_off;
        orig_SetCheckJniEnabled = reinterpret_cast<bool (*)(void *, bool)>(symbol);
        JavaVM *vm;
        env->GetJavaVM(&vm);
        orig_SetCheckJniEnabled(vm, false);
    }
    return true;
}

bool fuck_linker(const char *linker_path) {
    void *handle = dlopen("libsandhook-native.so", RTLD_NOW);

    if (!handle) {
        return false;
    }

    auto getSym = reinterpret_cast<void *(*)(const char*, const char*)>(dlsym(handle,
                                                                              "SandGetSym"));

    if (!getSym) {
        return false;
    }
    auto is_accessible_str = "__dl__ZN19android_namespace_t13is_accessibleERKNSt3__112basic_stringIcNS0_11char_traitsIcEENS0_9allocatorIcEEEE";
    void *is_accessible_addr = getSym(linker_path, is_accessible_str);
    if (is_accessible_addr) {
        MSHookFunction(is_accessible_addr, (void *) new_is_accessible,
                       (void **) &orig_is_accessible);
    }

    return true;
}
long callRedirectPathOpen(const char *pathname, int flags, int mode);
long callRedirectPathOpenat(int fd, const char *pathname, int flags, int mode);

#ifdef BINDER_IPC_32BIT
typedef __u32 binder_size_t;
typedef __u32 binder_uintptr_t;
#else
typedef __u64 binder_size_t;
typedef __u64 binder_uintptr_t;
#endif
struct binder_write_read {
    binder_size_t		write_size;	/* bytes to write */
    binder_size_t		write_consumed;	/* bytes consumed by driver */
    binder_uintptr_t	write_buffer;
    binder_size_t		read_size;	/* bytes to read */
    binder_size_t		read_consumed;	/* bytes consumed by driver */
    binder_uintptr_t	read_buffer;
};
// ioctl(int __fd, unsigned long int __request, void * arg)
HOOK_DEF(int,ioctl,int __fd, unsigned long int __request, void * arg)
{
    int rtn = orig_ioctl(__fd,__request,arg);
    constexpr auto magicNum = _IOWR('b', 1, struct binder_write_read);
    if(__request == magicNum)
    {
        //ALOGE("Notice -> binder read write, arg -> %lu",(unsigned long)arg);
        auto dir  =  _IOC_DIR(__request);   //根据命令获取传输方向
        auto type =  _IOC_TYPE(__request);  //根据命令获取类型
        auto nr   =  _IOC_NR(__request);    //根据命令获取类型命令
        auto size =  _IOC_SIZE(__request);  //根据命令获取传输数据大小
        //ALOGE("new call to ioctl, dir:%lu, type:%lu, nr:%lu, size:%lu\n", dir, type, nr, size);
    }
    return rtn;
}


bool do_hook_ioctl()
{
    MSHookFunction((void*)((int(*)(int,int,...))(ioctl)),(void*)(new_ioctl),
                   (void**)(&orig_ioctl));
    return true;
}

void do_hook_syscall()
{
    MSHookFunction((void*)((long(*)(long,...))(syscall)),(void*)(new_syscall),
                   (void**)(&orig_syscall));
}

bool relocate_linker(const char *linker_path) {
    intptr_t linker_addr, dlopen_off, symbol;
    if ((linker_addr = get_addr(linker_path)) == 0) {
        ALOGE("Cannot found linker addr.");
        return false;
    }
    if (resolve_symbol(linker_path, "__dl__Z9do_dlopenPKciPK17android_dlextinfoPKv",
                       &dlopen_off) == 0) {
        symbol = linker_addr + dlopen_off;
        MSHookFunction((void *) symbol, (void *) new_do_dlopen_CIVV,
                       (void **) &orig_do_dlopen_CIVV);
        return true;
    } else if (resolve_symbol(linker_path, "__dl__Z9do_dlopenPKciPK17android_dlextinfoPv",
                              &dlopen_off) == 0) {
        symbol = linker_addr + dlopen_off;
        MSHookFunction((void *) symbol, (void *) new_do_dlopen_CIVV,
                       (void **) &orig_do_dlopen_CIVV);
        return true;
    } else if (resolve_symbol(linker_path, "__dl__ZL10dlopen_extPKciPK17android_dlextinfoPv",
                              &dlopen_off) == 0) {
        symbol = linker_addr + dlopen_off;
        MSHookFunction((void *) symbol, (void *) new_do_dlopen_CIVV,
                       (void **) &orig_do_dlopen_CIVV);
        return true;
    } else if (
            resolve_symbol(linker_path, "__dl__Z20__android_dlopen_extPKciPK17android_dlextinfoPKv",
                           &dlopen_off) == 0) {
        symbol = linker_addr + dlopen_off;
        MSHookFunction((void *) symbol, (void *) new_do_dlopen_CIVV,
                       (void **) &orig_do_dlopen_CIVV);
        return true;
    } else if (
            resolve_symbol(linker_path, "__dl___loader_android_dlopen_ext",
                           &dlopen_off) == 0) {
        symbol = linker_addr + dlopen_off;
        MSHookFunction((void *) symbol, (void *) new_do_dlopen_CIVV,
                       (void **) &orig_do_dlopen_CIVV);
        return true;
    } else if (resolve_symbol(linker_path, "__dl__Z9do_dlopenPKciPK17android_dlextinfo",
                              &dlopen_off) == 0) {
        symbol = linker_addr + dlopen_off;
        MSHookFunction((void *) symbol, (void *) new_do_dlopen_CIV,
                       (void **) &orig_do_dlopen_CIV);
        return true;
    } else if (resolve_symbol(linker_path, "__dl__Z8__dlopenPKciPKv",
                              &dlopen_off) == 0) {
        symbol = linker_addr + dlopen_off;
        MSHookFunction((void *) symbol, (void *) new_do_dlopen_CIV,
                       (void **) &orig_do_dlopen_CIV);
        return true;
    } else if (resolve_symbol(linker_path, "__dl___loader_dlopen",
                              &dlopen_off) == 0) {
        symbol = linker_addr + dlopen_off;
        MSHookFunction((void *) symbol, (void *) new_do_dlopen_CIV,
                       (void **) &orig_do_dlopen_CIV);
        return true;
    } else if (resolve_symbol(linker_path, "__dl_dlopen",
                              &dlopen_off) == 0) {
        symbol = linker_addr + dlopen_off;
        MSHookFunction((void *) symbol, (void *) new_dlopen_CI,
                       (void **) &orig_dlopen_CI);
        return true;
    }
    return false;
}

#if defined(__aarch64__)
bool on_found_syscall_aarch64(const char *path, int num, void *func) {
    static int pass = 0;
    switch (num) {
        HOOK_SYSCALL(fchmodat)
        HOOK_SYSCALL(fchownat)
        HOOK_SYSCALL(renameat)
        HOOK_SYSCALL(mkdirat)
        HOOK_SYSCALL(mknodat)
        HOOK_SYSCALL(truncate)
        HOOK_SYSCALL(linkat)
        HOOK_SYSCALL(faccessat)
        HOOK_SYSCALL_(statfs)
        HOOK_SYSCALL_(getcwd)
        HOOK_SYSCALL_(openat)
        HOOK_SYSCALL(readlinkat)
        HOOK_SYSCALL(unlinkat)
        HOOK_SYSCALL(symlinkat)
        HOOK_SYSCALL(utimensat)
        HOOK_SYSCALL(chdir)
        HOOK_SYSCALL(execve)
        HOOK_SYSCALL(kill)
    }
    if (pass == 18) {
        return BREAK_FIND_SYSCALL;
    }
    return CONTINUE_FIND_SYSCALL;
}

bool on_found_linker_syscall_arch64(const char *path, int num, void *func) {
    static int pass = 0;
    switch (num) {
        case __NR_openat:
            MSHookFunction(func, (void *) new___openat, (void **) &orig___openat);
            return BREAK_FIND_SYSCALL;
    }
    if (pass == 5) {
        return BREAK_FIND_SYSCALL;
    }
    return CONTINUE_FIND_SYSCALL;
}
#else

bool on_found_linker_syscall_arm(const char *path, int num, void *func) {
    switch (num) {
        case __NR_openat:
            MSHookFunction(func, (void *) new___openat, (void **) &orig___openat);
            break;
        case __NR_open:
            MSHookFunction(func, (void *) new___open, (void **) &orig___open);
            break;
    }
    return CONTINUE_FIND_SYSCALL;
}

#endif

void InterruptHandler(int signum, siginfo_t* siginfo, void* uc) {
    ALOGE("Begin of abort() ###################################");
    old_sig_act.sa_sigaction(signum, siginfo, uc);
}

void startIOHook(JNIEnv *env, int api_level, bool hook_dlopen) {
    ALOGE("Starting IO Hook...");
    void *handle = dlopen("libc.so", RTLD_NOW);
    const char *linker = nullptr;
    const char *libc = nullptr;
    const char *art = nullptr;

    if (debug_kill) {
        struct sigaction sig{};
        sigemptyset(&sig.sa_mask);
        sig.sa_flags = SA_SIGINFO;
        sig.sa_sigaction = InterruptHandler;
        if (sigaction(SIGABRT, &sig, &old_sig_act) != -1) {
        }
        HOOK_SYMBOL(handle, sigaction);
    }

    if (api_level >= ANDROID_Q) {
        if (sizeof(void*) == 8) {
            art = "/apex/com.android.runtime/lib64/libart.so";
            linker = "/apex/com.android.runtime/bin/linker64";
            libc = "/apex/com.android.runtime/lib64/bionic/libc.so";
        } else {
            art = "/apex/com.android.runtime/lib/libart.so";
            linker = "/apex/com.android.runtime/bin/linker";
            libc = "/apex/com.android.runtime/lib/bionic/libc.so";
        }
    } else {
        if (sizeof(void*) == 8) {
            art = "/system/lib64/libart.so";
            linker = "/system/bin/linker64";
            libc = "/system/lib64/libc.so";
        } else {
            art = "/system/lib/libart.so";
            linker = "/system/bin/linker";
            libc = "/system/lib/libc.so";
        }
    }
    if (api_level >= ANDROID_Q) {
        fuck_linker(linker);
    }
    relocate_art(env, art);
    if (handle) {
#if defined(__aarch64__)
        if (!findSyscalls(libc, on_found_syscall_aarch64)) {
            HOOK_SYMBOL(handle, fchownat);
            HOOK_SYMBOL(handle, renameat);
            HOOK_SYMBOL(handle, mkdirat);
            HOOK_SYMBOL(handle, mknodat);
            HOOK_SYMBOL(handle, truncate);
            HOOK_SYMBOL(handle, linkat);
            if (!(patchEnv.host_packageName && strstr(patchEnv.app_packageName, "org.telegram.messenger"))) {
                ALOGE("hook readlinkat %s", patchEnv.app_packageName);
                HOOK_SYMBOL(handle, readlinkat);
            }
            HOOK_SYMBOL(handle, unlinkat);
            HOOK_SYMBOL(handle, symlinkat);
            HOOK_SYMBOL(handle, utimensat);
            HOOK_SYMBOL(handle, chdir);
            HOOK_SYMBOL(handle, execve);
            HOOK_SYMBOL(handle, statfs64);
            HOOK_SYMBOL(handle, kill);
            HOOK_SYMBOL(handle, vfork);
            HOOK_SYMBOL(handle, fstatat64);
        }
        if (hook_dlopen) {
            findSyscalls(linker, on_found_linker_syscall_arch64);
        }
#else
        HOOK_SYMBOL(handle, faccessat);
        HOOK_SYMBOL(handle, __openat);
        HOOK_SYMBOL(handle, fchmodat);
        HOOK_SYMBOL(handle, fchownat);
        HOOK_SYMBOL(handle, renameat);
        HOOK_SYMBOL(handle, fstatat64);
        HOOK_SYMBOL(handle, __statfs);
        HOOK_SYMBOL(handle, __statfs64);
        HOOK_SYMBOL(handle, mkdirat);
        HOOK_SYMBOL(handle, mknodat);
        HOOK_SYMBOL(handle, truncate);
        HOOK_SYMBOL(handle, linkat);
        HOOK_SYMBOL(handle, readlinkat);
        HOOK_SYMBOL(handle, unlinkat);
        HOOK_SYMBOL(handle, symlinkat);
        HOOK_SYMBOL(handle, utimensat);
        HOOK_SYMBOL(handle, __getcwd);
        HOOK_SYMBOL(handle, chdir);
        HOOK_SYMBOL(handle, execve);
        HOOK_SYMBOL(handle, kill);
        HOOK_SYMBOL(handle, vfork);
        HOOK_SYMBOL(handle, popen);
        HOOK_SYMBOL(handle, exit);
//        HOOK_SYMBOL(handle, send);
        HOOK_SYMBOL(handle, recv);
        if (api_level <= 20) {
            HOOK_SYMBOL(handle, access);
            HOOK_SYMBOL(handle, stat);
            HOOK_SYMBOL(handle, lstat);
            HOOK_SYMBOL(handle, fstatat);
            HOOK_SYMBOL(handle, __open);
            HOOK_SYMBOL(handle, chmod);
            HOOK_SYMBOL(handle, chown);
            HOOK_SYMBOL(handle, rename);
            HOOK_SYMBOL(handle, rmdir);
            HOOK_SYMBOL(handle, mkdir);
            HOOK_SYMBOL(handle, mknod);
            HOOK_SYMBOL(handle, link);
            HOOK_SYMBOL(handle, unlink);
            HOOK_SYMBOL(handle, readlink);
            HOOK_SYMBOL(handle, symlink);
        }
        if (api_level >= 29) {
            using android_fdsan_set_error_level_t = enum android_fdsan_error_level(
                    enum android_fdsan_error_level new_level);
            auto *android_fdsan_set_error_level_p = reinterpret_cast<android_fdsan_set_error_level_t *>(dlsym(
                    handle, "android_fdsan_set_error_level"));
            if (android_fdsan_set_error_level_p) {
                android_fdsan_set_error_level_p(ANDROID_FDSAN_ERROR_LEVEL_DISABLED);
            }
        }
#ifdef __arm__
        if (hook_dlopen && !relocate_linker(linker)) {
            findSyscalls(linker, on_found_linker_syscall_arm);
        }
#endif
#endif
        dlclose(handle);
    }
    // HOOK ioctl
    do_hook_ioctl();
    /*do_hook_syscall();*/
}




void
IOUniformer::startUniformer(JNIEnv *env, const char *so_path, const char *so_path_64, const char *native_path,
                            int api_level,
                            int preview_api_level,
                            bool hook_dlopen,
                            bool skip_kill_) {
    char api_level_chars[56];
    setenv("V_SO_PATH", so_path, 1);
    setenv("V_SO_PATH_64", so_path_64, 1);
    sprintf(api_level_chars, "%i", api_level);
    setenv("V_API_LEVEL", api_level_chars, 1);
    sprintf(api_level_chars, "%i", preview_api_level);
    setenv("V_PREVIEW_API_LEVEL", api_level_chars, 1);
    setenv("V_NATIVE_PATH", native_path, 1);
    startIOHook(env, api_level, hook_dlopen);
    skip_kill = skip_kill_;
}
