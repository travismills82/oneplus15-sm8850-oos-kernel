# SPDX-License-Identifier: GPL-2.0-only
TARGET_VIDC_ENABLE := false
ifeq ($(TARGET_KERNEL_DLKM_DISABLE), true)
	ifeq ($(TARGET_KERNEL_DLKM_VIDEO_OVERRIDE), true)
		TARGET_VIDC_ENABLE := true
	endif
else
TARGET_VIDC_ENABLE := true
endif

#Set FULL_VIRTUALIZATION_ENABLE to false to enable paravirtualization
FULL_VIRTUALIZATION_ENABLE := true

# Build video kernel driver
ifeq ($(TARGET_VIDC_ENABLE),true)
ifeq ($(call is-board-platform-in-list,$(TARGET_BOARD_PLATFORM)),true)
ifneq ($(ENABLE_HYP),true)
BOARD_VENDOR_KERNEL_MODULES += $(KERNEL_MODULES_OUT)/msm_video.ko
else
ifeq ($(TARGET_BOARD_PLATFORM),gen5)
ifeq ($(FULL_VIRTUALIZATION_ENABLE),true)
BOARD_VENDOR_KERNEL_MODULES += $(KERNEL_MODULES_OUT)/msm_video.ko
endif
endif
endif
endif
endif

BUILD_VIDEO_TECHPACK_SOURCE := true
