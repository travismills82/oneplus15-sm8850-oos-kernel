/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 */

#ifndef __HFI_QUEUE_H__
#define __HFI_QUEUE_H__

#include <linux/types.h>
#include <linux/device.h>

/**
 * struct for buffer payloads for TX/RX. Client of HFI queue allocates and
 * submits buffer
 * @kva: kernel(driver) virtual address
 * @dva: device virtual address
 * @buf_len: buffer length
 */
struct hfi_queue_buffer {
	u64 kva;
	u64 dva;
	u32 buf_len;
	u32 idx;
};

/**
 * enum for queue direction
 * @hfi_queue_tx: submit command buffer from driver to device
 * @hfi_queue_rx: submit response buffer from driver to device
 */
enum hfi_queue_buffer_queue_dir {
	hfi_queue_tx,
	hfi_queue_rx,
};

/**
 * struct for queueing buffer payloads for TX/RX.
 * @dir: buffer direction
 * @buf: buffer payload
 */
struct hfi_queue_buffer_queue {
	enum hfi_queue_buffer_queue_dir dir;
	struct hfi_queue_buffer *buf;
};

/**
 * struct containing info for queue creation.
 * @dev: client device creating queue.
 * @qname: name of the queue
 * @va: virtual address for driver/device
 * @va_sz: size of virtual region
 * @q_depth: depth of qeueue
 * @align: alignment requirements.
 * @dma_premapped: true - client dma maps the buffer.
 *                 false - Not supported.
 */
struct hfi_queue_create {
	struct device *dev;
	char *qname;
	void *va;
	u32 va_sz;
	u32 q_depth;
	u32 align;
	bool dma_premapped;
};

/**
 * enum for get_param_hfi_queue/set_param_hfi_queue
 */
enum hfi_queue_param_enum {

	/*
	 * hfi_queue_param_buffer_pool: supported by actions
	 *                             set_param_hfi_queue /
	 *                             get_param_hfi_queue.
	 * @get_param_hfi_queue: API call with hfi_queue_param_buffer_pool gets
	 *                       hfi_queue_buffer from buffer pool.
	 *                       Params for get_param_hfi_queue for
	 *                       hfi_queue_param_buffer_pool:
	 *                           @hfi_queue_handle: handle returned by
	 *                                              create_hfi_queue
	 *                           @payload: struct hfi_queue_buffer_queue*
	 *                           @payload_sz: sizeof(hfi_queue_buffer_queue)
	 *                           Return value:       0 - Success
	 *                                        -ENOBUFS - Queue is empty.
	 *
	 * @set_param_hfi_queue: API call with hfi_queue_param_buffer_pool
	 *                       adds hfi_queue_buffer to buffer pool.
	 *                       Params for set_param_hfi_queue for
	 *                       hfi_queue_param_buffer_pool:
	 *                           @hfi_queue_handle: handle returned by
	 *                                              create_hfi_queue
	 *                           @payload: struct hfi_queue_buffer_queue*
	 *                           @payload_sz: sizeof(hfi_queue_buffer_queue)
	 *                           Return value:     0 - Success
	 *                                        errno - error case.
	 */
	hfi_queue_param_buffer_pool,

	/*
	 * hfi_queue_param_buffer_queue: supported by actions
	 *                               set_param_hfi_queue /
	 *                               get_param_hfi_queue.
	 * @get_param_hfi_queue: API call with hfi_queue_param_buffer_queue
	 *                       gets hfi_queue_buffer from ring buffer
	 *                       shared between device and driver.
	 *                       Params for get_param_hfi_queue for
	 *                       hfi_queue_param_buffer_queue:
	 *                           @hfi_queue_handle: handle returned by
	 *                                              create_hfi_queue
	 *                           @payload: struct hfi_queue_buffer_queue*
	 *                           @payload_sz: sizeof(hfi_queue_buffer_queue)
	 *                           Return value:        0 - Success
	 *                                         -ENOBUFS - Queue is empty.
	 *
	 * @set_param_hfi_queue: API call with hfi_queue_param_buffer_pool
	 *                       adds hfi_queue_buffer to buffer pool.
	 *                       Params for set_param_hfi_queue for
	 *                       hfi_queue_param_buffer_queue:
	 *                           @hfi_queue_handle: handle returned by
	 *                                              create_hfi_queue
	 *                           @payload: struct hfi_queue_buffer_queue*
	 *                           @payload_sz: sizeof(hfi_queue_buffer_queue)
	 *                           Return value:     0 - Success
	 *                                         errno - error case.
	 */
	hfi_queue_param_buffer_queue,

	/*
	 * hfi_queue_param_reset_buffer_queue: supported by
	 *                                     set_param_hfi_queue.
	 * @get_param_hfi_queue: Get not supported
	 *                       hfi_queue_param_reset_buffer_queue
	 *                       Return value: -ENOTSUP
	 * @set_param_hfi_queue: reclaims all buffers fro shared queue
	 *                       and adds it buffer pool.
	 *                       Params for set_param_hfi_queue for
	 *                       hfi_queue_param_reset_buffer_queue:
	 *                           @hfi_queue_handle: handle returned by
	 *                                              create_hfi_queue
	 *                           @payload: NULL
	 *                           @payload_sz: 0
	 *                           Return value:     0 - Success
	 *                                         errno - error case.
	 */
	hfi_queue_param_reset_buffer_queue,

	hfi_queue_param_device_buffer_queue,
	/*
	 * hfi_queue_param_buffer: supported by get_param_hfi_queue.
	 * @get_param_hfi_queue: get a buffer from pool or qeueue.
	 *                      1. Will try to get buffer from pool
	 *                      2. If pool has no buffers will try to get from
	 *                         queue free buffer.
	 *                      3. if it can't find in pool or queue will return
	 *                         -ENOBUFS
	 *                      Params for get_param_hfi_queue for
	 *                      hfi_queue_param_buffer:
	 *                          @hfi_queue_handle: handle returned by
	 *                                             create_hfi_queue
	 *                          @payload: struct hfi_queue_buffer_queue*
	 *                          @payload_sz: sizeof(hfi_queue_buffer_queue)
	 *                          Return value:        0 - Success
	 *                                        -ENOBUFS - Queue is empty.
	 *
	 * @set_param_hfi_queue: returns -ENOTSUP
	 */
	hfi_queue_param_buffer,

	/*
	 * hfi_queue_kickoff: supported by set_param_hfi_queue
	 * @get_param_hfi_queue: returns -ENOTSUP
	 * @set_param_hfi_queue: submits all pending buffers in the shared
	 *                       queue to remote.
	 *                       Params for set_param_hfi_queue for
	 *                       hfi_queue_kickoff:
	 *                           @hfi_queue_handle: handle returned by
	 *                                              create_hfi_queue
	 *                           @payload: NULL
	 *                           @payload_sz: 0
	 *                           Return value:     0 - Success
	 *                                         errno - error case.
	 */
	hfi_queue_kickoff,
	hfi_queue_param_max
};

/**
 * get_queue_mem_req - Get the memory size for queue depth with alignment.
 * @q_depth: depth of queue
 * @align: alignment requirements
 * @return u32 - memory size required for queue.
 */
u32 get_queue_mem_req(u32 q_depth, u32 align);

/**
 * create_hfi_queue - Create a hfi queue object
 * @pqparams - params for queue that needs to be created.
 * @return void* - returns qhandle on success, NULL in failure.
 */

void *create_hfi_queue(struct hfi_queue_create *pqparams);

/**
 * get_param_hfi_queue - Get param API to get info from HFI queue module
 *
 * @hfi_queue_handle: handle returned by create_hfi_queue
 * @id: hfi_queue_param_enum
 * @payload: depends on id look at the enum hfi_queue_param_enum comments
 * @payload_sz: - depends on id look at the enum hfi_queue_param_enum comments
 * @return int: 0 Success, errno Failure
 */
int get_param_hfi_queue(void *hfi_queue_handle, enum hfi_queue_param_enum id,
			void *payload, u32 payload_sz);

/**
 * set_param_hfi_queue - Get param API to get info from HFI queue module
 *
 * @hfi_queue_handle: handle returned by create_hfi_queue
 * @id: hfi_queue_param_enum
 * @payload: depends on id look at the enum hfi_queue_param_enum comments
 * @payload_sz: - depends on id look at the enum hfi_queue_param_enum comments
 * @return int: 0 Success, errno Failure
 */
int set_param_hfi_queue(void *hfi_queue_handle, enum hfi_queue_param_enum id,
			void *payload, u32 payload_sz);

/**
 * destroy_hfi_queue - destroy HFI queue
 * @hfi_queue_handle - handle returned by create_hfi_queue
 */
void destroy_hfi_queue(void *hfi_queue_handle);

#endif /* __HFI_QUEUE_H__ */
