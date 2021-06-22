cmake_minimum_required(VERSION 3.4.1)

#set(LLVM_OBFUSCATOR_PATH "/home/kook/DevelopmentPath/AndroidSDK/ndk/18.1.5063045/toolchains/ollvm/prebuilt/darwin-x86_64")
#set(CMAKE_C_COMPILER ${LLVM_OBFUSCATOR_PATH}/bin/clang${EXE_SUFFIX})
#set(CMAKE_CXX_COMPILER ${LLVM_OBFUSCATOR_PATH}/bin/clang++${EXE_SUFFIX})
#set(OBFUSCATOR_FLAGS "-mllvm -sub -mllvm -fla -mllvm -split -mllvm -split_num=2")
#string(APPEND CMAKE_C_FLAGS " ${OBFUSCATOR_FLAGS}")
#string(APPEND CMAKE_CXX_FLAGS " ${OBFUSCATOR_FLAGS}")


if (${CMAKE_ANDROID_ARCH_ABI} STREQUAL "arm64-v8a")
    set(LIBRARY_NAME "Bugly_64")
else ()
    set(LIBRARY_NAME "Bugly")
endif ()

add_definitions("-DCORE_SO_NAME=\"lib${LIBRARY_NAME}.so\"")

include_directories(.fb)
include_directories(.)
set(CMAKE_ANDROID_STL_TYPE c++_static)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

add_library(
        ${LIBRARY_NAME}
        SHARED
        Jni/VAJni.cpp
        Jni/Helper.cpp
        Jni/JniHook.cpp
        Jni/ArtMethod.h
        Foundation/syscall/BinarySyscallFinder.cpp
        Foundation/fake_dlfcn.cpp
        Foundation/canonicalize_md.c
        Foundation/MapsRedirector.cpp
        Foundation/IORelocator.cpp
        Foundation/VMHook.cpp
        Foundation/Symbol.cpp
        Foundation/SandboxFs.cpp
        Foundation/hideSegments.cpp
        Substrate/hde64.c
        Substrate/SubstrateDebug.cpp
        Substrate/SubstrateHook.cpp
        Substrate/SubstratePosixMemory.cpp
        Substrate/And64InlineHook.cpp
        Foundation/SymbolFinder.cpp
)

find_library(log-lib log)

target_link_libraries(${LIBRARY_NAME} ${log-lib})