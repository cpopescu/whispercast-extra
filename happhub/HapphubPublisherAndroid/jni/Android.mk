LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
include $(LOCAL_PATH)/../../HapphubPublisherBase/Android.mk

include $(CLEAR_VARS)
LOCAL_MODULE := publisher

LOCAL_C_INCLUDES := \
  $(HAPPHUB_PUBLISHER_PATH)/install/android/$(TARGET_ARCH_ABI)/include
  
LOCAL_SRC_FILES := \
  PublisherImpl.cpp

LOCAL_STATIC_LIBRARIES := libpublisherBase
LOCAL_SHARED_LIBRARIES := libavutil libavcodec libavformat libswresample libswscale

LOCAL_LDLIBS += -llog

LOCAL_CFLAGS += -D__STDC_CONSTANT_MACROS -D__ANDROID

include $(BUILD_SHARED_LIBRARY)