/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) 2025 Qualcomm Innovation Center, Inc. All rights reserved.
 */

#undef TRACE_SYSTEM
#define TRACE_SYSTEM smci

#if !defined(_TRACE_SMCI_H) || defined(TRACE_HEADER_MULTI_READ)
#define _TRACE_SMCI_H
#include <linux/tracepoint.h>
#include <linux/firmware/qcom/si_object.h>

#include "smci_impl.h"

#define MAX_STR_LEN 48

TRACE_EVENT(process_invoke_req_handle,
	TP_PROTO(unsigned int cmd, uint32_t op, uint32_t counts),
	TP_ARGS(cmd, op, counts),
	TP_STRUCT__entry(
		__field(unsigned int, cmd)
		__field(uint32_t, op)
		__field(uint32_t, counts)
	),
	TP_fast_assign(
		__entry->cmd = cmd;
		__entry->op = op;
		__entry->counts = counts;
	),
	TP_printk("cmd=0x%04x op=0x%08x counts=%u",
		__entry->cmd, __entry->op, __entry->counts)
);

TRACE_EVENT(process_invoke_req_wait,
	TP_PROTO(const char *ob_name, enum si_object_type ob_type,
		uint32_t op, uint32_t counts),
	TP_ARGS(ob_name, ob_type, op, counts),
	TP_STRUCT__entry(
		__array(char,  str, MAX_STR_LEN)
		__field(enum si_object_type, ob_type)
		__field(uint32_t, op)
		__field(uint32_t, counts)
	),
	TP_fast_assign(
		strscpy(__entry->str, ob_name, MAX_STR_LEN);
		__entry->ob_type = ob_type;
		__entry->op = op;
		__entry->counts = counts;
	),
	TP_printk("ob_name=%s ob_type=0x%02x op=0x%08x counts=%u",
		__entry->str, __entry->ob_type, __entry->op, __entry->counts)
);

TRACE_EVENT(process_invoke_req_ret,
	TP_PROTO(const char *ob_name, unsigned int oic_context_id, uint32_t op,
		uint32_t counts, int32_t result, int ret),
	TP_ARGS(ob_name, oic_context_id, op, counts, result, ret),
	TP_STRUCT__entry(
		__array(char,  str, MAX_STR_LEN)
		__field(unsigned int, oic_context_id)
		__field(uint32_t, op)
		__field(uint32_t, counts)
		__field(int32_t, result)
		__field(int, ret)
	),
	TP_fast_assign(
		strscpy(__entry->str, ob_name, MAX_STR_LEN);
		__entry->oic_context_id = oic_context_id;
		__entry->op = op;
		__entry->counts = counts;
		__entry->ret = ret;
		__entry->result = result;
	),
	TP_printk("ob_name=%s oic_context_id=0x%08x op=0x%08x counts=%u result=%d ret=%d",
		__entry->str, __entry->oic_context_id, __entry->op, __entry->counts,
		__entry->result, __entry->ret)
);

TRACE_EVENT(process_accept_req_handle,
	TP_PROTO(uint64_t ac_txn_id, uint32_t ac_has_resp),
	TP_ARGS(ac_txn_id, ac_has_resp),
	TP_STRUCT__entry(
		__field(uint64_t, ac_txn_id)
		__field(int32_t, ac_has_resp)
	),
	TP_fast_assign(
		__entry->ac_txn_id = ac_txn_id;
		__entry->ac_has_resp = ac_has_resp;
	),
	TP_printk("ac_txn_id=0x%016llx ac_has_resp=0x%04x",
		__entry->ac_txn_id, __entry->ac_has_resp)
);

TRACE_EVENT(process_accept_req_wait,
	TP_PROTO(const char *si_comm, uint64_t ac_txn_id, uint32_t ac_has_resp,
		int32_t ac_result),
	TP_ARGS(si_comm, ac_txn_id, ac_has_resp, ac_result),
	TP_STRUCT__entry(
		__array(char,  str, MAX_STR_LEN)
		__field(uint64_t, ac_txn_id)
		__field(int32_t, ac_has_resp)
		__field(int32_t, ac_result)
	),
	TP_fast_assign(
		strscpy(__entry->str, si_comm, MAX_STR_LEN);
		__entry->ac_txn_id = ac_txn_id;
		__entry->ac_has_resp = ac_has_resp;
		__entry->ac_result = ac_result;
	),
	TP_printk("si_comm=%s ac_txn_id=0x%016llx ac_has_resp=0x%04x ac_result=%d",
		__entry->str, __entry->ac_txn_id, __entry->ac_has_resp, __entry->ac_result)
);

TRACE_EVENT(process_accept_req_ret,
	TP_PROTO(const char *si_comm, int64_t ac_cbobj_id, uint64_t ac_txn_id,
		uint32_t ac_op, uint32_t counts, int32_t ac_result),
	TP_ARGS(si_comm, ac_cbobj_id, ac_txn_id, ac_op, counts, ac_result),
	TP_STRUCT__entry(
		__array(char,  str, MAX_STR_LEN)
		__field(int64_t, ac_cbobj_id)
		__field(uint32_t, ac_txn_id)
		__field(uint32_t, ac_op)
		__field(uint32_t, counts)
		__field(int32_t, ac_result)
	),
	TP_fast_assign(
		strscpy(__entry->str, si_comm, MAX_STR_LEN);
		__entry->ac_cbobj_id = ac_cbobj_id;
		__entry->ac_txn_id = ac_txn_id;
		__entry->ac_op = ac_op;
		__entry->counts = counts;
		__entry->ac_result = ac_result;
	),
	TP_printk(
		"si_comm=%s ac_cbobj_id=0x%016llx ac_txn_id=0x%08x ac_op=0x%08x counts=%u ac_result=%d",
		__entry->str, __entry->ac_cbobj_id, __entry->ac_txn_id,
		__entry->ac_op, __entry->counts, __entry->ac_result)
);

TRACE_EVENT_CONDITION(wait_for_pending_txn,
	TP_PROTO(const char *cb_si_comm, struct cb_txn *txn, int ret),
	TP_ARGS(cb_si_comm, txn, ret),
	TP_CONDITION(txn),
	TP_STRUCT__entry(
		__array(char,  str, MAX_STR_LEN)
		__field(struct cb_txn*, txn)
		__field(int, ret)
	),
	TP_fast_assign(
		strscpy(__entry->str, cb_si_comm, MAX_STR_LEN);
		__entry->txn = txn;
		__entry->ret = ret;
	),
	TP_printk("cb_si_comm=%s cb_context_id=0x%08x cb_u_handle=0x%016llx op=0x%08lx ret=%d",
		__entry->str, __entry->txn->context_id, __entry->txn->u_handle, __entry->txn->op,
		__entry->ret)
);

TRACE_EVENT(mem_object_release,
	TP_PROTO(const char *cb_si_comm, int64_t cb_u_handle),
	TP_ARGS(cb_si_comm, cb_u_handle),
	TP_STRUCT__entry(
		__array(char,  str, MAX_STR_LEN)
		__field(int64_t, cb_u_handle)
	),
	TP_fast_assign(
		strscpy(__entry->str, cb_si_comm, MAX_STR_LEN);
		__entry->cb_u_handle = cb_u_handle;
	),
	TP_printk("cb_si_comm=%s cb_u_handle=0x%016llx",
		__entry->str, __entry->cb_u_handle)
);

TRACE_EVENT(cbo_dispatch_handle,
	TP_PROTO(const char *ob_name, unsigned int cb_context_id,
		int64_t cb_u_handle, unsigned long op),
	TP_ARGS(ob_name, cb_context_id, cb_u_handle, op),
	TP_STRUCT__entry(
		__array(char,  str, MAX_STR_LEN)
		__field(unsigned int, cb_context_id)
		__field(int64_t, cb_u_handle)
		__field(unsigned long, op)
	),
	TP_fast_assign(
		strscpy(__entry->str, ob_name, MAX_STR_LEN);
		__entry->cb_context_id = cb_context_id;
		__entry->cb_u_handle = cb_u_handle;
		__entry->op = op;
	),
	TP_printk("ob_name=%s cb_context_id=0x%08x cb_u_handle=0x%016llx op=0x%08lx",
		__entry->str, __entry->cb_context_id, __entry->cb_u_handle, __entry->op)
);

TRACE_EVENT(cbo_dispatch_wait,
	TP_PROTO(const char *ob_name, unsigned int context_id,
		unsigned int cb_txn_completion_done),
	TP_ARGS(ob_name, context_id, cb_txn_completion_done),
	TP_STRUCT__entry(
		__array(char,  str, MAX_STR_LEN)
		__field(unsigned int, context_id)
		__field(unsigned int, cb_txn_completion_done)
	),
	TP_fast_assign(
		strscpy(__entry->str, ob_name, MAX_STR_LEN);
		__entry->context_id = context_id;
		__entry->cb_txn_completion_done = cb_txn_completion_done;
	),
	TP_printk("ob_name=%s context_id=0x%08x cb_txn_completion_done=%u",
		__entry->str, __entry->context_id, __entry->cb_txn_completion_done)
);

TRACE_EVENT(cbo_dispatch_ret,
	TP_PROTO(const char *ob_name, unsigned int context_id, int errno),
	TP_ARGS(ob_name, context_id, errno),
	TP_STRUCT__entry(
		__array(char,  str, MAX_STR_LEN)
		__field(unsigned int, context_id)
		__field(int, errno)
	),
	TP_fast_assign(
		strscpy(__entry->str, ob_name, MAX_STR_LEN);
		__entry->context_id = context_id;
		__entry->errno = errno;
	),
	TP_printk("ob_name=%s, context_id=0x%08x errno=%d",
		__entry->str, __entry->context_id, __entry->errno)
);

#endif /* _TRACE_SMCI_H */
/*
 * Path must be relative to location of 'define_trace.h' header in kernel
 * Define path if not defined in bazel file
 */
#undef TRACE_INCLUDE_PATH
#define TRACE_INCLUDE_PATH \
	../../../../vendor/qcom/opensource/securemsm-kernel/smcinvoke/si_core_xts
#undef TRACE_INCLUDE_FILE
#define TRACE_INCLUDE_FILE trace_smci

/* This part must be outside protection */
#include <trace/define_trace.h>
