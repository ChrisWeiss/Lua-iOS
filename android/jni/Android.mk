# find project root
PROJECT_PATH:= $(shell (/usr/bin/git rev-parse --show-toplevel))

# set local compilation path
LOCAL_PATH:= $(PROJECT_PATH)/lua-5.2.2/src
#$(warning $(LOCAL_PATH))

# read list of c/cpp files
FILES := $(notdir $(wildcard $(LOCAL_PATH)/*.c))
#$(warning $(FILES))

# lua static lib
include $(CLEAR_VARS)

LOCAL_MODULE    := lua
LOCAL_SRC_FILES := $(FILES)
LOCAL_C_INCLUDES := $(LOCAL_PATH)
LOCAL_CFLAGS := "-DLUA_USE_ANDROID"
$(warning $(LOCAL_C_INCLUDES))

include $(BUILD_STATIC_LIBRARY)

# lua shared lib
include $(CLEAR_VARS)
LOCAL_MODULE    := luaShared
LOCAL_STATIC_LIBRARIES := lua
include $(BUILD_SHARED_LIBRARY)


