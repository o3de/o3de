#
# Android Makefile conversion
# 
# Leander Beernaert
#
# How to build: $ANDROID_NDK/ndk-build 
#
VERSION=1.17

LOCAL_PATH := $(call my-dir)/../

include $(CLEAR_VARS)

LOCAL_ARM_MODE := arm
LOCAL_ARM_NEON := true

LOCAL_MODULE    := HLSLcc

LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/include \
	$(LOCAL_PATH)/src \
	$(LOCAL_PATH)/src/cbstring
LOCAL_CFLAGS += -Wall -W
# For dynamic library
#LOCAL_CFLAGS += -DHLSLCC_DYNLIB
LOCAL_SRC_FILES := $(wildcard $(LOCAL_PATH)/src/*.c) \
	$(wildcard $(LOCAL_PATH)/src/cbstring/*.c) \
	$(wildcard $(LOCAL_PATH)/src/internal_includes/*.c)
#LOCAL_LDLIBS += -lGLESv3

include $(BUILD_STATIC_LIBRARY)

