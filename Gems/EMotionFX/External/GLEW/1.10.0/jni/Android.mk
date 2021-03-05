#
# Android Makefile conversion
# 
# Leander Beernaert
#
# How to build: $ANDROID_NDK/ndk-build 
#
VERSION=1.17

LOCAL_PATH := $(call my-dir)/../

MY_C_INCLUDES := $(LOCAL_PATH)/include
MY_CFLAGS := -DGLEW_NO_GLU -Wall -W
MY_LDLIBS += -lGLESv3 -lEGL
MY_SRC_FILES := src/glew.c #src/glewinfo.c src/visualinfo.c

include $(CLEAR_VARS)

LOCAL_ARM_MODE := arm
LOCAL_ARM_NEON := true

LOCAL_MODULE    := GLEW

LOCAL_C_INCLUDES := $(MY_C_INCLUDES)
LOCAL_CFLAGS += $(MY_CFLAGS)
LOCAL_SRC_FILES := $(MY_SRC_FILES)
LOCAL_LDLIBS += $(MY_LDLIBS)

include $(BUILD_SHARED_LIBRARY)

include $(CLEAR_VARS)

LOCAL_ARM_MODE := arm
LOCAL_ARM_NEON := true

LOCAL_MODULE    := GLEWmx

LOCAL_C_INCLUDES := $(MY_C_INCLUDES)
LOCAL_CFLAGS += -DGLEW_MX $(MY_CFLAGS)
LOCAL_SRC_FILES := $(MY_SRC_FILES)
LOCAL_LDLIBS += $(MY_LDLIBS)

include $(BUILD_SHARED_LIBRARY)
