// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 */

#define pr_fmt(fmt)	"[drm:%s:%d] " fmt, __func__, __LINE__

#include <linux/errno.h>

#include "hfi_utils.h"
#include "sde_kms.h"

#define HFI_PACK_PROP_SIZE(x, y) (x | ((y/(sizeof(uint32_t))) << 24))

struct hfi_util_kv_helper *hfi_util_kv_helper_alloc(u32 count)
{
	struct hfi_util_kv_helper *kv_helper;
	u32 size = sizeof(struct hfi_util_kv_helper) + (sizeof(struct hfi_kv_pairs) * count);

	if (size > HFI_UTIL_MAX_ALLOC) {
		SDE_INFO("exceeding limit - restricting to max size\n");
		size = HFI_UTIL_MAX_ALLOC;
	}

	kv_helper = kvzalloc(size, GFP_KERNEL);
	if (!kv_helper) {
		SDE_ERROR("error in alloc\n");
		return ERR_PTR(-ENOMEM);
	}

	kv_helper->cur = 0;
	kv_helper->max_size = size - sizeof(struct hfi_util_kv_helper);

	return kv_helper;
}

int hfi_util_kv_helper_reset(struct hfi_util_kv_helper *kv_helper)
{
	if (!kv_helper) {
		SDE_ERROR("invalid kv helper\n");
		return -EINVAL;
	}

	kv_helper->cur = 0;
	memset(kv_helper->kv_pairs, 0, kv_helper->max_size);

	return 0;
}

int hfi_util_kv_helper_add(struct hfi_util_kv_helper *kv_helper, u32 key, u32 *value)
{
	if (!kv_helper) {
		SDE_ERROR("invalid kv helper\n");
		return -EINVAL;
	}

	if (((kv_helper->cur + 1) * (sizeof(struct hfi_kv_pairs))) > kv_helper->max_size) {
		SDE_ERROR("overflow of kv helper memory\n");
		return -EINVAL;
	}

	kv_helper->kv_pairs[kv_helper->cur].key = key;
	kv_helper->kv_pairs[kv_helper->cur].value_ptr = value;
	kv_helper->cur++;

	return 0;
}

void *hfi_util_kv_helper_get_payload_addr(struct hfi_util_kv_helper *kv_helper)
{
	if (!kv_helper || !kv_helper->cur) {
		SDE_ERROR("invalid or empty kv helper\n");
		return NULL;
	}

	return &kv_helper->kv_pairs[0];
}

u32 hfi_util_kv_helper_get_count(struct hfi_util_kv_helper *kv_helper)
{
	if (!kv_helper) {
		SDE_ERROR("invalid kv helper\n");
		return 0;
	}

	return kv_helper->cur;
}


void hfi_util_kv_helper_dump(struct hfi_util_kv_helper *kv_helper)
{
	if (!kv_helper) {
		SDE_ERROR("invalid kv helper\n");
		return;
	}

	for (int i = 0; i < kv_helper->cur; i++)
		SDE_ERROR("info  - key[%d] =%d val=%p\n", i, kv_helper->kv_pairs[i].key,
				kv_helper->kv_pairs[i].value_ptr);

}

int hfi_util_kv_parser_init(struct hfi_util_kv_parser *kv_parser, u32 bytes, u32 *payload)
{
	if (!kv_parser || !payload || bytes > HFI_UTIL_MAX_ALLOC) {
		SDE_ERROR("invalid kv helper args max_size:%d payload:%pK kv_parser:%pK\n",
				bytes, payload, kv_parser);
		return -EINVAL;
	}

	if (bytes % (sizeof(u32))) {
		SDE_ERROR("unsipported align of size:%d, expecting u32/dword aligned", bytes);
		return -EINVAL;
	}

	kv_parser->max_index = bytes / (sizeof(u32));
	kv_parser->cur_offset = 0;
	kv_parser->payload = payload;

	return 0;
}

int hfi_util_kv_parser_get_next(struct hfi_util_kv_parser *kv_parser, u32 move,
		u32 *hfi_prop, u32 **payload, u32 *max_words)
{
	if (!kv_parser || !payload) {
		SDE_ERROR("invalid kv helper args\n");
		return -EINVAL;
	}

	kv_parser->cur_offset += move;
	*hfi_prop = 0;
	*payload = NULL;
	*max_words = 0;

	if (kv_parser->cur_offset ==  kv_parser->max_index) {
		/* enf of the memory parsing, bail out */
		return 0;
	}

	if (kv_parser->cur_offset >  kv_parser->max_index) {
		SDE_ERROR("invalid access attempt of kv_parser max_u32s:%d cur_index:%d\n",
				kv_parser->max_index, kv_parser->cur_offset);
		return -EINVAL;
	} else if ((kv_parser->max_index - kv_parser->cur_offset) < 2)  {
		SDE_ERROR("min two dwords required for next key value, max_u32s:%d cur_index:%d\n",
				kv_parser->max_index, kv_parser->cur_offset);
		return -EINVAL;
	}

	*hfi_prop = kv_parser->payload[kv_parser->cur_offset++];
	*payload = &(kv_parser->payload[kv_parser->cur_offset]);
	*max_words = kv_parser->max_index - kv_parser->cur_offset;

	return 0;
}

struct hfi_util_u32_prop_helper *hfi_util_u32_prop_helper_alloc(u32 size)
{
	struct hfi_util_u32_prop_helper *prop_helper;
	u32 max_sz = HFI_UTIL_MAX_ALLOC;
	u32 sz = (size + sizeof(struct hfi_util_u32_prop_helper));

	sz = min(sz, max_sz);

	prop_helper = kvzalloc(sz, GFP_KERNEL);
	if (!prop_helper) {
		SDE_ERROR("error in alloc\n");
		return ERR_PTR(-ENOMEM);
	}

	prop_helper->max_size = sz - sizeof(struct hfi_util_u32_prop_helper);
	prop_helper->cur = &prop_helper->prop_data[1];
	prop_helper->prop_count = 0;

	return prop_helper;
}

int hfi_util_u32_prop_helper_add_prop(struct hfi_util_u32_prop_helper *prop_helper,
		u32 prop_id, enum hfi_util_prop_type type, void *prop_value, u32 payload_sz)
{
	int rem;
	u32 payload_size;

	if (!prop_helper) {
		SDE_ERROR("invalid prop helper\n");
		return -EINVAL;
	}

	if (!prop_value) {
		SDE_ERROR("invalid prop value\n");
		return -EINVAL;
	}

	rem = prop_helper->max_size - hfi_util_u32_prop_helper_get_size(prop_helper);
	payload_size = payload_sz;
	if (payload_size > rem) {
		SDE_ERROR("prop_helper memory is full\n");
		return -EINVAL;
	}

	*prop_helper->cur = HFI_PACK_PROP_SIZE(prop_id, payload_size);
	prop_helper->cur++;

	prop_helper->prop_count++;
	prop_helper->prop_data[0] = prop_helper->prop_count;

	switch (type) {
	case HFI_VAL_U32:
		*(prop_helper->cur) = *((u32 *)prop_value);
		 prop_helper->cur++;
		break;
	case HFI_VAL_U64:
		*((u64 *)(prop_helper->cur)) = *((u64 *)prop_value);
		 prop_helper->cur +=  sizeof(u64);
		break;
	case HFI_VAL_U32_ARRAY:
		memcpy(prop_helper->cur, prop_value, payload_sz);
		prop_helper->cur += (payload_sz / (sizeof(u32)));
		break;
	default:
		SDE_ERROR("invalid data type %d for prop_id:%d\n", type, prop_id);
	}

	return 0;
}

int hfi_util_u32_prop_helper_add_prop_by_obj(struct hfi_util_u32_prop_helper *prop_helper,
		u32 prop_id, u32 obj_id, enum hfi_util_prop_type type,
		void *prop_value, u32 payload_sz)
{
	int rem;
	u32 payload_size;

	if (!prop_helper) {
		SDE_ERROR("invalid prop helper\n");
		return -EINVAL;
	}

	if (!prop_value) {
		SDE_ERROR("invalid prop value\n");
		return -EINVAL;
	}

	rem = prop_helper->max_size - hfi_util_u32_prop_helper_get_size(prop_helper);
	payload_size = sizeof(u32) + payload_sz; /* one u32 for object id */

	if (payload_size > rem) {
		SDE_ERROR("prop_helper memory is full\n");
		return -EINVAL;
	}

	*prop_helper->cur = HFI_PACK_PROP_SIZE(prop_id, payload_size);
	prop_helper->cur++;

	*prop_helper->cur = obj_id;
	prop_helper->cur++;

	prop_helper->prop_count++;
	prop_helper->prop_data[0] = prop_helper->prop_count;

	switch (type) {
	case HFI_VAL_U32:
		*(prop_helper->cur) = *((u32 *)prop_value);
		 prop_helper->cur++;
		break;
	case HFI_VAL_U64:
		*((u64 *)(prop_helper->cur)) = *((u64 *)prop_value);
		 prop_helper->cur +=  sizeof(u64);
		break;
	case HFI_VAL_U32_ARRAY:
		memcpy(prop_helper->cur, prop_value, payload_sz);
		prop_helper->cur += (payload_sz / (sizeof(u32)));
		break;
	default:
		SDE_ERROR("invalid data type %d for prop_id:%d\n", type, prop_id);
	}

	return 0;
}

int hfi_util_u32_prop_helper_reset(struct hfi_util_u32_prop_helper *prop_helper)
{
	if (!prop_helper) {
		SDE_ERROR("invalid prop helper\n");
		return -EINVAL;
	}

	prop_helper->prop_count = 0;
	prop_helper->cur = &prop_helper->prop_data[1];
	memset(prop_helper->prop_data, 0, prop_helper->max_size);

	return 0;
}

int hfi_util_u32_prop_helper_get_size(struct hfi_util_u32_prop_helper *prop_helper)
{
	if (!prop_helper || !prop_helper->cur) {
		SDE_ERROR("invalid prop helper\n");
		return 0;
	} else {
		return sizeof(u32) * (prop_helper->cur - prop_helper->prop_data);
	}
}

int hfi_util_u32_prop_helper_prop_count(struct hfi_util_u32_prop_helper *prop_helper)
{
	if (!prop_helper || !prop_helper->cur) {
		SDE_ERROR("invalid prop helper\n");
		return 0;
	} else {
		return prop_helper->prop_count;
	}
}

void *hfi_util_u32_prop_helper_get_payload_addr(struct hfi_util_u32_prop_helper *prop_helper)
{
	if (!prop_helper || !prop_helper->max_size) {
		SDE_ERROR("invalid prop helper\n");
		return NULL;
	} else {
		return prop_helper->prop_data;
	}
}
