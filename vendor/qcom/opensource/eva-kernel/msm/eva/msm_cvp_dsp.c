// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) 2018-2021, The Linux Foundation. All rights reserved.
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 */
#include <linux/module.h>
#include <linux/rpmsg.h>
#include <linux/of_platform.h>
#include <linux/of_fdt.h>
#include <linux/version.h>
#include <linux/fdtable.h>
#if (KERNEL_VERSION(6, 3, 0) <= LINUX_VERSION_CODE)
#include <linux/firmware/qcom/qcom_scm.h>
#else
#include <linux/qcom_scm.h>
#endif
#include <soc/qcom/secure_buffer.h>
#include "msm_cvp_core.h"
#include "msm_cvp.h"
#include "cvp_hfi.h"
#include "cvp_dump.h"

static atomic_t nr_maps;
struct cvp_dsp_apps gfa_cv;

static int cvp_reinit_dsp(void);

static void cvp_remove_dsp_sessions(void);

static int __fastrpc_driver_register(struct fastrpc_driver *driver)
{
#ifdef CVP_FASTRPC_ENABLED
	return fastrpc_driver_register(driver);
#else
	return -ENODEV;
#endif
}

static void __fastrpc_driver_unregister(struct fastrpc_driver *driver)
{
#ifdef CVP_FASTRPC_ENABLED
	return fastrpc_driver_unregister(driver);
#endif
}

#ifdef CVP_FASTRPC_ENABLED
static int __fastrpc_driver_invoke(struct fastrpc_device *dev,
				enum fastrpc_driver_invoke_nums invoke_num,
				unsigned long invoke_param)
{
	return fastrpc_driver_invoke(dev, invoke_num, invoke_param);
}
#endif	/* End of CVP_FASTRPC_ENABLED */

static int cvp_dsp_send_cmd(struct cvp_dsp_cmd_msg *cmd, uint32_t len)
{
	int rc = 0;
	struct cvp_dsp_apps *me = &gfa_cv;

	dprintk(CVP_DSP, "%s: cmd = %d\n", __func__, cmd->type);

	if (IS_ERR_OR_NULL(me->chan)) {
		dprintk(CVP_ERR, "%s: DSP GLink is not ready\n", __func__);
		rc = -EINVAL;
		goto exit;
	}
	rc = rpmsg_send(me->chan->ept, cmd, len);
	if (rc) {
		dprintk(CVP_ERR, "%s: DSP rpmsg_send failed rc=%d\n",
			__func__, rc);
		dprintk(CVP_ERR, "%s: CDSP SSR received\n",
			__func__);
		rc = -EINVAL;
		goto exit;
	}

exit:
	return rc;
}

static int cvp_dsp_send_cmd_sync(struct cvp_dsp_cmd_msg *cmd,
		uint32_t len, struct cvp_dsp_rsp_msg *rsp)
{
	int rc = 0;
	struct cvp_dsp_apps *me = &gfa_cv;

	dprintk(CVP_DSP, "%s: cmd = %d\n", __func__, cmd->type);

	me->pending_dsp2cpu_rsp.type = cmd->type;
	rc = cvp_dsp_send_cmd(cmd, len);
	if (rc) {
		dprintk(CVP_ERR, "%s: cvp_dsp_send_cmd failed rc=%d\n",
			__func__, rc);
		goto exit;
	}

	if (!wait_for_completion_timeout(&me->completions[cmd->type],
			msecs_to_jiffies(CVP_DSP_RESPONSE_TIMEOUT))) {
		dprintk(CVP_ERR, "%s cmd %d timeout\n", __func__, cmd->type);
		rc = -ETIMEDOUT;
		goto exit;
	}

exit:
	rsp->ret = me->pending_dsp2cpu_rsp.ret;
	rsp->dsp_state = me->pending_dsp2cpu_rsp.dsp_state;
	me->pending_dsp2cpu_rsp.type = CVP_INVALID_RPMSG_TYPE;
	return rc;
}

static int cvp_dsp_send_cmd_hfi_queue(phys_addr_t *phys_addr,
				uint32_t size_in_bytes,
				struct cvp_dsp_rsp_msg *rsp)
{
	int rc = 0;
	struct cvp_dsp_cmd_msg cmd;

	cmd.type = CPU2DSP_SEND_HFI_QUEUE;
	cmd.msg_ptr = (uint64_t)phys_addr;
	cmd.msg_ptr_len = size_in_bytes;
	cmd.hfi_version = 1;
	cmd.ddr_type = cvp_of_fdt_get_ddrtype();
	if (cmd.ddr_type < 0) {
		dprintk(CVP_WARN,
			"%s: Incorrect DDR type value %d, use default %d\n",
			__func__, cmd.ddr_type, DDR_TYPE_LPDDR5);
		/*return -EINVAL;*/
		cmd.ddr_type =  DDR_TYPE_LPDDR5;
	}

	dprintk(CVP_DSP,
		"%s: address of buffer, PA=0x%pK  size_buff=%d ddr_type=%d\n",
		__func__, phys_addr, size_in_bytes, cmd.ddr_type);

	rc = cvp_dsp_send_cmd_sync(&cmd, sizeof(struct cvp_dsp_cmd_msg), rsp);
	if (rc) {
		dprintk(CVP_ERR,
			"%s: cvp_dsp_send_cmd failed rc = %d\n",
			__func__, rc);
		goto exit;
	}
exit:
	return rc;
}

static int cvp_hyp_assign_to_dsp(uint64_t addr, uint32_t size)
{
	int rc = 0;
	struct cvp_dsp_apps *me = &gfa_cv;

	uint64_t hlosVMid = BIT(VMID_HLOS);
	struct qcom_scm_vmperm dspVM[DSP_VM_NUM] = {
		{VMID_HLOS, PERM_READ | PERM_WRITE | PERM_EXEC},
		{VMID_CDSP_Q6, PERM_READ | PERM_WRITE | PERM_EXEC}
	};

	if (!me->hyp_assigned) {
		rc = qcom_scm_assign_mem(addr, size, &hlosVMid, dspVM, DSP_VM_NUM);
		if (rc) {
			dprintk(CVP_ERR, "%s failed. rc=%d\n", __func__, rc);
			return rc;
		}
		me->addr = addr;
		me->size = size;
		me->hyp_assigned = true;
	}

	return rc;
}

static int cvp_hyp_assign_from_dsp(void)
{
	int rc = 0;
	struct cvp_dsp_apps *me = &gfa_cv;

	uint64_t dspVMids = BIT(VMID_HLOS) | BIT(VMID_CDSP_Q6);
	struct qcom_scm_vmperm hlosVM[HLOS_VM_NUM] = {
		{VMID_HLOS, PERM_READ | PERM_WRITE | PERM_EXEC},
	};

	if (me->hyp_assigned) {
		rc = qcom_scm_assign_mem(me->addr, me->size, &dspVMids, hlosVM, HLOS_VM_NUM);
		if (rc) {
			dprintk(CVP_ERR, "%s failed. rc=%d\n", __func__, rc);
			return rc;
		}
		me->addr = 0;
		me->size = 0;
		me->hyp_assigned = false;
	}

	return rc;
}

static int cvp_dsp_rpmsg_probe(struct rpmsg_device *rpdev)
{
	struct cvp_dsp_apps *me = &gfa_cv;
	const char *edge_name = NULL;
	int ret = 0;

	ret = of_property_read_string(rpdev->dev.parent->of_node,
			"label", &edge_name);
	if (ret) {
		dprintk(CVP_ERR, "glink edge 'label' not found in node\n");
		return ret;
	}

	if (strcmp(edge_name, "cdsp")) {
		dprintk(CVP_ERR,
			"%s: Failed to probe rpmsg device.Node name:%s\n",
			__func__, edge_name);
		return -EINVAL;
	}

	mutex_lock(&me->tx_lock);
	me->chan = rpdev;
	me->state = DSP_PROBED;
	mutex_unlock(&me->tx_lock);
	complete(&me->completions[CPU2DSP_MAX_CMD]);
	dprintk(CVP_DSP, "glink probed, DSP driver ready!\n");

	return ret;
}

static int eva_fastrpc_dev_unmap_dma(
		struct fastrpc_device *frpc_device,
		struct cvp_internal_buf *buf);

static int delete_dsp_session(struct msm_cvp_inst *inst,
		struct cvp_dsp_fastrpc_driver_entry *frpc_node)
{
	struct task_struct *task = NULL;
	struct cvp_hfi_ops *ops_tbl;
	int rc;

	if (!inst)
		return -EINVAL;

	task = inst->task;

	ops_tbl = inst->core->dev_ops;
	inst->pm_qos_latency = PM_QOS_RESUME_LATENCY_DEFAULT_VALUE;
	call_hfi_op(ops_tbl, pm_qos_update, ops_tbl->hfi_device_data);

	rc = msm_cvp_close(inst);
	if (rc)
		dprintk(CVP_ERR, "Warning: Failed to close cvp instance\n");
		return rc;

	if (task)
		put_task_struct(task);

	dprintk(CVP_DSP, "%s DSP2CPU_DETELE_SESSION Done\n", __func__);
	return rc;
}

static int eva_fastrpc_driver_get_name(
		struct cvp_dsp_fastrpc_driver_entry *frpc_node)
{
    int i = 0;
    struct cvp_dsp_apps *me = &gfa_cv;
    for (i = 0; i < MAX_FASTRPC_DRIVER_NUM; i++) {
        if (me->cvp_fastrpc_name[i].status == DRIVER_NAME_AVAILABLE) {
            frpc_node->driver_name_idx = i;
            frpc_node->cvp_fastrpc_driver.driver.name =
			me->cvp_fastrpc_name[i].name;
            me->cvp_fastrpc_name[i].status = DRIVER_NAME_USED;
            dprintk(CVP_DSP, "%s -> handle 0x%x get name %s\n",
			__func__, frpc_node->cvp_fastrpc_driver.handle,
                frpc_node->cvp_fastrpc_driver.driver.name);
            return 0;
        }
    }

    return -1;
}

static void eva_fastrpc_driver_release_name(
		struct cvp_dsp_fastrpc_driver_entry *frpc_node)
{
    struct cvp_dsp_apps *me = &gfa_cv;
    me->cvp_fastrpc_name[frpc_node->driver_name_idx].status =
		DRIVER_NAME_AVAILABLE;
}

/* The function may not return for up to 50ms */
static bool dequeue_frpc_node(struct cvp_dsp_fastrpc_driver_entry *node)
{
	struct cvp_dsp_apps *me = &gfa_cv;
	struct cvp_dsp_fastrpc_driver_entry *frpc_node = NULL;
	struct list_head *ptr = NULL, *next = NULL;
	u32 refcount, max_count = 10;
	bool rc = false;

	if (!node)
		return rc;

search_again:
	ptr = &me->fastrpc_driver_list.list;
	mutex_lock(&me->fastrpc_driver_list.lock);
	list_for_each_safe(ptr, next, &me->fastrpc_driver_list.list) {
		frpc_node = list_entry(ptr,
			struct cvp_dsp_fastrpc_driver_entry, list);

		if (frpc_node == node) {
			refcount = atomic_read(&frpc_node->refcount);
			if (refcount > 0) {
				mutex_unlock(&me->fastrpc_driver_list.lock);
				usleep_range(5000, 10000);
				if (max_count-- == 0) {
					dprintk(CVP_ERR, "%s timeout %d\n",
						__func__, refcount);
					WARN_ON(true);
					goto exit;
				}
				goto search_again;
			}
			list_del(&frpc_node->list);
			rc = true;
			break;
		}
	}
	mutex_unlock(&me->fastrpc_driver_list.lock);
exit:
	return rc;
}

/* The function may not return for up to 50ms */
static struct cvp_dsp_fastrpc_driver_entry *pop_frpc_node(void)
{
	struct cvp_dsp_apps *me = &gfa_cv;
	struct cvp_dsp_fastrpc_driver_entry *frpc_node = NULL;
	struct list_head *ptr = NULL, *next = NULL;
	u32 refcount, max_count = 10;

search_again:
	ptr = &me->fastrpc_driver_list.list;
	if (!ptr) {
		frpc_node = NULL;
		goto exit;
	}

	mutex_lock(&me->fastrpc_driver_list.lock);
	list_for_each_safe(ptr, next, &me->fastrpc_driver_list.list) {
		if (!ptr) {
			frpc_node = NULL;
			break;
		}
		frpc_node = list_entry(ptr,
			struct cvp_dsp_fastrpc_driver_entry, list);

		if (frpc_node) {
			refcount = atomic_read(&frpc_node->refcount);
			if (refcount > 0) {
				mutex_unlock(&me->fastrpc_driver_list.lock);
				usleep_range(5000, 10000);
				if (max_count-- == 0) {
					dprintk(CVP_ERR, "%s timeout\n",
							__func__);
					frpc_node = NULL;
					goto exit;
				}
				goto search_again;
			}
			list_del(&frpc_node->list);
			break;
		}
	}

	mutex_unlock(&me->fastrpc_driver_list.lock);
exit:
	return frpc_node;
}

/* The function may not return for up to 50ms */
static struct cvp_dsp_fastrpc_driver_entry *pop_frpc_node_with_handle(uint32_t handle)
{
	struct cvp_dsp_apps *me = &gfa_cv;
	struct cvp_dsp_fastrpc_driver_entry *frpc_node = NULL;
	struct list_head *ptr = NULL, *next = NULL;
	u32 refcount, max_count = 10;

search_again:
	ptr = &me->fastrpc_driver_list.list;
	if (!ptr) {
		frpc_node = NULL;
		goto exit;
	}

	mutex_lock(&me->fastrpc_driver_list.lock);
	list_for_each_safe(ptr, next, &me->fastrpc_driver_list.list) {
		if (!ptr) {
			frpc_node = NULL;
			break;
		}
		frpc_node = list_entry(ptr,
			struct cvp_dsp_fastrpc_driver_entry, list);

		if (frpc_node && frpc_node->handle == handle) {
			refcount = atomic_read(&frpc_node->refcount);
			if (refcount > 0) {
				mutex_unlock(&me->fastrpc_driver_list.lock);
				usleep_range(5000, 10000);
				if (max_count-- == 0) {
					dprintk(CVP_ERR, "%s timeout\n",
							__func__);
					frpc_node = NULL;
					goto exit;
				}
				goto search_again;
			}
			list_del(&frpc_node->list);
			break;
		}
	}

	mutex_unlock(&me->fastrpc_driver_list.lock);
exit:
	return frpc_node;
}

static void cvp_dsp_rpmsg_remove(struct rpmsg_device *rpdev)
{
	struct cvp_dsp_apps *me = &gfa_cv;
	u32 max_num_retries = 100;

	dprintk(CVP_WARN, "%s: CDSP SSR triggered\n", __func__);

	mutex_lock(&me->rx_lock);
	while (max_num_retries > 0) {
		if (me->pending_dsp2cpu_cmd_header.type !=
				CVP_INVALID_RPMSG_TYPE) {
			mutex_unlock(&me->rx_lock);
			usleep_range(1000, 5000);
			mutex_lock(&me->rx_lock);
		} else {
			break;
		}
		max_num_retries--;
	}

	if (!max_num_retries)
		dprintk(CVP_ERR, "stuck processing pending DSP cmds\n");

	mutex_lock(&me->tx_lock);
	cvp_hyp_assign_from_dsp();

	me->chan = NULL;
	me->state = DSP_UNINIT;
	mutex_unlock(&me->tx_lock);
	mutex_unlock(&me->rx_lock);

	/* Wait HW finish current frame processing */
	usleep_range(20000, 50000);
	cvp_remove_dsp_sessions();

	dprintk(CVP_WARN, "%s: CDSP SSR handled nr_maps %d\n", __func__,
			atomic_read(&nr_maps));
}

static int cvp_dsp_rpmsg_callback(struct rpmsg_device *rpdev,
	void *data, int len, void *priv, u32 addr)
{
	struct cvp_dsp_rsp_msg *rsp = (struct cvp_dsp_rsp_msg *)data;
	struct cvp_dsp_apps *me = &gfa_cv;

	dprintk(CVP_DSP, "%s: type = 0x%x ret = 0x%x len = 0x%x\n",
		__func__, rsp->type, rsp->ret, len);

	if (rsp->type < CPU2DSP_MAX_CMD && len == sizeof(*rsp)) {
		if (me->pending_dsp2cpu_rsp.type == rsp->type) {
			memcpy(&me->pending_dsp2cpu_rsp, rsp,
				sizeof(struct cvp_dsp_rsp_msg));
			complete(&me->completions[rsp->type]);
		} else {
			dprintk(CVP_ERR, "%s: CPU2DSP resp %d, pending %d\n",
					__func__, rsp->type,
					me->pending_dsp2cpu_rsp.type);
			goto exit;
		}
	} else if (rsp->type < CVP_DSP_MAX_CMD
			&& (len == sizeof(struct cvp_dsp2cpu_cmd)
			|| len == sizeof(struct cvp_dsp2cpu_cmd_v2))) {
		if (len < sizeof(struct cvp_dsp_cmd_header)) {
			dprintk(CVP_ERR, "%s: CPU2DSP header not large enough\n",
				__func__);
			goto exit;
		}

		if (me->pending_dsp2cpu_cmd_header.type != CVP_INVALID_RPMSG_TYPE) {
			dprintk(CVP_ERR,
				"%s: DSP2CPU cmd:%d pending %d %d expect %d\n",
					__func__, rsp->type,
				me->pending_dsp2cpu_cmd_header.type, len,
				sizeof(struct cvp_dsp2cpu_cmd));
			goto exit;
		}

		memcpy(&me->pending_dsp2cpu_cmd_header, rsp,
			sizeof(struct cvp_dsp_cmd_header));
		if (me->pending_dsp2cpu_cmd_header.ver == 0
			&& len == sizeof(struct cvp_dsp2cpu_cmd)) {
			// This is original version

			memcpy(&me->pending_dsp2cpu_cmd, rsp,
			  sizeof(struct cvp_dsp2cpu_cmd));
		} else if (me->pending_dsp2cpu_cmd_header.ver == 1 &&
			len == sizeof(struct cvp_dsp2cpu_cmd_v2)) {
			// This is V2
			memcpy(&me->pending_dsp2cpu_cmd_v2, rsp,
				sizeof(struct cvp_dsp2cpu_cmd_v2));
		} else {
			dprintk(CVP_ERR, "%s: Invalid version: %d, or cmd len: %d\n",
				__func__, me->pending_dsp2cpu_cmd_header.ver, len);
			return 0;
		}
	    complete(&me->completions[CPU2DSP_MAX_CMD]);
	} else {
		dprintk(CVP_ERR, "%s: Invalid type: %d, cmd len: %d\n", __func__, rsp->type, len);
		dprintk(CVP_ERR, "%s: v1=%zu v2=%zu\n", __func__,
			sizeof(struct cvp_dsp2cpu_cmd), sizeof(struct cvp_dsp2cpu_cmd_v2));
		return 0;
	}
	return 0;
exit:
	dprintk(CVP_ERR, "concurrent dsp cmd type = %d, rsp type = %d\n",
			me->pending_dsp2cpu_cmd_header.type,
			me->pending_dsp2cpu_rsp.type);
	return 0;
}

static bool dsp_session_exist(void)
{
	struct msm_cvp_core *core;
	struct msm_cvp_inst *inst = NULL;

	core = cvp_driver->cvp_core;
	if (core) {
		mutex_lock(&core->lock);
		list_for_each_entry(inst, &core->instances, list) {
			if (inst->session_type == MSM_CVP_DSP) {
				mutex_unlock(&core->lock);
				return true;
			}
		}
		mutex_unlock(&core->lock);
	}

	return false;
}

int cvp_dsp_suspend(bool force)
{
	int rc = 0;
	struct cvp_dsp_cmd_msg cmd;
	struct cvp_dsp_apps *me = &gfa_cv;
	struct cvp_dsp_rsp_msg rsp;
	bool retried = false;


	/* If not forced to suspend, check if DSP requested PC earlier */
	if (force == false)
		if (dsp_session_exist())
			if (me->state != DSP_SUSPEND)
				return -EBUSY;

	cmd.type = CPU2DSP_SUSPEND;

	mutex_lock(&me->tx_lock);
	if (me->state != DSP_READY)
		goto exit;

retry:
	/* Use cvp_dsp_send_cmd_sync after dsp driver is ready */
	rc = cvp_dsp_send_cmd_sync(&cmd,
			sizeof(struct cvp_dsp_cmd_msg),
			&rsp);
	if (rc) {
		dprintk(CVP_ERR,
			"%s: cvp_dsp_send_cmd failed rc = %d\n",
			__func__, rc);
		goto exit;
	}

	if (rsp.ret == CPU2DSP_EUNAVAILABLE)
		goto fatal_exit;

	if (rsp.ret == CPU2DSP_EFATAL) {
		dprintk(CVP_ERR, "%s: suspend dsp got EFATAL error\n",
				__func__);
		if (!retried) {
			mutex_unlock(&me->tx_lock);
			retried = true;
			rc = cvp_reinit_dsp();
			mutex_lock(&me->tx_lock);
			if (rc)
				goto fatal_exit;
			else
				goto retry;
		} else {
			goto fatal_exit;
		}
	}

	me->state = DSP_SUSPEND;
	dprintk(CVP_DSP, "DSP suspended, nr_map: %d\n", atomic_read(&nr_maps));
	goto exit;

fatal_exit:
	me->state = DSP_INVALID;
	cvp_hyp_assign_from_dsp();
	rc = -ENOTSUPP;
exit:
	mutex_unlock(&me->tx_lock);
	return rc;
}

int cvp_dsp_resume(void)
{
	int rc = 0;
	struct cvp_dsp_cmd_msg cmd;
	struct cvp_dsp_apps *me = &gfa_cv;

	cmd.type = CPU2DSP_RESUME;

	/*
	 * Deadlock against DSP2CPU_CREATE_SESSION in dsp_thread
	 * Probably get rid of this entirely as discussed before
	 */
	if (me->state != DSP_SUSPEND)
		dprintk(CVP_WARN, "%s DSP not in SUSPEND state\n", __func__);

	return rc;
}



static int eva_fastrpc_remove_buffers(struct cvp_dsp_fastrpc_driver_entry *frpc_node)
{
	struct msm_cvp_list *buf_list = NULL;
	struct list_head *ptr_dsp_buf = NULL, *next_dsp_buf = NULL;
	struct cvp_internal_buf *buf = NULL;
	int rc = 0;

	if (!frpc_node)
		return -EINVAL;

	buf_list = &frpc_node->cvpdspbufs;

	mutex_lock(&buf_list->lock);
	ptr_dsp_buf = &buf_list->list;
	list_for_each_safe(ptr_dsp_buf, next_dsp_buf, &buf_list->list) {
		if (!ptr_dsp_buf)
			break;
		buf = list_entry(ptr_dsp_buf, struct cvp_internal_buf, list);
		if (buf) {
			dprintk(CVP_DSP, "fd in list 0x%x\n", buf->fd);

			if (!buf->smem) {
				dprintk(CVP_DSP, "Empty smem\n");
				continue;
			}

			dprintk(CVP_DSP, "%s find device addr 0x%x\n", // Jingyu todo
				__func__, buf->smem->device_addr);

			rc = eva_fastrpc_dev_unmap_dma(
					frpc_node->cvp_fastrpc_device,
					buf);
			if (rc)
				dprintk_rl(CVP_WARN,
					"%s Failed to unmap buffer 0x%x\n",
					__func__, rc);

			rc = cvp_release_dsp_buffers(buf);
			if (rc)
				dprintk(CVP_ERR,
					"%s Failed to free buffer 0x%x\n",
					__func__, rc);

			list_del(&buf->list);

			cvp_kmem_cache_free(&cvp_driver->buf_cache, buf);
		}
	}

	return rc;
}

static void cvp_remove_dsp_sessions(void)
{
	struct cvp_dsp_apps *me = &gfa_cv;
	struct cvp_dsp_fastrpc_driver_entry *frpc_node = NULL;
	struct msm_cvp_inst *inst = NULL;
	struct list_head *s = NULL, *next_s = NULL;

	while ((frpc_node = pop_frpc_node())) {
		mutex_lock(&frpc_node->dsp_sessions.lock);
		s = &frpc_node->dsp_sessions.list;
		if (!s || !(s->next)) {
			mutex_unlock(&frpc_node->dsp_sessions.lock);
			return;
		}
		list_for_each_safe(s, next_s,
				&frpc_node->dsp_sessions.list) {
			if (!s || !next_s)
				break;
			inst = list_entry(s, struct msm_cvp_inst,
					dsp_list);
			if (inst) {
				list_del(&inst->dsp_list);
				frpc_node->session_cnt--;
				mutex_unlock(&frpc_node->dsp_sessions.lock);
				delete_dsp_session(inst, frpc_node);
				mutex_lock(&frpc_node->dsp_sessions.lock);
			}
		}
		mutex_unlock(&frpc_node->dsp_sessions.lock);
		eva_fastrpc_remove_buffers(frpc_node);

		dprintk(CVP_DSP, "%s DEINIT_MSM_CVP_LIST 0x%x\n",
				__func__, frpc_node->dsp_sessions);
		DEINIT_MSM_CVP_LIST(&frpc_node->dsp_sessions);
		dprintk(CVP_DSP, "%s DEINIT_MSM_CVP_LIST 0x%x\n",
				__func__, frpc_node->cvpdspbufs);
		DEINIT_MSM_CVP_LIST(&frpc_node->cvpdspbufs);
		dprintk(CVP_DSP, "%s list_del fastrpc node 0x%x\n",
				__func__, frpc_node);
		__fastrpc_driver_unregister(
				&frpc_node->cvp_fastrpc_driver);
		dprintk(CVP_DSP,
				"%s Unregistered fastrpc handle 0x%x\n",
				__func__, frpc_node->handle);
		mutex_lock(&me->driver_name_lock);
		eva_fastrpc_driver_release_name(frpc_node);
		mutex_unlock(&me->driver_name_lock);
		kfree(frpc_node);
		frpc_node = NULL;
	}

	dprintk(CVP_WARN, "%s: EVA SSR handled for CDSP\n", __func__);
}

int cvp_dsp_shutdown(void)
{
	struct cvp_dsp_apps *me = &gfa_cv;
	int rc = 0;
	struct cvp_dsp_cmd_msg cmd;
	struct cvp_dsp_rsp_msg rsp;

	cmd.type = CPU2DSP_SHUTDOWN;

	mutex_lock(&me->tx_lock);
	if (me->state == DSP_INVALID)
		goto exit;

	me->state = DSP_INACTIVE;
	rc = cvp_dsp_send_cmd_sync(&cmd, sizeof(struct cvp_dsp_cmd_msg), &rsp);
	if (rc) {
		dprintk(CVP_ERR,
			"%s: cvp_dsp_send_cmd failed with rc = %d\n",
			__func__, rc);
		cvp_hyp_assign_from_dsp();
		goto exit;
	}

	rc = cvp_hyp_assign_from_dsp();

exit:
	mutex_unlock(&me->tx_lock);
	return rc;
}

static const struct rpmsg_device_id cvp_dsp_rpmsg_match[] = {
	{ CVP_APPS_DSP_GLINK_GUID },
	{ },
};

static struct rpmsg_driver cvp_dsp_rpmsg_client = {
	.id_table = cvp_dsp_rpmsg_match,
	.probe = cvp_dsp_rpmsg_probe,
	.remove = cvp_dsp_rpmsg_remove,
	.callback = cvp_dsp_rpmsg_callback,
	.drv = {
		.name = "qcom,msm_cvp_dsp_rpmsg",
	},
};

static void cvp_dsp_set_queue_hdr_defaults(struct cvp_hfi_queue_header *q_hdr)
{
	q_hdr->qhdr_status = 0x1;
	q_hdr->qhdr_type = CVP_IFACEQ_DFLT_QHDR;
	q_hdr->qhdr_q_size = CVP_IFACEQ_QUEUE_SIZE / 4;
	q_hdr->qhdr_pkt_size = 0;
	q_hdr->qhdr_rx_wm = 0x1;
	q_hdr->qhdr_tx_wm = 0x1;
	q_hdr->qhdr_rx_req = 0x1;
	q_hdr->qhdr_tx_req = 0x0;
	q_hdr->qhdr_rx_irq_status = 0x0;
	q_hdr->qhdr_tx_irq_status = 0x0;
	q_hdr->qhdr_read_idx = 0x0;
	q_hdr->qhdr_write_idx = 0x0;
}

void cvp_dsp_init_hfi_queue_hdr(struct iris_hfi_device *device)
{
	u32 i;
	struct cvp_hfi_queue_table_header *q_tbl_hdr;
	struct cvp_hfi_queue_header *q_hdr;
	struct cvp_iface_q_info *iface_q;

	for (i = 0; i < CVP_IFACEQ_NUMQ; i++) {
		iface_q = &device->dsp_iface_queues[i];
		iface_q->q_hdr = CVP_IFACEQ_GET_QHDR_START_ADDR(
			device->dsp_iface_q_table.align_virtual_addr, i);
		cvp_dsp_set_queue_hdr_defaults(iface_q->q_hdr);
	}
	q_tbl_hdr = (struct cvp_hfi_queue_table_header *)
			device->dsp_iface_q_table.align_virtual_addr;
	q_tbl_hdr->qtbl_version = 0;
	q_tbl_hdr->device_addr = (void *)device;
	strscpy(q_tbl_hdr->name, "msm_cvp", sizeof(q_tbl_hdr->name));
	q_tbl_hdr->qtbl_size = CVP_IFACEQ_TABLE_SIZE;
	q_tbl_hdr->qtbl_qhdr0_offset =
				sizeof(struct cvp_hfi_queue_table_header);
	q_tbl_hdr->qtbl_qhdr_size = sizeof(struct cvp_hfi_queue_header);
	q_tbl_hdr->qtbl_num_q = CVP_IFACEQ_NUMQ;
	q_tbl_hdr->qtbl_num_active_q = CVP_IFACEQ_NUMQ;

	iface_q = &device->dsp_iface_queues[CVP_IFACEQ_CMDQ_IDX];
	q_hdr = iface_q->q_hdr;
	q_hdr->qhdr_start_addr = iface_q->q_array.align_device_addr;
	q_hdr->qhdr_type |= HFI_Q_ID_HOST_TO_CTRL_CMD_Q;

	iface_q = &device->dsp_iface_queues[CVP_IFACEQ_MSGQ_IDX];
	q_hdr = iface_q->q_hdr;
	q_hdr->qhdr_start_addr = iface_q->q_array.align_device_addr;
	q_hdr->qhdr_type |= HFI_Q_ID_CTRL_TO_HOST_MSG_Q;

	iface_q = &device->dsp_iface_queues[CVP_IFACEQ_DBGQ_IDX];
	q_hdr = iface_q->q_hdr;
	q_hdr->qhdr_start_addr = iface_q->q_array.align_device_addr;
	q_hdr->qhdr_type |= HFI_Q_ID_CTRL_TO_HOST_DEBUG_Q;
	/*
	 * Set receive request to zero on debug queue as there is no
	 * need of interrupt from cvp hardware for debug messages
	 */
	q_hdr->qhdr_rx_req = 0;
}

static int __reinit_dsp(void)
{
	int rc;
	uint64_t addr;
	uint32_t size;
	struct cvp_dsp_apps *me = &gfa_cv;
	struct cvp_dsp_rsp_msg rsp;
	struct msm_cvp_core *core;
	struct iris_hfi_device *device;

	core = cvp_driver->cvp_core;
	if (core && core->dev_ops)
		device = core->dev_ops->hfi_device_data;
	else
		return -EINVAL;

	if (!device) {
		dprintk(CVP_ERR, "%s: NULL device\n", __func__);
		return -EINVAL;
	}

	/* Force shutdown DSP */
	rc = cvp_dsp_shutdown();
	if (rc)
		return rc;
	/*
	 * Workaround to force delete DSP session resources
	 * To be removed after DSP optimization ready
	 */
	cvp_remove_dsp_sessions();

	dprintk(CVP_WARN, "Reinit EVA DSP interface: nr_map %d\n",
			atomic_read(&nr_maps));

	/* Resend HFI queue */
	mutex_lock(&me->tx_lock);
	if (!device->dsp_iface_q_table.align_virtual_addr) {
		dprintk(CVP_ERR, "%s: DSP HFI queue released\n", __func__);
		rc = -EINVAL;
		goto exit;
	}

	addr = (uint64_t)device->dsp_iface_q_table.mem_data.dma_handle;
	size = device->dsp_iface_q_table.mem_data.size;

	if (!addr || !size) {
		dprintk(CVP_DSP, "%s: HFI queue is not ready\n", __func__);
		goto exit;
	}

	rc = cvp_hyp_assign_to_dsp(addr, size);
	if (rc) {
		dprintk(CVP_ERR, "%s: cvp_hyp_assign_to_dsp. rc=%d\n",
			__func__, rc);
		goto exit;
	}

	rc = cvp_dsp_send_cmd_hfi_queue((phys_addr_t *)addr, size, &rsp);
	if (rc) {
		dprintk(CVP_WARN, "%s: Send HFI Queue failed rc = %d\n",
			__func__, rc);

		goto exit;
	}
	if (rsp.ret) {
		dprintk(CVP_ERR, "%s: DSP error %d %d\n", __func__,
				rsp.ret, rsp.dsp_state);
		rc = -ENODEV;
	}
exit:
	mutex_unlock(&me->tx_lock);
	return rc;
}

static int cvp_reinit_dsp(void)
{
	int rc;
	struct cvp_dsp_apps *me = &gfa_cv;

	rc = __reinit_dsp();
	if (rc)	{
		mutex_lock(&me->tx_lock);
		me->state = DSP_INVALID;
		cvp_hyp_assign_from_dsp();
		mutex_unlock(&me->tx_lock);
	}
	return rc;
}

static void cvp_put_fastrpc_node(struct cvp_dsp_fastrpc_driver_entry *node)
{
	if (node && (atomic_read(&node->refcount) > 0))
		atomic_dec(&node->refcount);
}

static struct cvp_dsp_fastrpc_driver_entry *cvp_get_fastrpc_node_with_handle(
			uint32_t handle)
{
	struct cvp_dsp_apps *me = &gfa_cv;
	struct list_head *ptr = NULL, *next = NULL;
	struct cvp_dsp_fastrpc_driver_entry *frpc_node = NULL, *tmp_node = NULL;

	mutex_lock(&me->fastrpc_driver_list.lock);
	list_for_each_safe(ptr, next, &me->fastrpc_driver_list.list) {
		if (!ptr)
			break;
		tmp_node = list_entry(ptr,
				struct cvp_dsp_fastrpc_driver_entry, list);
		if (handle == tmp_node->handle) {
			frpc_node = tmp_node;
			atomic_inc(&frpc_node->refcount);
			dprintk(CVP_DSP, "Find tmp_node with handle 0x%x\n",
				handle);
			break;
		}
	}
	mutex_unlock(&me->fastrpc_driver_list.lock);

	dprintk(CVP_DSP, "%s found fastrpc probe handle %pK pid 0x%x\n",
		__func__, frpc_node, handle);
	return frpc_node;
}

static void eva_fastrpc_driver_unregister(uint32_t handle, bool force_exit);

static int cvp_fastrpc_probe(struct fastrpc_device *rpc_dev)
{
	struct cvp_dsp_fastrpc_driver_entry *frpc_node = NULL;

	dprintk(CVP_DSP, "%s fastrpc probe handle 0x%x\n",
		__func__, rpc_dev->handle);

	frpc_node = cvp_get_fastrpc_node_with_handle(rpc_dev->handle);
	if (frpc_node) {
		frpc_node->cvp_fastrpc_device = rpc_dev;
		complete(&frpc_node->fastrpc_probe_completion);
		cvp_put_fastrpc_node(frpc_node);
	}

	return 0;
}

static int cvp_fastrpc_callback(struct fastrpc_device *rpc_dev,
			enum fastrpc_driver_status fastrpc_proc_num)
{
	dprintk(CVP_DSP, "%s handle 0x%x, proc %d\n", __func__,
			rpc_dev->handle, fastrpc_proc_num);

	/* fastrpc drive down when process gone
	 * any handling can happen here, such as
	 * eva_fastrpc_driver_unregister(rpc_dev->handle, true);
	 */
	eva_fastrpc_driver_unregister(rpc_dev->handle, true);

	return 0;
}


static struct fastrpc_driver cvp_fastrpc_client = {
	.probe = cvp_fastrpc_probe,
	.callback = cvp_fastrpc_callback,
};


static int eva_fastrpc_dev_map_dma(struct fastrpc_device *frpc_device,
			struct cvp_internal_buf *buf,
			uint32_t dsp_remote_map,
			uint64_t *v_dsp_addr)
{
#ifdef CVP_FASTRPC_ENABLED
	struct fastrpc_dev_map_dma frpc_map_buf = {0};
	int rc = 0;

	if (dsp_remote_map == 1) {
		frpc_map_buf.buf = buf->smem->dma_buf;
		frpc_map_buf.size = buf->smem->size;
		frpc_map_buf.attrs = 0;

		dprintk(CVP_DSP,
			"%s frpc_map_buf size %d, dma_buf %pK, map %pK, 0x%x\n",
			__func__, frpc_map_buf.size, frpc_map_buf.buf,
			&frpc_map_buf, (unsigned long)&frpc_map_buf);
		rc = __fastrpc_driver_invoke(frpc_device, FASTRPC_DEV_MAP_DMA,
			(unsigned long)(&frpc_map_buf));
		if (rc) {
			dprintk(CVP_ERR,
				"%s Failed to map buffer 0x%x\n", __func__, rc);
			return rc;
		}
		buf->fd = (s32)frpc_map_buf.v_dsp_addr;
		*v_dsp_addr = frpc_map_buf.v_dsp_addr;
		atomic_inc(&nr_maps);
	} else {
		dprintk(CVP_DSP, "%s Buffer not mapped to dsp\n", __func__);
		buf->fd = 0;
	}

	return rc;
#else
	return -ENODEV;
#endif	/* End of CVP_FASTRPC_ENABLED */
}

static int eva_fastrpc_dev_unmap_dma(struct fastrpc_device *frpc_device,
			struct cvp_internal_buf *buf)
{
#ifdef CVP_FASTRPC_ENABLED
	struct fastrpc_dev_unmap_dma frpc_unmap_buf = {0};
	int rc = 0;

	/* Only if buffer is mapped to dsp */
	if (buf->fd != 0) {
		frpc_unmap_buf.buf = buf->smem->dma_buf;
		rc = __fastrpc_driver_invoke(frpc_device, FASTRPC_DEV_UNMAP_DMA,
				(unsigned long)(&frpc_unmap_buf));
		if (rc) {
			dprintk_rl(CVP_ERR, "%s Failed to unmap buffer %d\n",
				__func__, rc);
			return rc;
		}
		if (atomic_read(&nr_maps) > 0)
			atomic_dec(&nr_maps);
	} else {
		dprintk(CVP_DSP, "%s buffer not mapped to dsp\n", __func__);
	}

	return rc;
#else
	return -ENODEV;
#endif	/* End of CVP_FASTRPC_ENABLED */
}

static int eva_fastrpc_dev_get_pid(struct fastrpc_device *frpc_device, int *pid)
{
#ifdef CVP_FASTRPC_ENABLED
	struct fastrpc_dev_get_hlos_pid get_pid = {0};
	int rc = 0;

	rc = __fastrpc_driver_invoke(frpc_device, FASTRPC_DEV_GET_HLOS_PID,
				(unsigned long)(&get_pid));
	if (rc) {
		dprintk(CVP_ERR, "%s Failed to get PID %x\n",
				__func__, rc);
		return rc;
	}
	*pid = get_pid.hlos_pid;

	return rc;
#else
	return -ENODEV;
#endif	/* End of CVP_FASTRPC_ENABLED */
}

static void eva_fastrpc_driver_add_sess(
	struct cvp_dsp_fastrpc_driver_entry *frpc,
	struct msm_cvp_inst *inst)
{
	mutex_lock(&frpc->dsp_sessions.lock);
	if (inst)
		list_add_tail(&inst->dsp_list, &frpc->dsp_sessions.list);
	else
		dprintk(CVP_ERR, "%s incorrect input %pK\n", __func__, inst);
	frpc->session_cnt++;
	mutex_unlock(&frpc->dsp_sessions.lock);
	dprintk(CVP_DSP, "add dsp sess %pK fastrpc_driver %pK\n", inst, frpc);
}

int cvp_dsp_fastrpc_unmap(uint32_t handle, struct cvp_internal_buf *buf)
{
	struct cvp_dsp_fastrpc_driver_entry *frpc_node = NULL;
	struct fastrpc_device *frpc_device = NULL;
	int rc = 0;

	frpc_node = cvp_get_fastrpc_node_with_handle(handle);
	if (!frpc_node) {
		dprintk(CVP_ERR, "%s no frpc node for dsp handle %d\n",
			__func__, handle);
		return -EINVAL;
	}
	frpc_device = frpc_node->cvp_fastrpc_device;
	rc = eva_fastrpc_dev_unmap_dma(frpc_device, buf);
	if (rc)
		dprintk(CVP_ERR, "%s Fail to unmap buffer 0x%x\n",
				__func__, rc);

	cvp_put_fastrpc_node(frpc_node);
	return rc;
}

int cvp_dsp_del_sess(uint32_t handle, struct msm_cvp_inst *inst)
{
	struct cvp_dsp_fastrpc_driver_entry *frpc_node = NULL;
	struct list_head *ptr = NULL, *next = NULL;
	struct msm_cvp_inst *sess;
	bool found = false;

	frpc_node = cvp_get_fastrpc_node_with_handle(handle);
	if (!frpc_node) {
		dprintk(CVP_ERR, "%s no frpc node for dsp handle %d\n",
				__func__, handle);
		return -EINVAL;
	}
	mutex_lock(&frpc_node->dsp_sessions.lock);
	list_for_each_safe(ptr, next, &frpc_node->dsp_sessions.list) {
		if (!ptr)
			break;
		sess = list_entry(ptr, struct msm_cvp_inst, dsp_list);
		if (sess == inst) {
			dprintk(CVP_DSP, "%s Find sess %pK to be deleted\n",
				__func__, inst);
			found = true;
			break;
		}
	}
	if (found) {
		list_del(&inst->dsp_list);
		frpc_node->session_cnt--;
	}
	mutex_unlock(&frpc_node->dsp_sessions.lock);

	cvp_put_fastrpc_node(frpc_node);
	return 0;
}

static int eva_fastrpc_driver_register(uint32_t handle)
{
	struct cvp_dsp_apps *me = &gfa_cv;
	int rc = 0;
	struct cvp_dsp_fastrpc_driver_entry *frpc_node = NULL;
	bool skip_deregister = true;

	dprintk(CVP_DSP, "%s -> cvp_get_fastrpc_node_with_handle hdl 0x%x\n",
			__func__, handle);
	frpc_node = cvp_get_fastrpc_node_with_handle(handle);

	if (frpc_node == NULL) {
		dprintk(CVP_DSP, "%s new fastrpc node hdl 0x%x\n",
				__func__, handle);
		frpc_node = kzalloc(sizeof(*frpc_node), GFP_KERNEL);
		if (!frpc_node) {
			dprintk(CVP_DSP, "%s allocate frpc node fail\n",
				__func__);
			return -EINVAL;
		}

		memset(frpc_node, 0, sizeof(*frpc_node));

		/* Setup fastrpc_node */
		frpc_node->handle = handle;
		frpc_node->cvp_fastrpc_driver = cvp_fastrpc_client;
		frpc_node->cvp_fastrpc_driver.handle = handle;
		mutex_lock(&me->driver_name_lock);
		rc = eva_fastrpc_driver_get_name(frpc_node);
		mutex_unlock(&me->driver_name_lock);
		if (rc) {
			dprintk(CVP_ERR, "%s fastrpc get name fail err %d\n",
				__func__, rc);
			goto fail_fastrpc_driver_get_name;
		}

		/* Init completion */
		init_completion(&frpc_node->fastrpc_probe_completion);

		mutex_lock(&me->fastrpc_driver_list.lock);
		list_add_tail(&frpc_node->list, &me->fastrpc_driver_list.list);
		INIT_MSM_CVP_LIST(&frpc_node->dsp_sessions);
		INIT_MSM_CVP_LIST(&frpc_node->cvpdspbufs);
		dprintk(CVP_DSP, "Add frpc node 0x%x to list\n", frpc_node);
		atomic_inc(&frpc_node->refcount);
		mutex_unlock(&me->fastrpc_driver_list.lock);

		/* register fastrpc device to this session */
		rc = __fastrpc_driver_register(&frpc_node->cvp_fastrpc_driver);
		if (rc) {
			dprintk(CVP_ERR, "%s fastrpc driver reg fail err %d\n",
				__func__, rc);
			skip_deregister = true;
			goto fail_fastrpc_driver_register;
		}

		/* signal wait reuse dsp timeout setup for now */
		if (!wait_for_completion_timeout(
				&frpc_node->fastrpc_probe_completion,
				msecs_to_jiffies(CVP_DSP_RESPONSE_TIMEOUT))) {
			dprintk(CVP_ERR, "%s fastrpc driver_register timeout %#x\n",
				__func__, frpc_node->handle);
			skip_deregister = false;
			goto fail_fastrpc_driver_register;
		}
		cvp_put_fastrpc_node(frpc_node);
	} else {
		dprintk(CVP_DSP, "%s fastrpc probe frpc_node %pK hdl 0x%x\n",
			__func__, frpc_node, handle);
		cvp_put_fastrpc_node(frpc_node);
	}

	return rc;

fail_fastrpc_driver_register:
	cvp_put_fastrpc_node(frpc_node);
	if (!dequeue_frpc_node(frpc_node)) {
		dprintk(CVP_DSP, "%s fastrpc node %pK hdl 0x%x released elsewhere\n",
			__func__, frpc_node, handle);
		return -EINVAL;
	}
	if (!skip_deregister)
		__fastrpc_driver_unregister(&frpc_node->cvp_fastrpc_driver);

	mutex_lock(&me->driver_name_lock);
	eva_fastrpc_driver_release_name(frpc_node);
	mutex_unlock(&me->driver_name_lock);
fail_fastrpc_driver_get_name:
	kfree(frpc_node);
	return -EINVAL;
}

static void print_internal_dsp_buffer(u32 tag, const char *str,
		struct cvp_dsp_fastrpc_driver_entry *frpc_node, struct cvp_internal_buf *cbuf)
{
	if (!(tag & msm_cvp_debug) || !frpc_node || !cbuf)
		return;

	if (cbuf->smem->dma_buf) {
		dprintk(tag,
		"%s: frpc handle: %x : fd %d off %d 0x%llx %s size %d iova %#x\n",
		str, frpc_node->handle, cbuf->fd,
		cbuf->offset, cbuf->smem->dma_buf, cbuf->smem->dma_buf->name,
		cbuf->size, cbuf->smem->device_addr);
	} else {
		dprintk(tag,
		"%s: frpc handle: %x : idx %2d fd %d off %d size %d iova %#x\n",
		str, frpc_node->handle, cbuf->index, cbuf->fd,
		cbuf->offset, cbuf->size, cbuf->smem->device_addr);
	}
}

static void eva_fastrpc_driver_unregister(uint32_t handle, bool force_exit)
{
	struct cvp_dsp_apps *me = &gfa_cv;
	struct cvp_dsp_fastrpc_driver_entry *frpc_node = NULL;
	struct cvp_dsp2cpu_cmd *dsp2cpu_cmd = &me->pending_dsp2cpu_cmd;
	struct cvp_internal_buf *cbuf, *dummy;
	struct msm_cvp_inst *inst = NULL;
	struct list_head *s = NULL, *next_s = NULL;

	dprintk(CVP_DSP, "%s Unregister fastrpc driver hdl %#x hdl %#x, f %d\n",
		__func__, handle, dsp2cpu_cmd->pid, (uint32_t)force_exit);

	if (handle != dsp2cpu_cmd->pid)
		dprintk(CVP_ERR, "Unregister pid != hndl %#x %#x\n",
				handle, dsp2cpu_cmd->pid);

	/* Found fastrpc node */
	frpc_node = pop_frpc_node_with_handle(handle);

	if (frpc_node == NULL) {
		dprintk(CVP_WARN, "%s fastrpc handle 0x%x unregistered/search timed out\n",
			__func__, handle);
		return;
	}

	// Session delete;
	mutex_lock(&frpc_node->dsp_sessions.lock);
	s = &frpc_node->dsp_sessions.list;
	if (s && (s->next)) {
		list_for_each_safe(s, next_s,
				&frpc_node->dsp_sessions.list) {
			if (!s || !next_s)
				break;
			inst = list_entry(s, struct msm_cvp_inst,
					dsp_list);
			if (inst) {
				list_del(&inst->dsp_list);
				frpc_node->session_cnt--;
				mutex_unlock(&frpc_node->dsp_sessions.lock);
				/* Delete DSP session */
				delete_dsp_session(inst, frpc_node);
				mutex_lock(&frpc_node->dsp_sessions.lock);
			}
		}
	}
	mutex_unlock(&frpc_node->dsp_sessions.lock);

	if ((frpc_node->session_cnt == 0) || force_exit) {
		dprintk(CVP_DSP, "%s session cnt %d, force %d\n",
		__func__, frpc_node->session_cnt, (uint32_t)force_exit);

		int rc = 0;

		// Clear remaining buffers.
		cbuf = (struct cvp_internal_buf *)0xdeadbeef;
		mutex_lock(&frpc_node->cvpdspbufs.lock);
		list_for_each_entry_safe(cbuf, dummy, &frpc_node->cvpdspbufs.list, list) {
			print_internal_dsp_buffer(CVP_MEM, "remove dspbufs", frpc_node, cbuf);

			struct msm_cvp_smem *smem = cbuf->smem;

			if (cbuf->ownership == CLIENT) {
				rc = msm_cvp_unmap_smem_frpc(frpc_node, smem, "frpc unregister");
				if (rc) {
					dprintk(CVP_ERR,
						"%s Fail to unmap smem 0x%x, error %d\n",
						__func__, smem, rc);
				} else
					msm_cvp_smem_put_dma_buf(smem->dma_buf);
			} else if (cbuf->ownership == DSP) {
				int rc = cvp_release_dsp_buffers(cbuf);

				if (rc)
					dprintk(CVP_ERR,
						"%s Fail to free buffer 0x%x, error %d\n",
						__func__, cbuf, rc);
			}
			list_del(&cbuf->list);
			cvp_kmem_cache_free(&cvp_driver->buf_cache, cbuf);
		}
		mutex_unlock(&frpc_node->cvpdspbufs.lock);

		DEINIT_MSM_CVP_LIST(&frpc_node->dsp_sessions);
		DEINIT_MSM_CVP_LIST(&frpc_node->cvpdspbufs);

		__fastrpc_driver_unregister(&frpc_node->cvp_fastrpc_driver);
		mutex_lock(&me->driver_name_lock);
		eva_fastrpc_driver_release_name(frpc_node);
		mutex_unlock(&me->driver_name_lock);
		kfree(frpc_node);
	} else {
		dprintk(CVP_WARN, "%s Fastrpc driver hdl %#x hdl %#x, f %d, session count is %d, abort unregistration\n",
						__func__, handle, dsp2cpu_cmd->pid, (uint32_t)force_exit, frpc_node->session_cnt);
	}
}

void cvp_dsp_send_debug_mask(void)
{
	struct cvp_dsp_cmd_msg cmd;
	struct cvp_dsp_apps *me = &gfa_cv;
	struct cvp_dsp_rsp_msg rsp;
	int rc;

	cmd.type = CPU2DSP_SET_DEBUG_LEVEL;
	cmd.eva_dsp_debug_mask = me->debug_mask;

	dprintk(CVP_DSP,
		"%s: debug mask 0x%x\n",
		__func__, cmd.eva_dsp_debug_mask);

	rc = cvp_dsp_send_cmd_sync(&cmd, sizeof(struct cvp_dsp_cmd_msg), &rsp);
	if (rc)
		dprintk(CVP_ERR,
			"%s: cvp_dsp_send_cmd failed rc = %d\n",
			__func__, rc);
}

void cvp_dsp_send_hfi_queue(void)
{
	struct msm_cvp_core *core;
	struct iris_hfi_device *device;
	struct cvp_dsp_apps *me = &gfa_cv;
	struct cvp_dsp_rsp_msg rsp = {0};
	uint64_t addr;
	uint32_t size;
	int rc;

	core = cvp_driver->cvp_core;
	if (core && core->dev_ops)
		device = core->dev_ops->hfi_device_data;
	else
		return;

	if (!device) {
		dprintk(CVP_ERR, "%s: NULL device\n", __func__);
		return;
	}

	dprintk(CVP_DSP, "Entering %s\n", __func__);

	mutex_lock(&device->lock);
	mutex_lock(&me->tx_lock);

	if (!device->dsp_iface_q_table.align_virtual_addr) {
		dprintk(CVP_ERR, "%s: DSP HFI queue released\n", __func__);
		goto exit;
	}

	addr = (uint64_t)device->dsp_iface_q_table.mem_data.dma_handle;
	size = device->dsp_iface_q_table.mem_data.size;

	if (!addr || !size) {
		dprintk(CVP_DSP, "%s: HFI queue is not ready\n", __func__);
		goto exit;
	}

	if (me->state != DSP_PROBED && me->state != DSP_INACTIVE) {
		dprintk(CVP_DSP,
			"%s: DSP is not probed or  not in proper state. me->state = %d\n",
			__func__, me->state);
		goto exit;
	}

	dprintk(CVP_DSP, "DSP probed successfully, me->state = %d\n", me->state);

	rc = cvp_hyp_assign_to_dsp(addr, size);
	if (rc) {
		dprintk(CVP_ERR, "%s: cvp_hyp_assign_to_dsp. rc=%d\n",
			__func__, rc);
		goto exit;
	}

	if (me->state == DSP_PROBED) {
		cvp_dsp_init_hfi_queue_hdr(device);
		dprintk(CVP_WARN,
			"%s: Done init of HFI queue headers\n", __func__);
	}

	rc = cvp_dsp_send_cmd_hfi_queue((phys_addr_t *)addr, size, &rsp);
	if (rc) {
		dprintk(CVP_WARN, "%s: Send HFI Queue failed rc = %d\n",
			__func__, rc);

		goto exit;
	}

	if (rsp.ret == CPU2DSP_EUNSUPPORTED) {
		dprintk(CVP_WARN, "%s unsupported cmd %d\n",
			__func__, rsp.type);
		goto exit;
	}

	if (rsp.ret == CPU2DSP_EFATAL || rsp.ret == CPU2DSP_EUNAVAILABLE) {
		dprintk(CVP_ERR, "%s fatal error returned %d %d\n",
				__func__, rsp.dsp_state, rsp.ret);
		me->state = DSP_INVALID;
		cvp_hyp_assign_from_dsp();
		goto exit;
	} else if (rsp.ret == CPU2DSP_EINVALSTATE) {
		dprintk(CVP_ERR, "%s dsp invalid state %d\n",
				__func__, rsp.dsp_state);
		mutex_unlock(&me->tx_lock);
		if (cvp_reinit_dsp()) {
			dprintk(CVP_ERR, "%s reinit dsp fail\n", __func__);
			mutex_unlock(&device->lock);
			return;
		}
		mutex_lock(&me->tx_lock);
	}

	dprintk(CVP_DSP, "%s: dsp initialized\n", __func__);
	me->state = DSP_READY;

exit:
	mutex_unlock(&me->tx_lock);
	mutex_unlock(&device->lock);
}
/* 32 or 64 bit CPU Side Ptr <-> 2 32 bit DSP Pointers. Dirty Fix. */
static void *get_inst_from_dsp(uint32_t session_cpu_high, uint32_t session_cpu_low)
{
	struct msm_cvp_core *core;
	struct msm_cvp_inst *sess_inst;
	void *inst;

	if ((session_cpu_high == 0) && (sizeof(void *) == BITPTRSIZE32)) {
		inst = (void *)((uintptr_t)session_cpu_low);
	} else if ((session_cpu_high != 0) && (sizeof(void *) == BITPTRSIZE64)) {
		inst = (void *)((uintptr_t)(((uint64_t)session_cpu_high) << 32
							| session_cpu_low));
	} else {
		dprintk(CVP_ERR,
			"%s Invalid _cpu_high = 0x%x _cpu_low = 0x%x\n",
				__func__, session_cpu_high, session_cpu_low);
		inst = NULL;
		return inst;
	}

	core = cvp_driver->cvp_core;
	if (core) {
		mutex_lock(&core->lock);
		list_for_each_entry(sess_inst, &core->instances, list) {
			if (sess_inst->session_type == MSM_CVP_DSP) {
				if (sess_inst == (struct msm_cvp_inst *)inst) {
					mutex_unlock(&core->lock);
					return inst;
				}
			}
		}
		mutex_unlock(&core->lock);
		inst = NULL;
	} else {
		return NULL;
	}

	return inst;
}

static void print_power(const struct eva_power_req *pwr_req)
{
	if (pwr_req) {
		dprintk(CVP_DSP, "Clock: Fdu %d Mpu %d Od %d Ica %d Vadl %d",
				pwr_req->clock_fdu, pwr_req->clock_mpu,
				pwr_req->clock_od, pwr_req->clock_ica,
				pwr_req->clock_vadl);
		dprintk(CVP_DSP, "Tof %d, Rge %d, Xra %d, Lsr %d, Fw %d\n",
				pwr_req->clock_tof, pwr_req->clock_rge,
				pwr_req->clock_xra, pwr_req->clock_lsr,
				pwr_req->clock_fw);
		dprintk(CVP_DSP, "OpClock: Fdu %d Mpu %d Od %d Ica %d Vadl %d",
				pwr_req->op_clock_fdu, pwr_req->op_clock_mpu,
				pwr_req->op_clock_od, pwr_req->op_clock_ica,
				pwr_req->op_clock_vadl);
		dprintk(CVP_DSP, "Tof %d, Rge %d, Xra %d, Lsr %d, Fw %d\n",
				pwr_req->op_clock_tof, pwr_req->op_clock_rge,
				pwr_req->op_clock_xra, pwr_req->op_clock_lsr,
				pwr_req->op_clock_fw);
		dprintk(CVP_DSP, "Actual Bw: Ddr %d, SysCache %d",
				pwr_req->bw_ddr, pwr_req->bw_sys_cache);
		dprintk(CVP_DSP, "OpBw: Ddr %d, SysCache %d\n",
				pwr_req->op_bw_ddr, pwr_req->op_bw_sys_cache);
	}
}

void __dsp_cvp_sess_create(struct cvp_dsp_cmd_msg *cmd)
{
	struct cvp_dsp_apps *me = &gfa_cv;
	struct msm_cvp_inst *inst = NULL, *inst_temp = NULL;
	uint64_t inst_handle = 0;
	uint32_t pid;
	int rc = 0;
	struct cvp_dsp_cmd_header *dsp2cpu_cmd_header = &me->pending_dsp2cpu_cmd_header;
	struct cvp_dsp2cpu_cmd *dsp2cpu_cmd = &me->pending_dsp2cpu_cmd;
	struct cvp_dsp2cpu_cmd_v2 *dsp2cpu_cmd_v2 = &me->pending_dsp2cpu_cmd_v2;
	uint32_t version = dsp2cpu_cmd_header->ver;
	struct cvp_dsp_fastrpc_driver_entry *frpc_node = NULL;
	struct pid *pid_s = NULL;
	struct task_struct *task = NULL;
	struct cvp_hfi_ops *ops_tbl;
	struct fastrpc_device *frpc_device;
	struct list_head *s = NULL, *next_s = NULL;
	bool found_inst = false;

	cmd->ret = 0;

	if (version == 0) {
		dprintk(CVP_DSP,
			"%s sess Type %d Mask %d Prio %d Sec %d hdl 0x%x\n",
			__func__, dsp2cpu_cmd->session_type,
			dsp2cpu_cmd->kernel_mask,
			dsp2cpu_cmd->session_prio,
			dsp2cpu_cmd->is_secure,
			dsp2cpu_cmd->pid);


		frpc_node = cvp_get_fastrpc_node_with_handle(dsp2cpu_cmd->pid);
	} else { // Version 1
		frpc_node = cvp_get_fastrpc_node_with_handle(dsp2cpu_cmd_v2->pid);
	}
	if (!frpc_node) {
		dprintk(CVP_WARN, "%s cannot get fastrpc node from pid %x\n",
				__func__, version ? dsp2cpu_cmd_v2->pid : dsp2cpu_cmd->pid);
		goto fail_lookup;
	}
	if (!frpc_node->cvp_fastrpc_device) {
		dprintk(CVP_WARN, "%s invalid fastrpc device from pid %x\n",
				__func__, version ? dsp2cpu_cmd_v2->pid : dsp2cpu_cmd->pid);
		goto fail_pid;
	}

	frpc_device = frpc_node->cvp_fastrpc_device;

	rc = eva_fastrpc_dev_get_pid(frpc_device, &pid);
	if (rc) {
		dprintk(CVP_ERR,
			"%s Failed to map buffer 0x%x\n", __func__, rc);
		goto fail_pid;
	}
	pid_s = find_get_pid(pid);
	if (pid_s == NULL) {
		dprintk(CVP_WARN, "%s incorrect pid %x\n", __func__, pid);
		goto fail_pid;
	}
	dprintk(CVP_DSP, "%s get pid_s 0x%x from hdl 0x%x\n", __func__,
			pid_s, version ? dsp2cpu_cmd_v2->pid : dsp2cpu_cmd->pid);

	task = get_pid_task(pid_s, PIDTYPE_TGID);
	if (!task) {
		dprintk(CVP_WARN, "%s task doesn't exist\n", __func__);
		goto fail_pid;
	}

	inst = msm_cvp_open(MSM_CVP_DSP, task);
	if (!inst) {
		dprintk(CVP_ERR, "%s Failed create instance\n", __func__);
		goto fail_msm_cvp_open;
	}

	if (version == 0) {
		inst->dsp_handle = dsp2cpu_cmd->pid;
		inst->fastrpc_entry = frpc_node;
		inst->prop.kernel_mask = dsp2cpu_cmd->kernel_mask;
		inst->prop.type =  dsp2cpu_cmd->session_type;
		inst->prop.priority = dsp2cpu_cmd->session_prio;
		inst->prop.is_secure = dsp2cpu_cmd->is_secure;
		inst->prop.dsp_mask = dsp2cpu_cmd->dsp_access_mask;
		inst->prop.pkt_concurrency = 8;
	} else { // Version 1
		inst->dsp_handle = dsp2cpu_cmd->pid;
		inst->fastrpc_entry = frpc_node;

		struct eva_kmd_sys_properties *props = &dsp2cpu_cmd_v2->prop_data;
		struct eva_kmd_sys_property *prop_array;
		int i = 0;

		if (props == NULL) {
			dprintk(CVP_ERR, "Invalid properties\n");
			goto fail_get_session_info;
		}
		if (props->prop_num > MAX_KMD_PROP_NUM_PER_PACKET) {
			dprintk(CVP_ERR, "Too many properties %d to set\n",
				props->prop_num);
			goto fail_get_session_info;
		}

		prop_array = &props->prop_data[0];

		for (i = 0; i < props->prop_num; i++) {
			if (msm_cvp_set_sysprop_sess(inst, &prop_array[i], i)) {
				dprintk(CVP_ERR,
					"unrecognized sys property to set %d\n",
					prop_array[i].prop_type);
				goto fail_get_session_info;
			}
		}
	}

	eva_fastrpc_driver_add_sess(frpc_node, inst);
	rc = msm_cvp_session_create(inst);
	if (rc) {
		dprintk(CVP_ERR, "Warning: send Session Create failed\n");
		goto fail_get_session_info;
	} else {
		dprintk(CVP_DSP, "%s DSP Session Create done\n", __func__);
	}

	/* Get session id */
	rc = msm_cvp_get_session_info(inst, &cmd->session_id);
	if (rc) {
		dprintk(CVP_ERR, "Warning: get session index failed %d\n", rc);
		goto fail_get_session_info;
	}

	inst_handle = (uint64_t)inst;
	cmd->session_cpu_high = (uint32_t)((inst_handle & HIGH32) >> 32);
	cmd->session_cpu_low = (uint32_t)(inst_handle & LOW32);

	cvp_put_fastrpc_node(frpc_node);

	inst->task = task;
	dprintk(CVP_DSP,
		"%s CREATE_SESS id 0x%x, cpu_low 0x%x, cpu_high 0x%x inst %pK, inst->session %pK\n",
		__func__, cmd->session_id, cmd->session_cpu_low,
		cmd->session_cpu_high, inst, inst->session);

	ops_tbl = inst->core->dev_ops;
	call_hfi_op(ops_tbl, pm_qos_update, ops_tbl->hfi_device_data);

	return;

fail_get_session_info:
	mutex_lock(&frpc_node->dsp_sessions.lock);
	s = &frpc_node->dsp_sessions.list;
	if (s && (s->next)) {
		list_for_each_safe(s, next_s,
				&frpc_node->dsp_sessions.list) {
			if (!s || !next_s)
				break;
			inst_temp = list_entry(s, struct msm_cvp_inst,
					dsp_list);
			if (inst_temp == inst) {
				found_inst = true;
				break;
			}
		}
	}
	if (found_inst) {
		list_del(&inst->dsp_list);
		frpc_node->session_cnt--;
		/* close dsp inst */
		msm_cvp_close(inst);
	} else {
		dprintk(CVP_WARN, "Failed DSP session %llx already deleted\n", inst);
	}
	mutex_unlock(&frpc_node->dsp_sessions.lock);
fail_msm_cvp_open:
	put_task_struct(task);
fail_pid:
	cvp_put_fastrpc_node(frpc_node);
	return;
fail_lookup:
	/* unregister fastrpc driver */
	eva_fastrpc_driver_unregister(dsp2cpu_cmd->pid, false);
	cmd->ret = -1;
}

void __dsp_cvp_sess_delete(struct cvp_dsp_cmd_msg *cmd)
{
	struct cvp_dsp_apps *me = &gfa_cv;
	struct msm_cvp_inst *inst, *inst_temp = NULL;
	int rc = 0;
	struct cvp_dsp2cpu_cmd *dsp2cpu_cmd = &me->pending_dsp2cpu_cmd;
	struct cvp_dsp_fastrpc_driver_entry *frpc_node = NULL;
	struct list_head *s = NULL, *next_s = NULL;
	bool found_inst = false;

	cmd->ret = 0;

	dprintk(CVP_DSP,
		"%s sess id 0x%x low 0x%x high 0x%x, pid 0x%x\n",
		__func__, dsp2cpu_cmd->session_id,
		dsp2cpu_cmd->session_cpu_low,
		dsp2cpu_cmd->session_cpu_high,
		dsp2cpu_cmd->pid);

	inst = (struct msm_cvp_inst *)get_inst_from_dsp(
			dsp2cpu_cmd->session_cpu_high,
			dsp2cpu_cmd->session_cpu_low);
	if (!inst) {
		dprintk(CVP_ERR, "%s incorrect session ID\n", __func__);
		cmd->ret = -1;
		goto dsp_fail_delete;
	}

	frpc_node = cvp_get_fastrpc_node_with_handle(dsp2cpu_cmd->pid);
	if (!frpc_node) {
		dprintk(CVP_ERR,
			"%s pid 0x%x not registered with fastrpc, but allow delete session\n",
			__func__, dsp2cpu_cmd->pid);
		cmd->ret = -1;
		goto dsp_fail_delete;
	} else {
		cvp_put_fastrpc_node(frpc_node);
	}

	mutex_lock(&frpc_node->dsp_sessions.lock);
	s = &frpc_node->dsp_sessions.list;
	if (s && (s->next)) {
		list_for_each_safe(s, next_s,
				&frpc_node->dsp_sessions.list) {
			if (!s || !next_s)
				break;
			inst_temp = list_entry(s, struct msm_cvp_inst,
					dsp_list);
			if (inst_temp == inst) {
				found_inst = true;
				break;
			}
		}
	}
	if (found_inst) {
		list_del(&inst->dsp_list);
		frpc_node->session_cnt--;
		mutex_unlock(&frpc_node->dsp_sessions.lock);
		/* Delete DSP session */
		rc = delete_dsp_session(inst, frpc_node);
	} else {
		mutex_unlock(&frpc_node->dsp_sessions.lock);
		dprintk(CVP_WARN, "DSP double deleted session %llx\n", inst);
	}

	if (rc) {
		cmd->ret = -1;
	}

dsp_fail_delete:
	return;
}

void __dsp_cvp_set_session_configs(struct cvp_dsp_cmd_msg *cmd)
{
	struct cvp_dsp_apps *me = &gfa_cv;
	struct msm_cvp_inst *inst;
	struct cvp_dsp2cpu_cmd_v2 *dsp2cpu_cmd_v2 = &me->pending_dsp2cpu_cmd_v2;

	cmd->ret = 0;
	dprintk(CVP_DSP,
		"%s sess id 0x%x, low 0x%x, high 0x%x\n",
		__func__, dsp2cpu_cmd_v2->session_id,
		dsp2cpu_cmd_v2->session_cpu_low,
		dsp2cpu_cmd_v2->session_cpu_high);

	inst = (struct msm_cvp_inst *)get_inst_from_dsp(
			dsp2cpu_cmd_v2->session_cpu_high,
			dsp2cpu_cmd_v2->session_cpu_low);

	if (!inst) {
		cmd->ret = -1;
		goto dsp_fail_set_session_configs;
	}

	struct eva_kmd_sys_properties *props = &dsp2cpu_cmd_v2->prop_data;
	struct eva_kmd_sys_property *prop_array;
	int i = 0;

	if (props == NULL) {
		dprintk(CVP_ERR, "Invalid properties\n");
		goto dsp_fail_set_session_configs;
	}
	if (props->prop_num > MAX_KMD_PROP_NUM_PER_PACKET) {
		dprintk(CVP_ERR, "Too many properties %d to set\n",
			props->prop_num);
		goto dsp_fail_set_session_configs;
	}

	prop_array = &props->prop_data[0];

	for (i = 0; i < props->prop_num; i++) {
		if (msm_cvp_set_sysprop_sess(inst, &prop_array[i], i)) {
			dprintk(CVP_ERR,
				"unrecognized sys property to set %d\n",
				prop_array[i].prop_type);
			goto dsp_fail_set_session_configs;
		}
	}

dsp_fail_set_session_configs:
	return;
}

void __dsp_cvp_power_req(struct cvp_dsp_cmd_msg *cmd)
{
	struct cvp_dsp_apps *me = &gfa_cv;
	struct msm_cvp_inst *inst;
	int rc;
	struct cvp_dsp2cpu_cmd *dsp2cpu_cmd = &me->pending_dsp2cpu_cmd;

	cmd->ret = 0;
	dprintk(CVP_DSP,
		"%s sess id 0x%x, low 0x%x, high 0x%x\n",
		__func__, dsp2cpu_cmd->session_id,
		dsp2cpu_cmd->session_cpu_low,
		dsp2cpu_cmd->session_cpu_high);

	inst = (struct msm_cvp_inst *)get_inst_from_dsp(
			dsp2cpu_cmd->session_cpu_high,
			dsp2cpu_cmd->session_cpu_low);

	if (!inst) {
		cmd->ret = -1;
		goto dsp_fail_power_req;
	}

	print_power(&dsp2cpu_cmd->power_req);

	inst->prop.cycles[HFI_HW_FDU] = dsp2cpu_cmd->power_req.clock_fdu;
	inst->prop.cycles[HFI_HW_MPU] = dsp2cpu_cmd->power_req.clock_mpu;
	inst->prop.cycles[HFI_HW_OD] = dsp2cpu_cmd->power_req.clock_od;
	inst->prop.cycles[HFI_HW_ICA] = dsp2cpu_cmd->power_req.clock_ica;
	inst->prop.cycles[HFI_HW_VADL] = dsp2cpu_cmd->power_req.clock_vadl;
	inst->prop.cycles[HFI_HW_TOF] = dsp2cpu_cmd->power_req.clock_tof;
	inst->prop.cycles[HFI_HW_RGE] = dsp2cpu_cmd->power_req.clock_rge;
	inst->prop.cycles[HFI_HW_XRA] = dsp2cpu_cmd->power_req.clock_xra;
	inst->prop.cycles[HFI_HW_LSR] = dsp2cpu_cmd->power_req.clock_lsr;
	inst->prop.fw_cycles = dsp2cpu_cmd->power_req.clock_fw;
	inst->prop.ddr_bw = dsp2cpu_cmd->power_req.bw_ddr;
	inst->prop.ddr_cache = dsp2cpu_cmd->power_req.bw_sys_cache;
	inst->prop.op_cycles[HFI_HW_FDU] = dsp2cpu_cmd->power_req.op_clock_fdu;
	inst->prop.op_cycles[HFI_HW_MPU] = dsp2cpu_cmd->power_req.op_clock_mpu;
	inst->prop.op_cycles[HFI_HW_OD] = dsp2cpu_cmd->power_req.op_clock_od;
	inst->prop.op_cycles[HFI_HW_ICA] = dsp2cpu_cmd->power_req.op_clock_ica;
	inst->prop.op_cycles[HFI_HW_VADL] = dsp2cpu_cmd->power_req.op_clock_vadl;
	inst->prop.op_cycles[HFI_HW_TOF] = dsp2cpu_cmd->power_req.op_clock_tof;
	inst->prop.op_cycles[HFI_HW_RGE] = dsp2cpu_cmd->power_req.op_clock_rge;
	inst->prop.op_cycles[HFI_HW_XRA] = dsp2cpu_cmd->power_req.op_clock_xra;
	inst->prop.op_cycles[HFI_HW_LSR] = dsp2cpu_cmd->power_req.op_clock_lsr;
	inst->prop.fw_op_cycles = dsp2cpu_cmd->power_req.op_clock_fw;
	inst->prop.ddr_op_bw = dsp2cpu_cmd->power_req.op_bw_ddr;
	inst->prop.ddr_op_cache = dsp2cpu_cmd->power_req.op_bw_sys_cache;

	rc = msm_cvp_update_power(inst);
	if (rc) {
		/*
		 *May need to define more error types
		 * Check UMD implementation
		 */
		dprintk(CVP_ERR, "%s Failed update power\n", __func__);
		cmd->ret = -1;
		goto dsp_fail_power_req;
	}

	dprintk(CVP_DSP, "%s DSP2CPU_POWER_REQUEST Done\n", __func__);
dsp_fail_power_req:
	return;
}

void __dsp_cvp_buf_register(struct cvp_dsp_cmd_msg *cmd)
{
	struct cvp_dsp_apps *me = &gfa_cv;
	struct msm_cvp_inst *inst;
	struct eva_kmd_arg *kmd;
	struct eva_kmd_buffer *kmd_buf;
	struct cvp_dsp_fastrpc_driver_entry *frpc_node = NULL;
	int rc;
	struct cvp_dsp2cpu_cmd *dsp2cpu_cmd = &me->pending_dsp2cpu_cmd;

	cmd->ret = 0;

	dprintk(CVP_DSP,
		"%s sess id 0x%x, low 0x%x, high 0x%x, pid 0x%x\n",
		__func__, dsp2cpu_cmd->session_id,
		dsp2cpu_cmd->session_cpu_low,
		dsp2cpu_cmd->session_cpu_high,
		dsp2cpu_cmd->pid);

	inst = (struct msm_cvp_inst *)get_inst_from_dsp(
			dsp2cpu_cmd->session_cpu_high,
			dsp2cpu_cmd->session_cpu_low);
	if (!inst) {
		dprintk(CVP_ERR, "%s incorrect session ID\n", __func__);
		cmd->ret = -1;
		return;
	}

	frpc_node = cvp_get_fastrpc_node_with_handle(dsp2cpu_cmd->pid);
	if (!frpc_node) {
		dprintk(CVP_WARN, "%s cannot get fastrpc node from pid %x\n",
				__func__, dsp2cpu_cmd->pid);
		return;
	}

	kmd = kzalloc(sizeof(*kmd), GFP_KERNEL);
	if (!kmd) {
		dprintk(CVP_ERR, "%s kzalloc failure\n", __func__);
		cmd->ret = -1;
		cvp_put_fastrpc_node(frpc_node);
		return;
	}

	kmd->type = EVA_KMD_REGISTER_BUFFER;
	kmd_buf = (struct eva_kmd_buffer *)&(kmd->data.regbuf);
	kmd_buf->type = EVA_KMD_BUFTYPE_INPUT;
	kmd_buf->index = dsp2cpu_cmd->sbuf.index;
	kmd_buf->fd = dsp2cpu_cmd->sbuf.fd;
	kmd_buf->size = dsp2cpu_cmd->sbuf.size;
	kmd_buf->offset = dsp2cpu_cmd->sbuf.offset;
	kmd_buf->pixelformat = 0;
	kmd_buf->flags = EVA_KMD_FLAG_UNSECURE;

	rc = msm_cvp_register_buffer(inst, kmd_buf);
	if (rc) {
		dprintk(CVP_ERR, "%s Failed to register buffer\n", __func__);
		cmd->ret = -1;
		goto dsp_fail_buf_reg;
	}
	dprintk(CVP_DSP, "%s register buffer done\n", __func__);

	atomic_inc(&frpc_node->smem_count);

	cmd->sbuf.iova = kmd_buf->reserved[0];
	cmd->sbuf.size = kmd_buf->size;
	cmd->sbuf.fd = kmd_buf->fd;
	cmd->sbuf.index = kmd_buf->index;
	cmd->sbuf.offset = kmd_buf->offset;
	dprintk(CVP_DSP, "%s: fd %d, iova 0x%x\n", __func__,
			cmd->sbuf.fd, cmd->sbuf.iova);
dsp_fail_buf_reg:
	cvp_put_fastrpc_node(frpc_node);
	kfree(kmd);
}

void __dsp_cvp_buf_deregister(struct cvp_dsp_cmd_msg *cmd)
{
	struct cvp_dsp_apps *me = &gfa_cv;
	struct msm_cvp_inst *inst;
	struct eva_kmd_arg *kmd;
	struct eva_kmd_buffer *kmd_buf;
	struct cvp_dsp_fastrpc_driver_entry *frpc_node = NULL;
	int rc;
	struct cvp_dsp2cpu_cmd *dsp2cpu_cmd = &me->pending_dsp2cpu_cmd;

	cmd->ret = 0;

	dprintk(CVP_DSP,
		"%s : sess id 0x%x, low 0x%x, high 0x%x, hdl 0x%x\n",
		__func__, dsp2cpu_cmd->session_id,
		dsp2cpu_cmd->session_cpu_low,
		dsp2cpu_cmd->session_cpu_high,
		dsp2cpu_cmd->pid);

	inst = (struct msm_cvp_inst *)get_inst_from_dsp(
			dsp2cpu_cmd->session_cpu_high,
			dsp2cpu_cmd->session_cpu_low);
	if (!inst) {
		dprintk(CVP_ERR, "%s incorrect session ID\n", __func__);
		cmd->ret = -1;
		return;
	}

	frpc_node = cvp_get_fastrpc_node_with_handle(dsp2cpu_cmd->pid);
	if (!frpc_node) {
		dprintk(CVP_WARN, "%s cannot get fastrpc node from pid %x\n",
				__func__, dsp2cpu_cmd->pid);
		return;
	}

	kmd = kzalloc(sizeof(*kmd), GFP_KERNEL);
	if (!kmd) {
		dprintk(CVP_ERR, "%s kzalloc failure\n", __func__);
		cmd->ret = -1;
		cvp_put_fastrpc_node(frpc_node);
		return;
	}

	kmd->type = EVA_KMD_UNREGISTER_BUFFER;
	kmd_buf = (struct eva_kmd_buffer *)&(kmd->data.regbuf);
	kmd_buf->type = EVA_KMD_UNREGISTER_BUFFER;

	kmd_buf->type = EVA_KMD_BUFTYPE_INPUT;
	kmd_buf->index = dsp2cpu_cmd->sbuf.index;
	kmd_buf->fd = dsp2cpu_cmd->sbuf.fd;
	kmd_buf->size = dsp2cpu_cmd->sbuf.size;
	kmd_buf->offset = dsp2cpu_cmd->sbuf.offset;
	kmd_buf->pixelformat = 0;
	kmd_buf->flags = EVA_KMD_FLAG_UNSECURE;

	rc = msm_cvp_unregister_buffer(inst, kmd_buf);
	if (rc) {
		dprintk(CVP_ERR, "%s Failed to deregister buffer\n", __func__);
		cmd->ret = -1;
		goto fail_dsp_buf_dereg;
	}

	atomic_dec(&frpc_node->smem_count);

	dprintk(CVP_DSP, "%s deregister buffer done\n", __func__);
fail_dsp_buf_dereg:
	cvp_put_fastrpc_node(frpc_node);
	kfree(kmd);
}

void __dsp_cvp_mem_alloc(struct cvp_dsp_cmd_msg *cmd)
{
	struct cvp_dsp_apps *me = &gfa_cv;
	int rc;
	struct cvp_internal_buf *buf = NULL;
	struct cvp_dsp2cpu_cmd *dsp2cpu_cmd = &me->pending_dsp2cpu_cmd;
	uint64_t v_dsp_addr = 0;

	struct fastrpc_device *frpc_device = NULL;
	struct cvp_dsp_fastrpc_driver_entry *frpc_node = NULL;

	cmd->ret = 0;

	dprintk(CVP_DSP,
		"%s hdl 0x%x\n",
		__func__, dsp2cpu_cmd->pid);

	frpc_node = cvp_get_fastrpc_node_with_handle(dsp2cpu_cmd->pid);
	if (!frpc_node) {
		dprintk(CVP_ERR, "%s Failed to find fastrpc node 0x%x\n",
				__func__, dsp2cpu_cmd->pid);
		goto fail_fastrpc_node;
	}
	frpc_device = frpc_node->cvp_fastrpc_device;

	buf = cvp_kmem_cache_zalloc(&cvp_driver->buf_cache, GFP_KERNEL);
	if (!buf)
		goto fail_kzalloc_buf;

	rc = cvp_allocate_dsp_bufs(buf,
			dsp2cpu_cmd->sbuf.size,
			dsp2cpu_cmd->sbuf.type);
	if (rc)
		goto fail_allocate_dsp_buf;

	rc = eva_fastrpc_dev_map_dma(frpc_device, buf,
			dsp2cpu_cmd->sbuf.dsp_remote_map,
			&v_dsp_addr);
	if (rc) {
		dprintk(CVP_ERR, "%s Failed to map buffer 0x%x\n", __func__,
			rc);
		goto fail_fastrpc_dev_map_dma;
	}

	mutex_lock(&frpc_node->cvpdspbufs.lock);
	list_add_tail(&buf->list, &frpc_node->cvpdspbufs.list);
	mutex_unlock(&frpc_node->cvpdspbufs.lock);

	atomic_inc(&frpc_node->smem_count);

	dprintk(CVP_DSP, "%s allocate buffer done, addr 0x%llx\n",
		__func__, v_dsp_addr);

	cmd->sbuf.size = buf->smem->size;
	cmd->sbuf.fd = buf->fd;
	cmd->sbuf.offset = 0;
	cmd->sbuf.iova = buf->smem->device_addr;
	cmd->sbuf.v_dsp_addr = v_dsp_addr;
	dprintk(CVP_DSP, "%s: size %d, iova 0x%x, v_dsp_addr 0x%llx pid %#x, fd %#x, refcount %d\n",
		__func__, cmd->sbuf.size, cmd->sbuf.iova,
		cmd->sbuf.v_dsp_addr, dsp2cpu_cmd->pid, buf->smem->fd, buf->smem->refcount);

	cvp_put_fastrpc_node(frpc_node);
	return;

fail_fastrpc_dev_map_dma:
	cvp_release_dsp_buffers(buf);
fail_allocate_dsp_buf:
	cvp_kmem_cache_free(&cvp_driver->buf_cache, buf);
fail_kzalloc_buf:
fail_fastrpc_node:
	cmd->ret = -1;
	cvp_put_fastrpc_node(frpc_node);
	return;

}

void __dsp_cvp_mem_free(struct cvp_dsp_cmd_msg *cmd)
{
	struct cvp_dsp_apps *me = &gfa_cv;
	int rc;
	struct cvp_internal_buf *buf = NULL;
	struct list_head *ptr = NULL, *next = NULL;
	struct msm_cvp_list *buf_list = NULL;
	struct cvp_dsp2cpu_cmd *dsp2cpu_cmd = &me->pending_dsp2cpu_cmd;

	struct fastrpc_device *frpc_device = NULL;
	struct cvp_dsp_fastrpc_driver_entry *frpc_node = NULL;

	cmd->ret = 0;

	dprintk(CVP_DSP,
		"%s hnl 0x%x\n",
		__func__, dsp2cpu_cmd->pid);

	frpc_node = cvp_get_fastrpc_node_with_handle(dsp2cpu_cmd->pid);
	if (!frpc_node) {
		dprintk(CVP_ERR,
			"%s Failed to find fastrpc node 0x%x, mem free failed\n",
			__func__, dsp2cpu_cmd->pid);
		cmd->ret = -1;
		return;
	} else {
		frpc_device = frpc_node->cvp_fastrpc_device;
	}

	buf_list = &frpc_node->cvpdspbufs;
	mutex_lock(&buf_list->lock);
	list_for_each_safe(ptr, next, &buf_list->list) {
		if (!ptr)
			break;
		buf = list_entry(ptr, struct cvp_internal_buf, list);

		if (!buf->smem) {
			dprintk(CVP_DSP, "Empyt smem\n");
			continue;
		}

		/* Verify with device addr */
		if ((buf->smem->device_addr == dsp2cpu_cmd->sbuf.iova) &&
			(buf->fd == dsp2cpu_cmd->sbuf.fd)) {
			dprintk(CVP_DSP, "%s find device addr 0x%x\n",
				__func__, buf->smem->device_addr);
			dprintk(CVP_DSP, "fd in list 0x%x, fd from dsp 0x%x\n",
				buf->fd, dsp2cpu_cmd->sbuf.fd);

			if (frpc_node) {
				rc = eva_fastrpc_dev_unmap_dma(frpc_device, buf);
				if (rc) {
					cmd->ret = -1;
					goto fail_fastrpc_dev_unmap_dma;
				}
			}

			rc = cvp_release_dsp_buffers(buf);
			if (rc) {
				dprintk(CVP_ERR,
					"%s Failed to free buffer 0x%x\n",
					__func__, rc);
				cmd->ret = -1;
				goto fail_release_buf;
			}

			list_del(&buf->list);
			atomic_dec(&frpc_node->smem_count);

			cvp_kmem_cache_free(&cvp_driver->buf_cache, buf);
			break;
		}
	}

fail_release_buf:
fail_fastrpc_dev_unmap_dma:
	mutex_unlock(&buf_list->lock);
	if (frpc_node)
		cvp_put_fastrpc_node(frpc_node);
}

void __dsp_cvp_sess_start(struct cvp_dsp_cmd_msg *cmd)
{
	struct cvp_dsp_apps *me = &gfa_cv;
	struct msm_cvp_inst *inst;
	struct cvp_session_queue *sq;
	int rc;
	struct cvp_dsp2cpu_cmd *dsp2cpu_cmd = &me->pending_dsp2cpu_cmd;

	cmd->ret = 0;

	dprintk(CVP_DSP,
		"%s sess id 0x%x, low 0x%x, high 0x%x, pid 0x%x\n",
		__func__, dsp2cpu_cmd->session_id,
		dsp2cpu_cmd->session_cpu_low,
		dsp2cpu_cmd->session_cpu_high,
		dsp2cpu_cmd->pid);

	inst = (struct msm_cvp_inst *)get_inst_from_dsp(
			dsp2cpu_cmd->session_cpu_high,
			dsp2cpu_cmd->session_cpu_low);

	if (!inst || !is_cvp_inst_valid(inst)) {
		dprintk(CVP_ERR, "%s incorrect session ID %llx\n", __func__, inst);
		cmd->ret = -1;
		return;
	}

	sq = &inst->session_queue;
	spin_lock(&sq->lock);
	if (sq->state == QUEUE_START) {
		spin_unlock(&sq->lock);
		dprintk(CVP_WARN, "DSP double started session %llx\n", inst);
		return;
	}
	spin_unlock(&sq->lock);

	rc = msm_cvp_session_start(inst, (struct eva_kmd_arg *)NULL);
	if (rc) {
		dprintk(CVP_ERR, "%s Failed to start session %llx\n", __func__, inst);
		cmd->ret = -1;
		return;
	}
	dprintk(CVP_DSP, "%s session started\n", __func__);
}

void __dsp_cvp_sess_stop(struct cvp_dsp_cmd_msg *cmd)
{
	struct cvp_dsp_apps *me = &gfa_cv;
	struct msm_cvp_inst *inst;
	struct cvp_session_queue *sq;
	int rc;
	struct cvp_dsp2cpu_cmd *dsp2cpu_cmd = &me->pending_dsp2cpu_cmd;

	cmd->ret = 0;

	dprintk(CVP_DSP,
		"%s sess id 0x%x, low 0x%x, high 0x%x, pid 0x%x\n",
		__func__, dsp2cpu_cmd->session_id,
		dsp2cpu_cmd->session_cpu_low,
		dsp2cpu_cmd->session_cpu_high,
		dsp2cpu_cmd->pid);

	inst = (struct msm_cvp_inst *)get_inst_from_dsp(
			dsp2cpu_cmd->session_cpu_high,
			dsp2cpu_cmd->session_cpu_low);

	if (!inst || !is_cvp_inst_valid(inst)) {
		dprintk(CVP_ERR, "%s incorrect session ID %llx\n", __func__, inst);
		cmd->ret = -1;
		return;
	}

	sq = &inst->session_queue;
	spin_lock(&sq->lock);
	if (sq->state == QUEUE_STOP) {
		spin_unlock(&sq->lock);
		dprintk(CVP_WARN, "DSP double stopped session %llx\n", inst);
		return;
	}
	spin_unlock(&sq->lock);

	rc = msm_cvp_session_stop(inst, (struct eva_kmd_arg *)NULL);
	if (rc) {
		dprintk(CVP_ERR, "%s Failed to stop session\n", __func__);
		cmd->ret = -1;
		return;
	}
	dprintk(CVP_DSP, "%s session stoppd\n", __func__);
}

void __dsp_cvp_set_session_name(struct cvp_dsp_cmd_msg *cmd)
{
	struct cvp_dsp_apps *me = &gfa_cv;
	struct msm_cvp_inst *inst;
	struct cvp_dsp2cpu_cmd *dsp2cpu_cmd = &me->pending_dsp2cpu_cmd;
	struct cvp_session_prop *session_prop;

	cmd->ret = 0;

	dprintk(CVP_DSP,
		"%s sess id 0x%x, low 0x%x, high 0x%x, pid 0x%x\n",
		__func__, dsp2cpu_cmd->session_id,
		dsp2cpu_cmd->session_cpu_low,
		dsp2cpu_cmd->session_cpu_high,
		dsp2cpu_cmd->pid);

	inst = (struct msm_cvp_inst *)get_inst_from_dsp(
			dsp2cpu_cmd->session_cpu_high,
			dsp2cpu_cmd->session_cpu_low);

	if (!inst || !is_cvp_inst_valid(inst)) {
		dprintk(CVP_ERR, "%s incorrect session ID %llx\n", __func__, inst);
		cmd->ret = -1;
		return;
	}
	session_prop = &inst->prop;
	memcpy(session_prop->session_name, dsp2cpu_cmd->session_name, SESSION_NAME_MAX_LEN);

}

void __dsp_cvp_pd_init(struct cvp_dsp_cmd_msg *cmd)
{
	struct cvp_dsp_apps *me = &gfa_cv;
	struct cvp_dsp2cpu_cmd *dsp2cpu_cmd = &me->pending_dsp2cpu_cmd;
	struct cvp_dsp_fastrpc_driver_entry *frpc_node = NULL;
	// struct cvp_session_prop *session_prop;
	struct task_struct *task = NULL;
	struct fastrpc_device *frpc_device;

	int rc = 0;
	struct pid *pid_s = NULL;
	uint32_t pid;

	cmd->ret = 0;

	rc = eva_fastrpc_driver_register(dsp2cpu_cmd->pid);
	if (rc) {
		dprintk(CVP_ERR, "%s Register fastrpc driver fail\n", __func__);
		cmd->ret = -1;
		return;
	}
	frpc_node = cvp_get_fastrpc_node_with_handle(dsp2cpu_cmd->pid);
	if (!frpc_node) {
		dprintk(CVP_WARN, "%s cannot get fastrpc node from pid %x\n",
				__func__, dsp2cpu_cmd->pid);
		goto fail_lookup;
	}
	if (!frpc_node->cvp_fastrpc_device) {
		dprintk(CVP_WARN, "%s invalid fastrpc device from pid %x\n",
				__func__, dsp2cpu_cmd->pid);
		goto fail_pid;
	}

	frpc_device = frpc_node->cvp_fastrpc_device;

	rc = eva_fastrpc_dev_get_pid(frpc_device, &pid);
	if (rc) {
		dprintk(CVP_ERR,
			"%s Failed to map buffer 0x%x\n", __func__, rc);
		goto fail_pid;
	}
	pid_s = find_get_pid(pid);
	if (pid_s == NULL) {
		dprintk(CVP_WARN, "%s incorrect pid %x\n", __func__, pid);
		goto fail_pid;
	}
	dprintk(CVP_DSP, "%s get pid_s 0x%x from hdl 0x%x\n", __func__,
			pid_s, dsp2cpu_cmd->pid);

	task = get_pid_task(pid_s, PIDTYPE_TGID);
	if (!task) {
		dprintk(CVP_WARN, "%s task doesn't exist\n", __func__);
		goto fail_pid;
	}

	cvp_put_fastrpc_node(frpc_node);
	return;

fail_pid:
	cvp_put_fastrpc_node(frpc_node);
fail_lookup:
	/* unregister fastrpc driver */
	eva_fastrpc_driver_unregister(dsp2cpu_cmd->pid, false);
	cmd->ret = -1;
}

static bool __is_buf_valid(struct msm_cvp_inst *inst,
		struct eva_kmd_buffer *buf,
		struct cvp_dsp_fastrpc_driver_entry *frpc_node)
{
	struct cvp_internal_buf *cbuf = (struct cvp_internal_buf *)0xdeadbeef;
	bool found = false;

	if (!inst || !inst->core || !buf || !frpc_node) {
		dprintk(CVP_ERR, "%s: invalid params\n", __func__);
		return false;
	}

	if (buf->offset) {
		dprintk(CVP_ERR,
			"%s: offset is deprecated, set to 0.\n",
			__func__);
		return false;
	}

	mutex_lock(&frpc_node->cvpdspbufs.lock);
	list_for_each_entry(cbuf, &frpc_node->cvpdspbufs.list, list) {
		if (cbuf->fd == buf->fd) {
			if (cbuf->size != buf->size) {
				dprintk(CVP_ERR, "%s: buf size mismatch\n",
					__func__);
				mutex_unlock(&frpc_node->cvpdspbufs.lock);
				return false;
			}
			found = true;
			break;
		}
	}
	mutex_unlock(&frpc_node->cvpdspbufs.lock);
	if (found) {
		print_internal_dsp_buffer(CVP_ERR, "duplicate", frpc_node, cbuf);
		return false;
	}

	return true;
}

static struct file *msm_cvp_fget(unsigned int fd, struct task_struct *task,
			fmode_t mask, unsigned int refs)
{
	struct file *file;

#if (KERNEL_VERSION(5, 13, 0) > LINUX_VERSION_CODE)
	struct files_struct *files = task->files;

	if (!files)
		return NULL;

	rcu_read_lock();
	file = fcheck_files(files, fd);
	rcu_read_unlock();
#else
	unsigned int ret_fd = fd;

#if (KERNEL_VERSION(6, 13, 0) <= LINUX_VERSION_CODE)
	file = fget_task_next(task, &ret_fd);
#else
	file = task_lookup_next_fdget_rcu(task, &ret_fd);
#endif
	if (ret_fd != fd)
		dprintk(CVP_ERR, "%s FAILED to get file from fd = %u, %u\n",
			__func__, fd, ret_fd);
#endif

	return file;
}

static struct dma_buf *cvp_dma_buf_get(struct file *file, int fd,
			struct task_struct *task)
{
	if (file->f_op != gfa_cv.dmabuf_f_op) {
		dprintk(CVP_WARN, "fd doesn't refer to dma_buf\n");
		return ERR_PTR(-EINVAL);
	}

	return file->private_data;
}

int msm_cvp_map_buf_dsp(struct msm_cvp_inst *inst,
			struct eva_kmd_buffer *buf)
{
	int rc = 0;
	struct cvp_internal_buf *cbuf = NULL;
	struct msm_cvp_smem *smem = NULL;
	struct dma_buf *dma_buf = NULL;
	struct file *file;
	struct cvp_dsp_fastrpc_driver_entry *frpc_node = NULL;

	if (!inst->task)
		return -EINVAL;

	frpc_node = inst->fastrpc_entry;
	if (!frpc_node) {
		dprintk(CVP_ERR, "%s: invalid frpc node\n", __func__);
		return -EINVAL;
	}

	if (!__is_buf_valid(inst, buf, frpc_node))
		return -EINVAL;

	file = msm_cvp_fget(buf->fd, inst->task, FMODE_PATH, 1);
	if (file == NULL) {
		dprintk(CVP_WARN, "%s fail to get file from fd %d %s\n",
						__func__, buf->fd, inst->proc_name);
		return -EINVAL;
	}

	dma_buf = cvp_dma_buf_get(
			file,
			buf->fd,
			inst->task);
	if (dma_buf == ERR_PTR(-EINVAL)) {
		dprintk(CVP_ERR, "%s: Invalid fd = %d", __func__, buf->fd);
		rc = -EINVAL;
		goto exit;
	}

	if (dma_buf->size < buf->size) {
		dprintk(CVP_ERR, "%s DSP client buffer too large %d > %d\n",
			__func__, buf->size, dma_buf->size);
		rc =  -EINVAL;
		goto exit;
	}

	dprintk(CVP_MEM, "dma_buf from internal %llu\n", dma_buf);

	cbuf = cvp_kmem_cache_zalloc(&cvp_driver->buf_cache, GFP_KERNEL);
	if (!cbuf) {
		rc = -ENOMEM;
		goto exit;
	}

	smem = cvp_kmem_cache_zalloc(&cvp_driver->smem_cache, GFP_KERNEL);
	if (!smem) {
		rc = -ENOMEM;
		goto exit;
	}

	smem->dma_buf = dma_buf;
	smem->cached = false;
	smem->pkt_type = 0;
	smem->buf_idx = 0;
	smem->fd = buf->fd;
	dprintk(CVP_MEM, "%s: dma_buf = %llx\n", __func__, dma_buf);
	rc = msm_cvp_map_smem(inst, smem, "map dsp");
	if (rc) {
		print_client_buffer(CVP_ERR, "map failed", inst, buf);
		goto exit;
	}

	atomic_inc(&smem->refcount);
	cbuf->smem = smem;
	cbuf->fd = buf->fd;
	cbuf->size = buf->size;
	cbuf->offset = buf->offset;
	cbuf->ownership = CLIENT;
	cbuf->index = buf->index;

	buf->reserved[0] = (uint32_t)smem->device_addr;
	buf->size = dma_buf->size; // Pass to QDI for reference

	mutex_lock(&frpc_node->cvpdspbufs.lock);
	list_add_tail(&cbuf->list, &frpc_node->cvpdspbufs.list);
	mutex_unlock(&frpc_node->cvpdspbufs.lock);

	return rc;

exit:
	if (smem) {
		if (smem->device_addr) {
			rc = msm_cvp_unmap_smem(inst, smem, "unmap dsp");
			if (rc)
				dprintk(CVP_ERR, "%s: Fail to unmap smem 0x%x, error %d\n",
					__func__, smem, rc);
		}
		cvp_kmem_cache_free(&cvp_driver->smem_cache, smem);
	}
	if (cbuf)
		cvp_kmem_cache_free(&cvp_driver->buf_cache, cbuf);
	fput(file);
	return rc;
}

int msm_cvp_unmap_buf_dsp(struct msm_cvp_inst *inst,
			struct eva_kmd_buffer *buf)
{
	int rc = 0;
	bool found;
	struct cvp_internal_buf *cbuf = (struct cvp_internal_buf *)0xdeadbeef;
	struct cvp_hal_session *session;
	struct cvp_dsp_fastrpc_driver_entry *frpc_node = NULL;

	if (!inst || !inst->core || !buf) {
		dprintk(CVP_ERR, "%s: invalid params\n", __func__);
		return -EINVAL;
	}

	session = (struct cvp_hal_session *)inst->session;
	if (!session) {
		dprintk(CVP_ERR, "%s: invalid session\n", __func__);
		return -EINVAL;
	}

	frpc_node = inst->fastrpc_entry;
	if (!frpc_node) {
		dprintk(CVP_ERR, "%s: invalid frpc node\n", __func__);
		return -EINVAL;
	}

	mutex_lock(&frpc_node->cvpdspbufs.lock);
	found = false;
	list_for_each_entry(cbuf, &frpc_node->cvpdspbufs.list, list) {
		if (cbuf->fd == buf->fd) {
			found = true;
			break;
		}
	}
	if (!found) {
		mutex_unlock(&frpc_node->cvpdspbufs.lock);
		print_client_buffer(CVP_ERR, "invalid", inst, buf);
		return -EINVAL;
	}

	if (cbuf->smem->device_addr) {
		u64 idx = frpc_node->unused_dsp_bufs.ktid;

		frpc_node->unused_dsp_bufs.smem[idx] = *(cbuf->smem);
		frpc_node->unused_dsp_bufs.nr++;
		frpc_node->unused_dsp_bufs.nr =
			(frpc_node->unused_dsp_bufs.nr > MAX_FRAME_BUFFER_NUMS) ?
			MAX_FRAME_BUFFER_NUMS : frpc_node->unused_dsp_bufs.nr;
		frpc_node->unused_dsp_bufs.ktid = ++idx % MAX_FRAME_BUFFER_NUMS;

		rc = msm_cvp_unmap_smem(inst, cbuf->smem, "unmap dsp");
		if (rc)
			dprintk(CVP_ERR, "%s: Fail to unmap smem 0x%x, error %d\n",
				__func__, cbuf->smem, rc);
		else
			msm_cvp_smem_put_dma_buf(cbuf->smem->dma_buf);
		atomic_dec(&cbuf->smem->refcount);
	}
	list_del(&cbuf->list);
	mutex_unlock(&frpc_node->cvpdspbufs.lock);

	cvp_kmem_cache_free(&cvp_driver->smem_cache, cbuf->smem);
	cvp_kmem_cache_free(&cvp_driver->buf_cache, cbuf);
	return rc;
}

static int cvp_dsp_thread(void *data)
{
	int rc = 0, old_state;
	struct cvp_dsp_apps *me = &gfa_cv;
	struct cvp_dsp_cmd_msg cmd;
	struct cvp_hfi_ops *ops_tbl;
	struct msm_cvp_core *core;

	core = cvp_driver->cvp_core;
	if (!core) {
		dprintk(CVP_ERR, "%s: Failed to find core\n", __func__);
		rc = -EINVAL;
		goto exit;
	}

	ops_tbl = (struct cvp_hfi_ops *)core->dev_ops;
	if (!ops_tbl) {
		dprintk(CVP_ERR, "%s Invalid device handle\n", __func__);
		rc = -EINVAL;
		goto exit;
	}

wait_dsp:
	rc = wait_for_completion_interruptible(
			&me->completions[CPU2DSP_MAX_CMD]);

	if (me->state == DSP_INVALID)
		goto exit;

	if (me->state == DSP_UNINIT)
		goto wait_dsp;

	if (me->state == DSP_PROBED) {
		cvp_dsp_send_hfi_queue();
		goto wait_dsp;
	}

	/* Set the cmd to 0 to avoid sending previous session values in case the command fails*/
	memset(&cmd, 0, sizeof(struct cvp_dsp_cmd_msg));
	cmd.type = me->pending_dsp2cpu_cmd_header.type;

	if (rc == -ERESTARTSYS) {
		dprintk(CVP_WARN, "%s received interrupt signal\n", __func__);
	} else {
		mutex_lock(&me->rx_lock);
		if (me->state == DSP_UNINIT) {
			/* DSP SSR may have happened */
			mutex_unlock(&me->rx_lock);
			goto wait_dsp;
		}
		switch (me->pending_dsp2cpu_cmd_header.type) {
		case DSP2CPU_POWERON:
		{
			mutex_lock(&me->tx_lock);
			if (me->state == DSP_READY) {
				cmd.ret = 0;
				mutex_unlock(&me->tx_lock);
				break;
			}

			old_state = me->state;
			me->state = DSP_READY;
			rc = call_hfi_op(ops_tbl, resume, ops_tbl->hfi_device_data);
			if (rc) {
				dprintk(CVP_WARN, "%s Failed to resume cvp\n",
						__func__);
				me->state = old_state;
				mutex_unlock(&me->tx_lock);
				cmd.ret = 1;
				break;
			}
			mutex_unlock(&me->tx_lock);
			cmd.ret = 0;
			break;
		}
		case DSP2CPU_POWEROFF:
		{
			me->state = DSP_SUSPEND;
			cmd.ret = 0;
			break;
		}
		case DSP2CPU_CREATE_SESSION:
		{
			__dsp_cvp_sess_create(&cmd);

			break;
		}
		case DSP2CPU_DETELE_SESSION:
		{
			__dsp_cvp_sess_delete(&cmd);

			break;
		}
		case DSP2CPU_POWER_REQUEST:
		{
			__dsp_cvp_power_req(&cmd);

			break;
		}
		case DSP2CPU_SET_SESSION_CONFIGS:
		{
			__dsp_cvp_set_session_configs(&cmd);

			break;
		}
		case DSP2CPU_REGISTER_BUFFER:
		{
			__dsp_cvp_buf_register(&cmd);

			break;
		}
		case DSP2CPU_DEREGISTER_BUFFER:
		{
			__dsp_cvp_buf_deregister(&cmd);

			break;
		}
		case DSP2CPU_MEM_ALLOC:
		{
			__dsp_cvp_mem_alloc(&cmd);

			break;
		}
		case DSP2CPU_MEM_FREE:
		{
			__dsp_cvp_mem_free(&cmd);

			break;
		}
		case DSP2CPU_START_SESSION:
		{
			__dsp_cvp_sess_start(&cmd);

			break;
		}
		case DSP2CPU_STOP_SESSION:
		{
			__dsp_cvp_sess_stop(&cmd);

			break;
		}
		case DSP2CPU_SET_SESSION_NAME:
		{
			__dsp_cvp_set_session_name(&cmd);

			break;
		}
		case DSP2CPU_PD_INIT:
		{
			__dsp_cvp_pd_init(&cmd);

			break;
		}

		default:
			dprintk(CVP_ERR, "unrecognaized dsp cmds: %d\n",
					me->pending_dsp2cpu_cmd_header.type);
			break;
		}
		me->pending_dsp2cpu_cmd.type = CVP_INVALID_RPMSG_TYPE;
		me->pending_dsp2cpu_cmd_v2.header.type = CVP_INVALID_RPMSG_TYPE;
		me->pending_dsp2cpu_cmd_header.type = CVP_INVALID_RPMSG_TYPE;
		mutex_unlock(&me->rx_lock);
	}
	/* Responds to DSP */
	rc = cvp_dsp_send_cmd(&cmd, sizeof(struct cvp_dsp_cmd_msg));
	if (rc)
		dprintk(CVP_ERR,
			"%s: cvp_dsp_send_cmd failed rc = %d cmd type=%d\n",
			__func__, rc, cmd.type);
	goto wait_dsp;
exit:
	dprintk(CVP_DBG, "dsp thread exit\n");
	return rc;
}

int cvp_dsp_device_init(void)
{
	struct cvp_dsp_apps *me = &gfa_cv;
	char tname[16];
	int rc;
	int i;
	char name[CVP_FASTRPC_DRIVER_NAME_SIZE] = "qcom,fastcv0\0";

	add_va_node_to_list(CVP_DBG_DUMP, &gfa_cv, sizeof(struct cvp_dsp_apps),
        "cvp_dsp_apps-gfa_cv", false);

	mutex_init(&me->tx_lock);
	mutex_init(&me->rx_lock);
	me->state = DSP_INVALID;
	me->hyp_assigned = false;

	for (i = 0; i <= CPU2DSP_MAX_CMD; i++)
		init_completion(&me->completions[i]);

	me->pending_dsp2cpu_cmd.type = CVP_INVALID_RPMSG_TYPE;
	me->pending_dsp2cpu_cmd_v2.header.type = CVP_INVALID_RPMSG_TYPE;
	me->pending_dsp2cpu_cmd_header.type = CVP_INVALID_RPMSG_TYPE;
	me->pending_dsp2cpu_rsp.type = CVP_INVALID_RPMSG_TYPE;

	INIT_MSM_CVP_LIST(&me->fastrpc_driver_list);

	mutex_init(&me->driver_name_lock);
	for (i = 0; i < MAX_FASTRPC_DRIVER_NUM; i++) {
		me->cvp_fastrpc_name[i].status = DRIVER_NAME_AVAILABLE;
		snprintf(me->cvp_fastrpc_name[i].name, sizeof(name), name);
		name[11]++;
	}

	rc = register_rpmsg_driver(&cvp_dsp_rpmsg_client);
	if (rc) {
		dprintk(CVP_ERR,
			"%s : register_rpmsg_driver failed rc = %d\n",
			__func__, rc);
		goto register_bail;
	}
	snprintf(tname, sizeof(tname), "cvp-dsp-thread");

	mutex_lock(&me->tx_lock);
	if (me->state == DSP_INVALID)
		me->state = DSP_UNINIT;
	mutex_unlock(&me->tx_lock);

	me->dsp_thread = kthread_run(cvp_dsp_thread, me, tname);
	if (!me->dsp_thread) {
		dprintk(CVP_ERR, "%s create %s fail", __func__, tname);
		rc = -ECHILD;
		me->state = DSP_INVALID;
		goto register_bail;
	}
	return 0;

register_bail:
	return rc;
}

void cvp_dsp_device_exit(void)
{
	struct cvp_dsp_apps *me = &gfa_cv;
	int i;

	mutex_lock(&me->tx_lock);
	me->state = DSP_INVALID;
	mutex_unlock(&me->tx_lock);

	DEINIT_MSM_CVP_LIST(&me->fastrpc_driver_list);

	for (i = 0; i <= CPU2DSP_MAX_CMD; i++)
		complete_all(&me->completions[i]);

	mutex_destroy(&me->tx_lock);
	mutex_destroy(&me->rx_lock);
	mutex_destroy(&me->driver_name_lock);
	unregister_rpmsg_driver(&cvp_dsp_rpmsg_client);
}
