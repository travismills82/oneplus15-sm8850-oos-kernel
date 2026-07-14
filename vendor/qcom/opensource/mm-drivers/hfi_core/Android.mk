LOCAL_PATH := $(call my-dir)
LOCAL_MODULE_DDK_BUILD := true
include $(CLEAR_VARS)

# This makefile is only for DLKM
ifneq ($(findstring vendor,$(LOCAL_PATH)),)

ifneq ($(findstring opensource,$(LOCAL_PATH)),)
	MSM_HFI_CORE_BLD_DIR := $(TOP)/vendor/qcom/opensource/mm-drivers/hfi_core
endif # opensource

DLKM_DIR := $(TOP)/device/qcom/common/dlkm

LOCAL_ADDITIONAL_DEPENDENCIES := $(wildcard $(LOCAL_PATH)/**/*) $(wildcard $(LOCAL_PATH)/*)

###########################################################
# This is set once per LOCAL_PATH, not per (kernel) module
KBUILD_OPTIONS := MSM_HFI_CORE_ROOT=$(MSM_HFI_CORE_BLD_DIR)
KBUILD_OPTIONS += MODNAME=msm_hfi_core
KBUILD_OPTIONS += BOARD_PLATFORM=$(TARGET_BOARD_PLATFORM)

###########################################################
include $(CLEAR_VARS)
LOCAL_SRC_FILES           := $(wildcard $(LOCAL_PATH)/**/*) $(wildcard $(LOCAL_PATH)/*)
LOCAL_MODULE              := hfi-core-module-symvers
LOCAL_MODULE_STEM         := Module.symvers
LOCAL_MODULE_KBUILD_NAME  := Module.symvers
LOCAL_MODULE_PATH         := $(KERNEL_MODULES_OUT)

include $(DLKM_DIR)/Build_external_kernelmodule.mk
###########################################################
include $(CLEAR_VARS)
LOCAL_SRC_FILES   := $(wildcard $(LOCAL_PATH)/**/*) $(wildcard $(LOCAL_PATH)/*)
LOCAL_MODULE              := msm_hfi_core.ko
LOCAL_MODULE_KBUILD_NAME  := msm_hfi_core.ko
LOCAL_MODULE_TAGS         := optional
LOCAL_MODULE_DEBUG_ENABLE := true
LOCAL_MODULE_PATH         := $(KERNEL_MODULES_OUT)

include $(DLKM_DIR)/Build_external_kernelmodule.mk
###########################################################
endif # DLKM check
