/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (C) 2020-2025 Oplus. All rights reserved.
 */


#ifndef _OPLUS_SA_FAIR_H_
#define _OPLUS_SA_FAIR_H_

extern int global_sched_ddl_enabled;
extern struct ux_sched_cputopo ux_sched_cputopo;

bool should_ux_task_skip_cpu(struct task_struct *task, unsigned int dst_cpu);
bool should_ux_task_skip_eas(struct task_struct *p);
bool set_ux_task_to_prefer_cpu(struct task_struct *task, int *orig_target_cpu);
void oplus_replace_next_task_fair(struct rq *rq, struct task_struct **p, bool *repick);
void oplus_check_preempt_wakeup(struct rq *rq, struct task_struct *p, bool *preempt, bool *nopreempt);

int oplus_idle_cpu(int cpu);
inline int get_task_cls_for_scene(struct task_struct *task);

#ifdef CONFIG_OPLUS_ADD_CORE_CTRL_MASK
bool oplus_cpu_halted(unsigned int cpu);
void init_ux_halt_mask(struct cpumask *halt_mask);
#endif

/* register vender hook in kernel/sched/fair.c */
#if IS_ENABLED(CONFIG_OPLUS_FEATURE_VT_CAP)
void android_rvh_place_entity_handler(void *unused, struct cfs_rq *cfs_rq, struct sched_entity *se, int initial, u64 *vruntime);
#endif
void android_rvh_update_deadline_handler(void *unused, struct cfs_rq *cfs_rq, struct sched_entity *se, bool *skip_preempt);
void android_rvh_check_preempt_wakeup_fair_handler(void *unused, struct rq *rq, struct task_struct *p, bool *preempt, bool *nopreempt,
	int wake_flags, struct sched_entity *se, struct sched_entity *pse);
#ifndef CONFIG_OPLUS_SYSTEM_KERNEL_QCOM
void android_rvh_post_init_entity_util_avg_handler(void *unused, struct sched_entity *se);
#endif
void android_rvh_replace_next_task_fair_handler(void *unused, struct rq *rq, struct task_struct **p, bool *repick, struct task_struct *prev);
void android_rvh_can_migrate_task_handler(void *unused, struct task_struct *p, int dst_cpu, int *can_migrate);
void android_rvh_enqueue_entity_handler(void *unused, struct cfs_rq *cfs, struct sched_entity *se);
void android_rvh_dequeue_entity_handler(void *unused, struct cfs_rq *cfs, struct sched_entity *se);

#endif /* _OPLUS_SA_FAIR_H_ */
