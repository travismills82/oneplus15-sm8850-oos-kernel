// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2020 Oplus. All rights reserved.
 */

#define pr_fmt(fmt) "geas: " fmt

#include <trace/hooks/sched.h>
#include <trace/hooks/cpufreq.h>

#include <linux/syscore_ops.h>
#include <linux/init.h>
#include <linux/notifier.h>
#include <linux/cpu.h>
#include <linux/moduleparam.h>
#include <linux/cpumask.h>
#include <linux/cpufreq.h>
#include <linux/slab.h>
#include <linux/sched.h>
#include <linux/tick.h>
#include <trace/events/power.h>
#include <linux/sysfs.h>
#include <linux/kmemleak.h>
#include <linux/module.h>
#include <linux/input.h>
#include <linux/kthread.h>
#include <linux/spinlock.h>
#include <linux/circ_buf.h>
#include <linux/ktime.h>
#include <linux/perf_event.h>
#include <linux/errno.h>
#include <linux/topology.h>
#include <linux/platform_device.h>
#include <linux/pm_opp.h>
#include <linux/energy_model.h>
#include "../../../kernel/sched/sched.h"
#include "geas_cpu_common.h"
#include "geas_cpu_sched.h"
#include "sharebuck.h"
#define CREATE_TRACE_POINTS
#include "trace_geas.h"

#include "geas_cpu_sysctrl.h"
#include "geas_dyn_em.h"
#include "pipline_eas.h"
#include "geas_cpu_para.h"

struct geas_sched_cluster_info *geas_sched_info[GEAS_SCHED_CLUSTER_NR];
struct geas_sched_rq_info *geas_rq_info;
int geas_targetload[GEAS_SCHED_CLUSTER_NR] = {85, 90, 90, 90};
u8 geas_cpu_num = 0;
static ktime_t geas_ktime_last;
static bool geas_sched_ktime_suspended;
int numb_of_clusters;
int geas_cpu_inited = true;
int geas_cpu_enable = false;
int geas_cpu_dbg_enabled = 0;

struct list_head cluster_head;
#define for_each_sched_cluster(cluster) \
	list_for_each_entry_rcu(cluster, &cluster_head, list)

/*********************************
 * frame group common function
 *********************************/
static inline void move_list(struct list_head *dst, struct list_head *src)
{
	struct list_head *first, *last;

	first = src->next;
	last = src->prev;

	first->prev = dst;
	dst->prev = last;
	last->next = dst;

	/* Ensure list sanity before making the head visible to all CPUs. */
	smp_mb();
	dst->next = first;
}

static void get_possible_siblings(int cpuid, struct cpumask *cluster_cpus)
{
	int cpu;
	struct cpu_topology *cpu_topo, *cpuid_topo = &cpu_topology[cpuid];

	if (cpuid_topo->cluster_id == -1)
		return;

	for_each_possible_cpu(cpu) {
		cpu_topo = &cpu_topology[cpu];

		if (cpuid_topo->cluster_id != cpu_topo->cluster_id)
			continue;
		cpumask_set_cpu(cpu, cluster_cpus);
	}
}

static void insert_cluster(struct geas_sched_cluster_info *cluster, struct list_head *head)
{
	struct geas_sched_cluster_info *tmp;
	struct list_head *iter = head;

	list_for_each_entry(tmp, head, list) {
		if (arch_scale_cpu_capacity(cpumask_first(&cluster->cpus))
			< arch_scale_cpu_capacity(cpumask_first(&tmp->cpus)))
			break;
		iter = &tmp->list;
	}

	list_add(&cluster->list, iter);
}

static void cleanup_clusters(struct list_head *head)
{
	struct geas_sched_cluster_info *cluster, *tmp;

	list_for_each_entry_safe(cluster, tmp, head, list) {
		list_del(&cluster->list);
		numb_of_clusters--;
		kfree(cluster);
	}
}

static struct geas_sched_cluster_info *alloc_new_cluster(const struct cpumask *cpus)
{
	struct geas_sched_cluster_info *cluster = NULL;

	cluster = kzalloc(sizeof(struct geas_sched_cluster_info), GFP_ATOMIC);
	if (!cluster) {
		pr_err("alloc_new_cluster failed return err\n");
		return NULL;
	}

	INIT_LIST_HEAD(&cluster->list);
	cluster->cpus = *cpus;
	cluster->cpu_num = cpumask_weight(cpus);
	return cluster;
}

static inline int add_cluster(const struct cpumask *cpus, struct list_head *head)
{
	unsigned long capacity = 0, insert_capacity = 0;
	struct geas_sched_cluster_info *cluster = NULL;

	capacity = arch_scale_cpu_capacity(cpumask_first(cpus));
	/* If arch_capacity is no different between mid cluster and max cluster,
	 * just combind them
	 */
	list_for_each_entry_rcu(cluster, head, list) {
		insert_capacity = arch_scale_cpu_capacity(cpumask_first(&cluster->cpus));
		if (capacity == insert_capacity) {
			pr_info("insert cluster=%*pbl is same as exist cluster=%*pbl\n",
				cpumask_pr_args(cpus), cpumask_pr_args(&cluster->cpus));
			cpumask_or(&cluster->cpus, &cluster->cpus, cpus);
			return 0;
		}
	}

	cluster = alloc_new_cluster(cpus);
	if (!cluster) {
		pr_err("alloc_new_cluster failed return err\n");
		return -ENOMEM;
	}
	insert_cluster(cluster, head);

	geas_sched_info[numb_of_clusters] = cluster;

	numb_of_clusters++;
	return 0;
}

static inline void assign_cluster_ids(struct list_head *head)
{
	struct geas_sched_cluster_info *cluster;
	int pos = 0;

	list_for_each_entry(cluster, head, list)
		cluster->id = pos++;
}

static bool build_clusters(void)
{
	struct cpumask cpus = *cpu_possible_mask;
	struct cpumask cluster_cpus;
	struct list_head new_head;
	int i, ret;

	INIT_LIST_HEAD(&cluster_head);
	INIT_LIST_HEAD(&new_head);

	/* If this work failed, our cluster_head can still used with only one cluster struct */
	for_each_cpu(i, &cpus) {
		cpumask_clear(&cluster_cpus);
		get_possible_siblings(i, &cluster_cpus);
		if (cpumask_empty(&cluster_cpus)) {
			pr_err("cluster_cpus empty err cpu %d\n", i);
			cleanup_clusters(&new_head);
			return false;
		}
		cpumask_andnot(&cpus, &cpus, &cluster_cpus);
		ret = add_cluster(&cluster_cpus, &new_head);
		if (ret)
			return false;
	}

	assign_cluster_ids(&new_head);
	move_list(&cluster_head, &new_head);
	return true;
}

int get_cpufreq_dsufreq_map(int cpu,
		struct cpufreq_dsufreq_map **cpufreq_dsufreq_map)
{
	*cpufreq_dsufreq_map = cpu0freq_dsufreq_map;
	return sizeof(cpu0freq_dsufreq_map) / sizeof(struct cpufreq_dsufreq_map);
}

int get_geas_opp_table(int cpu,
		struct geas_opp_table **freq_table_inp,
		struct em_perf_domain *pd, u32 *coef)
{
	int idx = 0, max_opp_ct, ret;
	struct device *dev;
	struct geas_opp_table *freq_table = NULL;
	struct dev_pm_opp *opp;
	struct device_node *np;
	struct em_perf_table *em_table = pd->em_table;

	dev = get_cpu_device(cpu);
	if (!dev) {
		pr_err("cpu_dev not found @ cpu %d\n", cpu);
		return -EINVAL;
	}
	max_opp_ct = dev_pm_opp_get_opp_count(dev);
	if (max_opp_ct <= 0)
		return max_opp_ct;

	np = of_node_get(dev->of_node);
	if (!np) {
		pr_err("invalid device node for cpu %d", cpu);
		return -EINVAL;
	}

	if (max_opp_ct != pd->nr_perf_states) {
		pr_err("invalid opp count @ %d, nr_perf_states= %d\n",
			max_opp_ct, pd->nr_perf_states);
		return -EINVAL;
	}
	ret = of_property_read_u32(np, "dynamic-power-coefficient", coef);
	of_node_put(np);
	if (ret || !coef) {
		pr_err("Couldn't find proper 'dynamic-power-coefficient' in DT\n");
		return -EINVAL;
	}

	freq_table = kcalloc(max_opp_ct, sizeof(*freq_table), GFP_KERNEL);
	if (!freq_table)
		return -ENOMEM;

	for (; idx < max_opp_ct; idx++) {
		unsigned long freq = em_table->state[idx].frequency * 1000;
		opp = dev_pm_opp_find_freq_ceil(dev, &freq);
		if (IS_ERR(opp)) {
			pr_err("Error get freq for cpu %d freq->%lu @ index%d\n", cpu, freq, idx);
			goto fetch_err_exit;
		}
		freq_table[idx].frequency_khz = em_table->state[idx].frequency;
		freq_table[idx].volt_mv = dev_pm_opp_get_voltage(opp) / 1000; /* mV */
		pr_debug("cpu%d %d: freq:%lu Khz volt:%lu mv\n", cpu, idx,
				freq_table[idx].frequency_khz,
				freq_table[idx].volt_mv);

		dev_pm_opp_put(opp);
	}
	*freq_table_inp = freq_table;

	return max_opp_ct;
fetch_err_exit:
	kfree(freq_table);
	return -EINVAL;
}

int geas_sched_get_numb_of_clusters(void)
{
	return numb_of_clusters;
}

struct geas_sched_rq_info *geas_sched_rq_info_byid(int id)
{
	return &geas_rq_info[id];
}

struct geas_sched_cluster_info *geas_sched_cluster_info_byid(int id)
{
	return geas_sched_info[id];
}
struct geas_sched_cluster_info *geas_sched_cpu_cluster_info(int cpu)
{
	return geas_rq_info[cpu].cluster;
}

int geas_sched_get_pd_first_cpu(struct em_perf_domain *pd)
{
	if (unlikely(pd == NULL))
		return -1;
	return cpumask_first(to_cpumask(pd->cpus));
}


u64 geas_sched_ktime_get_ns(void)
{
	if (unlikely(geas_sched_ktime_suspended))
		return ktime_to_ns(geas_ktime_last);
	return ktime_get_ns();
}

struct geas_sched_cluster_info *geas_sched_get_pd_cluster_info(
								struct em_perf_domain *pd)
{
	int cpu;
	if (unlikely(pd == NULL))
		return NULL;
	cpu = geas_sched_get_pd_first_cpu(pd);
	return geas_sched_cpu_cluster_info(cpu);
}

unsigned long get_pd_scale_cpu(struct perf_domain *pd)
{
	if (unlikely(pd == NULL)) {
		return 1024;
	}
	return arch_scale_cpu_capacity(geas_sched_get_pd_first_cpu(pd->em_pd));
}

int geas_sched_get_pd_fmax(struct em_perf_domain *pd)
{
	if (unlikely(pd == NULL))
		return 0;

	return pd->em_table->state[pd->nr_perf_states - 1].frequency;
}

bool geas_sched_dynamic_energy_model_valid(struct geas_sched_cluster_info *cl)
{
	return cl->dem.valid;
}

struct geas_util_to_data *get_cluster_util_to_data_array(struct geas_sched_cluster_info *cl, int em_type)
{
	return cl->dem.ems[em_type].util_to_data;
}
struct geas_util_to_data *get_cluster_util_to_data_by_emtype(struct geas_sched_cluster_info *cl, u8 em_type)
{
	return cl->dem.ems[em_type].util_to_data;
}
u32 get_cluster_util_to_raito_by_emtype(struct geas_sched_cluster_info *cl, u8 em_type, u16 util)
{
	struct geas_util_to_data *util_to_data = get_cluster_util_to_data_by_emtype(cl, em_type);
	util = min(util, 1024);
	return util_to_data[util].freq;
}

u16 get_cluster_util_to_cost(struct geas_sched_cluster_info *cl, unsigned long util, int em_type)
{
	struct geas_util_to_data *array = get_cluster_util_to_data_array(cl, em_type);
	if (util >= 1024)
		util = 1023;
	return array[util].cost;
}

u16 geas_sched_get_cpu_util_to_cost(int cpu, unsigned long util, int em_type)
{
	return get_cluster_util_to_cost(geas_sched_cpu_cluster_info(cpu), util, em_type);
}

struct geas_util_to_data *geas_sched_get_cluster_util_to_data_array(struct geas_sched_cluster_info *cl, int em_type)
{
	return cl->dem.ems[em_type].util_to_data;
}

struct geas_util_to_data *geas_sched_get_cluster_util_to_data(struct geas_sched_cluster_info *cl, unsigned int util, int em_type)
{
	if (util >= 1024)
		util = 1023;
	return &geas_sched_get_cluster_util_to_data_array(cl, em_type)[util];
}

struct geas_util_to_data *geas_sched_get_cpu_util_to_data(int cpu, unsigned int util, int em_type)
{
	struct geas_sched_cluster_info *cl = geas_sched_cpu_cluster_info(cpu);

	return geas_sched_get_cluster_util_to_data(cl, util, em_type);
}

struct shb_config_data **geas_sched_get_cluster_shb_config_array(struct geas_sched_cluster_info *cl, int em_type)
{
	return cl->dem.ems[em_type].shb_config_data;
}

struct shb_config_data *geas_sched_get_cluster_shb_config(struct geas_sched_cluster_info *cl,
			unsigned idx_x, unsigned int idx_y, int em_type)
{
	struct shb_config_data **array = geas_sched_get_cluster_shb_config_array(cl, em_type);
	return &array[idx_x][idx_y];
}

void update_util_to_target_load(void)
{ }

int update_targetload_ctrl_values(const struct ctl_table *table, int write,
						void __user *buffer, size_t *lenp, loff_t * ppos)
{
	int ret;
	static DEFINE_MUTEX(mutex);

	mutex_lock(&mutex);
	ret = proc_dointvec(table, write, buffer, lenp, ppos);
	if (write) {
		update_util_to_target_load();
	}
	mutex_unlock(&mutex);
	return ret;
}

static void geas_sched_resume(void)
{
	geas_sched_ktime_suspended = false;
}

static int geas_sched_suspend(void)
{
	geas_ktime_last = ktime_get();
	geas_sched_ktime_suspended = true;
	return 0;
}

static struct syscore_ops geas_syscore_ops = {
	.resume		= geas_sched_resume,
	.suspend	= geas_sched_suspend
};

/* depend on walt */
void  geas_rq_info_init(int id, struct geas_sched_cluster_info *info)
{
	geas_rq_info[id].cluster = info;
	geas_rq_info[id].cpu = id;
}

int geas_update_cluster_info(struct geas_sched_cluster_info *info)
{
	struct cpufreq_policy *policy;
	int first_cpu;
	int i;

	first_cpu = cpumask_first(&info->cpus);
	info->shbuck_cluster = NULL;
	info->shb_type = 0;
	info->policy_id = first_cpu;
	for_each_cpu(i, &info->cpus) {
		geas_rq_info_init(i, info);
	}

	policy = cpufreq_cpu_get_raw(first_cpu);
	/*
	 * walt_update_cluster_topology() must be called AFTER policies
	 * for all cpus are initialized. If not, simply BUG().
	 */
	if (policy) {
		info->max_possible_freq = policy->cpuinfo.max_freq;
		info->max_freq = policy->max;
	} else {
		pr_err("geas_update_cluster_info failed get policy id %d\n", info->id);
		return -1;
	}

	info->cur_freq = 1;
	info->cur_cap = 1;
	info->scale_cpu = arch_scale_cpu_capacity(first_cpu);
	info->dem.cur_enable = -1;
	info->dem.valid = 0;
	info->dem.cluster = info;
	info->dem.numb_of_em = 0;
	info->scaling_min_cap = 0;
	info->scaling_max_cap = 1024;

	pr_err("creating cluster info for %d", info->id);
	return 0;
}

int geas_cluster_info_init(void)
{
	int nr_cpu = cpumask_weight(cpu_possible_mask);
	struct geas_sched_cluster_info *cluster = NULL;
	int ret;

	if (build_clusters() == false)
		return -1;
	geas_rq_info = kcalloc(nr_cpu, sizeof(struct geas_sched_rq_info), GFP_KERNEL);
	if (!geas_rq_info) {
		pr_err("%s alloc mem size %lu failed \n", __func__, sizeof(struct geas_sched_rq_info));
		return -ENOMEM;
	}

	for_each_sched_cluster(cluster) {
		ret = geas_update_cluster_info(cluster);
		if (ret < 0)
			return -1;

		pr_info("%s %d id %d policy_id %u max_freq %u max_possible_freq %u\n",
			__func__, __LINE__, cluster->id, cluster->policy_id,
			cluster->max_freq, cluster->max_possible_freq);
	}
	return 0;
}

int geas_sched_core_init(void)
{
	int ret;
	ret = geas_cluster_info_init();
	if (ret < 0)
		return ret;

	pr_info("geas_sched_core_init success");
	return 0;
}

extern int (*geas_update_ofb_base_info_ptr)(struct multitask_ofb_base_info *base_info);
extern int (*geas_update_load_info_ptr)(struct multitask_load_info *load_info);
int geas_cpu_component_init(void)
{
	int ret, cpu;
	geas_cpu_inited = false;
	ret = geas_sched_core_init();
	if (ret != 0)
		return ret;

	for_each_possible_cpu(cpu)
		geas_cpu_num++;

	ret = geas_dyn_em_init();
	if (ret != 0)
		return ret;

	register_syscore_ops(&geas_syscore_ops);

	ret = sharebuck_init();
	if (ret != 0)
		return ret;

	ret = task_manager_init();
	if (ret != 0)
		return ret;

	ret = geas_sysctrl_init();
	if (ret != 0)
		return ret;

	/* Ensure all is init before geas_cpu_inited is true */
	smp_mb();
	geas_cpu_inited = true;
	geas_cpu_enable = true;
	pr_err("geas_sched_component_init success cluster %d geas_cpu_num %d\n", numb_of_clusters, geas_cpu_num);

	geas_update_ofb_base_info_ptr = geas_update_ofb_base_info;
	geas_update_load_info_ptr = geas_update_load_info;

	return 0;
}

static int geas_module_init(void)
{
	return geas_cpu_component_init();
}

static void __exit geas_module_exit(void)
{
}

module_exit(geas_module_exit);

late_initcall(geas_module_init);
MODULE_LICENSE("GPL v2");
