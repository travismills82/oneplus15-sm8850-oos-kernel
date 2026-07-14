MM_DRV_DLKM_ENABLE := true
ifeq ($(TARGET_KERNEL_DLKM_DISABLE), true)
	ifeq ($(TARGET_KERNEL_DLKM_MM_DRV_OVERRIDE), false)
		MM_DRV_DLKM_ENABLE := false
	endif
endif

ifeq ($TARGET_USES_QMAA, true)
	ifeq ($(TARGET_USES_QMAA_OVERRIDE_MM_DRV), false)
		MM_DRV_DKLM_ENABLE := false
	endif
endif

ifeq ($(MM_DRV_DLKM_ENABLE), true)
	ifneq ($(TARGET_BOARD_PLATFORM), taro)
		PRODUCT_PACKAGES += sync_fence.ko msm_ext_display.ko
		DISPLAY_MM_DRIVER += sync_fence.ko msm_ext_display.ko
	endif

	ifneq ($(filter $(TARGET_BOARD_PLATFORM), vienna),$(TARGET_BOARD_PLATFORM))
		PRODUCT_PACKAGES += msm_hw_fence.ko
		DISPLAY_MM_DRIVER += msm_hw_fence.ko
	endif

	ifeq ($(filter $(TARGET_BOARD_PLATFORM), canoe vienna seraph),$(TARGET_BOARD_PLATFORM))
			PRODUCT_PACKAGES += msm_hfi_core.ko
			DISPLAY_MM_DRIVER += msm_hfi_core.ko
	endif
endif
