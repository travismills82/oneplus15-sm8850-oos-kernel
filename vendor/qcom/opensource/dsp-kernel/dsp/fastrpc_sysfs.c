// Copyright (c) 2025 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: GPL-2.0-only

#include "../include/uapi/misc/fastrpc.h"
#include <linux/device.h>
#include "fastrpc_shared.h"
#include <linux/platform_device.h>
#include <linux/types.h>


/*
 * Callback function whenever user app reads
 * /sys/kernel/fastrpc/<dsp>/name
 *
 * Returns name of channel
 */
static ssize_t domain_name_show(struct kobject *kobj,
	struct kobj_attribute *attr, char *buf)
{
	struct fastrpc_domain *domain = container_of(kobj,
		struct fastrpc_domain, kobj_sysfs);

	return sysfs_emit(buf, "%s\n", domain->name);
}

/*
 * Callback function whenever user app reads
 * /sys/kernel/fastrpc/<dsp>/id
 *
 * Returns logical domain id of channel
 */
static ssize_t domain_id_show(struct kobject *kobj,
	struct kobj_attribute *attr, char *buf)
{
	struct fastrpc_domain *domain = container_of(kobj,
		struct fastrpc_domain, kobj_sysfs);

	return sysfs_emit(buf, "%d\n", domain->id);
}

/*
 * Callback function whenever user app reads
 * /sys/kernel/fastrpc/<dsp>/instance_id
 *
 * Returns logical instance id of channel
 */
static ssize_t instance_id_show(struct kobject *kobj,
	struct kobj_attribute *attr, char *buf)
{
	struct fastrpc_domain *domain = container_of(kobj,
		struct fastrpc_domain, kobj_sysfs);

	return sysfs_emit(buf, "%d\n", domain->instance_id);
}

/*
 * Callback function whenever user app reads
 * /sys/kernel/fastrpc/<dsp>/status
 *
 * Returns status of remote channel (if its up or down)
 */
static ssize_t domain_status_show(struct kobject *kobj,
	struct kobj_attribute *attr, char *buf)
{
	struct fastrpc_domain *domain = container_of(kobj,
		struct fastrpc_domain, kobj_sysfs);

	return sysfs_emit(buf, "%d\n", domain->status);
}

/*
 * Callback function whenever user app reads
 * /sys/kernel/fastrpc/<dsp>/type
 *
 * Returns type of remote channel
 */
static ssize_t domain_type_show(struct kobject *kobj,
	struct kobj_attribute *attr, char *buf)
{
	struct fastrpc_domain *domain = container_of(kobj,
		struct fastrpc_domain, kobj_sysfs);

	return sysfs_emit(buf, "%d\n", domain->type);
}

/*
 * Callback function whenever user app reads
 * /sys/kernel/fastrpc/<dsp>/legacy_name
 *
 * Returns legacy_name of remote channel
 */
static ssize_t domain_legacy_name_show(struct kobject *kobj,
	struct kobj_attribute *attr, char *buf)
{
	struct fastrpc_domain *domain = container_of(kobj,
		struct fastrpc_domain, kobj_sysfs);

	return sysfs_emit(buf, "%s\n", domain->legacy_name);
}

/*
 * Callback function whenever user app reads
 * /sys/kernel/fastrpc/<dsp>/legacy_id
 *
 * Returns legacy_id of remote channel
 */
static ssize_t domain_legacy_id_show(struct kobject *kobj,
	struct kobj_attribute *attr, char *buf)
{
	struct fastrpc_domain *domain = container_of(kobj,
		struct fastrpc_domain, kobj_sysfs);

	return sysfs_emit(buf, "%d\n", domain->legacy_id);
}

/* Parent sysfs kobject for "/sys/kernel/fastrpc" */
static struct kset *fastrpc_kset = NULL;

/* Attributes for each channel */
static struct kobj_attribute name_attr = __ATTR(name, 0444,
	domain_name_show, NULL);
static struct kobj_attribute id_attr = __ATTR(domain_id, 0444,
	domain_id_show, NULL);
static struct kobj_attribute instance_id_attr = __ATTR(instance_id,
	0444, instance_id_show, NULL);
static struct kobj_attribute status_attr = __ATTR(status, 0444,
	domain_status_show, NULL);
static struct kobj_attribute type_attr = __ATTR(type, 0444,
	domain_type_show, NULL);
static struct kobj_attribute legacy_name_attr = __ATTR(legacy_name, 0444,
	domain_legacy_name_show, NULL);
static struct kobj_attribute legacy_id_attr = __ATTR(legacy_id, 0444,
	domain_legacy_id_show, NULL);

/* Define default attribute list for a domain */
static struct attribute *dsp_attrs[] = {
	&name_attr.attr,
	&id_attr.attr,
	&status_attr.attr,
	&type_attr.attr,
	&instance_id_attr.attr,
	NULL, /* Null terminator for the attribute array */
};

/* Attribute group for fastrpc domain */
ATTRIBUTE_GROUPS(dsp);

/*
 * One dsp of each type will also be marked as the legacy dsp for backward
 * compatibility (eg: NSP type as CDSP, LPASS / HPASS type as ADSP).
 * Attribute list for these dsp's also needs to contain the legacy name and
 * domain id, in addition to the other default attributes. Define the
 * attribute list for such devices.
 */
static struct attribute *dsp_with_legacy_attrs[] = {
	&name_attr.attr,
	&id_attr.attr,
	&status_attr.attr,
	&type_attr.attr,
	&instance_id_attr.attr,
	&legacy_name_attr.attr,
	&legacy_id_attr.attr,
	NULL, /* Null terminator for the attribute array */
};

/* Attribute group for fastrpc domain with legacy domain */
ATTRIBUTE_GROUPS(dsp_with_legacy);

/* Sysfs kobject type for fastrpc domain */
static const struct kobj_type frpc_kobj_type = {
	.sysfs_ops = &kobj_sysfs_ops,
	.default_groups = dsp_groups,
};

/* Sysfs kobject type for fastrpc domain with legacy attributes */
static const struct kobj_type frpc_legacy_kobj_type = {
	.sysfs_ops = &kobj_sysfs_ops,
	.default_groups = dsp_with_legacy_groups,
};

/*
 * Creates a sysfs interface for the given fastrpc channel context.
 *
 * @param domain Pointer to domain struct to create the sysfs interface for.
 *
 * @return 0 on success, a negative error code on failure.
 */
int fastrpc_sysfs_domain_create(struct fastrpc_domain *domain)
{
	int err = 0;
	struct kobject *obj = &domain->kobj_sysfs;
	const char *name = domain->name;

	if (!fastrpc_kset) {
		pr_err("%s: parent kobj not initialized for %s\n",
			__func__, name);
		return -EINVAL;
	}

	/* Mark that new kobj is part of parent fastrpc kset */
	obj->kset = fastrpc_kset;

	/*
	 * Create attribute files under each channel directory.
	 * Eg:	/sys/kernel/fastrpc/<dsp>/name
	 * 		/sys/kernel/fastrpc/<dsp>/domain_id
	 * 		/sys/kernel/fastrpc/<dsp>/status
	 */
	if (domain->legacy)
		/*
		 * If the domain is marked as legacy, intialize obj with
		 * frpc_legacy_kobj_type which include legacy attr
		 * E.g. /sys/kernel/fastrpc/<dsp>/legacy_name
		 *      /sys/kernel/fastrpc/<dsp>/legacy_id
		 */
		err = kobject_init_and_add(obj, &frpc_legacy_kobj_type, NULL, name);
	else
		/*
		 * Intilaize the obj with frpc_kobj_type
		 */
		err = kobject_init_and_add(obj, &frpc_kobj_type, NULL, name);

	if (err) {
		pr_err("%s: failed to create sysfs group for %s\n",
			__func__, name);
		kobject_put(obj);
		return err;
	}
	/*
	 * Send an uevent to notify usersapce that a new domain node is created.
	 * This will trigger selinux to apply the right access policy for sysfs
	 * files.
	 */
	kobject_uevent(obj, KOBJ_ADD);
	pr_info("%s: created sysfs group for %s\n",
		__func__, name);
	return 0;
}

/*
 * Removes sysfs directory of a channel.
 *
 * This function is responsible for deleting the sysfs directory
 * associated with a specific channel context.
 * It takes a pointer to the channel context as an argument.
 *
 * @param domain Pointer to the domain to remove sysfs directory
 */
void fastrpc_sysfs_domain_remove(struct fastrpc_domain *domain)
{
	struct kobject *obj = &domain->kobj_sysfs;

	/* Remove the domain sysfs node */
	if (obj->state_initialized)
		kobject_put(obj);
	pr_info("%s: removed sysfs group for %s\n",
		__func__, domain->name);
}

/*
 * fastrpc_sysfs_register_kset - Register the fastrpc kset
 *
 * Creates a kset to create a parent directory "fastrpc" under /sys/kernel.
 *
 * Return: 0 on success, -ENOMEM on failure
 */
int fastrpc_sysfs_register_kset(void)
{
	/* Create kset to create a parent directory fastrpc under /sys/kernel */
	fastrpc_kset = kset_create_and_add(FASTRPC_DEVICE_NAME, NULL, kernel_kobj);
	if (!fastrpc_kset) {
		pr_err("Error: %s: failed to create parent kobj\n", __func__);
		return -ENOMEM;
	}

	return 0;
}

/*
 * fastrpc_sysfs_deregister_kset - Deregister the fastrpc kset from sysfs
 *
 * This function deregisters the fastrpc kset from the sysfs file system.
 *
 * @return: None
 */
void fastrpc_sysfs_deregister_kset(void)
{
	/* Delete the parent kset */
	if (fastrpc_kset)
		kset_unregister(fastrpc_kset);
}