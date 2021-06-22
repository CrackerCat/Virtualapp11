#include <zconf.h>
#include <syscall.h>
#include <linux/fcntl.h>
#include <map>
#include "hideSegments.h"

#define FIX_MMAP_LENGTH(length) (((length)&0xFFF)!=0?(((length)|0xFFF)+1):(length))

namespace hideSegments {

    static std::vector<std::string> needhides;

    static inline __always_inline bool needHideSegment(const char *filePath) {
        auto size=needhides.size();
        for (int i = 0; i < size; ++i) {
            if (strstr(filePath, needhides[i].c_str()))
                return true;
        }
        return false;
    }

    static inline __always_inline int get_fd_realpath(int fd, char *pathbuf, size_t buf_size) {
        memset(pathbuf, 0, buf_size);
        char proc_fd_file[PATH_MAX];
        sprintf(proc_fd_file, "/proc/self/fd/%d", fd);
        return syscall(__NR_readlinkat, AT_FDCWD, proc_fd_file, pathbuf, buf_size);
    }

    void* (*orig_mmap64)(void *__addr, size_t __size, int __prot, int __flags, int __fd,off64_t __offset);
    void* new_mmap64(void *__addr, size_t __size, int __prot, int __flags, int __fd,off64_t __offset){
        if (!__addr or __fd <= 0) {
            return orig_mmap64(__addr, __size, __prot, __flags, __fd, __offset);
        }
        char filePath[PATH_MAX];
        get_fd_realpath(__fd, filePath, PATH_MAX);

        if (!needHideSegment(filePath))
            return orig_mmap64(__addr, __size, __prot, __flags, __fd, __offset);

        //LOGD("mmap %s %p %lx !",filePath,__addr,__size);

        __size=FIX_MMAP_LENGTH(__size);

        void *ret = orig_mmap64(__addr, __size, PROT_WRITE | PROT_READ, __flags | MAP_ANON, 0, 0);

        if (ret == MAP_FAILED) {
            ALOGE("couldn't mmap \"%s\" : %s", filePath, strerror(errno));
            return nullptr;
        }
        if (__offset != 0 and lseek64(__fd, __offset, SEEK_SET) == -1L) { // 移动到当前segment处
            ALOGE("couldn't lseek64 \"%s\" : %s", filePath, strerror(errno));
            goto error;
        }
        if (read(__fd, ret, __size) == -1) { // 读出内容到mmap出的缓存区
            ALOGE("couldn't read \"%s\" : %s", filePath, strerror(errno));
            goto error;
        }
        if (mprotect(ret, __size, __prot) == -1) { // 根据文件内容设置内存属性.
            ALOGE("couldn't mprotect \"%s\" : %s", filePath, strerror(errno));
            goto error;
        }

        getHiddenSegments(std::string(filePath))
                .push_back(Segment(ret,(char*)ret+__size,__offset,__prot));

        ALOGD("hide %s %p success!",filePath,ret);
        return ret;

        error:
        if(ret)
            munmap(ret,__size);
        return orig_mmap64(__addr, __size, __prot, __flags, __fd, __offset);
    }


    void addHideSegment(const std::string& filePath) {
        needhides.push_back(filePath);
    }

    std::vector<Segment>& getHiddenSegments(const std::string& filePath){
        static std::map<std::string,std::vector<Segment>> allSegments;
        auto it = allSegments.find(filePath);
        if (it == allSegments.end()) {
            allSegments.insert(std::pair<std::string, std::vector<Segment>>(filePath, std::vector<Segment>()));
            return getHiddenSegments(filePath);
        } else {
            return it->second;
        }
    }

}