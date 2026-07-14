// SPDX-License-Identifier: GPL-2.0
/* Copyright (c) 2011-2018, The Linux Foundation. All rights reserved.
 * Copyright (c) 2018, Linaro Limited
 * Copyright (c) 2022-2023 Qualcomm Innovation Center, Inc. All rights reserved.
 */
#include <linux/device.h>
#include <linux/dma-mapping.h>
#include <linux/idr.h>
#include <linux/list.h>
#include <linux/module.h>
#include <linux/of_platform.h>
#include <linux/rpmsg.h>
#include <linux/pm_qos.h>
#include "../include/uapi/misc/fastrpc.h"
#include <linux/of_reserved_mem.h>
#include "fastrpc_shared.h"
#include <linux/soc/qcom/pdr.h>
#include <linux/delay.h>
#include <linux/remoteproc.h>
#include <linux/rpmsg/qcom_glink.h>

void fastrpc_channel_ctx_put(struct fastrpc_channel_ctx *cctx);
void fastrpc_channel_ctx_get(struct fastrpc_channel_ctx *cctx);
void fastrpc_update_gctx(struct fastrpc_channel_ctx *cctx, int flag);
void fastrpc_lowest_capacity_corecount(struct device *dev, struct fastrpc_channel_ctx *cctx);
int fastrpc_init_privileged_gids(struct device *dev, char *prop_name,
						struct gid_list *gidlist);
int fastrpc_setup_service_locator(struct fastrpc_channel_ctx *cctx, char *client_name,
					char *service_name, char *service_path, int spd_session);
void fastrpc_register_wakeup_source(struct device *dev,
	const char *client_name, struct wakeup_source **device_wake_source);
int fastrpc_mmap_remove_ssr(struct fastrpc_channel_ctx *cctx, bool is_pdr);
void fastrpc_queue_pd_status(struct fastrpc_user *fl, int domain, int status, int sessionid);
void frpc_coredump(struct fastrpc_channel_ctx *cctx,
	struct list_head *active_users_list);

struct fastrpc_channel_ctx* get_current_channel_ctx(struct device *dev)
{
	return dev_get_drvdata(dev->parent);
}

/*
 * Callback function for kernel worker thread to trigger dsp ssr in case
 * of a timeout of a kernel rpc call
 */
static void fastrpc_handle_ssr_request(struct work_struct *work)
{
	int rc = 0;
	struct fastrpc_ssr_handler *ssr_handler =
		container_of(work, struct fastrpc_ssr_handler, ssr_work);
	void *rphandle = ssr_handler->rphandle;

	if (!rphandle) {
		pr_err("Error: %s: invalid rproc handle for domain %d\n",
			__func__, ssr_handler->domain_id);
		goto bail;
	}

	/* Shut down DSP */
	rc = rproc_shutdown(rphandle);
	if (rc) {
		pr_err("Error: %s: rproc_shutdown failed with rc %d and rphandle %pK\n",
			__func__, rc, rphandle);
		goto bail;
	}

	/* Reboot DSP */
	rc = rproc_boot(rphandle);
	if (rc) {
		pr_err("Error: %s: rproc_boot failed with rc %d and rphandle %pK\n",
			__func__, rc, rphandle);
		goto bail;
	}
	pr_info("%s : SSR completed successfully", __func__);

bail:
	return;
}

/*
 * Callback function invoked when the timer for a kernel rpc call expires
 *
 * If kernel rpc call times out, it indicates that the dsp is potentially
 * in an irrecoverable state as fastrpc on rootpd is unresponsive. So,
 * trigger an ssr on the dsp
 */

void ssr_timer_callback(struct timer_list *timer)
{
	struct fastrpc_channel_ctx *cctx = NULL;
	unsigned long flags;
	void *rphandle = NULL;
	struct fastrpc_ssr_handler *ssr_handler = NULL;
	struct fastrpc_invoke_ctx *ctx =
		container_of(timer, struct fastrpc_invoke_ctx, ssr_timer);

	if (!ctx) {
		pr_err("Error: %s: invoke ctx is null\n", __func__);
		return;
	}

	cctx = ctx->fl->cctx;
	if (!cctx) {
		pr_err("Error: %s channel ctx is null for handle 0x%x, sc 0x%x, pid %d, tid %d\n",
			__func__, ctx->handle, ctx->sc, ctx->tgid, ctx->pid);
		return;
	}

	fastrpc_channel_ctx_get(cctx);
	spin_lock_irqsave(&cctx->lock, flags);

	/* Ensure that ssr is triggered only once for a channel */
	if (cctx->startshutdown)
		goto bail;
	else
		cctx->startshutdown = true;

	if (cctx->rpdev && !atomic_read(&cctx->teardown)) {
		/* Get remote processor handle associated with device */
		rphandle = rproc_get_by_child(&cctx->rpdev->dev);
		if (!rphandle) {
			pr_err("Error: %s: rproc_get_by_child failed for domain %d\n",
				__func__, cctx->domain_id);
			goto bail;
		}
	} else {
		pr_err("Error: %s: channel already down for domain %d, handle 0x%x, sc 0x%x, pid %d, tid %d\n",
			__func__, cctx->domain_id, ctx->handle, ctx->sc,
			ctx->tgid, ctx->pid);
		goto bail;
	}

	ssr_handler = &cctx->domain->ssr_handler;
	if (!ssr_handler) {
		pr_err("Error: %s: failed to get ssr handler for domain %d\n",
			__func__, cctx->domain_id);
		goto bail;
	}

	ssr_handler->domain_id = cctx->domain_id;

	spin_unlock_irqrestore(&cctx->lock, flags);
	fastrpc_channel_ctx_put(cctx);

	/* Launch kernel worker thread to trigger ssr */
	ssr_handler->rphandle = rphandle;
	INIT_WORK(&ssr_handler->ssr_work, fastrpc_handle_ssr_request);
	schedule_work(&ssr_handler->ssr_work);
	return;

bail:
	spin_unlock_irqrestore(&cctx->lock, flags);
	fastrpc_channel_ctx_put(cctx);
}

/*
 * Retrieves legacy information for a given fastrpc_domain.
 *
 * This function maps the domain's type to its corresponding legacy name
 * and ID, based on the following table:
 *
 *   Domain Type       | Legacy Name              | Legacy ID
 *   ------------------|--------------------------|---------------
 *   SDSP              | domains[SDSP_DOMAIN_ID]  | SDSP_DOMAIN_ID
 *   LPASS             | domains[ADSP_DOMAIN_ID]  | ADSP_DOMAIN_ID
 *   NSP(instance 0)   | domains[CDSP_DOMAIN_ID]  | CDSP_DOMAIN_ID
 *   NSP(instance 1)   | domains[CDSP1_DOMAIN_ID] | CDSP1_DOMAIN_ID
 *
 * @param domain Pointer to the fastrpc_domain structure to retrieve
 * legacy info
 *
 * @return 0 on success, or a negative error code on failure
 *
 * Error codes:
 *   -EINVAL: Invalid domain type
 */
static int fastrpc_retrieve_legacy_info(struct fastrpc_domain *domain)
{
	int err = 0;

	switch (domain->type) {
	case FASTRPC_SDSP:
		domain->legacy_name = (char *)legacy_domains[SDSP_DOMAIN_ID];
		domain->legacy_id = SDSP_DOMAIN_ID;
		break;
	case FASTRPC_LPASS:
		domain->legacy_name = (char *)legacy_domains[ADSP_DOMAIN_ID];
		domain->legacy_id = ADSP_DOMAIN_ID;
		break;
	case FASTRPC_NSP:
		if (domain->instance_id == 0) {
			domain->legacy_name = (char *)legacy_domains[CDSP_DOMAIN_ID];
			domain->legacy_id = CDSP_DOMAIN_ID;
		} else if (domain->instance_id == 1) {
			domain->legacy_name = (char *)legacy_domains[CDSP1_DOMAIN_ID];
			domain->legacy_id = CDSP1_DOMAIN_ID;
		}
		break;
	default:
		err = -EINVAL;
		break;
	}
	return err;
}

/*
 * Configures the service locator for a given fastrpc channel context.
 *
 * This function sets up the service locator for the specified domain type,
 * registering the necessary services and clients.
 *
 * @param data Pointer to the fastrpc channel context structure
 *
 * @return 0 on success, or a negative error code on failure
 *
 * Supported domain types:
 *   - LPASS: Sets up service locators for audio, sensors, and OIS
 *            PDR services
 *   - SDSP: Sets up service locator for sensors PDR SLPI service
 *
 * Note: Other domain types are currently unsupported and will return 0
 *		 without configuring any services.
 */
static int fastrpc_configure_service_locator(
	struct fastrpc_channel_ctx *data)
{
	struct fastrpc_domain *domain = data->domain;
	int err = 0;

	switch (domain->type) {
	case FASTRPC_LPASS:
		err = fastrpc_setup_service_locator(
				data, AUDIO_PDR_SERVICE_LOCATION_CLIENT_NAME,
				AUDIO_PDR_ADSP_SERVICE_NAME, ADSP_AUDIOPD_NAME, 0);
		if (err)
			return err;

		err = fastrpc_setup_service_locator(
				data, SENSORS_PDR_ADSP_SERVICE_LOCATION_CLIENT_NAME,
				SENSORS_PDR_ADSP_SERVICE_NAME, ADSP_SENSORPD_NAME, 1);
		if (err)
			return err;

		err = fastrpc_setup_service_locator(
				data, OIS_PDR_ADSP_SERVICE_LOCATION_CLIENT_NAME,
				OIS_PDR_ADSP_SERVICE_NAME, ADSP_OISPD_NAME, 2);
		if (err)
			return err;
		break;

	case FASTRPC_SDSP:
		err = fastrpc_setup_service_locator(
				data, SENSORS_PDR_SLPI_SERVICE_LOCATION_CLIENT_NAME,
				SENSORS_PDR_SLPI_SERVICE_NAME, SLPI_SENSORPD_NAME, 0);
		if (err)
			return err;
		break;
	default:
		break;
	}
	return err;
}

/*
 * Configures device nodes for a given fastrpc channel context and device.
 *
 * This function registers device nodes for the specified channel
 *
 * For NSP domains:
 *   - Registers a single device node with domain name
 *   - If the domain has a legacy set to true,
 *     registers two additional device nodes with the legacy name
 *     one for secure and non-secure.
 *
 * For non-NSP domains:
 *   - Registers a single device node with the domain name
 *   - If the domain has a legacy name,
 *     registers an additional secure device node with the
 *     legacy name
 *
 * @param data Pointer to the fastrpc channel context structure
 * @param rdev Pointer to the device structure
 *
 * @return 0 on success, or a negative error code on failure
 */
static int fastrpc_configure_device_nodes(struct fastrpc_channel_ctx *data,
	struct device *rdev)
{
	struct fastrpc_domain *domain = data->domain;
	int err = 0;

	err = fastrpc_device_register(rdev, data, true, false,
				domain->name);
	if (err)
		return err;

	if (domain->legacy) {
		err = fastrpc_retrieve_legacy_info(domain);
		if (err)
			return err;

		/* Register a secure device with legacy name */
		err = fastrpc_device_register(rdev, data, true, true,
			domain->legacy_name);
		if (err)
			return err;

		/* For NSP register a non secure device with legacy name*/
		if (domain->type == FASTRPC_NSP) {
			err = fastrpc_device_register(rdev, data, false, true,
				domain->legacy_name);
			if (err)
				return err;
		}
	}

	return 0;
}

static void fastrpc_remove_device_nodes(struct fastrpc_channel_ctx *cctx)
{
	if (cctx->fdevice)
		misc_deregister(&cctx->fdevice->miscdev);

	if(cctx->legacy_fdevice)
		misc_deregister(&cctx->legacy_fdevice->miscdev);

	if(cctx->legacy_secure_fdevice)
		misc_deregister(&cctx->legacy_secure_fdevice->miscdev);

	return;
}
/*
 * Configures wake-up sources for FastRPC channel context.
 *
 * This function registers wake-up sources for various devices associated
 * with the FastRPC channel context, enabling them to trigger interrupts
 * or wake up the system from a low-power state when necessary.
 *
 * @param data Pointer to the FastRPC channel context structure.
 */
static void fastrpc_configure_wakeup_source(struct fastrpc_channel_ctx *data)
{
	mutex_lock(&data->wake_mutex);

	/* Register wake-up source for non-secure fdevice, if present. */
	if (data->fdevice) {
		fastrpc_register_wakeup_source(
			data->fdevice->miscdev.this_device,
			FASTRPC_NON_SECURE_WAKE_SOURCE_CLIENT_NAME,
			&data->wake_source);
	}

	mutex_unlock(&data->wake_mutex);
	return;
}

/*
 * Probe function for the fastrpc rpmsg device nodes.
 *
 * This function is called when fastrpc's rpmsg node for a remote channel
 * is probed. It configures all channel related data structures in the
 * driver.
 *
 * @param rpdev Pointer to the RPMSG device structure.
 * @return 0 on success, negative error code on failure.
 */
static int fastrpc_rpmsg_probe(struct rpmsg_device *rpdev)
{
	struct device *rdev = &rpdev->dev;
	struct fastrpc_channel_ctx *data;
	int i, err, vmcount;
	struct fastrpc_domain *domain = NULL;
	bool secure_dsp;
	unsigned int vmids[FASTRPC_MAX_VMIDS];

	dev_info(rdev, "%s started\n", __func__);

	if (of_reserved_mem_device_init_by_idx(rdev, rdev->of_node, 0))
		dev_info(rdev, "no reserved DMA memory for FASTRPC\n");

	vmcount = of_property_read_variable_u32_array(rdev->of_node,
				"qcom,vmids", &vmids[0], 0, FASTRPC_MAX_VMIDS);
	if (vmcount < 0)
		vmcount = 0;
	else if (!qcom_scm_is_available())
		return -EPROBE_DEFER;

	data = kzalloc(sizeof(*data), GFP_KERNEL);
	if (!data)
		return -ENOMEM;

	err = fastrpc_populate_domain_from_dt(rdev, &domain);
	if (err)
		return err;

	err = fastrpc_init_privileged_gids(rdev, "qcom,fastrpc-gids", &data->gidlist);
	if (err)
		dev_err(rdev, "Privileged gids init failed.\n");

	if (vmcount) {
		data->vmcount = vmcount;
		data->perms = BIT(QCOM_SCM_VMID_HLOS);
		for (i = 0; i < data->vmcount; i++) {
			data->vmperms[i].vmid = vmids[i];
			data->vmperms[i].perm = QCOM_SCM_PERM_RWX;
		}
	}

	atomic_set(&data->teardown, 0);
	secure_dsp = !(of_property_read_bool(rdev->of_node, "qcom,non-secure-domain"));
	data->secure = secure_dsp;

	of_property_read_u32(rdev->of_node, "qcom,rpc-latency-us",
			&data->qos_latency);

	fastrpc_lowest_capacity_corecount(rdev, data);
	if (data->lowest_capacity_core_count > 0 &&
	    of_property_read_bool(rdev->of_node, "qcom,single-core-latency-vote"))
		data->lowest_capacity_core_count = 1;

	/* Read dtsi property to determine where sid needs to be prepended to pa */
	err = of_property_read_u32(rdev->of_node, "qcom,dsp-iova-format",
			&data->iova_format);

	err = of_property_read_u32(rdev->of_node, "qcom,rootheap-buffer-size",
		&data->rootheap_buf_size);
	err = of_property_read_u32(rdev->of_node, "qcom,rootheap-buffer-count",
			&data->rootheap_buf_count);

	kref_init(&data->refcount);
	dev_set_drvdata(&rpdev->dev, data);
	rdev->dma_mask = &data->dma_mask;
	dma_set_mask_and_coherent(rdev, DMA_BIT_MASK(32));
	INIT_LIST_HEAD(&data->users);
	INIT_LIST_HEAD(&data->gmaps);
	INIT_LIST_HEAD(&data->rootheap_bufs.list);
	mutex_init(&data->wake_mutex);
	spin_lock_init(&data->lock);
	spin_lock_init(&(data->gmsg_log.tx_lock));
	spin_lock_init(&(data->gmsg_log.rx_lock));
	idr_init(&data->ctx_idr);
	ida_init(&data->tgid_frpc_ida);
	init_completion(&data->ssr_complete);
	init_completion(&data->rpmsg_remove_start);
	init_waitqueue_head(&data->ssr_wait_queue);
	data->domain_id = domain->id;
	data->max_sess_per_proc = FASTRPC_MAX_SESSIONS_PER_PROCESS;
	data->rpdev = rpdev;
	data->domain = domain;
	data->unsigned_support = false;
	data->cpuinfo_todsp = FASTRPC_CPUINFO_DEFAULT;

	err = of_platform_populate(rdev->of_node, NULL, NULL, rdev);
	if (err)
		goto populate_error;

	if (domain->type == FASTRPC_NSP) {
		data->unsigned_support = true;
		data->cpuinfo_todsp = FASTRPC_CPUINFO_EARLY_WAKEUP;
	}

	/* Configure device nodes for DSP */
	err = fastrpc_configure_device_nodes(data, rdev);
		if (err)
			goto fdev_error;

	/* Configure service locators for DSP */
	err = fastrpc_configure_service_locator(data);
		if (err)
			goto fdev_error;

	/* Configure_wakeup_sources */
	fastrpc_configure_wakeup_source(data);

	/* Create default user for channel */
	err = fastrpc_channel_default_user_create(data);
	if (err)
		goto fdev_error;

	/* Update domain status and global ctx */
	domain->status = DSP_STATUS_UP;
	domain->cctx = data;
	dev_info(rdev, "Opened rpmsg channel for %s", domain->name);
	return 0;

fdev_error:
	if (data->default_user)
		fastrpc_channel_default_user_delete(data);
	kfree(data);

populate_error:
	if (data->fdevice)
		misc_deregister(&data->fdevice->miscdev);

	return err;
}

/*
 * Remove function for the fastrpc RPMSG device driver.
 *
 * This function is called when the RPMSG device is removed or shut down.
 * It releases any resources allocated by the fastrpc driver and performs
 * any necessary cleanup.
 *
 * @param rpdev Pointer to the RPMSG device structure.
 */
static void fastrpc_rpmsg_remove(struct rpmsg_device *rpdev)
{
	struct fastrpc_channel_ctx *cctx = dev_get_drvdata(&rpdev->dev);
	struct fastrpc_domain *domain = cctx->domain;
	struct fastrpc_user *user, *n;
	unsigned long flags;
	int i = 0, err;
	struct list_head active_users_list;

	INIT_LIST_HEAD(&active_users_list);
	dev_info(cctx->dev, "%s started", __func__);

	/* No invocations past this point */
	spin_lock_irqsave(&cctx->lock, flags);
	atomic_set(&cctx->teardown, 1);
	domain->status = DSP_STATUS_DOWN;
	domain->cctx = NULL;
	cctx->staticpd_status = false;

	list_for_each_entry_safe(user, n, &cctx->users, user) {
		/*
		 * Ensure atomic_read(&user->state) == DSP_CREATE_COMPLETE before
		 * taking a reference which make sure the process state remains stable
		 * during teardown. All state change occurs under the same lock.
		 */
		if (atomic_read(&user->state) == DSP_CREATE_COMPLETE) {
			err = fastrpc_file_get(user);
			if (err) {
				dev_warn(cctx->dev, "Warning: %s: user-obj for fl (%pK) being released\n",
					__func__, user);
				continue;
			}

			/*
			 * Add active user-objects to a dedicated active_users_list to
			 * avoid access to the objects which are in the device release
			 * process. Utilize active_users_list for core dumps.
			 */
			list_add_tail(&user->active_user_ssr, &active_users_list);
		}
	}
	spin_unlock_irqrestore(&cctx->lock, flags);
	complete_all(&cctx->rpmsg_remove_start);
	frpc_coredump(cctx, &active_users_list);
	list_for_each_entry_safe(user, n, &active_users_list,
		active_user_ssr) {
		list_del(&user->active_user_ssr);
		fastrpc_file_put(user, true);
	}
	spin_lock_irqsave(&cctx->lock, flags);
	list_for_each_entry_safe(user, n, &cctx->users, user) {
		fastrpc_queue_pd_status(user, cctx->domain_id, FASTRPC_DSP_SSR,
			user->sessionid);
		fastrpc_notify_users(user);
	}
	spin_unlock_irqrestore(&cctx->lock, flags);
	fastrpc_remove_device_nodes(cctx);
	for (i = 0; i < FASTRPC_MAX_SPD; i++) {
		if (cctx->spd[i].pdrhandle)
			pdr_handle_release(cctx->spd[i].pdrhandle);
	}

	spin_lock_irqsave(&cctx->lock, flags);
	/*
	 * If there are other ongoing remote invocations, wait for them to
	 * complete before cleaning up the channel resources, to avoid UAF.
	 */
	while (cctx->invoke_cnt > 0) {
		spin_unlock_irqrestore(&cctx->lock, flags);
		wait_event_interruptible(cctx->ssr_wait_queue,
				cctx->invoke_cnt == 0);
		spin_lock_irqsave(&cctx->lock, flags);
	}
	spin_unlock_irqrestore(&cctx->lock, flags);

	/*
	 * As remote channel is down, corresponding SMMU devices will also
	 * be removed. So free all SMMU mappings of every process using this
	 * channel to avoid any UAF later.
	 */
	list_for_each_entry(user, &cctx->users, user) {
 		fastrpc_free_user(user);
 	}

	mutex_lock(&cctx->wake_mutex);
	if (cctx->wake_source) {
		wakeup_source_unregister(cctx->wake_source);
		cctx->wake_source = NULL;
	}
	if (cctx->wake_source_secure) {
		wakeup_source_unregister(cctx->wake_source_secure);
		cctx->wake_source_secure = NULL;
	}
	mutex_unlock(&cctx->wake_mutex);

	dev_info(cctx->dev, "Closing rpmsg channel for %s", cctx->domain->name);
	kfree(cctx->gidlist.gids);
	of_platform_depopulate(&rpdev->dev);
	fastrpc_mmap_remove_ssr(cctx, false);
	cctx->dev = NULL;
	cctx->rpdev = NULL;
	cctx->domain = NULL;
	// Wake up all process releases, if waiting for SSR to complete
	complete_all(&cctx->ssr_complete);
	fastrpc_channel_ctx_put(cctx);
}

static int fastrpc_rpmsg_callback(struct rpmsg_device *rpdev, void *data,
				  int len, void *priv, u32 addr)
{
	struct fastrpc_channel_ctx *cctx = dev_get_drvdata(&rpdev->dev);
	bool is_glink_wakeup = false;

#if IS_ENABLED(CONFIG_RPMSG_QCOM_GLINK_SMEM)
	is_glink_wakeup = qcom_glink_is_wakeup(true);
#endif

	return fastrpc_handle_rpc_response(cctx, data, len, is_glink_wakeup);
}

static const struct of_device_id fastrpc_rpmsg_of_match[] = {
	{ .compatible = "qcom,fastrpc" },
	{ },
};
MODULE_DEVICE_TABLE(of, fastrpc_rpmsg_of_match);

static struct rpmsg_driver fastrpc_driver = {
	.probe = fastrpc_rpmsg_probe,
	.remove = fastrpc_rpmsg_remove,
	.callback = fastrpc_rpmsg_callback,
	.drv = {
		.name = "qcom,fastrpc",
		.of_match_table = fastrpc_rpmsg_of_match,
	},
};

int fastrpc_transport_send(struct fastrpc_channel_ctx *cctx, void *rpc_msg, uint32_t rpc_msg_size) {
	int err = 0;

	if (atomic_read(&cctx->teardown))
		return -EPIPE;

	err = rpmsg_send(cctx->rpdev->ept, rpc_msg, rpc_msg_size);
	return err;
}

int fastrpc_transport_init(void) {
	int ret;

	ret = register_rpmsg_driver(&fastrpc_driver);
	if (ret < 0) {
		pr_err("fastrpc: failed to register rpmsg driver\n");
		return ret;
	}

	return 0;
}

void fastrpc_transport_deinit(void) {
	unregister_rpmsg_driver(&fastrpc_driver);
}

int fastrpc_reserve_dma_heap(struct fastrpc_tvm_dma_heap **tvm_dma_heap)
{
	return -EINVAL;
}

void fastrpc_unreserve_dma_heap(struct fastrpc_tvm_dma_heap *tvm_dma_heap)
{
	return;
}

inline int __fastrpc_dma_alloc(struct fastrpc_buf *buf)
{
	buf->virt = dma_alloc_coherent(buf->dev, buf->size,
				(dma_addr_t *)&buf->phys, GFP_KERNEL);

	return (buf->virt)? 0: -ENOMEM;
}

void __fastrpc_dma_buf_free(struct fastrpc_buf *buf)
{
	uint32_t sid_pos = (buf->smmucb ? buf->smmucb->sid_pos :
							DSP_DEFAULT_BUS_WIDTH);

	dma_free_coherent(buf->dev, buf->size, buf->virt,
		IOVA_TO_PHYSADDR(buf->phys, sid_pos));
}

bool fastrpc_is_device_crashing(struct fastrpc_channel_ctx *cctx)
{
	struct rproc *rphandle = NULL;
	bool crash = false;

	if (cctx->rpdev) {
		rphandle = rproc_get_by_child(&cctx->rpdev->dev);
		/* Mark device as crashing if SSR is disabled and rproc has crashed */
		if (rphandle && rphandle->recovery_disabled &&
			rphandle->state == RPROC_CRASHED)
			crash = true;
	}
	return crash;
}
