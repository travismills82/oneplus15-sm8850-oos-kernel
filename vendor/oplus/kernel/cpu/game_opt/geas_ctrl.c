#include "geas_ctrl.h"
#include "game_ctrl.h"
#include <linux/ioctl.h>


#if IS_ENABLED(CONFIG_OPLUS_FEATURE_GEAS)

int (*game_update_geas_fdrive_params)(struct frame_drive_params * fdrive_datas) = NULL;
EXPORT_SYMBOL(game_update_geas_fdrive_params);

int (*game_update_geas_gpu_params)(struct gpu_params * gpu_datas) = NULL;
EXPORT_SYMBOL(game_update_geas_gpu_params);

int (*game_update_geas_memlat_params)(struct memlat_params * memlat_datas) = NULL;
EXPORT_SYMBOL(game_update_geas_memlat_params);

int (*game_update_geas_bwmon_params)(struct bwmon_params * bwmon_datas) = NULL;
EXPORT_SYMBOL(game_update_geas_bwmon_params);

int (*game_update_geas_emi_params)(struct emi_params * emi_datas) = NULL;
EXPORT_SYMBOL(game_update_geas_emi_params);

int (*game_update_geas_npu_params)(struct npu_params * npu_datas) = NULL;
EXPORT_SYMBOL(game_update_geas_npu_params);

#if IS_ENABLED(CONFIG_CPU_IDLE_GOV_QCOM_LPM)
extern int (*game_lpm_disable_cpu)(int cpu, u64 *timeout);

static DEFINE_PER_CPU(int, cpu_lpm_disable);
static DEFINE_MUTEX(lpm_mutex);
static int g_lpm = 0;

static int gameopt_update_lpm_cpu(void __user *uarg)
{
	int cpu;
	int ret = 0;
	if (uarg == NULL) {
		return -EINVAL;
	}

	if (copy_from_user(&cpu, uarg, sizeof(int))) {
		return -EFAULT;
	}

	pr_err("gameopt_update_lpm_cpu%d", cpu);

	mutex_lock(&lpm_mutex);
	if (cpu == CPU_LPM_RESET) {
		g_lpm = 0;
		int i;
		for_each_possible_cpu(i) {
			per_cpu(cpu_lpm_disable, i) = 0;
		}
	} else if (cpu >= 0 && cpu < 8) {
		g_lpm = 1;
		per_cpu(cpu_lpm_disable, cpu) = 1;
	}
	mutex_unlock(&lpm_mutex);

	return ret;
}

int gameopt_lpm_disable_cpu(int cpu, u64 *timeout)
{
	if (!g_lpm)
		return 0;
	int lpm_disable = per_cpu(cpu_lpm_disable, cpu);
	if (lpm_disable) {
		*timeout =  10 * NSEC_PER_MSEC;
		return 1;
	}
	return 0;
}
#endif /* CONFIG_CPU_IDLE_GOV_QCOM_LPM */

static long update_geas_params(void __user *uarg)
{
	struct geas_params info;
	int ret = 0;

	pr_err("%s start", __func__);

	if (uarg == NULL) {
		ret = -EINVAL;
		goto ERROR_HANDLE;
	}

	if (copy_from_user(&info, uarg, sizeof(struct geas_params))) {
		ret = EFAULT;
		goto ERROR_HANDLE;
	}

	if ((info.geasFlag & FRDR_FLAG) && game_update_geas_fdrive_params != NULL) {
		info.fdrive_datas.resv[0] = info.resv[0];
		game_update_geas_fdrive_params(&(info.fdrive_datas));
	}

	if ((info.geasFlag & GPU_FLAG) && game_update_geas_gpu_params != NULL) {
		info.gpu_datas.resv[0] = info.resv[0];
		game_update_geas_gpu_params(&(info.gpu_datas));
	}

	if ((info.geasFlag & MEM_FALG) && game_update_geas_memlat_params != NULL) {
		info.memlat_datas.resv[0] = info.resv[0];
		game_update_geas_memlat_params(&(info.memlat_datas));
	}

	if ((info.geasFlag & BWM_FLAG) && game_update_geas_bwmon_params != NULL) {
		info.bwmon_datas.resv[0] = info.resv[0];
		game_update_geas_bwmon_params(&(info.bwmon_datas));
	}

	if ((info.geasFlag & EMI_FLAG) && game_update_geas_emi_params != NULL) {
		info.emi_datas.resv[0] = info.resv[0];
		game_update_geas_emi_params(&(info.emi_datas));
	}

	if ((info.geasFlag & NPU_FLAG) && game_update_geas_npu_params != NULL) {
		info.npu_datas.resv[0] = info.resv[0];
		game_update_geas_npu_params(&(info.npu_datas));
	}

	goto out;

ERROR_HANDLE:
	pr_err("%s: kzalloc hwmon_node_ext fail, %d\n", __func__, ret);
	if (game_update_geas_fdrive_params != NULL)
		game_update_geas_fdrive_params(NULL);
	if (game_update_geas_gpu_params != NULL)
		game_update_geas_gpu_params(NULL);
	if (game_update_geas_memlat_params != NULL)
		game_update_geas_memlat_params(NULL);
	if (game_update_geas_bwmon_params != NULL)
		game_update_geas_bwmon_params(NULL);
	if (game_update_geas_emi_params != NULL)
		game_update_geas_emi_params(NULL);
#if 0
	if (game_update_geas_npu_params != NULL)
		game_update_geas_npu_params(NULL);
#endif

out:
	pr_err("%s end, ret = %d", __func__, ret);
	return ret;
}

static long geas_ctrl_ioctl(struct file* file, unsigned int cmd, unsigned long arg) {
	long ret = 0;
	void __user *uarg = (void __user *)arg;
	if ((_IOC_TYPE(cmd) !=  GEAS_MAGIC)) {
		return -EINVAL;
	}

	switch (cmd) {
	case CMD_ID_UPDATE_GEAS_PARAMS:
		update_geas_params(uarg);
		// TODO
	break;

#if IS_ENABLED(CONFIG_CPU_IDLE_GOV_QCOM_LPM)
	case CMD_ID_UPDATE_LPM_CPU:
		gameopt_update_lpm_cpu(uarg);
	break;
#endif /* CONFIG_CPU_IDLE_GOV_QCOM_LPM */
	default:
		return -ENOTTY;
	}

	return ret;
}

static int geas_ctrl_open(struct inode *inode, struct file *file) {
	return 0;
}

static int geas_ctrl_release(struct inode *inode, struct file *file) {
	return 0;
}

#if IS_ENABLED(CONFIG_COMPAT)
static long compat_geas_ctrl_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	return geas_ctrl_ioctl(file, cmd, (unsigned long)compat_ptr(arg));
}
#endif /* CONFIG_COMPAT */

static const struct proc_ops geas_ctrl_proc_ops = {
	.proc_ioctl     = geas_ctrl_ioctl,
	.proc_open      = geas_ctrl_open,
	.proc_release   = geas_ctrl_release,
#if IS_ENABLED(CONFIG_COMPAT)
	.proc_compat_ioctl	= compat_geas_ctrl_ioctl,
#endif /* CONFIG_COMPAT */
	.proc_lseek     = default_llseek,
};

#if IS_ENABLED(CONFIG_CPU_IDLE_GOV_QCOM_LPM)
static int lpm_disable_show(struct seq_file *m, void *v)
{
	if (g_debug_enable == 1) {
		mutex_lock(&lpm_mutex);
		int cpu;
		for_each_possible_cpu(cpu) {
			seq_printf(m, "CPU%d: %d\n", cpu, per_cpu(cpu_lpm_disable, cpu));
		}
		mutex_unlock(&lpm_mutex);
	}
	return 0;
}

static int lpm_disable_open(struct inode *inode, struct file *filp)
{
	return single_open(filp, lpm_disable_show, inode);
}

static const struct proc_ops lpm_disable_proc_ops = {
	.proc_open		= lpm_disable_open,
	.proc_read		= seq_read,
	.proc_lseek		= seq_lseek,
	.proc_release	= single_release,
};
#endif /* CONFIG_CPU_IDLE_GOV_QCOM_LPM */

int geas_ctrl_init(void) {
	if (unlikely(!game_opt_dir)) {
		return -ENOTDIR;
	}

	proc_create_data("geas_ctrl", 0664, game_opt_dir, &geas_ctrl_proc_ops, NULL);
#if IS_ENABLED(CONFIG_CPU_IDLE_GOV_QCOM_LPM)
	proc_create_data("lpm_disable", 0664, game_opt_dir, &lpm_disable_proc_ops, NULL);

	int cpu;
	for_each_possible_cpu(cpu) {
		per_cpu(cpu_lpm_disable, cpu) = 0;
	}
	game_lpm_disable_cpu = gameopt_lpm_disable_cpu;
#endif /* CONFIG_CPU_IDLE_GOV_QCOM_LPM */
	return 0;
}

#endif
