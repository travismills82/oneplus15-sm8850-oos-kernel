/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) 2025 Qualcomm Innovation Center, Inc. All rights reserved.
 */

#ifndef __QPTF_H__
#define __QPTF_H__

struct qptm;
struct powerzone_ops;

struct powerzone {
	struct qptm *qptm;
	struct device_node *qptm_np;
	struct powerzone_ops *ops;
	uint16_t ch_id;
	struct list_head node;
	bool attached;
	struct list_head qnode;
	struct mutex lock;
	void *devdata;
};

struct powerzone_ops {
	int (*set_enable)(struct powerzone *pz, bool enable);
	bool (*get_enable)(struct powerzone *pz);
	u64 (*get_energy)(struct powerzone *pz);
	u64 (*get_max_energy)(struct powerzone *pz);
	u64 (*get_power)(struct powerzone *pz);
	u64 (*get_max_power)(struct powerzone *pz);
};

#if IS_ENABLED(CONFIG_QCOM_POWER_TELEMETRY_FRAMEWORK)
struct powerzone *qptm_channel_register(struct device *dev, int channel_id,
			struct powerzone_ops *ops, void *data);
void qptm_channel_unregister(struct powerzone *pz);
void qptm_power_data_update(void);
struct qptm *get_qptm_by_powerzone(struct powerzone *pz);

/* Client API */
int qptm_available_node_count(void);
const char *qptm_get_node_name(struct qptm *qptm);
int qptm_get_node_id(struct qptm *qptm);
int qptm_for_each_node(int (*cb)(struct qptm *, void *), void *data);
int qptm_get_power_in_uw(struct qptm *qptm, u64 *power);
int qptm_get_energy_in_uj(struct qptm *qptm, u64 *energy);
void qptm_data_update_notifier_register(struct notifier_block *n);
void qptm_data_update_notifier_unregister(struct notifier_block *n);
#else
static inline struct powerzone *qptm_channel_register(struct device *dev, int channel_id,
			struct powerzone_ops *ops, void *data)
{
	return NULL;
}
static inline void qptm_channel_unregister(struct powerzone *pz) { };
static inline void qptm_power_data_update(void) { };
static inline struct qptm *get_qptm_by_powerzone(struct powerzone *pz)
{
	return NULL;
}
static inline int qptm_available_node_count(void)
{
	return -EINVAL;
}
static inline const char *qptm_get_node_name(struct qptm *qptm)
{
	return NULL;
}
static inline int qptm_get_node_id(struct qptm *qptm)
{
	return -EINVAL;
}
static inline int qptm_for_each_node(int (*cb)(struct qptm *, void *), void *data)
{
	return -EINVAL;
}
static inline int qptm_get_power_in_uw(struct qptm *qptm, u64 *power)
{
	return -EINVAL;
}
static inline int qptm_get_energy_in_uj(struct qptm *qptm, u64 *energy)
{
	return -EINVAL;
}
static inline void qptm_data_update_notifier_register(struct notifier_block *n) { };
static inline void qptm_data_update_notifier_unregister(struct notifier_block *n) { };
#endif

#endif /* __QPTF_H__ */
