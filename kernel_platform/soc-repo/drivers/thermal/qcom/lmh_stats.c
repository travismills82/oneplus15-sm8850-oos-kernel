// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 */

#include <linux/cpufreq.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/cpu.h>
#include <linux/types.h>
#include <linux/scmi_protocol.h>
#include <linux/sched/clock.h>

#define CREATE_TRACE_POINTS
#include "trace_lmh_stats.h"

struct lmh_cpufreq_thermal_domain {
	struct kobject lmh_kobj;
	struct notifier_block limit_notify_nb;
	struct cpufreq_policy *policy;

	u64 *freq_limit_time;
	unsigned int *freq_limit;
	unsigned int *freq_limit_cnt;
	unsigned int freq_cnt;
	unsigned int index;
	unsigned int max_state;
	unsigned long long last_time;
	int domain_id;
};

#define HZ_TO_KHZ 1000

static int lmh_get_cpufreq_index(struct lmh_cpufreq_thermal_domain *domain, unsigned long freq)
{
	int index;

	for (index = 0; index < domain->max_state; index++) {
		if (domain->freq_limit[index] == freq)
			return index;
	}

	return -1;
}

static int lmh_cpufreq_thermal_alloc_mem(struct device *dev, struct cpufreq_policy *policy,
						struct lmh_cpufreq_thermal_domain *domain)
{
	unsigned int count;
	unsigned int alloc_size;

	count = cpufreq_table_count_valid_entries(policy);
	if (!count)
		return -1;

	alloc_size = count * sizeof(u64) + count * sizeof(int) + count * sizeof(int);

	domain->freq_limit_time = devm_kzalloc(dev, alloc_size, GFP_KERNEL);
	if (!domain->freq_limit_time)
		return -ENOMEM;

	domain->freq_limit = (unsigned int *)(domain->freq_limit_time + count);
	domain->freq_limit_cnt = domain->freq_limit + count;
	domain->max_state = count;

	return 0;
}

static int lmh_scmi_limit_notify_cb(struct notifier_block *nb, unsigned long event, void *data)
{
	struct lmh_cpufreq_thermal_domain *domain = container_of(nb,
			struct lmh_cpufreq_thermal_domain, limit_notify_nb);
	struct scmi_perf_limits_report *limit_notify = data;
	unsigned int cpu = cpumask_first(domain->policy->related_cpus);
	int index;

	if (domain->last_time == 0) {
		domain->freq_limit[0] = limit_notify->range_max_freq / HZ_TO_KHZ;
		domain->last_time = local_clock();
		domain->freq_limit_cnt[0]++;
		domain->freq_cnt++;
		return 0;
	}

	domain->freq_limit_time[domain->index] += local_clock() - domain->last_time;
	domain->last_time = local_clock();
	index = lmh_get_cpufreq_index(domain, limit_notify->range_max_freq / HZ_TO_KHZ);
	if (index != -1) {
		domain->freq_limit_cnt[index]++;
		domain->index = index;
	} else {
		domain->freq_cnt++;
		domain->freq_limit[domain->freq_cnt - 1] = limit_notify->range_max_freq / HZ_TO_KHZ;
		domain->index = domain->freq_cnt - 1;
		domain->freq_limit_cnt[domain->freq_cnt - 1]++;
	}
	trace_thermal_hw_freq_limit(cpu, limit_notify->range_max_freq / HZ_TO_KHZ);

	return 0;
}

static int lmh_scmi_cpu_domain_id(struct device *cpu_dev)
{
	struct device_node *np = cpu_dev->of_node;
	struct of_phandle_args domain_id;
	int index;

	if (of_parse_phandle_with_args(np, "clocks", "#clock-cells", 0, &domain_id)) {

		index = of_property_match_string(np, "power-domain-names", "perf");
		if (index < 0)
			return -EINVAL;

		if (of_parse_phandle_with_args(np, "power-domains", "#power-domain-cells", index,
						&domain_id))
			return -EINVAL;
	}

	return domain_id.args[0];
}

static ssize_t dcvsh_freq_limit_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	struct lmh_cpufreq_thermal_domain *domain;

	domain = container_of(kobj, struct lmh_cpufreq_thermal_domain, lmh_kobj);
	return scnprintf(buf, PAGE_SIZE, "%u\n", domain->freq_limit[domain->index]);
}

static ssize_t dcvsh_freq_limit_residency_show(struct kobject *kobj, struct kobj_attribute *attr,
					char *buf)
{
	struct lmh_cpufreq_thermal_domain *domain;
	unsigned long long time;
	int i, len;

	domain = container_of(kobj, struct lmh_cpufreq_thermal_domain, lmh_kobj);

	len = scnprintf(buf, PAGE_SIZE, "%-8s\t %-10s\t %-10s\n", "Freq(KHz)", "Residency",
						"Count");
	for (i = 0; i < domain->freq_cnt; i++) {
		time = domain->freq_limit_time[i];
		if (i == domain->index)
			time += local_clock() - domain->last_time;

		len += scnprintf(buf + len, PAGE_SIZE, "%-8u\t %-10llu\t %-10u\n",
		domain->freq_limit[i], nsec_to_clock_t(time), domain->freq_limit_cnt[i]);
	}

	return len;
}

static struct kobj_attribute dcvsh_freq_limit =
	__ATTR_RO_MODE(dcvsh_freq_limit, 0444);

static struct kobj_attribute dcvsh_freq_limit_residency =
	__ATTR_RO_MODE(dcvsh_freq_limit_residency, 0444);

static struct attribute *lmh_attrs[] = {
	&dcvsh_freq_limit.attr,
	&dcvsh_freq_limit_residency.attr,
	NULL
};

ATTRIBUTE_GROUPS(lmh);

static const struct kobj_type lmh_ktype = {
	.sysfs_ops = &kobj_sysfs_ops,
	.default_groups = lmh_groups,
};

static int lmh_stats_driver_probe(struct platform_device *pdev)
{
	struct lmh_cpufreq_thermal_domain *domain;
	struct scmi_device *sdev = cpufreq_get_driver_data();
	struct cpufreq_policy *policy;
	struct device *cpu_dev;
	struct device *dev = &pdev->dev;
	struct device_node *dev_phandle;
	int ret = 0;
	int cpu = 0;

	domain = devm_kzalloc(dev, sizeof(*domain), GFP_KERNEL);
	if (!domain)
		return -ENOMEM;

	dev_phandle = of_parse_phandle(dev->of_node, "qcom,cpu", 0);
	for_each_possible_cpu(cpu) {
		cpu_dev = get_cpu_device(cpu);
		if (cpu_dev && cpu_dev->of_node == dev_phandle)
			break;
	}

	policy = cpufreq_cpu_get(cpu);
	if (!policy) {
		pr_err("Error getting policy for CPU%d\n", cpu);
		ret = -EPROBE_DEFER;
		goto err;
	}

	domain->policy = policy;

	if (lmh_cpufreq_thermal_alloc_mem(dev, domain->policy, domain) != 0) {
		pr_err("Alloc mem fail for pointer\n");
		ret = -ENOMEM;
		goto err;
	}

	kobject_init(&domain->lmh_kobj, &lmh_ktype);
	ret = kobject_add(&domain->lmh_kobj, kernel_kobj, "lmh_stats_%d", cpu);
	if (ret) {
		kobject_put(&domain->lmh_kobj);
		pr_err("Failed to create node in cpu%d device\n", cpu);
		goto err;
	}

	domain->domain_id = lmh_scmi_cpu_domain_id(cpu_dev);
	domain->limit_notify_nb.notifier_call = lmh_scmi_limit_notify_cb;
	ret = sdev->handle->notify_ops->event_notifier_register(sdev->handle, SCMI_PROTOCOL_PERF,
						SCMI_EVENT_PERFORMANCE_LIMITS_CHANGED,
						&domain->domain_id,
						&domain->limit_notify_nb);
	if (ret)
		pr_err("Failed to register for limits change notifier for domain %d\n",
						domain->domain_id);

	cpufreq_cpu_put(policy);
	platform_set_drvdata(pdev, domain);

	return 0;

err:
	if (domain->policy)
		cpufreq_cpu_put(domain->policy);

	return ret;
}

static void lmh_stats_driver_remove(struct platform_device *pdev)
{
	struct lmh_cpufreq_thermal_domain *domain = platform_get_drvdata(pdev);
	struct scmi_device *sdev = cpufreq_get_driver_data();

	sdev->handle->notify_ops->event_notifier_unregister(sdev->handle, SCMI_PROTOCOL_PERF,
						SCMI_EVENT_PERFORMANCE_LIMITS_CHANGED,
						&domain->domain_id,
						&domain->limit_notify_nb);
	kobject_put(&domain->lmh_kobj);
}

static const struct of_device_id lmh_stats_match[] = {
	{ .compatible = "qcom,lmh-stats" },
	{}
};

static struct platform_driver lmh_stats_driver = {
	.probe = lmh_stats_driver_probe,
	.remove = lmh_stats_driver_remove,
	.driver = {
		.name = "lmh-stats",
		.of_match_table = lmh_stats_match,
	},
};

static int __init lmh_stats_init(void)
{
	return platform_driver_register(&lmh_stats_driver);
}
module_init(lmh_stats_init);

static void __exit lmh_stats_exit(void)
{
	platform_driver_unregister(&lmh_stats_driver);
}
module_exit(lmh_stats_exit);

MODULE_DESCRIPTION("QCOM LMH Stats Driver");
MODULE_LICENSE("GPL");
