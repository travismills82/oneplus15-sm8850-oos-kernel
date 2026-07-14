# SPDX-License-Identifier: GPL-2.0-only
#
# Makefile for the overlay filesystem.
#

LINUXINCLUDE += -include $(srctree)/fs/internal.h

obj-$(CONFIG_OPLUS_OVERLAY_FS) += oplus_overlay.o

oplus_overlay-y := super.o namei.o util.o inode.o file.o dir.o readdir.o \
		copy_up.o export.o params.o xattrs.o
