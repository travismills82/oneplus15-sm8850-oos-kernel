# Build audio kernel driver
ifneq ($(TARGET_BOARD_AUTO),true)
ifeq ($(TARGET_USES_QMAA),true)
  ifeq ($(TARGET_USES_QMAA_OVERRIDE_BLUETOOTH), true)
     ifeq ($(call is-board-platform-in-list,$(TARGET_BOARD_PLATFORM)),true)
           BT_KERNEL_DRIVER := $(KERNEL_MODULES_OUT)/btpower.ko
           BT_KERNEL_DRIVER += $(KERNEL_MODULES_OUT)/radio-i2c-rtc6226-qca.ko
           ifeq ($(TARGET_USES_QMAA_OVERRIDE_BLUETOOTH_AUDIO), true)
             ifeq ($(call is-board-platform-in-list, sun canoe), true)
               BT_KERNEL_DRIVER += $(KERNEL_MODULES_OUT)/btfmcodec.ko
               BT_KERNEL_DRIVER += $(KERNEL_MODULES_OUT)/btfm_slim_codec.ko
               BT_KERNEL_DRIVER += $(KERNEL_MODULES_OUT)/bt_fm_swr.ko
             else
               BT_KERNEL_DRIVER += $(KERNEL_MODULES_OUT)/bt_fm_slim.ko
             endif
           endif
           ifeq ($(TARGET_USES_QTI_UWB), true)
             BT_KERNEL_DRIVER += $(KERNEL_MODULES_OUT)/spi_cnss_proto.ko
           endif
           ifeq ($(BOARD_HAVE_STANDALONE_THREAD), true)
             BT_KERNEL_DRIVER += $(KERNEL_MODULES_OUT)/thqspi_proto.ko
           endif
           BOARD_VENDOR_KERNEL_MODULES += $(BT_KERNEL_DRIVER)
     endif
  endif
else
  ifeq ($(call is-board-platform-in-list,$(TARGET_BOARD_PLATFORM)),true)
     BT_KERNEL_DRIVER := $(KERNEL_MODULES_OUT)/btpower.ko
     BT_KERNEL_DRIVER += $(KERNEL_MODULES_OUT)/radio-i2c-rtc6226-qca.ko
     ifeq ($(TARGET_USES_QMAA_OVERRIDE_BLUETOOTH_AUDIO), true)
       ifeq ($(call is-board-platform-in-list, sun canoe), true)
         BT_KERNEL_DRIVER += $(KERNEL_MODULES_OUT)/btfmcodec.ko
         BT_KERNEL_DRIVER += $(KERNEL_MODULES_OUT)/btfm_slim_codec.ko
         BT_KERNEL_DRIVER += $(KERNEL_MODULES_OUT)/bt_fm_swr.ko
       else
         BT_KERNEL_DRIVER += $(KERNEL_MODULES_OUT)/bt_fm_slim.ko
       endif
     endif
     ifeq ($(TARGET_USES_QTI_UWB), true)
       BT_KERNEL_DRIVER += $(KERNEL_MODULES_OUT)/spi_cnss_proto.ko
     endif
     ifeq ($(BOARD_HAVE_STANDALONE_THREAD), true)
         BT_KERNEL_DRIVER += $(KERNEL_MODULES_OUT)/thqspi_proto.ko
     endif
     BOARD_VENDOR_KERNEL_MODULES += $(BT_KERNEL_DRIVER)
  endif
endif
endif
