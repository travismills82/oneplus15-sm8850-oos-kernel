// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) 2015-2019, The Linux Foundation. All rights reserved.
 * Copyright (C) 2017-2020, Pixelworks, Inc.
 *
 * These files contain modifications made by Pixelworks, Inc., in 2019-2020.
 */

#include "pw_iris_lightup.h"
#include "pw_iris_log.h"

void iris_mult_addr_pad_i7p(uint8_t **p, uint32_t *poff, uint32_t left_len)
{

	switch (left_len) {
	case 4:
		iris_set_ocp_type(*p, 0xFFFFFFFF);
		*p += 4;
		*poff += 4;
		break;
	case 8:
		iris_set_ocp_type(*p, 0xFFFFFFFF);
		iris_set_ocp_base_addr(*p, 0xFFFFFFFF);
		*p += 8;
		*poff += 8;
		break;
	case 12:
		iris_set_ocp_type(*p, 0xFFFFFFFF);
		iris_set_ocp_base_addr(*p, 0xFFFFFFFF);
		iris_set_ocp_first_val(*p, 0xFFFFFFFF);
		*p += 12;
		*poff += 12;
		break;
	case 0:
		break;
	default:
		IRIS_LOGE("%s()%d, left len not aligh to 4.", __func__, __LINE__);
		break;
	}
}