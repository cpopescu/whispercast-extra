TARGET_OS := linux

NDK_CROSS_PREFIX := $(shell "$(NDK_ROOT)/ndk-which" gcc)
NDK_CROSS_PREFIX := $(subst gcc,,$(NDK_CROSS_PREFIX))

NDK_SYSROOT := $(NDK_ROOT)/platforms/$(TARGET_PLATFORM)/arch-$(TARGET_ARCH)

########################### prepare
__DIR := $(call my-dir)
__DIR_FULL := $(shell readlink -m "$(__DIR)")
include $(CLEAR_VARS)

__LIBS :=
__LIBS_SEARCH :=  
__LIBS_IDX :=

########################### libraries

ifeq ($(USE_X264),1)
include $(__DIR_FULL)/Android-x264.mk
endif
include $(__DIR_FULL)/Android-ffmpeg.mk

########################### postprocess
__RPL_CHECK := $(shell touch /tmp/x && rpl a a /tmp/x > /dev/null 2>&1 && echo SUCCESS)
ifneq ($(__RPL_CHECK),SUCCESS)
$(error $(TARGET_ARCH_ABI): The "rpl" utility is not installed, bailing out..)
endif

__LIBS_DIR_RELATIVE := $(HAPPHUB_PUBLISHER_PROJECT)/$(TARGET_ARCH_ABI)/libs

ifneq ($(HAPPHUB_CLEAN),clean)
$(info $(TARGET_ARCH_ABI): Postprocessing libraries...)

__LIBS_DIR := $(LOCAL_PATH)/$(__LIBS_DIR_RELATIVE)
__IGNORE := $(shell mkdir -p "$(__LIBS_DIR)")

__LIBS_FULL := $(foreach __x,$(__LIBS),$(shell basename "`readlink -f "$(HAPPHUB_INSTALL_DIR)/lib/$(__x)"`"))
__IGNORE := $(foreach __x,$(__LIBS_IDX),$(shell cp "$(HAPPHUB_INSTALL_DIR)/lib/$(word $(__x),$(__LIBS_FULL))" "$(__LIBS_DIR)/$(word $(__x),$(__LIBS))"))
 
__LIBS_REPLACE := $(foreach __x,$(foreach __x,$(__LIBS_SEARCH),$(shell echo "$(__x)" | sed -r 's^((.*)\.so)(.*)^\3^g')),$(shell echo "$(__x)" | sed -r 's^(.{1})^\\\\x00^g'))
__LIBS_REPLACE := $(foreach __x,$(__LIBS_IDX),$(shell echo $(word $(__x),$(__LIBS))$(word $(__x),$(__LIBS_REPLACE))))

__LIBS_PERFORM1 = $(shell rpl -e "$(word $(__x),$(__LIBS_SEARCH))" "$(word $(__x),$(__LIBS_REPLACE))" "$(__LIBS_DIR)/$(word $(__y),$(__LIBS))" > /dev/null 2>&1)
__LIBS_PERFORM2 = $(foreach __x,$(__LIBS_IDX),$(__LIBS_PERFORM1))
__IGNORE := $(foreach __y,$(__LIBS_IDX),$(__LIBS_PERFORM2))
endif

define ___F_INSTALL
include $(CLEAR_VARS)
LOCAL_MODULE := $(1)
LOCAL_SRC_FILES := $(__LIBS_DIR_RELATIVE)/$(1).so
include $(PREBUILT_SHARED_LIBRARY)
endef

$(foreach __y,$(__LIBS_MODULE),$(eval $(call ___F_INSTALL,$(__y))))