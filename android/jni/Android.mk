# find project root
LUA_PROJECT_PATH:= $(realpath $(join $(dir $(call this-makefile)), /../.. ))
LUA_PROJECT_PATH:= $(LUA_PROJECT_PATH)/lua-5.2.2/src
#$(warning $(LUA_PROJECT_PATH))
# set local compilation path
LOCAL_PATH:= $(LUA_PROJECT_PATH)

# list of c/cpp files
LUA_FILES := $(filter-out %luac.c,$(wildcard $(LUA_PROJECT_PATH)/*.c))

include $(CLEAR_VARS)
LOCAL_MODULE    := lua
LOCAL_SRC_FILES := $(LUA_FILES)
LOCAL_C_INCLUDES := $(LUA_PROJECT_PATH)
LOCAL_EXPORT_C_INCLUDES := $(LUA_PROJECT_PATH)
#special luaconf.h directive..
LOCAL_CFLAGS += "-DLUA_USE_ANDROID"
LOCAL_CFLAGS += -fdiagnostics-show-category=name
LOCAL_CFLAGS += -Wall
LOCAL_CFLAGS += -Woverloaded-virtual
LOCAL_CFLAGS += -Werror
LOCAL_CFLAGS += -Wno-invalid-noreturn
include $(BUILD_STATIC_LIBRARY)

#lua shared lib
#include $(CLEAR_VARS)
#LOCAL_MODULE    := luaShared
#LOCAL_STATIC_LIBRARIES := lua
#include $(BUILD_SHARED_LIBRARY)


