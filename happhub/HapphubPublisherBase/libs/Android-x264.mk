X264_PATH := $(call my-dir)
X264_PATH_FULL := $(shell readlink -m "$(HAPPHUB_PUBLISHER_PATH)")
include $(CLEAR_VARS)

X264_PWD := $(shell pwd)

X264_VERSION := 0.129.x

ifeq ($(TARGET_ARCH_ABI),armeabi-v7a)
X264_EXTRA_CFLAGS += -march=armv7-a -mfpu=neon
endif

X264_SRC_DIR := $(X264_PATH)/x264/x264-$(X264_VERSION)
X264_BUILD_DIR := $(HAPPHUB_BUILD_DIR)/x264
ifneq ($(HAPPHUB_CLEAN),clean)
__IGNORE := $(shell mkdir -p "$(X264_BUILD_DIR)")
endif 

X264_CONFIGURATION := \
    --prefix="$(HAPPHUB_INSTALL_DIR)" \
    --host=arm-linux \
    --cross-prefix="$(NDK_CROSS_PREFIX)" \
    --sysroot="$(NDK_SYSROOT)" \
    --disable-cli \
    --enable-shared \
    --enable-pic \
    --extra-cflags="$(X264_EXTRA_CFLAGS)"
    
ifneq ($(HAPPHUB_CLEAN),clean)
X264_CONFIGURATION_OLD := $(shell cat "$(X264_BUILD_DIR)/.configured" 2> /dev/null)
ifneq ($(X264_CONFIGURATION),$(X264_CONFIGURATION_OLD))
    $(info $(TARGET_ARCH_ABI): Configuring X264 with:)
    $(info $(X264_CONFIGURATION))
    X264_CONFIGURATION_OUT := $(subst ",\",$(X264_CONFIGURATION))
    X264_CONFIGURE_CMD := "$(X264_SRC_DIR)/configure" $(X264_CONFIGURATION) && echo "$(X264_CONFIGURATION_OUT)" > "$(X264_BUILD_DIR)/.configured"

    __IGNORE := $(shell cd "$(X264_BUILD_DIR)" && $(X264_CONFIGURE_CMD))
endif
endif

ifneq ($(HAPPHUB_CLEAN),clean)
$(info $(TARGET_ARCH_ABI): Building X264...)
__IGNORE := $(shell cd "$(X264_BUILD_DIR)" && make install)
__IGNORE := $(shell cd "$(X264_PWD)")
endif

__LIBS += libx264.so
__LIBS_MODULE += libx264
__LIBS_SEARCH += libx264.so.129
__LIBS_IDX += $(words $(__LIBS))
