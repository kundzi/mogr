LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE    := render
LOCAL_SRC_FILES := render.cpp

LOCAL_LDLIBS += -llog -lGLESv2 -lEGL -landroid

include $(BUILD_SHARED_LIBRARY)
