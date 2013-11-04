LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE    := render
LOCAL_SRC_FILES := render.cpp

include $(BUILD_SHARED_LIBRARY)
