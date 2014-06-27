# select target arch
#APP_ABI := armeabi armeabi-v7a x86 mips
#APP_ABI := armeabi
#APP_ABI := armeabi-v7a
APP_ABI := armeabi-v7a
#APP_ABI := x86

# CLANG versions available
#NDK_TOOLCHAIN_VERSION := clang3.3
NDK_TOOLCHAIN_VERSION := clang3.4
# GCC versions available
#NDK_TOOLCHAIN_VERSION := 4.6
#NDK_TOOLCHAIN_VERSION := 4.8


# target android platform
# min version for pthread_rwlock_t
APP_PLATFORM := android-9
#APP_PLATFORM := android-18

# which STL lib to use..
#APP_STL := gnustl_shared
#APP_STL := stlport_shared
#APP_STL := c++_shared
#APP_STL := c++_static
#APP_STL := gnustl_static


# enable c++11 extentions in source code
# using c++_std will enable this by default
# clang and gcc 4.8
#APP_CPPFLAGS += -std=c++11
# gcc 4.6
#APP_CPPFLAGS += -std=c++0x

# enable c11
APP_CFLAGS += -std=c99

