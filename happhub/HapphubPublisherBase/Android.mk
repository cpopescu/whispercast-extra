HAPPHUB_PUBLISHER_PATH := $(call my-dir)
HAPPHUB_PUBLISHER_PATH_FULL := $(shell readlink -m "$(HAPPHUB_PUBLISHER_PATH)")

HAPPHUB_PUBLISHER_PROJECT := $(shell basename $(HAPPHUB_PUBLISHER_PATH_FULL))

HAPPHUB_CLEAN := $(findstring clean,$(MAKECMDGOALS))

HAPPHUB_DIR := $(call my-dir)
HAPPHUB_DIR_FULL := $(shell readlink -m "$(HAPPHUB_DIR)")

HAPPHUB_INSTALL_DIR := $(HAPPHUB_DIR_FULL)/install/android/$(TARGET_ARCH_ABI)
HAPPHUB_BUILD_DIR := $(HAPPHUB_DIR_FULL)/build/android/$(TARGET_ARCH_ABI)

ifneq ($(HAPPHUB_CLEAN),clean)
__IGNORE := $(shell mkdir -p "$(HAPPHUB_INSTALL_DIR)")
__IGNORE := $(shell mkdir -p "$(HAPPHUB_BUILD_DIR)")
else
__IGNORE := $(shell rm -rf "$(HAPPHUB_INSTALL_DIR)")
__IGNORE := $(shell rm -rf "$(HAPPHUB_BUILD_DIR)")
endif

include $(HAPPHUB_PUBLISHER_PATH)/libs/Android.mk

ifneq ($(HAPPHUB_CLEAN),clean)
__IGNORE := $(shell mkdir -p "$(HAPPHUB_INSTALL_DIR)/include")
__IGNORE := $(shell cp "$(HAPPHUB_PUBLISHER_PATH_FULL)/src/BufferIO.h" "$(HAPPHUB_INSTALL_DIR)/include/")
__IGNORE := $(shell cp "$(HAPPHUB_PUBLISHER_PATH_FULL)/src/Log.h" "$(HAPPHUB_INSTALL_DIR)/include/")
__IGNORE := $(shell cp "$(HAPPHUB_PUBLISHER_PATH_FULL)/src/Publisher.h" "$(HAPPHUB_INSTALL_DIR)/include/")
__IGNORE := $(shell cp "$(HAPPHUB_PUBLISHER_PATH_FULL)/src/Threads.h" "$(HAPPHUB_INSTALL_DIR)/include/")
__IGNORE := $(shell cp "$(HAPPHUB_PUBLISHER_PATH_FULL)/src/VideoConverter.h" "$(HAPPHUB_INSTALL_DIR)/include/")
endif

include $(CLEAR_VARS)
LOCAL_MODULE := publisherBase

LOCAL_C_INCLUDES := \
  $(HAPPHUB_PUBLISHER_PATH)/install/android/$(TARGET_ARCH_ABI)/include
  
# The source paths are relative to the project path (ANDROID-PROJECT/jni),
# and we are assuming the ANDROID-PROJECT is on the same level as us...

LOCAL_SRC_FILES := \
  ../../$(HAPPHUB_PUBLISHER_PROJECT)/src/BufferIO.cpp \
  ../../$(HAPPHUB_PUBLISHER_PROJECT)/src/Log.cpp \
  ../../$(HAPPHUB_PUBLISHER_PROJECT)/src/Publisher.cpp \
  ../../$(HAPPHUB_PUBLISHER_PROJECT)/src/Threads.cpp \
  ../../$(HAPPHUB_PUBLISHER_PROJECT)/src/VideoConverter.cpp

LOCAL_SHARED_LIBRARIES := libavutil libavcodec libavformat libswscale libswresample  

LOCAL_CFLAGS += -D__STDC_CONSTANT_MACROS -D__ANDROID 

include $(BUILD_STATIC_LIBRARY)