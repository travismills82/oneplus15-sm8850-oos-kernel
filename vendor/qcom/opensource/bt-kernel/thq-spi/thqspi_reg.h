/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: GPL-2.0-only
 */

#ifndef __LINUX_THQSPI_REG_H
#define __LINUX_THQSPI_REG_H

/* SPI SLAVE MODULE registers */
#define SPI_SLV_REG_DMA_SIZE                             0x0100  // DMA Size
#define SPI_SLV_REG_WRBUF_SPC_AVA                        0x0200  // Write buffer space available
#define SPI_SLV_REG_RDBUF_BYTE_AVA                       0x0300  // Read buffer byte available
#define SPI_SLV_REG_SPI_CONFIG                           0x0400  // SPI configuration
#define SPI_SLV_REG_SPI_STATUS                           0x0500  // SPI Status
#define SPI_SLV_REG_HOST_CTRL_BYTE_SIZE                  0x0600  // Host control register access byte size

#define SPI_SLV_REG_HOST_CTRL_CONFIG                     0x0700  // Host control register configure
#define SPI_SLV_REG_HOST_CTRL_NOADDRINC                  (1 << 6)

#define SPI_SLV_REG_HOST_CTRL_RD_PORT                    0x0800  // Host control register read port
#define SPI_SLV_REG_HOST_CTRL_WR_PORT                    0x0A00  // Host control register write port
#define SPI_SLV_REG_INTR_CAUSE                           0x0C00  // Interrupt cause

#define SPI_SLV_REG_INT_PACKET_AVAIL                     (1 << 0)
#define SPI_SLV_REG_INT_RDBUF_ERROR                      (1 << 1)
#define SPI_SLV_REG_INT_WRBUF_ERROR                      (1 << 2)
#define SPI_SLV_REG_INT_ADDRESS_ERROR                    (1 << 3)
#define SPI_SLV_REG_INT_LOCAL_CPU                        (1 << 4)
#define SPI_SLV_REG_INT_COUNTER                          (1 << 5)
#define SPI_SLV_REG_INT_CPU_ON                           (1 << 6)
#define SPI_SLV_REG_INT_ALL_CPU                          (1 << 7)
#define SPI_SLV_REG_INT_HCR_WR_DONE                      (1 << 8)
#define SPI_SLV_REG_INT_HCR_RD_DONE                      (1 << 9)
#define SPI_SLV_REG_INT_WRBUF_BEL_WATERMARK              (1 << 10)

#define SPI_SLV_REG_INTR_ENABLE                          0x0D00  // Interrupt enable
#define SPI_SLV_REG_WRBUF_WRPTR                          0x0E00  // Holds the write pointer for the write buffer
#define SPI_SLV_REG_WRBUF_RDPTR                          0x0F00  // Holds the read pointer for the write buffer
#define SPI_SLV_REG_RDBUF_WRPTR                          0x1000  // Holds the write pointer for the read buffer
#define SPI_SLV_REG_RDBUF_RDPTR                          0x1100  // Holds the read pointer for the read buffer
#define SPI_SLV_REG_RDBUF_WATERMARK                      0x1200  // Read buffer water mark
#define SPI_SLV_REG_WRBUF_WATERMARK                      0x1300  // Write buffer water mark
#define SPI_SLV_REG_RDBUF_LOOKAHEAD1                     0x1400  // Read buffer lookahead 1
#define SPI_SLV_REG_RDBUF_LOOKAHEAD2                     0x1500  // Read buffer lookahead 2

#define SPI_INT_ENABLE_DEFAULT                           (SPI_SLV_REG_INT_PACKET_AVAIL | SPI_SLV_REG_INT_RDBUF_ERROR | SPI_SLV_REG_INT_WRBUF_ERROR | SPI_SLV_REG_INT_ADDRESS_ERROR | SPI_SLV_REG_INT_LOCAL_CPU | SPI_SLV_REG_INT_COUNTER)

#define SPI_SLV_MAX_IO_BUFFER_BYTES                      3200

#define SPI_INTERNAL_8BIT_MODE                           0x40
#define SPI_READ_8BIT_MODE                               0x80
#define SPI_WRITE_8BIT_MODE                              0x00

#define SPI_INTERNAL_16BIT_MODE                          0x4000
#define SPI_EXTERNAL_16BIT_MODE                          0x0000
#define SPI_READ_16BIT_MODE                              0x8000
#define SPI_WRITE_16BIT_MODE                             0x0000
#define SPI_REG_ADDRESS_MASK                             ((1 << 14) - 1)

#define SPI_CMD_MASK_16BIT_MODE                          0x3FFF

#define SPI_SLV_REG_SPI_CONFIG_VAL_RESET                 (0x1 << 15)

#define SPI_SLV_REG_SPI_CONFIG_SWAP_BIT_LOC              0x2
#define SPI_SLV_REG_SPI_CONFIG_16BITMODE_BIT_LOC         0x1

#define SPI_SLV_REG_SPI_CONFIG_SWAP_BIT_MSK              ~(1<<SPI_SLV_REG_SPI_CONFIG_SWAP_BIT_LOC)
#define SPI_SLV_REG_SPI_CONFIG_16BITMODE_BIT_MSK         ~(1<<SPI_SLV_REG_SPI_CONFIG_16BITMODE_BIT_LOC)

#define SPI_SLV_REG_SPI_CONFIG_MBOX_INTR_EN              (1 << 8)
#define SPI_SLV_REG_SPI_CONFIG_IO_ENABLE                 (1 << 7)
#define SPI_SLV_REG_SPI_CONFIG_KEEP_AWAKE_FOR_INTR       (1 << 4)
#define SPI_SLV_REG_SPI_CONFIG_KEEP_AWAKE_EN             (1 << 3)
#define SPI_SLV_REG_SPI_CONFIG_SWAP                      (1 << 2)
#define SPI_SLV_REG_SPI_CONFIG_16BITMODE                 (1 << 1)
#define SPI_SLV_REG_SPI_CONFIG_PREFETCH_MODE             (1 << 0)

/* Default setting for SPI_SLV_REG_SPI_CONFIG register */
#define SPI_CONFIG_DEFAULT                               (SPI_SLV_REG_SPI_CONFIG_MBOX_INTR_EN | SPI_SLV_REG_SPI_CONFIG_IO_ENABLE | SPI_SLV_REG_SPI_CONFIG_PREFETCH_MODE)

#define HOST_CTRL_REG_DIR_RD                             (0 << 14)
#define HOST_CTRL_REG_DIR_WR                             (1 << 14)
#define HOST_CTRL_REG_ENABLE                             (1 << 15)

/* Should always work the first time....no retry needed.  */
#define SPI_SLV_TRANS_RETRY_COUNT                        (5)
#define SPI_SLV_IO_DELAY                                 (25) /* microseconds */

#define SPI_SLV_RESET_DELAY                              (25) /* microseconds */

#define HOST_CTRL_REG_ADDRESS_MSK                        (0x3FFF)
#define SPI_SLV_REG_SPI_STATUS_BIT_HOST_ACCESS_DONE_MSK  (0x1)
#define SPI_SLV_REG_SPI_STATUS_BIT_HOST_ACCESS_DONE_VAL  (0x1)

#endif //__LINUX_THQSPI_REG_H
