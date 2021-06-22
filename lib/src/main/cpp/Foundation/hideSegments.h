//
// Created by Alienware on 2019/11/30.
//

#ifndef PEAK_ROOT_SUPPORT_FAKESEGMENTS_H
#define PEAK_ROOT_SUPPORT_FAKESEGMENTS_H

#include <cerrno>
#include <vector>
#include <string>
#include <limits.h>
#include <sys/mman.h>

#include "Log.h"

#define ASHMEM_SET_NAME 0x41007701

namespace hideSegments{

    typedef struct Segment{
        void *startAddress;
        void *endAddress;
        off64_t offset;
        int permission;

        Segment(void *startAddress, void *endAddress, off64_t offset, int permission)
                : startAddress(startAddress), endAddress(endAddress), offset(offset),
                  permission(permission) {}
    } Segment;

    std::vector<Segment>& getHiddenSegments(const std::string& filePath);

    void addHideSegment(const std::string& filePath);

    void* new_mmap64(void *__addr, size_t __size, int __prot, int __flags, int __fd,off64_t __offset);
    extern void* (*orig_mmap64)(void *__addr, size_t __size, int __prot, int __flags, int __fd,off64_t __offset);

}

#endif //PEAK_ROOT_SUPPORT_FAKESEGMENTS_H
