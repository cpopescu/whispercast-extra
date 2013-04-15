FFMPEG_PATH := $(call my-dir)
FFMPEG_PATH_FULL := $(shell readlink -m "$(HAPPHUB_PUBLISHER_PATH)")
include $(CLEAR_VARS)

FFMPEG_PWD := $(shell pwd)

FFMPEG_VERSION := 1.1.1

FFMPEG_SRC_DIR := $(FFMPEG_PATH)/ffmpeg/ffmpeg-$(FFMPEG_VERSION)
FFMPEG_BUILD_DIR := $(HAPPHUB_BUILD_DIR)/ffmpeg
ifneq ($(HAPPHUB_CLEAN),clean)
__IGNORE := $(shell mkdir -p "$(FFMPEG_BUILD_DIR)")
endif

FFMPEG_CONFIGURATION := \
    --prefix="$(HAPPHUB_INSTALL_DIR)" \
    --extra-cflags="-I$(HAPPHUB_INSTALL_DIR)/include -DFF_API_AV_GETTIME=0" \
    --extra-ldflags="-L$(HAPPHUB_INSTALL_DIR)/lib" \
    --arch=$(TARGET_ARCH) \
    --target-os=$(TARGET_OS) \
    --enable-cross-compile \
    --cross-prefix="$(NDK_CROSS_PREFIX)" \
    --sysroot="$(NDK_SYSROOT)" \
    --enable-memalign-hack \
    --disable-yasm \
    --enable-shared \
    --disable-static \
    --disable-programs \
    --disable-avdevice \
    --disable-avfilter \
    --disable-everything \
    --enable-decoder=aac \
    --enable-decoder=flv \
    --enable-decoder=h264 \
    --enable-encoder=aac \
    --enable-encoder=flv \
    --enable-muxer=mpegts \
    --enable-muxer=flv \
    --enable-demuxer=mpegts \
    --enable-demuxer=mpegtsraw \
    --enable-demuxer=flv \
    --enable-protocol=file \
    --enable-protocol=http \
    --extra-version=android
    
ifeq ($(USE_X264),1)
FFMPEG_CONFIGURATION += --enable-gpl --enable-libx264 --enable-encoder=libx264
endif
    
ifeq ($(NDK_DEBUG),1)
FFMPEG_CONFIGURATION += --disable-stripping --disable-optimizations
endif
    
ifneq ($(HAPPHUB_CLEAN),clean)
FFMPEG_CONFIGURATION_OLD := $(shell cat "$(FFMPEG_BUILD_DIR)/.configured" 2> /dev/null)
ifneq ($(FFMPEG_CONFIGURATION),$(FFMPEG_CONFIGURATION_OLD))
    $(info $(TARGET_ARCH_ABI): Configuring FFMPEG with:)
    $(info $(FFMPEG_CONFIGURATION))
    FFMPEG_CONFIGURATION_OUT := $(subst ",\",$(FFMPEG_CONFIGURATION))
    FFMPEG_CONFIGURE_CMD := "$(FFMPEG_SRC_DIR)/configure" $(FFMPEG_CONFIGURATION) && echo $(FFMPEG_CONFIGURATION_OUT) > "$(FFMPEG_BUILD_DIR)/.configured"
    __IGNORE := $(shell cd "$(FFMPEG_BUILD_DIR)" && $(FFMPEG_CONFIGURE_CMD))
endif
endif

ifneq ($(HAPPHUB_CLEAN),clean)
$(info $(TARGET_ARCH_ABI): Building FFMPEG...)
__IGNORE := $(shell cd "$(FFMPEG_BUILD_DIR)" && make install)
__IGNORE := $(shell cd "$(FFMPEG_PWD)")
endif

__LIBS += libavcodec.so
__LIBS_MODULE += libavcodec
__LIBS_SEARCH := $(__LIBS_SEARCH) libavcodec.so.54
__LIBS_IDX += $(words $(__LIBS))   

__LIBS += libavformat.so
__LIBS_MODULE += libavformat
__LIBS_SEARCH += libavformat.so.54
__LIBS_IDX += $(words $(__LIBS))   

__LIBS += libavutil.so
__LIBS_MODULE += libavutil
__LIBS_SEARCH += libavutil.so.52
__LIBS_IDX += $(words $(__LIBS))   

__LIBS += libswresample.so
__LIBS_MODULE += libswresample
__LIBS_SEARCH += libswresample.so.0
__LIBS_IDX += $(words $(__LIBS))

__LIBS += libswscale.so
__LIBS_MODULE += libswscale
__LIBS_SEARCH += libswscale.so.2
__LIBS_IDX += $(words $(__LIBS))