#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/seq_file.h>
#include <linux/dcache.h>
#include <linux/string.h>

#include "procfs.h"
#include "block_metrics.h"
#if (LINUX_VERSION_CODE < KERNEL_VERSION(6, 6, 0))
#include "f2fs_metrics.h"
#endif /* (LINUX_VERSION_CODE < KERNEL_VERSION(6, 6, 0)) */
#include "ufs_metrics.h"
#include "abnormal_io.h"

#define STORAGE_DIR_NODE "oplus_storage"
#define IO_METRICS_DIR_NODE "io_metrics"
#define IO_METRICS_STAT_DIR_NODE "forever"
#define IO_METRICS_CONTROL_DIR_NODE "control"
#define DUMP_PATH_LEN 1024
static char abnormal_io_dump_path[DUMP_PATH_LEN];
bool proc_show_enabled = true;
module_param(proc_show_enabled, bool, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(proc_show_enabled, " Debug proc");

#define LABEL_BUF_LEN 50
static char label_buf[LABEL_BUF_LEN] = {"common"};
static struct proc_dir_entry *storage_procfs;
static struct proc_dir_entry *io_metrics_procfs;
static struct proc_dir_entry *io_metrics_stat_procfs;
static struct proc_dir_entry *io_metrics_control_procfs;
#ifdef CONFIG_OPLUS_FEATURE_STORAGE_IOLATENCY_STATS
static struct proc_dir_entry *ioLatencyStat_procfs;
#endif /* CONFIG_OPLUS_FEATURE_STORAGE_IOLATENCY_STATS */

#define CREATE_IO_METRICS_CONTROL_NODE(__name)                                 \
    proc_create(#__name, S_IRUGO | S_IWUGO, io_metrics_control_procfs, &__name ## _proc_fops)

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0)
#define OPS_TYPE struct proc_ops
#define OPEN_MEMBER .proc_open
#define READ_MEMBER .proc_read
#define WRITE_MEMBER .proc_write
#define LSEEK_MEMBER .proc_lseek
#define RELEASE_MEMBER .proc_release
#else
#define OPS_TYPE struct file_operations
#define OPEN_MEMBER .open
#define READ_MEMBER .read
#define WRITE_MEMBER .write
#define LSEEK_MEMBER .llseek
#define RELEASE_MEMBER .release
#endif

#define DEFINE_IO_METRICS_CONTROL(__name)                           \
static int __name ## _open(struct inode *inode, struct file *file)  \
{                                                                   \
    return single_open(file, __name ## _show, file);     \
}                                                                   \
                                                                    \
static const OPS_TYPE __name ## _proc_fops = {                       \
    OPEN_MEMBER    = __name ## _open,                               \
    READ_MEMBER    = seq_read,                                      \
    WRITE_MEMBER   = __name ## _write,                              \
    LSEEK_MEMBER   = seq_lseek,                                     \
    RELEASE_MEMBER = single_release,                                \
}

static const OPS_TYPE block_metrics_proc_fops = {
    OPEN_MEMBER    = block_metrics_proc_open,
    READ_MEMBER    = seq_read,
    LSEEK_MEMBER   = seq_lseek,
    RELEASE_MEMBER = single_release,
};

#if (LINUX_VERSION_CODE < KERNEL_VERSION(6, 6, 0))
static const OPS_TYPE f2fs_metrics_proc_fops = {
    OPEN_MEMBER    = f2fs_metrics_proc_open,
    READ_MEMBER    = seq_read,
    LSEEK_MEMBER   = seq_lseek,
    RELEASE_MEMBER = single_release,
};
#endif /* (LINUX_VERSION_CODE < KERNEL_VERSION(6, 6, 0)) */

static const OPS_TYPE ufs_metrics_proc_fops = {
    OPEN_MEMBER    = ufs_metrics_proc_open,
    READ_MEMBER    = seq_read,
    LSEEK_MEMBER   = seq_lseek,
    RELEASE_MEMBER = single_release,
};

#ifdef CONFIG_OPLUS_FEATURE_STORAGE_IOLATENCY_STATS
static const OPS_TYPE ioLatencyStat_proc_fops = {
    OPEN_MEMBER    = ioLatencyStat_proc_open,
    READ_MEMBER    = seq_read,
    LSEEK_MEMBER   = seq_lseek,
    RELEASE_MEMBER = single_release,
};

static const OPS_TYPE io_dist_stats_500ms_proc_fops = {
    OPEN_MEMBER    = io_dist_stats_500ms_proc_open,
    READ_MEMBER    = seq_read,
    LSEEK_MEMBER   = seq_lseek,
    RELEASE_MEMBER = single_release,
};

static const OPS_TYPE io_dist_stats_2s_proc_fops = {
    OPEN_MEMBER    = io_dist_stats_2s_proc_open,
    READ_MEMBER    = seq_read,
    LSEEK_MEMBER   = seq_lseek,
    RELEASE_MEMBER = single_release,
};

static const OPS_TYPE io_dist_stats_5s_proc_fops = {
    OPEN_MEMBER    = io_dist_stats_5s_proc_open,
    READ_MEMBER    = seq_read,
    LSEEK_MEMBER   = seq_lseek,
    RELEASE_MEMBER = single_release,
};
#endif /* CONFIG_OPLUS_FEATURE_STORAGE_IOLATENCY_STATS */

static int enable_show(struct seq_file *seq_filp, void *data)
{
    struct file *file = (struct file *)seq_filp->private;

    seq_printf(seq_filp, "%d\n", io_metrics_enabled);

    if (proc_show_enabled || unlikely(io_metrics_debug_enabled)) {
        io_metrics_print("%s(%d) read %s/%s: %d\n",
            current->comm, current->pid,
            file->f_path.dentry->d_parent->d_iname,
            file->f_path.dentry->d_iname,
            io_metrics_enabled);
    }
    return 0;
}

static ssize_t enable_write(struct file *file, const char __user *buf,
                           size_t len, loff_t *ppos)
{
    char buffer[32] = {0};
    int value, ret;

    len = (len > 31) ? 31 : len;
    if (copy_from_user(buffer, buf, len)) {
        return -EFAULT;
    }

    ret = kstrtoint(strstrip(buffer), 10, &value);
    if (ret)
        return ret;

    WRITE_ONCE(io_metrics_enabled, value);

    io_metrics_print("%s(%d) write %d to %s/%s\n",
        current->comm, current->pid, value,
        file->f_path.dentry->d_parent->d_iname,
        file->f_path.dentry->d_iname);

    *ppos += len;
    return len;
}
DEFINE_IO_METRICS_CONTROL(enable);

static int debug_enable_show(struct seq_file *seq_filp, void *data)
{
    struct file *file = (struct file *)seq_filp->private;

    seq_printf(seq_filp, "%d\n", io_metrics_debug_enabled);

    if (proc_show_enabled || unlikely(io_metrics_debug_enabled)) {
        io_metrics_print("%s(%d) read %s/%s: %d\n",
            current->comm, current->pid,
            file->f_path.dentry->d_parent->d_iname,
            file->f_path.dentry->d_iname,
            io_metrics_debug_enabled);
    }
    return 0;
}

static ssize_t debug_enable_write(struct file *file, const char __user *buf,
                                size_t len, loff_t *ppos)
{
    char buffer[32] = {0};
    int value, ret;

    len = (len > 31) ? 31 : len;
    if (copy_from_user(buffer, buf, len)) {
        return -EFAULT;
    }

    ret = kstrtoint(strstrip(buffer), 10, &value);
    if (ret)
        return ret;

    WRITE_ONCE(io_metrics_debug_enabled, value);

    io_metrics_print("%s(%d) write %d to %s/%s\n",
        current->comm, current->pid, value,
        file->f_path.dentry->d_parent->d_iname,
        file->f_path.dentry->d_iname);

    *ppos += len;
    return len;
}
DEFINE_IO_METRICS_CONTROL(debug_enable);

static int reset_stat_show(struct seq_file *seq_filp, void *data)
{
    struct file *file = (struct file *)seq_filp->private;

    // reset_stat is write-only but we return 0 for consistency
    seq_printf(seq_filp, "0\n");

    if (proc_show_enabled || unlikely(io_metrics_debug_enabled)) {
        io_metrics_print("%s(%d) read %s/%s\n",
            current->comm, current->pid,
            file->f_path.dentry->d_parent->d_iname,
            file->f_path.dentry->d_iname);
    }
    return 0;
}

static ssize_t reset_stat_write(struct file *file, const char __user *buf,
                               size_t len, loff_t *ppos)
{
    char buffer[32] = {0};
    int value, ret;

    len = (len > 31) ? 31 : len;
    if (copy_from_user(buffer, buf, len)) {
        return -EFAULT;
    }

    ret = kstrtoint(strstrip(buffer), 10, &value);
    if (ret)
        return ret;

    if (value == 1) {
        io_metrics_print("reset_stat start\n");
        io_metrics_reset();
        strncpy(label_buf, "common", sizeof("common"));
    }

    io_metrics_print("%s(%d) write %d to %s/%s\n",
        current->comm, current->pid, value,
        file->f_path.dentry->d_parent->d_iname,
        file->f_path.dentry->d_iname);

    *ppos += len;
    return len;
}
DEFINE_IO_METRICS_CONTROL(reset_stat);

static int abnormal_io_enabled_show(struct seq_file *seq_filp, void *data)
{
    struct file *file = (struct file *)seq_filp->private;
    int value = atomic_read(&abnormal_io_enabled);

    seq_printf(seq_filp, "%d\n", value);

    if (proc_show_enabled || unlikely(io_metrics_debug_enabled)) {
        io_metrics_print("%s(%d) read %s/%s: %d\n",
            current->comm, current->pid,
            file->f_path.dentry->d_parent->d_iname,
            file->f_path.dentry->d_iname,
            value);
    }
    return 0;
}

static ssize_t abnormal_io_enabled_write(struct file *file, const char __user *buf,
                                        size_t len, loff_t *ppos)
{
    char buffer[32] = {0};
    int value, ret;

    len = (len > 31) ? 31 : len;
    if (copy_from_user(buffer, buf, len)) {
        return -EFAULT;
    }

    ret = kstrtoint(strstrip(buffer), 10, &value);
    if (ret)
        return ret;

#if (LINUX_VERSION_CODE < KERNEL_VERSION(6, 6, 0))
    if (0 == value) {
        abnormal_io_exit();
    } else if (1 == value) {
        abnormal_io_init();
    }
#else
    io_metrics_print("Greater than or equal kernel-6.6 do not support abnormal io\n");
#endif

    io_metrics_print("%s(%d) write %d to %s/%s\n",
        current->comm, current->pid, value,
        file->f_path.dentry->d_parent->d_iname,
        file->f_path.dentry->d_iname);

    *ppos += len;
    return len;
}
DEFINE_IO_METRICS_CONTROL(abnormal_io_enabled);

static int abnormal_io_trigger_show(struct seq_file *seq_filp, void *data)
{
    struct file *file = (struct file *)seq_filp->private;

    seq_printf(seq_filp, "%d\n", abnormal_io_trigger);

    if (proc_show_enabled || unlikely(io_metrics_debug_enabled)) {
        io_metrics_print("%s(%d) read %s/%s: %d\n",
            current->comm, current->pid,
            file->f_path.dentry->d_parent->d_iname,
            file->f_path.dentry->d_iname,
            abnormal_io_trigger);
    }
    return 0;
}

static ssize_t abnormal_io_trigger_write(struct file *file, const char __user *buf,
                                        size_t len, loff_t *ppos)
{
    char buffer[32] = {0};
    int value, ret;

    len = (len > 31) ? 31 : len;
    if (copy_from_user(buffer, buf, len)) {
        return -EFAULT;
    }

    ret = kstrtoint(strstrip(buffer), 10, &value);
    if (ret)
        return ret;

    if (value == 1) {
        ret = abnormal_io_dump_to_file("/data/persist_log/DCS/de/storage/storage_io.hex");
        if (!ret) {
            io_metrics_print("abnormal_io_dump_to_file err(%d)\n", ret);
        }
    }

    io_metrics_print("%s(%d) write %d to %s/%s\n",
        current->comm, current->pid, value,
        file->f_path.dentry->d_parent->d_iname,
        file->f_path.dentry->d_iname);

    *ppos += len;
    return len;
}
DEFINE_IO_METRICS_CONTROL(abnormal_io_trigger);

static int abnormal_io_dump_min_interval_s_show(struct seq_file *seq_filp, void *data)
{
    struct file *file = (struct file *)seq_filp->private;

    seq_printf(seq_filp, "%d\n", abnormal_io_dump_min_interval_s);

    if (proc_show_enabled || unlikely(io_metrics_debug_enabled)) {
        io_metrics_print("%s(%d) read %s/%s: %d\n",
            current->comm, current->pid,
            file->f_path.dentry->d_parent->d_iname,
            file->f_path.dentry->d_iname,
            abnormal_io_dump_min_interval_s);
    }
    return 0;
}

static ssize_t abnormal_io_dump_min_interval_s_write(struct file *file, const char __user *buf,
                                                    size_t len, loff_t *ppos)
{
    char buffer[32] = {0};
    int value, ret;

    len = (len > 31) ? 31 : len;
    if (copy_from_user(buffer, buf, len)) {
        return -EFAULT;
    }

    ret = kstrtoint(strstrip(buffer), 10, &value);
    if (ret)
        return ret;

    WRITE_ONCE(abnormal_io_dump_min_interval_s, value);

    io_metrics_print("%s(%d) write %d to %s/%s\n",
        current->comm, current->pid, value,
        file->f_path.dentry->d_parent->d_iname,
        file->f_path.dentry->d_iname);

    *ppos += len;
    return len;
}
DEFINE_IO_METRICS_CONTROL(abnormal_io_dump_min_interval_s);

static int abnormal_io_dump_limit_1_day_show(struct seq_file *seq_filp, void *data)
{
    struct file *file = (struct file *)seq_filp->private;

    seq_printf(seq_filp, "%d\n", abnormal_io_dump_limit_1_day);

    if (proc_show_enabled || unlikely(io_metrics_debug_enabled)) {
        io_metrics_print("%s(%d) read %s/%s: %d\n",
            current->comm, current->pid,
            file->f_path.dentry->d_parent->d_iname,
            file->f_path.dentry->d_iname,
            abnormal_io_dump_limit_1_day);
    }
    return 0;
}

static ssize_t abnormal_io_dump_limit_1_day_write(struct file *file, const char __user *buf,
                                                 size_t len, loff_t *ppos)
{
    char buffer[32] = {0};
    int value, ret;

    len = (len > 31) ? 31 : len;
    if (copy_from_user(buffer, buf, len)) {
        return -EFAULT;
    }

    ret = kstrtoint(strstrip(buffer), 10, &value);
    if (ret)
        return ret;

    WRITE_ONCE(abnormal_io_dump_limit_1_day, value);

    io_metrics_print("%s(%d) write %d to %s/%s\n",
        current->comm, current->pid, value,
        file->f_path.dentry->d_parent->d_iname,
        file->f_path.dentry->d_iname);

    *ppos += len;
    return len;
}
DEFINE_IO_METRICS_CONTROL(abnormal_io_dump_limit_1_day);

static int abnormal_io_dump_path_show(struct seq_file *seq_filp, void *data)
{
    struct file *file = (struct file *)seq_filp->private;

    abnormal_io_dump_path[DUMP_PATH_LEN - 1] = '\0';
    seq_printf(seq_filp, "%s\n", abnormal_io_dump_path);

    if (proc_show_enabled || unlikely(io_metrics_debug_enabled)) {
        io_metrics_print("%s(%d) read %s/%s: %s\n",
            current->comm, current->pid,
            file->f_path.dentry->d_parent->d_iname,
            file->f_path.dentry->d_iname,
            abnormal_io_dump_path);
    }
    return 0;
}

static ssize_t abnormal_io_dump_path_write(struct file *file, const char __user *buf,
                                          size_t len, loff_t *ppos)
{
    int ret = 0;

    len = (len > DUMP_PATH_LEN - 1) ? DUMP_PATH_LEN - 1 : len;
    if (copy_from_user(abnormal_io_dump_path, buf, len)) {
        return -EFAULT;
    }
    abnormal_io_dump_path[len] = '\0';
    /* Ensure there's enough space for "/storage_io.hex" + null terminator */
    if (len + sizeof("/storage_io.hex") > DUMP_PATH_LEN) {
        io_metrics_print("abnormal_io_dump_path too long\n");
        return -ENAMETOOLONG;
    }
    strncat(abnormal_io_dump_path, "/storage_io.hex", DUMP_PATH_LEN - len - 1);
    ret = abnormal_io_dump_to_file((const char *)abnormal_io_dump_path);
    if (!ret) {
        io_metrics_print("abnormal_io_dump_to_file err(%d)\n", ret);
        goto out;
    }

    io_metrics_print("%s(%d) write %s to %s/%s\n",
        current->comm, current->pid, abnormal_io_dump_path,
        file->f_path.dentry->d_parent->d_iname,
        file->f_path.dentry->d_iname);

out:
    *ppos += len;
    return len;
}
DEFINE_IO_METRICS_CONTROL(abnormal_io_dump_path);

static int label_show(struct seq_file *seq_filp, void *data)
{
    struct file *file = (struct file *)seq_filp->private;
    seq_printf(seq_filp, "%s\n", label_buf);

    if (proc_show_enabled) {
        io_metrics_print("%s(%d) read %s/%s: %s\n",
            current->comm, current->pid,
            file->f_path.dentry->d_parent->d_iname,
            file->f_path.dentry->d_iname,
            label_buf);
    }

    return 0;
}

static ssize_t label_write(struct file *file, const char __user *buf,
                          size_t len, loff_t *ppos)
{
    len = len < (LABEL_BUF_LEN - 1) ? len : (LABEL_BUF_LEN - 1);
    if (copy_from_user(label_buf, buf, len)) {
        return -EFAULT;
    }
    label_buf[len] = '\0';
    io_metrics_print("%s(%d) write %s to %s/%s\n",
        current->comm, current->pid, label_buf,
        file->f_path.dentry->d_parent->d_iname,
        file->f_path.dentry->d_iname);

    *ppos += len;
    return len;
}
DEFINE_IO_METRICS_CONTROL(label);

enum node_type {
    F2FS = 0,
    BLOCK,
    UFS,
    CONTROL,
};

struct io_metrics_procfs_node_type {
    const char *name;
    enum node_type node_type;
    umode_t mode;
} io_metrics_procfs_node[] = {
#if (LINUX_VERSION_CODE < KERNEL_VERSION(6, 6, 0))
    /* filesystem layer */
    {"f2fs_discard_cnt",             F2FS, S_IRUGO},
    {"f2fs_discard_len",             F2FS, S_IRUGO},
    {"f2fs_fg_gc_cnt",               F2FS, S_IRUGO},
    {"f2fs_fg_gc_avg_time",          F2FS, S_IRUGO},
    {"f2fs_fg_gc_seg_cnt",           F2FS, S_IRUGO},
    {"f2fs_bg_gc_cnt",               F2FS, S_IRUGO},
    {"f2fs_bg_gc_avg_time",          F2FS, S_IRUGO},
    {"f2fs_bg_gc_seg_cnt",           F2FS, S_IRUGO},
    {"f2fs_cp_cnt",                  F2FS, S_IRUGO},
    {"f2fs_cp_avg_time",             F2FS, S_IRUGO},
    {"f2fs_cp_max_time",             F2FS, S_IRUGO},
    {"f2fs_ipu_cnt",                 F2FS, S_IRUGO},
    {"f2fs_fsync_cnt",               F2FS, S_IRUGO},
#endif /* (LINUX_VERSION_CODE < KERNEL_VERSION(6, 6, 0)) */
    /* block layer */
    {"bio_read_cnt",                BLOCK, S_IRUGO},
    {"bio_read_avg_size",           BLOCK, S_IRUGO},
    {"bio_read_size_dist",          BLOCK, S_IRUGO},
    {"bio_read_avg_time",           BLOCK, S_IRUGO},
    {"bio_read_max_time",           BLOCK, S_IRUGO},
    {"bio_read_4k_blk_avg_time",    BLOCK, S_IRUGO},
    {"bio_read_4k_blk_max_time",    BLOCK, S_IRUGO},
    {"bio_read_4k_blk_lat_dist",    BLOCK, S_IRUGO},
    {"bio_read_4k_drv_avg_time",    BLOCK, S_IRUGO},
    {"bio_read_4k_drv_max_time",    BLOCK, S_IRUGO},
    {"bio_read_4k_drv_lat_dist",    BLOCK, S_IRUGO},
    {"bio_read_512k_blk_avg_time",  BLOCK, S_IRUGO},
    {"bio_read_512k_blk_max_time",  BLOCK, S_IRUGO},
    {"bio_read_512k_blk_lat_dist",  BLOCK, S_IRUGO},
    {"bio_read_512k_drv_avg_time",  BLOCK, S_IRUGO},
    {"bio_read_512k_drv_max_time",  BLOCK, S_IRUGO},
    {"bio_read_512k_drv_lat_dist",  BLOCK, S_IRUGO},
    {"bio_write_cnt",               BLOCK, S_IRUGO},
    {"bio_write_avg_size",          BLOCK, S_IRUGO},
    {"bio_write_size_dist",         BLOCK, S_IRUGO},
    {"bio_write_avg_time",          BLOCK, S_IRUGO},
    {"bio_write_max_time",          BLOCK, S_IRUGO},
    {"bio_write_4k_blk_avg_time",   BLOCK, S_IRUGO},
    {"bio_write_4k_blk_max_time",   BLOCK, S_IRUGO},
    {"bio_write_4k_blk_lat_dist",   BLOCK, S_IRUGO},
    {"bio_write_4k_drv_avg_time",   BLOCK, S_IRUGO},
    {"bio_write_4k_drv_max_time",   BLOCK, S_IRUGO},
    {"bio_write_4k_drv_lat_dist",   BLOCK, S_IRUGO},
    {"bio_write_512k_blk_avg_time", BLOCK, S_IRUGO},
    {"bio_write_512k_blk_max_time", BLOCK, S_IRUGO},
    {"bio_write_512k_blk_lat_dist", BLOCK, S_IRUGO},
    {"bio_write_512k_drv_avg_time", BLOCK, S_IRUGO},
    {"bio_write_512k_drv_max_time", BLOCK, S_IRUGO},
    {"bio_write_512k_drv_lat_dist", BLOCK, S_IRUGO},
    /* ufs layer */
    {"ufs_total_read_size_mb",        UFS, S_IRUGO},
    {"ufs_total_read_time_ms",        UFS, S_IRUGO},
    {"ufs_total_write_size_mb",       UFS, S_IRUGO},
    {"ufs_total_write_time_ms",       UFS, S_IRUGO},
    {"ufs_read_lat_dist",             UFS, S_IRUGO},
    {"ufs_write_lat_dist",            UFS, S_IRUGO},
    {NULL,                               0,                 0}
};

static int create_proc_directory(const char *name, struct proc_dir_entry *parent,
                                struct proc_dir_entry **dir)
{
    *dir = proc_mkdir(name, parent);
    if (!*dir) {
        io_metrics_print("Failed to create procfs directory: %s\n", name);
        return -1;
    }
    return 0;
}

static const OPS_TYPE *get_proc_ops(enum node_type node_type)
{
    switch (node_type) {
#if (LINUX_VERSION_CODE < KERNEL_VERSION(6, 6, 0))
        case F2FS:
            return &f2fs_metrics_proc_fops;
#endif
        case BLOCK:
            return &block_metrics_proc_fops;
        case UFS:
            return &ufs_metrics_proc_fops;
        default:
            return NULL;
    }
}

/**
 * Create metrics node (in 'forever' directory)
 * @node: Node information
 * return: 0 on success, other values on failure
 */
static int create_metrics_node(const struct io_metrics_procfs_node_type *node)
{
    struct proc_dir_entry *pnode;
    const void *ops = get_proc_ops(node->node_type);

    if (!ops) {
        io_metrics_print("No proc operations for node: %s\n", node->name);
        return -1;
    }

    pnode = proc_create(node->name, node->mode, io_metrics_stat_procfs, ops);
    if (!pnode) {
        io_metrics_print("Failed to create metrics node: %s\n", node->name);
        return -1;
    }

    return 0;
}

/**
 * Create all proc nodes
 * return: 0 on success, other values on failure
 */
static int create_all_proc_nodes(void)
{
    int i;
    // Create metrics nodes
    for (i = 0; io_metrics_procfs_node[i].name; i++) {
        if (create_metrics_node(&io_metrics_procfs_node[i]) != 0)
            return -1;
    }

    // Create control nodes individually (following label node pattern)
    CREATE_IO_METRICS_CONTROL_NODE(enable);
    CREATE_IO_METRICS_CONTROL_NODE(debug_enable);
    CREATE_IO_METRICS_CONTROL_NODE(reset_stat);
    CREATE_IO_METRICS_CONTROL_NODE(abnormal_io_enabled);
    CREATE_IO_METRICS_CONTROL_NODE(abnormal_io_trigger);
    CREATE_IO_METRICS_CONTROL_NODE(abnormal_io_dump_min_interval_s);
    CREATE_IO_METRICS_CONTROL_NODE(abnormal_io_dump_limit_1_day);
    CREATE_IO_METRICS_CONTROL_NODE(abnormal_io_dump_path);
    CREATE_IO_METRICS_CONTROL_NODE(label);

    return 0;
}

int io_metrics_procfs_init(void)
{
    // 1. Create root directory: /proc/oplus_storage
    if (create_proc_directory(STORAGE_DIR_NODE, NULL, &storage_procfs) != 0)
        goto error_out;

    // 2. Create parent directory: /proc/oplus_storage/io_metrics
    if (create_proc_directory(IO_METRICS_DIR_NODE, storage_procfs, &io_metrics_procfs) != 0)
        goto error_out;

    // 3. Create metrics directory: /proc/oplus_storage/io_metrics/forever
    if (create_proc_directory(IO_METRICS_STAT_DIR_NODE, io_metrics_procfs, &io_metrics_stat_procfs) != 0)
        goto error_out;

    // 4. Create control directory: /proc/oplus_storage/io_metrics/control
    if (create_proc_directory(IO_METRICS_CONTROL_DIR_NODE, io_metrics_procfs, &io_metrics_control_procfs) != 0)
        goto error_out;

#ifdef CONFIG_OPLUS_FEATURE_STORAGE_IOLATENCY_STATS
    ioLatencyStat_procfs = proc_create_data("ioLatencyStat", S_IRUGO, io_metrics_procfs,
        &ioLatencyStat_proc_fops, NULL);
    proc_create_data("io_dist_stats_500ms", S_IRUGO, io_metrics_procfs,
        &io_dist_stats_500ms_proc_fops, NULL);
    proc_create_data("io_dist_stats_2s", S_IRUGO, io_metrics_procfs,
        &io_dist_stats_2s_proc_fops, NULL);
    proc_create_data("io_dist_stats_5s", S_IRUGO, io_metrics_procfs,
        &io_dist_stats_5s_proc_fops, NULL);
#endif /* CONFIG_OPLUS_FEATURE_STORAGE_IOLATENCY_STATS */

    // Create all nodes
    if (create_all_proc_nodes() != 0)
        goto error_out;

    return 0;

error_out:
    // Clean up created directories in reverse order on failure
    if (io_metrics_control_procfs)
        remove_proc_entry(IO_METRICS_CONTROL_DIR_NODE, io_metrics_procfs);
    if (io_metrics_stat_procfs)
        remove_proc_entry(IO_METRICS_STAT_DIR_NODE, io_metrics_procfs);
    if (io_metrics_procfs)
        remove_proc_entry(IO_METRICS_DIR_NODE, storage_procfs);
    if (storage_procfs)
        remove_proc_entry(STORAGE_DIR_NODE, NULL);

    return -1;
}

void io_metrics_procfs_exit(void)
{
    // Destroy directories in hierarchical order
    if (storage_procfs) {
        if (io_metrics_procfs) {
            if (io_metrics_control_procfs)
                remove_proc_entry(IO_METRICS_CONTROL_DIR_NODE, io_metrics_procfs);
            if (io_metrics_stat_procfs)
                remove_proc_entry(IO_METRICS_STAT_DIR_NODE, io_metrics_procfs);
            remove_proc_entry(IO_METRICS_DIR_NODE, storage_procfs);
        }
        remove_proc_entry(STORAGE_DIR_NODE, NULL);
    }
}
