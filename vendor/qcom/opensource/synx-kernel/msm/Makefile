# SPDX-License-Identifier: GPL-2.0-only

obj-m += synx-driver.o
ifeq ($(BOARD_PLATFORM), gen5)
synx-driver-objs := synx/synx_compat.o synx/synx_stub.o
else
obj-m += synx/ipclite.o
synx-driver-objs := synx/synx.o synx/synx_util.o synx/synx_debugfs.o synx/synx_global.o \
			synx/synx_compat.o synx/synx_test_ioctl.o
synx-driver-$(CONFIG_DEBUG_FS) += synx/synx_debugfs_util.o
endif
