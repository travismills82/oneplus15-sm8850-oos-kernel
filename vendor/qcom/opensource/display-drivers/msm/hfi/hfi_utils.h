/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 */

#ifndef _HFI_UTILS_H_
#define _HFI_UTILS_H_

#include "hfi_adapter.h"

#define HFI_UTIL_MAX_ALLOC   4096
#define HFI_PROP_ID(val)    (val & 0xfffff)
#define HFI_PROP_SZ(val)    (val >> 24)
#define HFI_VAL_L32(val)    (val & 0xffffffff)
#define HFI_VAL_H32(val)    (val >> 32)

/* Pack feature version into HFI property ID */
#define HFI_PACK_VERSION(major, minor, prop_id) \
	((prop_id & ~(0xF0F000)) | (((minor) & 0xF) << 20) | (((major) & 0xF) << 12))

/**
 * struct hfi_util_kv_helper - helper struct for a kv pair
 * @max_size:	Maximum size of a kv pair struct
 * @cur:	Current offset
 * @count:	Number of a kv pairs
 * @kv_pairs:	Array of kv pair struct
 */
struct hfi_util_kv_helper {
	u32 max_size;
	u32 cur;
	u32 count;
	struct hfi_kv_pairs kv_pairs[];
};

/**
 * struct hfi_util_kv_parser - parser for a kv pair
 * @max_index:	Max index of the of a kv pair parser
 * @cur_offset:	Current offset
 * @payload:	Payload of a kv pair
 */
struct hfi_util_kv_parser {
	u32 max_index;
	u32 cur_offset;
	u32 *payload;
};

/**
 * struct hfi_util_u32_prop_helper - helper for u32 prop
 * @max_size:	max size of the of a 32-bit HFI property
 * @cur:	current offset
 * @count:	count of a kv pair
 * @kv_pairs:	structure for a kv pair
 */
struct hfi_util_u32_prop_helper {
	u32 max_size;
	u32 prop_count;
	u32 *cur;
	u32 prop_data[];
};

enum hfi_util_prop_type {
	HFI_VAL_U32,
	HFI_VAL_U64,
	HFI_VAL_U32_ARRAY,
};

/**
 * hfi_util_kv_helper_alloc - helper function to allocate size for kv pair
 * @count:	Number of kv pair
 */
struct hfi_util_kv_helper *hfi_util_kv_helper_alloc(u32 count);

/**
 * hfi_util_kv_helper_reset - reset helper function for kv pair
 * @kv_helper:	Pointer to struct hfi_util_kv_helper
 */
int hfi_util_kv_helper_reset(struct hfi_util_kv_helper *kv_helper);

/**
 * hfi_util_kv_helper_add - function to add kv pair HFI property
 * @kv_helper:	Pointer to struct hfi_util_kv_helper
 * @key:	u32 packed key
 * @value:	u32 value of kv pair
 */
int hfi_util_kv_helper_add(struct hfi_util_kv_helper *kv_helper, u32 key, u32 *value);

/**
 * hfi_util_kv_helper_get_count - get count of kv pair properties
 * @kv_helper:	Pointer to struct hfi_util_kv_helper
 */
u32 hfi_util_kv_helper_get_count(struct hfi_util_kv_helper *kv_helper);

/**
 * hfi_util_kv_helper_get_payload_addr - get payload address of kv pair
 * @kv_helper:	Pointer to struct hfi_util_kv_helper
 */
void *hfi_util_kv_helper_get_payload_addr(struct hfi_util_kv_helper *kv_helper);

/**
 * hfi_util_kv_helper_dump - helper function to dump the key and value of kv pair
 * @kv_helper:	Pointer to struct hfi_util_kv_helper
 */
void hfi_util_kv_helper_dump(struct hfi_util_kv_helper *kv_helper);

/**
 * hfi_util_kv_parser_init - initialize the kv pair parser
 * @kv_helper:	Pointer to struct hfi_util_kv_helper
 */
int hfi_util_kv_parser_init(struct hfi_util_kv_parser *kv_parser, u32 bytes, u32 *payload);

/**
 * hfi_util_kv_parser_get_next - Retrieves the next kv pair from the parser
 * @kv_helper:	Pointer to struct hfi_util_kv_helper
 * @move:	Offset to move the current position
 * @hfi_prop:	Pointer to HFI property
 * @payload:	Pointer to store the address of the payload data
 * @max_words:	Pointer to store the maximum number of words in the payload.
 */
int hfi_util_kv_parser_get_next(struct hfi_util_kv_parser *kv_parser, u32 move,
		u32 *hfi_prop, u32 **payload, u32 *max_words);

/**
 * hfi_util_u32_prop_helper_alloc - helper function to allocate size for kv pair
 * @count:	Number of u32 properties
 */
struct hfi_util_u32_prop_helper *hfi_util_u32_prop_helper_alloc(u32 count);

/**
 * hfi_util_u32_prop_helper_add_prop - Helper function to add u32 HFI property
 * @prop_helper:Pointer to the hfi_util_u32_prop_helper struct
 * @prop_id:	Identifier for the property
 * @type:	Type of the property
 * @prop_value:	Pointer to the property value.
 * @sz:		Size of the property value.
 */
int hfi_util_u32_prop_helper_add_prop(struct hfi_util_u32_prop_helper *prop_helper,
		u32 prop_id, enum hfi_util_prop_type type, void *prop_value, u32 sz);

/**
 * hfi_util_u32_prop_helper_add_prop_by_obj - Helper function to add u32 HFI property
	by object id.
 * @prop_helper:Pointer to the hfi_util_u32_prop_helper struct
 * @prop_id:	Identifier for the property
 * @type:	Type of the property
 * @prop_value:	Pointer to the property value.
 * @sz:		Size of the property value.
 */
int hfi_util_u32_prop_helper_add_prop_by_obj(struct hfi_util_u32_prop_helper *prop_helper,
		u32 prop_id, u32 obj_id, enum hfi_util_prop_type type, void *prop_value, u32 sz);

/**
 * hfi_util_u32_prop_helper_reset - reset function for u32 HFI property
 * @prop_helper:	Pointer to struct hfi_util_u32_prop_helper
 */
int hfi_util_u32_prop_helper_reset(struct hfi_util_u32_prop_helper *prop_helper);

/**
 * hfi_util_u32_prop_helper_prop_count - number of u32 HFI properties
 * @prop_helper:	Pointer to struct hfi_util_u32_prop_helper
 */
int hfi_util_u32_prop_helper_prop_count(struct hfi_util_u32_prop_helper *prop_helper);

/**
 * hfi_util_u32_prop_helper_get_size - Retrieve the size of u32 HFI properties
 * @prop_helper:	Pointer to struct hfi_util_u32_prop_helper
 */
int hfi_util_u32_prop_helper_get_size(struct hfi_util_u32_prop_helper *prop_helper);

/**
 * hfi_util_u32_prop_helper_prop_count - Get paylaod address
 * @prop_helper:	Pointer to struct hfi_util_u32_prop_helper
 */
void *hfi_util_u32_prop_helper_get_payload_addr(struct hfi_util_u32_prop_helper *prop_helper);


#endif  // _HFI_UTILS_H_
