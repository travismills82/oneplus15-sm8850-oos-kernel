/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 */

#ifndef MSM_CVP_SW_DBG_H
#define MSM_CVP_SW_DBG_H

#include "msm_cvp_internal.h"
#include "msm_cvp_buf.h"

#define DBG_BUF_CNT 256
#define TRACE_SESS_SIZE 16

#define SMMU_XTENSA_NOC_ERROR 1
#define SMMU_CORE_NOC_ERROR 2

#define MAX_ENTRIES 64

struct smem_data {
	u32 size;
	u32 flags;
	u32 device_addr;
	u32 cached;
	u32 refcount;
	u32 pkt_type;
	u32 buf_idx;
};

struct cvp_buf_data {
	u32 device_addr;
	u32 size;
};

struct inst_snapshot {
	u32 session;
	u32 smem_index;
	u32 dsp_index;
	u32 persist_index;
	struct smem_data smem_log[MAX_ENTRIES];
	struct cvp_buf_data dsp_buf_log[MAX_ENTRIES];
	struct cvp_buf_data persist_buf_log[MAX_ENTRIES];
};

struct cvp_noc_log {
	u32 used;
	u32 err_ctrl_swid_low;
	u32 err_ctrl_swid_high;
	u32 err_ctrl_mainctl_low;
	u32 err_ctrl_errvld_low;
	u32 err_ctrl_errclr_low;
	u32 err_ctrl_errlog0_low;
	u32 err_ctrl_errlog0_high;
	u32 err_ctrl_errlog1_low;
	u32 err_ctrl_errlog1_high;
	u32 err_ctrl_errlog2_low;
	u32 err_ctrl_errlog2_high;
	u32 err_ctrl_errlog3_low;
	u32 err_ctrl_errlog3_high;
	u32 err_core_swid_low;
	u32 err_core_swid_high;
	u32 err_core_mainctl_low;
	u32 err_core_errvld_low;
	u32 err_core_errclr_low;
	u32 err_core_errlog0_low;
	u32 err_core_errlog0_high;
	u32 err_core_errlog1_low;
	u32 err_core_errlog1_high;
	u32 err_core_errlog2_low;
	u32 err_core_errlog2_high;
	u32 err_core_errlog3_low;
	u32 err_core_errlog3_high;
};

struct cvp_debug_log {
	u32 snapshot_index;
	struct cvp_noc_log noc_log;
	struct inst_snapshot snapshot[16];
};

struct eva_kmd_session {
	u32 session_id;
	u32 instance_state;
	u32 session_type;
	u32 hfi_error_code;
	u32 prev_hfi_error_code;
	u32 session_error_code;
};

struct eva_smmu_debug {
	u32 fauting_addr;
	u32 smmu_fault_cnt;
	u32 error_type;
	u32 noc_error_type;
};

struct eva_kmd_debug_log {
	struct cvp_debug_log log;
	struct eva_smmu_debug smmu_debug;
};

struct eva_kmd_buf {
	u32 session_id;
	u32 map_type; //0: map , 1: unmap
	u32 iova;
	u32 size;
};

struct eva_kmd_trace {
	struct eva_kmd_buf kmd_buf[DBG_BUF_CNT];
	struct eva_kmd_session kmd_session[TRACE_SESS_SIZE];
	struct eva_kmd_debug_log kmd_debug_log;
};

struct eva_kmd_debug {
	u32 kmd_buf_offset;
	struct mutex dbg_lock;
	u32 kmd_buf_cnt;
	u32 kmd_sess_cnt;
	u32 kmd_queue_dump_cnt;
};

void eva_kmd_buf_dump(struct msm_cvp_inst *inst,
		struct msm_cvp_smem *smem, int buf_map_type);
void eva_kmd_session_dump(struct msm_cvp_inst *inst);
void eva_kmd_debug_log_dump(void);
void eva_cmd_msg_queue_dump(void);

#endif
