// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) 2019-2021, The Linux Foundation. All rights reserved.
 * Copyright (c) 2022-2023 Qualcomm Innovation Center, Inc. All rights reserved.
 */

#include <dt-bindings/interconnect/qcom,icc.h>
#include <linux/interconnect.h>
#include <linux/of.h>
#include <soc/qcom/of_common.h>

#include "kgsl_bus.h"
#include "kgsl_device.h"
#include "kgsl_trace.h"

#if IS_ENABLED(CONFIG_OPLUS_FEATURE_GEAS_GPU)

#define KBPS_TO_MHZ(kbps, w) (mult_frac(kbps, 1024, w * 1000000ULL))

#include "adreno.h"
#include "adreno_hfi.h"

struct gpu_params {
	int imin;
	int imax;
	int amin;
	int amax;
	int ascale;
	int fmin;
	int fmax;
	int resv[2];
};

static struct gpu_params gpu_data;
struct kgsl_device *kgsl_device_ptr;
int geas_update_gpu_params(struct gpu_params *data)
{
	struct adreno_device *adreno_dev = NULL;
	struct adreno_hwsched *hwsched = NULL;
	struct kgsl_pwrctrl *pwr = NULL;
	int update_imin = 0, update_imax = 0, update_amin = 0, update_amax = 0;

	memcpy(&gpu_data, data, sizeof(struct gpu_params));

	if (kgsl_device_ptr && !kgsl_device_ptr->host_based_dcvs) {
		adreno_dev = ADRENO_DEVICE(kgsl_device_ptr);
		hwsched = &(adreno_dev->hwsched);
		pwr = &kgsl_device_ptr->pwrctrl;
		if ((data->imin == -1) || ((data->imin > 0) && (data->imin < pwr->ddr_table_count)))
			update_imin = 1;
		if ((data->imax == -1) || ((data->imax > 0) && (data->imax < pwr->ddr_table_count)))
			update_imax = 1;
		if ((data->amin > 0) || (data->amin == -1))
			update_amin = 1;
		if ((data->amax > 0) || (data->amax == -1))
			update_amax = 1;

		pr_err("%s, update_imin = %d, update_imax = %d, update_amin = %d, update_amax = %d", __func__, update_imin, update_imax, update_amin, update_amax);

		kgsl_mutex_lock(&kgsl_device_ptr->mutex);
		if (update_imin) {
			if (data->imin > 0) {
				u64 kbps = (u64)pwr->ddr_table[data->imin];
				hwsched->sysfs_dcvs_tunables[GPU_TUNING_KEY_BUS_MIN_FREQUENCY].value = KBPS_TO_MHZ(kbps, 4) + 1;
				pr_err("%s, data->imin = %d, freq = %lld", __func__, data->imin, (u64)KBPS_TO_MHZ(kbps, 4) + 1);
			} else {
				hwsched->sysfs_dcvs_tunables[GPU_TUNING_KEY_BUS_MIN_FREQUENCY].value = -1;
			}
			kgsl_device_ptr->ftbl->gmu_based_dcvs_pwr_ops(kgsl_device_ptr,  GPU_TUNING_KEY_BUS_MIN_FREQUENCY,
					GPU_PWRLEVEL_OP_TUNING_ATTR);
		}
		if (update_imax) {
			if (data->imax > 0) {
				u64 kbps = (u64)pwr->ddr_table[data->imax];
				hwsched->sysfs_dcvs_tunables[GPU_TUNING_KEY_BUS_MAX_FREQUENCY].value = KBPS_TO_MHZ(kbps, 4) + 1;
				pr_err("%s, data->imax = %d, freq = %lld", __func__, data->imax, (u64)KBPS_TO_MHZ(kbps, 4) + 1);
			} else {
				hwsched->sysfs_dcvs_tunables[GPU_TUNING_KEY_BUS_MAX_FREQUENCY].value = -1;
			}
			kgsl_device_ptr->ftbl->gmu_based_dcvs_pwr_ops(kgsl_device_ptr,  GPU_TUNING_KEY_BUS_MAX_FREQUENCY,
					GPU_PWRLEVEL_OP_TUNING_ATTR);
		}
		if (update_amin) {
			hwsched->sysfs_dcvs_tunables[GPU_TUNING_KEY_BUS_MIN_AB_MBPS].value = data->amin;
			kgsl_device_ptr->ftbl->gmu_based_dcvs_pwr_ops(kgsl_device_ptr,  GPU_TUNING_KEY_BUS_MIN_AB_MBPS,
					GPU_PWRLEVEL_OP_TUNING_ATTR);
		}
		if (update_amax) {
			hwsched->sysfs_dcvs_tunables[GPU_TUNING_KEY_BUS_MAX_AB_MBPS].value = data->amax;
			kgsl_device_ptr->ftbl->gmu_based_dcvs_pwr_ops(kgsl_device_ptr,  GPU_TUNING_KEY_BUS_MAX_AB_MBPS,
					GPU_PWRLEVEL_OP_TUNING_ATTR);
		}
		kgsl_mutex_unlock(&kgsl_device_ptr->mutex);
	}

	return 0;
}
EXPORT_SYMBOL(geas_update_gpu_params);

#endif

static u32 _ab_buslevel_update(struct kgsl_pwrctrl *pwr,
		u32 ib)
{
	if (!ib)
		return 0;

	/*
	 * In the absence of any other settings, make ab 25% of ib
	 * where the ib vote is in kbps
	 */
	if ((!pwr->bus_percent_ab) && (!pwr->bus_ab_mbytes))
		return 25 * ib / 100000;

	if (pwr->bus_width)
		return pwr->bus_ab_mbytes;

	return (pwr->bus_percent_ab * pwr->bus_max) / 100;
}

int kgsl_bus_update(struct kgsl_device *device,
			 enum kgsl_bus_vote vote_state)
{
	struct kgsl_pwrctrl *pwr = &device->pwrctrl;
	int buslevel;
	u32 ab;

	/* the bus should be ON to update the active frequency */
	if ((vote_state != KGSL_BUS_VOTE_OFF) &&
		!(test_bit(KGSL_PWRFLAGS_AXI_ON, &pwr->power_flags)))
		return 0;
	/*
	 * If the bus should remain on calculate our request and submit it,
	 * otherwise request bus level 0, off.
	 */
	switch (vote_state) {
	case KGSL_BUS_VOTE_OFF:
		/* If the bus is being turned off, reset to default level */
		pwr->cur_dcvs_buslevel = 0;
		pwr->bus_mod = 0;
		pwr->bus_percent_ab = 0;
		pwr->bus_ab_mbytes = 0;
		ab = 0;
		break;
	case KGSL_BUS_VOTE_ON:
		{
		/* FIXME: this might be wrong? */
		int cur = pwr->pwrlevels[pwr->active_pwrlevel].bus_freq;
#if IS_ENABLED(CONFIG_OPLUS_FEATURE_GEAS_GPU)
		int imin = gpu_data.imin;
		int imax = gpu_data.imax;
		int amin = gpu_data.amin;
		int amax = gpu_data.amax;
#endif
		buslevel = min_t(int, pwr->pwrlevels[0].bus_max,
				cur + pwr->bus_mod);
		buslevel = max_t(int, buslevel, 1);
		pwr->cur_dcvs_buslevel = buslevel;
		ab = _ab_buslevel_update(pwr, pwr->ddr_table[buslevel]);
#if IS_ENABLED(CONFIG_OPLUS_FEATURE_GEAS_GPU)
		if (imin > 0 && imin < pwr->ddr_table_count) {
			buslevel = max_t(int, imin, buslevel);
		}
		if (imax > 0 && imax < pwr->ddr_table_count) {
			buslevel = min_t(int, imax, buslevel);
		}
		pwr->cur_dcvs_buslevel = buslevel;

		if (amin > 0) {
			ab = max_t(int, amin, ab);
		}
		if (amax > 0) {
			ab = min_t(int, amax, ab);
		}
#endif
		break;
		}
	case KGSL_BUS_VOTE_MINIMUM:
		/* Request bus level 1, minimum non-zero value */
		pwr->cur_dcvs_buslevel = 1;
		pwr->bus_mod = 0;
		pwr->bus_percent_ab = 0;
		pwr->bus_ab_mbytes = 0;
		ab = _ab_buslevel_update(pwr,
			pwr->ddr_table[pwr->cur_dcvs_buslevel]);
		break;
	case KGSL_BUS_VOTE_RT_HINT_ON:
		pwr->rt_bus_hint_active = true;
		/* Only update IB during bus hint */
		ab = pwr->cur_ab;
		break;
	case KGSL_BUS_VOTE_RT_HINT_OFF:
		pwr->rt_bus_hint_active = false;
		/* Only update IB during bus hint */
		ab = pwr->cur_ab;
		break;
	}

	buslevel = pwr->rt_bus_hint_active ?
		max(pwr->cur_dcvs_buslevel, pwr->rt_bus_hint) :
		pwr->cur_dcvs_buslevel;

	return device->ftbl->gpu_bus_set(device, buslevel, ab);
}

void kgsl_icc_set_tag(struct kgsl_pwrctrl *pwr, int buslevel)
{
	if (buslevel == pwr->pwrlevels[0].bus_max)
		icc_set_tag(pwr->icc_path, QCOM_ICC_TAG_ALWAYS | QCOM_ICC_TAG_PERF_MODE);
	else
		icc_set_tag(pwr->icc_path, QCOM_ICC_TAG_ALWAYS);
}

static void validate_pwrlevels(struct kgsl_device *device, u32 *ibs,
		int count)
{
	struct kgsl_pwrctrl *pwr = &device->pwrctrl;
	int i;

	for (i = 0; i < pwr->num_pwrlevels - 1; i++) {
		struct kgsl_pwrlevel *pwrlevel = &pwr->pwrlevels[i];

		if (pwrlevel->bus_freq >= count) {
			dev_err(device->dev, "Bus setting for GPU freq %d is out of bounds\n",
				pwrlevel->gpu_freq);
			pwrlevel->bus_freq = count - 1;
		}

		if (pwrlevel->bus_max >= count) {
			dev_err(device->dev, "Bus max for GPU freq %d is out of bounds\n",
				pwrlevel->gpu_freq);
			pwrlevel->bus_max = count - 1;
		}

		if (pwrlevel->bus_min >= count) {
			dev_err(device->dev, "Bus min for GPU freq %d is out of bounds\n",
				pwrlevel->gpu_freq);
			pwrlevel->bus_min = count - 1;
		}

		if (pwrlevel->bus_min > pwrlevel->bus_max) {
			dev_err(device->dev, "Bus min is bigger than bus max for GPU freq %d\n",
				pwrlevel->gpu_freq);
			pwrlevel->bus_min = pwrlevel->bus_max;
		}
	}
}

u32 *kgsl_bus_get_table(struct platform_device *pdev,
		const char *name, int *count)
{
	u32 *levels;
	int i, num = of_property_count_elems_of_size(pdev->dev.of_node,
		name, sizeof(u32));

	/* If the bus wasn't specified, then build a static table */
	if (num <= 0)
		return ERR_PTR(-EINVAL);

	levels = kcalloc(num, sizeof(*levels), GFP_KERNEL);
	if (!levels)
		return ERR_PTR(-ENOMEM);

	for (i = 0; i < num; i++)
		of_property_read_u32_index(pdev->dev.of_node,
			name, i, &levels[i]);

	*count = num;
	return levels;
}

int kgsl_bus_init(struct kgsl_device *device, struct platform_device *pdev)
{
	struct kgsl_pwrctrl *pwr = &device->pwrctrl;
	int count;
	int ddr = of_fdt_get_ddrtype();

	if (ddr >= 0) {
		char str[32];

		snprintf(str, sizeof(str), "qcom,bus-table-ddr%d", ddr);

		pwr->ddr_table = kgsl_bus_get_table(pdev, str, &count);
		if (!IS_ERR(pwr->ddr_table))
			goto done;
	}

	/* Look if a generic table is present */
	pwr->ddr_table = kgsl_bus_get_table(pdev, "qcom,bus-table-ddr", &count);
	if (IS_ERR(pwr->ddr_table)) {
		int ret = PTR_ERR(pwr->ddr_table);

		pwr->ddr_table = NULL;
		return ret;
	}
done:
	pwr->ddr_table_count = count;

	validate_pwrlevels(device, pwr->ddr_table, pwr->ddr_table_count);

	pwr->icc_path = of_icc_get(&pdev->dev, "gpu_icc_path");
	if (IS_ERR(pwr->icc_path) && !gmu_core_scales_bandwidth(device)) {
		WARN(1, "The CPU has no way to set the GPU bus levels\n");

		kfree(pwr->ddr_table);
		pwr->ddr_table = NULL;
		return PTR_ERR(pwr->icc_path);
	}
#if IS_ENABLED(CONFIG_OPLUS_FEATURE_GEAS_GPU)
	kgsl_device_ptr = device;
#endif

	return 0;
}

void kgsl_bus_close(struct kgsl_device *device)
{
	kfree(device->pwrctrl.ddr_table);
	device->pwrctrl.ddr_table = NULL;
	icc_put(device->pwrctrl.icc_path);
	device->pwrctrl.icc_path = NULL;
}
