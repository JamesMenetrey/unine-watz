# Build script for the Android Native Development Kit (NDK).
# Android.mk describes sources and shared libraries to the NDK build system.

include $(call all-subdir-makefiles)

###################### some-ta ######################
LOCAL_PATH := $(call my-dir)

# TODO: path has to be passed as argument
OPTEE_CLIENT_EXPORT = $(LOCAL_PATH)/../../optee_client/out/export

include $(CLEAR_VARS)
LOCAL_CFLAGS += -DANDROID_BUILD
LOCAL_CFLAGS += -Wall

LOCAL_SRC_FILES += host/main.c

LOCAL_C_INCLUDES := $(LOCAL_PATH)/ta/include \
		$(OPTEE_CLIENT_EXPORT)/include \

LOCAL_SHARED_LIBRARIES := libteec
LOCAL_MODULE := some_ta
LOCAL_VENDOR_MODULE := true
LOCAL_MODULE_TAGS := optional
include $(BUILD_EXECUTABLE)

include $(LOCAL_PATH)/ta/Android.mk
