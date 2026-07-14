// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 */

#include <linux/file.h>
#include <linux/fs.h>
#include <linux/module.h>
#include <linux/sync_file.h>
#include <linux/uaccess.h>
#include <linux/vmalloc.h>

#include "synx_debugfs.h"

#define DMA_MAX_OBJS           4096

// Definition for interface of kernel and userspace for test ioctls

#define SYNX_TEST_PRIVATE_MAGIC_NUM 't'

#define SYNX_TEST_PRIVATE_IOCTL_CMD \
	_IOWR(SYNX_TEST_PRIVATE_MAGIC_NUM, 254, struct synx_test_private_ioctl_arg)

#define SYNX_DMA_CREATE                      1
#define SYNX_DMA_ARRAY_CREATE                2
#define SYNX_DMA_SIGNAL                      3

/**
 * struct synx_test_private_ioctl_arg - Sync driver ioctl argument
 *
 * @id        : IOCTL command id
 * @size      : Size of command payload
 * @result    : Result of command execution
 * @reserved  : Reserved
 * @ioctl_ptr : Pointer to user data
 */
struct synx_test_private_ioctl_arg {
	__u32 id;
	__u32 size;
	__u32 result;
	__u32 reserved;
	__u64 ioctl_ptr;
};

/**
 * struct synx_dma_info - Dma fence fd information
 *
 * @dma_fd    : dma fence fd
 * @reserved  : Reserved
 */
struct synx_dma_info {
	__s32 dma_fd;
	__u32 reserved;
};

/**
 * struct synx_common_arr_info - Generic structure to store list of information
 *
 * @list        : List of generic info
 * @num_objs    : Number of objects in the list
 * @reserved    : Reserved
 */
struct synx_common_arr_info {
	__u64 list;
	__u32 num_objs;
	__u32 reserved;
};


/**
 * struct synx_dma_create_info - Create information for dma fd
 *
 * @type     : dma create params type
 * @indv     : Params to create an individual dma fd
 * @arr      : Params to create an array of dma fd
 */
struct synx_dma_create_info {
	__u32 type;
	union {
		struct synx_dma_info		indv;
		struct synx_common_arr_info	arr;
	};
};

/**
 * struct synx_dma_array_info - Dma_array fence fd information
 *
 * @dma_fd        : Pointer to dma fds to merge
 * @flags         : Reserved
 * @num_objs      : Number of objects in the list
 * @dma_array_fd  : dma array fd
 * @reserved      : Reserved
 */
struct synx_dma_array_info {
	__u64 dma_fds;
	__u32 flags;
	__u32 num_objs;
	__s32 dma_array_fd;
	__u32 reserved;
};

/**
 * struct synx_dma_signal_indv_info - Dma fence fd signal indv information
 *
 * @dma_fd    : dma fence fd
 * @status    : status
 * @reserved  : Reserved
 */
struct synx_dma_signal_indv_info {
	__s32 dma_fd;
	__s32 status;
	__u32 reserved;
};

/**
 * struct synx_dma_create_info - Create information for dma fd
 *
 * @type     : dma create params type
 * @indv     : Params to create an individual dma fd
 * @arr      : Params to create an array of dma fd
 */
struct synx_dma_signal_info {
	__u32 type;
	union {
		struct synx_dma_signal_indv_info	indv;
		struct synx_common_arr_info		arr;
	};
};


// Definitions for kernelspace test ioctls structures

/**
 * struct synx_test_session - Dma Session information
 *
 * @dma_context : dma context information
 */
struct synx_test_session {
	u64 dma_context;
};

/**
 * enum synx_dma_create_type - Type of create params
 *
 * SYNX_DMA_CREATE_INDV_PARAMS : Dma create filled with dma_create_indv_params struct
 * SYNX_DMA_CREATE_ARR_PARAMS  : Dma create filled with dma_create_arr_params struct
 */
enum synx_dma_create_type {
	SYNX_DMA_CREATE_INDV_PARAMS = 0x01,
	SYNX_DMA_CREATE_ARR_PARAMS  = 0x02,
};

/**
 * struct synx_dma_create_indv_params - dma indvidual create indv handle parameter
 *
 * @dma_fence : Dma fence pointer
 */
struct synx_dma_create_indv_params {
	struct dma_fence *dma_fence;
};

/**
 * struct synx_dma_create_arr_params - dma array of handles parameter
 * @list          : List of dma fences
 * @num_fences    : No of fences passed to framework
 */
struct synx_dma_create_arr_params {
	struct synx_dma_create_indv_params	*list;
	uint32_t				num_fences;
};

/**
 * struct dma_create_params - dma create parameter
 *
 * @type : dma params type filled by client
 * @indv : Params for an individual handle/fence
 * @arr  : Params for an array of handles/fences
 */
struct synx_dma_create_params {
	enum synx_dma_create_type type;
	union {
		struct synx_dma_create_indv_params	indv;
		struct synx_dma_create_arr_params	arr;
	};
};

/**
 * enum synx_dma_array_create_flags - Defines the behaviour of dma fence array
 *
 * SYNX_DMA_ARRAY_SIGNAL_ALL : Unblock dma array fence when all children are signaled
 * SYNX_DMA_ARRAY_SIGNAL_ANY : Unblock dma array fence when any one of child is signaled
 */
enum synx_dma_array_create_flags {
	SYNX_DMA_ARRAY_SIGNAL_ALL = 0x00,
	SYNX_DMA_ARRAY_SIGNAL_ANY = 0x01,
};

/**
 * struct dma_array_create_params - dma_array parameter
 *
 * @dma_fences      : List of dma fences
 * @num_fences      : No of fences passed to framework
 * @flags           : Flag to decide type of dma array fence creation
 * @dma_array_fence : New dma_array fence fd (returned from the framework)
 */
struct synx_dma_array_create_params {
	struct dma_fence                 **dma_fences;
	uint32_t                         num_fences;
	enum synx_dma_array_create_flags flags;
	struct dma_fence                 *dma_array_fence;
};


/**
 * enum synx_dma_signal_type - Signal status
 *
 * SYNX_DMA_SIGNAL_INDV_PARAMS : Dma signal filled with dma_signal_indv_params struct
 * SYNX_DMA_SIGNAL_ARR_PARAMS  : Dma signal filled with dma_signal_arr_params struct
 */
enum synx_dma_signal_type {
	SYNX_DMA_SIGNAL_INDV_PARAMS = 0x01,
	SYNX_DMA_SIGNAL_ARR_PARAMS  = 0x02,
};

/**
 * struct synx_dma_signal_indv_params - dma indvidual signal indv handle parameter
 *
 * @dma_fence : Dma fence pointer
 */
struct synx_dma_signal_indv_params {
	struct dma_fence *dma_fence;
	int32_t status;
};

/**
 * struct synx_dma_signal_arr_params - dma array of handles parameter
 * @list          : List of dma fences
 * @num_fences    : No of fences passed to framework
 */
struct synx_dma_signal_arr_params {
	struct synx_dma_signal_indv_params	*list;
	uint32_t				num_fences;
};

/**
 * struct dma_signal_params - dma signal parameter
 *
 * @type : dma params type filled by client
 * @indv : Params for an individual handle/fence
 * @arr  : Params for an array of handles/fences
 */
struct synx_dma_signal_params {
	enum synx_dma_signal_type type;
	union {
		struct synx_dma_signal_indv_params	indv;
		struct synx_dma_signal_arr_params	arr;
	};
};

static atomic64_t test_dma_seq_counter = ATOMIC64_INIT(1);

bool synx_test_fence_enable_signaling(struct dma_fence *fence)
{
	return true;
}

const char *synx_test_fence_driver_name(struct dma_fence *fence)
{
	return "Global Synx Test Ioctl node";
}

void synx_test_fence_release(struct dma_fence *fence)
{
	/* release the memory allocated during create */
	kfree(fence->lock);
	kfree(fence);
	dprintk(SYNX_MEM, "Released backing fence %pK\n", fence);
}
EXPORT_SYMBOL_GPL(synx_test_fence_release);

static struct dma_fence_ops synx_test_fence_ops = {
	.wait = dma_fence_default_wait,
	.enable_signaling = synx_test_fence_enable_signaling,
	.get_driver_name = synx_test_fence_driver_name,
	.get_timeline_name = synx_test_fence_driver_name,
	.release = synx_test_fence_release,
};

static int synx_test_create_sync_fd(struct dma_fence *fence)
{
	int fd;
	struct sync_file *sync_file;

	if (IS_ERR_OR_NULL(fence)) {
		dprintk(SYNX_ERR, "invalid fence\n");
		return -SYNX_INVALID;
	}

	fd = get_unused_fd_flags(O_CLOEXEC);
	if (fd < 0)
		return fd;

	sync_file = sync_file_create(fence);
	if (IS_ERR_OR_NULL(sync_file)) {
		dprintk(SYNX_ERR, "error creating sync file\n");
		goto err;
	}

	fd_install(fd, sync_file->file);
	return fd;

err:
	put_unused_fd(fd);
	return -SYNX_INVALID;
}


// Functions for test ioctl framework
static int synx_test_open(struct inode *inode, struct file *pfile)
{
	struct synx_test_session *session;

	session = kzalloc(sizeof(*session), GFP_KERNEL);
	if (IS_ERR_OR_NULL(session)) {
		dprintk(SYNX_ERR, "synx test session allocation failed\n");
		return -SYNX_NOMEM;
	}
	session->dma_context = dma_fence_context_alloc(1);

	pfile->private_data = session;

	return SYNX_SUCCESS;
}

static int synx_test_release(struct inode *inode, struct file *pfile)
{
	struct synx_test_session *session = pfile->private_data;

	if (IS_ERR_OR_NULL(session))
		return -SYNX_INVALID;

	kfree(session);

	return SYNX_SUCCESS;
}

/* function would be called from atomic context */
void synx_dma_fence_callback(struct dma_fence *fence,
	struct dma_fence_cb *cb)
{
	s32 status;

	dprintk(SYNX_DBG,
		"callback called for fence %pK for cb %pK\n",
		fence, cb);

	status = dma_fence_get_status_locked(fence);

	dprintk(SYNX_DBG,
		"status %d of dma fence %pK\n",
		status, fence);

	kfree(cb);
}
EXPORT_SYMBOL_GPL(synx_dma_fence_callback);

static struct dma_fence *synx_dma_fence_util_init(u64 context)
{
	spinlock_t *fence_lock;
	struct dma_fence *fence = NULL;
	struct dma_fence_cb *fence_cb;
	int rc = 0;
	u64 seq = 0;

	fence_lock = kzalloc(sizeof(*fence_lock), GFP_KERNEL);
	if (IS_ERR_OR_NULL(fence_lock)) {
		dprintk(SYNX_ERR, "Unable to allocate memory\n");
		return ERR_PTR(-SYNX_NOMEM);
	}

	fence = kzalloc(sizeof(*fence), GFP_KERNEL);
	if (IS_ERR_OR_NULL(fence)) {
		kfree(fence_lock);
		dprintk(SYNX_ERR, "Unable to allocate memory\n");
		return ERR_PTR(-SYNX_NOMEM);
	}
	spin_lock_init(fence_lock);

	seq = atomic64_inc_return(&test_dma_seq_counter);
	dma_fence_init(fence, &synx_test_fence_ops, fence_lock, context, seq);

	/* move synx to ACTIVE state and register cb */
	dma_fence_enable_sw_signaling(fence);

	fence_cb = kzalloc(sizeof(*fence_cb), GFP_KERNEL);
	if (IS_ERR_OR_NULL(fence_cb)) {
		dprintk(SYNX_ERR, "Unable to allocate memory\n");
		dma_fence_put(fence);
		return ERR_PTR(-SYNX_NOMEM);
	}

	// add callback
	rc = dma_fence_add_callback(fence,
			fence_cb, synx_dma_fence_callback);
	if (rc != 0) {
		if (rc == -ENOENT) {
			dprintk(SYNX_DBG, "dma fence %pK is already signaled\n", fence);
		} else {
			dprintk(SYNX_ERR, "Error adding callback for dma fence %pK err%d\n",
				fence, rc);
		}

		dma_fence_put(fence);
		kfree(fence_cb);
		fence = NULL;
		return ERR_PTR(rc);
	}

	dprintk(SYNX_DBG, "Allocated new dma fence %pK fence_cb %pK\n", fence, fence_cb);
	return fence;
}

static int synx_dma_fence_create(struct synx_dma_create_params *params, u64 context)
{
	int i = 0;
	// Create indv dma-fence
	if (params->type == SYNX_DMA_CREATE_INDV_PARAMS) {
		params->indv.dma_fence = synx_dma_fence_util_init(context);
		if (IS_ERR_OR_NULL(params->indv.dma_fence)) {
			dprintk(SYNX_ERR, "Failed to create dma fence err %ld\n",
				PTR_ERR(params->indv.dma_fence));
			return -SYNX_ENODATA;
		}
	}
	// Create dma_fence array
	else if (params->type == SYNX_DMA_CREATE_ARR_PARAMS) {
		for (i = 0; i < params->arr.num_fences; i++) {
			params->arr.list[i].dma_fence = synx_dma_fence_util_init(context);
			if (IS_ERR_OR_NULL(params->arr.list[i].dma_fence)) {
				dprintk(SYNX_ERR, "Failed to create dma fence err %ld\n",
					PTR_ERR(params->arr.list[i].dma_fence));
				goto bail;
			}
		}
	} else {
		dprintk(SYNX_ERR, "Invalid create type %d\n", params->type);
		return -SYNX_INVALID;
	}

	return SYNX_SUCCESS;

bail:
	// Release the dma fence which was already created
	while (i--) {
		// Signal dma fence with error before releasing
		if (!dma_fence_is_signaled(params->arr.list[i].dma_fence)) {
			dma_fence_set_error(params->arr.list[i].dma_fence, -SYNX_ENODATA);
			dma_fence_signal(params->arr.list[i].dma_fence);
		}
		dma_fence_put(params->arr.list[i].dma_fence);
	}
	return -SYNX_ENODATA;
}
static int synx_dma_array_fence_create(struct synx_dma_array_create_params *params, u64 context)
{
	struct dma_fence *fence = NULL;
	int rc = SYNX_SUCCESS;
	int i = 0;
	struct dma_fence_array *new_array = NULL;
	struct dma_fence **fences = NULL;
	struct dma_fence_cb *fence_cb;
	u64 seq;

	// Create dma_fence array
	fences = kcalloc(params->num_fences, sizeof(*fences), GFP_KERNEL);
	if (IS_ERR_OR_NULL(fences))
		return -SYNX_INVALID;

	// Take reference on child dma-fences
	for (i = 0; i < params->num_fences; i++) {
		/* obtain dma fence reference */
		if (dma_fence_is_array((struct dma_fence *)params->dma_fences[i])) {
			dprintk(SYNX_ERR, "No support for nested dma fence array\n");
			goto bail;
		} else {
			dma_fence_get((struct dma_fence *)params->dma_fences[i]);
		}
		fences[i] = (struct dma_fence *)params->dma_fences[i];
	}

	seq = atomic64_inc_return(&test_dma_seq_counter);

	// Create a dma array
	if (params->flags == SYNX_DMA_ARRAY_SIGNAL_ANY)
		new_array = dma_fence_array_create(params->num_fences, fences,
			context, seq, true);
	else
		new_array = dma_fence_array_create(params->num_fences, fences,
			context, seq, false);

	if (IS_ERR_OR_NULL(new_array)) {
		dprintk(SYNX_ERR, "Failed to create dma_fence_array\n");
		rc = -SYNX_ENODATA;
		goto bail;
	}

	// get base dma_fence
	params->dma_array_fence = (void *)&new_array->base;
	fence = &new_array->base;

	/* move synx to ACTIVE state and register cb */
	dma_fence_enable_sw_signaling(fence);

	fence_cb = kzalloc(sizeof(*fence_cb), GFP_KERNEL);

	if (IS_ERR_OR_NULL(fence_cb)) {
		dprintk(SYNX_ERR, "Unable to allocate memory\n");
		dma_fence_put(fence);
		fence = NULL;
		return -SYNX_NOMEM;
	}

	// add callback
	rc = dma_fence_add_callback(fence,
			fence_cb, synx_dma_fence_callback);

	if (rc != 0) {
		if (rc == -ENOENT) {
			dprintk(SYNX_DBG, "dma fence %pK is already signaled\n", fence);
		} else {
			dprintk(SYNX_ERR, "Error adding callback for dma fence %pK err%d\n",
				fence, rc);
		}

		dma_fence_put(fence);
		kfree(fence_cb);
		fence = NULL;
		return rc;
	}

	dprintk(SYNX_DBG, "Allocated new dma fence array %pK fence_cb %pK\n", fence, fence_cb);
	return rc;
bail:
	// Cleanup references
	while (i--)
		dma_fence_put(fences[i]);

	kfree(fences);
	return rc;
}

static int synx_handle_dma_create(
	struct synx_test_private_ioctl_arg *k_ioctl,
	struct synx_test_session *session)
{
	int result = 0;
	struct synx_dma_create_info dma_create_info = {0};
	struct synx_dma_info *dma_list = NULL;
	struct synx_dma_create_params params = {0};

	if (k_ioctl->size != sizeof(dma_create_info))
		return -SYNX_INVALID;

	if (copy_from_user(&dma_create_info,
			u64_to_user_ptr(k_ioctl->ioctl_ptr),
			k_ioctl->size))
		return -EFAULT;

	if (dma_create_info.type == SYNX_DMA_CREATE_INDV_PARAMS) {
		params.type = SYNX_DMA_CREATE_INDV_PARAMS;
		params.indv.dma_fence = NULL;
	} else if (dma_create_info.type == SYNX_DMA_CREATE_ARR_PARAMS) {
		dma_list = kcalloc(dma_create_info.arr.num_objs, sizeof(*dma_list), GFP_KERNEL);
		if (IS_ERR_OR_NULL(dma_list))
			return -SYNX_NOMEM;

		if (copy_from_user(dma_list,
			u64_to_user_ptr(dma_create_info.arr.list),
			sizeof(*dma_list) * dma_create_info.arr.num_objs)) {
			kfree(dma_list);
			return -EFAULT;
		}

		if (dma_create_info.arr.num_objs >= DMA_MAX_OBJS) {
			dprintk(SYNX_ERR, "Exceeding the limit %d to create dma fd\n",
				dma_create_info.arr.num_objs);
			kfree(dma_list);
			return -SYNX_INVALID;
		}

		params.type = SYNX_DMA_CREATE_ARR_PARAMS;
		params.arr.num_fences = dma_create_info.arr.num_objs;
		params.arr.list = kcalloc(params.arr.num_fences,
			sizeof(struct synx_dma_create_indv_params), GFP_KERNEL);

		if (IS_ERR_OR_NULL(params.arr.list)) {
			dprintk(SYNX_ERR, "Failed to allocate memory for dma fence array\n");
			kfree(dma_list);
			return -ENOMEM;
		}
	} else {
		dprintk(SYNX_ERR, "Invalid create type %d\n", dma_create_info.type);
		return -SYNX_INVALID;
	}

	result = synx_dma_fence_create(&params, session->dma_context);
	if (result)
		goto bail;

	// To create file fd to pass to userspace
	if (dma_create_info.type == SYNX_DMA_CREATE_INDV_PARAMS) {
		dma_create_info.indv.dma_fd =
			synx_test_create_sync_fd((struct dma_fence *)params.indv.dma_fence);

		dprintk(SYNX_DBG, "create new file fd %d for dma_fence %pK result %d\n",
			dma_create_info.indv.dma_fd,
			(struct dma_fence *)params.indv.dma_fence, result);

		// Since one reference is taken by fd the init reference can be put
		dma_fence_put((struct dma_fence *)params.indv.dma_fence);
	} else if (dma_create_info.type == SYNX_DMA_CREATE_ARR_PARAMS) {
		for (int i = 0; i < params.arr.num_fences; i++) {
			dma_list[i].dma_fd =
				synx_test_create_sync_fd(
					(struct dma_fence *)params.arr.list[i].dma_fence);

			dprintk(SYNX_DBG, "create new file fd %d for dma_fence %pK\n",
				dma_list[i].dma_fd,
				(struct dma_fence *)params.arr.list[i].dma_fence);

			// Since one reference is taken by fd the init reference can be put
			dma_fence_put((struct dma_fence *)params.arr.list[i].dma_fence);
		}

		if (copy_to_user(u64_to_user_ptr(dma_create_info.arr.list),
			dma_list,
			sizeof(*dma_list) * dma_create_info.arr.num_objs)) {
			result = -EFAULT;
			goto bail;
		}
	}

	if (copy_to_user(u64_to_user_ptr(k_ioctl->ioctl_ptr),
			&dma_create_info,
			k_ioctl->size))
		result = -EFAULT;

bail:
	if (params.type == SYNX_DMA_CREATE_ARR_PARAMS) {
		kfree(dma_list);
		kfree(params.arr.list);
	}

	return result;
}

static int synx_handle_dma_array_create(
	struct synx_test_private_ioctl_arg *k_ioctl,
	struct synx_test_session *session)
{
	int result = 0, i = 0;
	struct dma_fence **child_dma_fence_list = NULL;
	int32_t *child_dma_fds = NULL;
	struct synx_dma_array_info dma_array_info;
	struct synx_dma_array_create_params params = {0};

	if (k_ioctl->size != sizeof(dma_array_info))
		return -SYNX_INVALID;

	if (copy_from_user(&dma_array_info,
			u64_to_user_ptr(k_ioctl->ioctl_ptr),
			k_ioctl->size))
		return -EFAULT;


	child_dma_fds = kcalloc(dma_array_info.num_objs,
		sizeof(int32_t), GFP_KERNEL);
	if (IS_ERR_OR_NULL(child_dma_fds)) {
		dprintk(SYNX_ERR, "Memory allocation failed\n");
		return -SYNX_NOMEM;
	}

	if (copy_from_user(child_dma_fds,
			u64_to_user_ptr(dma_array_info.dma_fds),
			dma_array_info.num_objs * sizeof(int32_t))) {
		kfree(child_dma_fds);
		return -EFAULT;
	}

	if (dma_array_info.num_objs >= DMA_MAX_OBJS) {
		dprintk(SYNX_ERR, "Exceeding the limit %d to combine dma fences\n",
			dma_array_info.num_objs);
		kfree(child_dma_fds);
		return -SYNX_INVALID;
	}

	child_dma_fence_list = kcalloc(dma_array_info.num_objs,
		sizeof(struct dma_fence), GFP_KERNEL);

	if (IS_ERR_OR_NULL(child_dma_fence_list)) {
		dprintk(SYNX_ERR, "Memory allocation failed\n");
		kfree(child_dma_fds);
		return -SYNX_NOMEM;
	}

	for (i = 0; i < dma_array_info.num_objs; i++) {
		child_dma_fence_list[i] =
			sync_file_get_fence(child_dma_fds[i]);

		if (IS_ERR_OR_NULL(child_dma_fence_list[i])) {
			dprintk(SYNX_DBG, "Dma_fd %d is not valid\n",
					child_dma_fds[i]);
			result = -SYNX_INVALID;
			goto bail;
		}
		dprintk(SYNX_DBG, "got dma_fence %pK from fd %d\n",
			child_dma_fence_list[i], child_dma_fds[i]);
	}

	params.num_fences = dma_array_info.num_objs;
	params.dma_fences = child_dma_fence_list;
	params.flags = dma_array_info.flags;
	params.dma_array_fence = NULL;

	result = synx_dma_array_fence_create(&params, session->dma_context);
	if (result)
		goto bail;

	// To create file fd to pass to userspace
	dma_array_info.dma_array_fd =
		synx_test_create_sync_fd((struct dma_fence *)params.dma_array_fence);

	dprintk(SYNX_DBG, "create new file fd %d for dma_array_fence %pK\n",
		dma_array_info.dma_array_fd,
		(struct dma_fence *)params.dma_array_fence);

	// Since one reference is taken by fd the init reference can be put
	dma_fence_put((struct dma_fence *)params.dma_array_fence);

	if (copy_to_user(u64_to_user_ptr(k_ioctl->ioctl_ptr),
			&dma_array_info,
			k_ioctl->size))
		result = -EFAULT;

bail:
	// Releasing reference of child_dma_fences taken by sync_file_get_fence
	while (i--)
		dma_fence_put((struct dma_fence *)child_dma_fence_list[i]);

	kfree(child_dma_fence_list);
	kfree(child_dma_fds);
	return result;
}

static int synx_handle_dma_signal(
	struct synx_test_private_ioctl_arg *k_ioctl,
	struct synx_test_session *session)
{
	int result = SYNX_SUCCESS;
	struct synx_dma_signal_info dma_signal_info;
	struct synx_dma_signal_params params = {0};
	unsigned long flags;

	if (k_ioctl->size != sizeof(dma_signal_info))
		return -SYNX_INVALID;

	if (copy_from_user(&dma_signal_info,
			u64_to_user_ptr(k_ioctl->ioctl_ptr),
			k_ioctl->size))
		return -EFAULT;

	if (dma_signal_info.type == SYNX_DMA_SIGNAL_INDV_PARAMS) {
		params.type = dma_signal_info.type;
		params.indv.dma_fence = sync_file_get_fence(dma_signal_info.indv.dma_fd);
		params.indv.status = dma_signal_info.indv.status;

		if (IS_ERR_OR_NULL(params.indv.dma_fence)) {
			dprintk(SYNX_ERR, "Invalid params\n");
			return -SYNX_INVALID;
		}
		if (dma_fence_is_array((struct dma_fence *)params.indv.dma_fence)) {
			dprintk(SYNX_ERR, "Cannot signal dma fence array %pK",
				(struct dma_fence *)params.indv.dma_fence);

			// Put the reference taken by file_get_fence
			dma_fence_put((struct dma_fence *)params.indv.dma_fence);
			return -SYNX_INVALID;
		}

		dprintk(SYNX_DBG, "Signaling dma_fence %pK dma_fd %d\n",
			(struct dma_fence *)params.indv.dma_fence,
			dma_signal_info.indv.dma_fd);

		spin_lock_irqsave(((struct dma_fence *)params.indv.dma_fence)->lock, flags);
		if (!dma_fence_is_signaled((struct dma_fence *)params.indv.dma_fence)) {
			/*
			 * Treating SYNX_STATE_SIGNALED_SUCESS and 0
			 * as success for dma fence signaling
			 */
			if ((params.indv.status != 2 && params.indv.status != 0))
				dma_fence_set_error((struct dma_fence *)params.indv.dma_fence,
						-params.indv.status);

			dma_fence_signal_locked((struct dma_fence *)params.indv.dma_fence);
		}
		spin_unlock_irqrestore(((struct dma_fence *)params.indv.dma_fence)->lock, flags);

		// Put the reference taken by file_get_fence
		dma_fence_put((struct dma_fence *)params.indv.dma_fence);
	} else {
		dprintk(SYNX_ERR, "invalid/not supported signal type %d\n",
			dma_signal_info.type);
		return -SYNX_INVALID;
	}

	return result;
}


static long synx_test_ioctl(struct file *pfile, unsigned int cmd, unsigned long arg)
{
	s32 rc = 0;
	struct synx_test_private_ioctl_arg k_ioctl;
	struct synx_test_session *session = pfile->private_data;

	if (cmd != SYNX_TEST_PRIVATE_IOCTL_CMD) {
		dprintk(SYNX_ERR, "invalid ioctl cmd\n");
		return -ENOIOCTLCMD;
	}

	if (copy_from_user(&k_ioctl,
			(struct synx_test_private_ioctl_arg *)arg,
			sizeof(k_ioctl))) {
		dprintk(SYNX_ERR, "invalid ioctl args\n");
		return -EFAULT;
	}

	if (!k_ioctl.ioctl_ptr)
		return -SYNX_INVALID;

	dprintk(SYNX_VERB, "Enter test cmd %u from pid %d\n",
		k_ioctl.id, current->pid);

	switch (k_ioctl.id) {
	case SYNX_DMA_CREATE:
		rc = synx_handle_dma_create(&k_ioctl, session);
		break;
	case SYNX_DMA_ARRAY_CREATE:
		rc = synx_handle_dma_array_create(&k_ioctl, session);
		break;
	case SYNX_DMA_SIGNAL:
		rc = synx_handle_dma_signal(&k_ioctl, session);
		break;
	default:
		rc = -SYNX_INVALID;
	}

	dprintk(SYNX_VERB, "exit with status %d\n", rc);

	return rc;
}

const struct file_operations synx_test_fops = {
	.open           = synx_test_open,
	.release        = synx_test_release,
	.unlocked_ioctl = synx_test_ioctl,
};
