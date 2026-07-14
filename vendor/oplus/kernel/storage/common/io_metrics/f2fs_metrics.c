#include "io_metrics_entry.h"
#include "f2fs_metrics.h"
#include "procfs.h"
#include "fs/f2fs/f2fs.h"
#include "fs/f2fs/segment.h"
#include "fs/f2fs/node.h"
#include <trace/events/f2fs.h>
#include <linux/atomic.h>
#include <linux/string.h>
#include <linux/compiler.h>
#include <linux/cache.h>  // For cacheline size definitions

/* Module parameters - control debug output for different operations */
bool f2fs_issue_discard_enabled = false;
bool f2fs_gc_begin_enabled = false;
bool f2fs_gc_end_enabled = false;
bool f2fs_write_checkpoint_enabled = false;
bool f2fs_sync_file_enter_enabled = false;
bool f2fs_dataread_start_enabled = false;
bool f2fs_dataread_end_enabled = false;
bool f2fs_datawrite_start_enabled = false;
bool f2fs_datawrite_end_enabled = false;

module_param(f2fs_issue_discard_enabled, bool, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(f2fs_issue_discard_enabled, "Enable f2fs_issue_discard debug (default: false)");
module_param(f2fs_gc_begin_enabled, bool, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(f2fs_gc_begin_enabled, "Enable f2fs_gc_begin debug (default: false)");
module_param(f2fs_gc_end_enabled, bool, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(f2fs_gc_end_enabled, "Enable f2fs_gc_end debug (default: false)");
module_param(f2fs_write_checkpoint_enabled, bool, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(f2fs_write_checkpoint_enabled, "Enable f2fs_write_checkpoint debug (default: false)");
module_param(f2fs_sync_file_enter_enabled, bool, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(f2fs_sync_file_enter_enabled, "Enable f2fs_sync_file_enter debug (default: false)");
module_param(f2fs_dataread_start_enabled, bool, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(f2fs_dataread_start_enabled, "Enable f2fs_dataread_start debug (default: false)");
module_param(f2fs_dataread_end_enabled, bool, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(f2fs_dataread_end_enabled, "Enable f2fs_dataread_end debug (default: false)");
module_param(f2fs_datawrite_start_enabled, bool, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(f2fs_datawrite_start_enabled, "Enable f2fs_datawrite_start debug (default: false)");
module_param(f2fs_datawrite_end_enabled, bool, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(f2fs_datawrite_end_enabled, "Enable f2fs_datawrite_end debug (default: false)");

/* GC type definitions */
enum {
    GC_BG = 0,  // Background GC
    GC_FG       // Foreground GC
};

#define TOTAL_GC_TYPES 2  // Total number of GC types

/*
 * Metrics structures with cacheline alignment to prevent false sharing
 * Each structure fits within a single cacheline (64 bytes)
 */
static struct {
    atomic64_t elapse_time;    // Accumulated time (ns)
    atomic64_t begin_time;     // Last GC start time (ns)
    atomic64_t cnt;            // Total GC count
    atomic64_t avg_time;       // Average time per GC (ns)
    atomic64_t segs;           // Total number of recovered segments
    atomic64_t avg_segs;       // Average segments recovered per GC
    atomic64_t efficiency;     // Average valid block ratio
    char pad[8];               // Padding to 64-byte cacheline
} f2fs_gc_metrics[TOTAL_GC_TYPES] __cacheline_aligned;

static struct {
    atomic64_t cnt;            // Total checkpoint count
    atomic64_t elapse_time;    // Accumulated time (ns)
    atomic64_t begin_time;     // Last checkpoint start time (ns)
    atomic64_t avg_time;       // Average time per checkpoint (ns)
    atomic64_t max_time;       // Maximum time (ns)
    atomic_t inplace_count;    // In-place update count
    char pad[20];              // Padding to 64-byte cacheline
} f2fs_cp_metrics __cacheline_aligned;

static struct {
    atomic64_t discard_cnt;    // Number of discard operations
    atomic64_t discard_len;    // Total discard length (blocks)
    atomic64_t fsync_cnt;      // fsync operation count
    char pad[40];              // Padding to 64-byte cacheline
} f2fs_metrics __cacheline_aligned;

/* Global variable to track current GC type */
static atomic_t gc_t;

static void gc_end_update_stats(int gc_t, u64 gc_elapse, unsigned int free_seg)
{
    u64 cnt = atomic64_inc_return(&f2fs_gc_metrics[gc_t].cnt);
    u64 elapse_time = atomic64_add_return(gc_elapse, &f2fs_gc_metrics[gc_t].elapse_time);
    u64 segs = atomic64_add_return(free_seg, &f2fs_gc_metrics[gc_t].segs);

    if (likely(cnt > 0)) {
        atomic64_set(&f2fs_gc_metrics[gc_t].avg_time, elapse_time / cnt);
        atomic64_set(&f2fs_gc_metrics[gc_t].avg_segs, segs / cnt);
    } else {
        atomic64_set(&f2fs_gc_metrics[gc_t].cnt, 0);
        atomic64_set(&f2fs_gc_metrics[gc_t].avg_time, 0);
        atomic64_set(&f2fs_gc_metrics[gc_t].avg_segs, 0);
        atomic64_set(&f2fs_gc_metrics[gc_t].elapse_time, 0);
        atomic64_set(&f2fs_gc_metrics[gc_t].segs, 0);
        atomic64_set(&f2fs_gc_metrics[gc_t].efficiency, 0);
    }
}

/* Metric retrieval functions - one per metric type */
static u64 get_discard_cnt(void) { return atomic64_read(&f2fs_metrics.discard_cnt); }
static u64 get_discard_len(void) { return atomic64_read(&f2fs_metrics.discard_len); }
static u64 get_fg_gc_cnt(void) { return atomic64_read(&f2fs_gc_metrics[GC_FG].cnt); }
static u64 get_fg_gc_avg_time(void) { return atomic64_read(&f2fs_gc_metrics[GC_FG].avg_time); }
static u64 get_fg_gc_seg_cnt(void) { return atomic64_read(&f2fs_gc_metrics[GC_FG].segs); }
static u64 get_bg_gc_cnt(void) { return atomic64_read(&f2fs_gc_metrics[GC_BG].cnt); }
static u64 get_bg_gc_avg_time(void) { return atomic64_read(&f2fs_gc_metrics[GC_BG].avg_time); }
static u64 get_bg_gc_seg_cnt(void) { return atomic64_read(&f2fs_gc_metrics[GC_BG].segs); }
static u64 get_cp_cnt(void) { return atomic64_read(&f2fs_cp_metrics.cnt); }
static u64 get_cp_avg_time(void) { return atomic64_read(&f2fs_cp_metrics.avg_time); }
static u64 get_cp_max_time(void) { return atomic64_read(&f2fs_cp_metrics.max_time); }
static u64 get_ipu_cnt(void) { return (u64)atomic_read(&f2fs_cp_metrics.inplace_count); }
static u64 get_fsync_cnt(void) { return atomic64_read(&f2fs_metrics.fsync_cnt); }

/* Define metric function types */
typedef u64 (*metric_value_func)(void);

/* Metric mapping table */
static const struct {
    const char *name;
    size_t name_len;
    metric_value_func get_value;
} f2fs_metric_maps[] = {
    {"f2fs_discard_cnt",           sizeof("f2fs_discard_cnt")-1,           get_discard_cnt},
    {"f2fs_discard_len",           sizeof("f2fs_discard_len")-1,           get_discard_len},
    {"f2fs_fg_gc_cnt",             sizeof("f2fs_fg_gc_cnt")-1,             get_fg_gc_cnt},
    {"f2fs_fg_gc_avg_time",        sizeof("f2fs_fg_gc_avg_time")-1,        get_fg_gc_avg_time},
    {"f2fs_fg_gc_seg_cnt",         sizeof("f2fs_fg_gc_seg_cnt")-1,         get_fg_gc_seg_cnt},
    {"f2fs_bg_gc_cnt",             sizeof("f2fs_bg_gc_cnt")-1,             get_bg_gc_cnt},
    {"f2fs_bg_gc_avg_time",        sizeof("f2fs_bg_gc_avg_time")-1,        get_bg_gc_avg_time},
    {"f2fs_bg_gc_seg_cnt",         sizeof("f2fs_bg_gc_seg_cnt")-1,         get_bg_gc_seg_cnt},
    {"f2fs_cp_cnt",                sizeof("f2fs_cp_cnt")-1,                get_cp_cnt},
    {"f2fs_cp_avg_time",           sizeof("f2fs_cp_avg_time")-1,           get_cp_avg_time},
    {"f2fs_cp_max_time",           sizeof("f2fs_cp_max_time")-1,           get_cp_max_time},
    {"f2fs_ipu_cnt",               sizeof("f2fs_ipu_cnt")-1,               get_ipu_cnt},
    {"f2fs_fsync_cnt",             sizeof("f2fs_fsync_cnt")-1,             get_fsync_cnt},
    {NULL, 0, NULL}  // Terminator
};

static u64 get_metric_value(const char *file_name) {
    int i;
    int metric_name_len = strlen(file_name);
    // Look up metric in mapping table
    for (i = 0; f2fs_metric_maps[i].name; i++) {
        if (metric_name_len != f2fs_metric_maps[i].name_len)
            continue;

        if (strncmp(file_name, f2fs_metric_maps[i].name, metric_name_len) == 0) {
            return f2fs_metric_maps[i].get_value();
        }
    }

    return 0;
}

static void cb_f2fs_issue_discard(void *ignore, struct block_device *dev,
                                 block_t blkstart, block_t blklen)
{
    if (unlikely(!io_metrics_enabled))
        return;

    atomic64_inc(&f2fs_metrics.discard_cnt);
    atomic64_add(blklen, &f2fs_metrics.discard_len);

    if (unlikely(f2fs_issue_discard_enabled)) {
        io_metrics_print("f2fs_issue_discard: len:%u\n", blklen);
    }
}

#if LINUX_VERSION_CODE <= KERNEL_VERSION(6, 1, 0)
static void cb_f2fs_gc_begin(void *ignore, struct super_block *sb, bool sync,
           bool background, long long dirty_nodes, long long dirty_dents,
           long long dirty_imeta, unsigned int free_sec,
           unsigned int free_seg, int reserved_seg,
           unsigned int prefree_seg)
#else
static void cb_f2fs_gc_begin(void *ignore, struct super_block *sb, int gc_type, bool no_bg_gc,
           unsigned int nr_free_secs,
           long long dirty_nodes, long long dirty_dents,
           long long dirty_imeta, unsigned int free_sec,
           unsigned int free_seg, int reserved_seg,
           unsigned int prefree_seg)
#endif
{
    int current_gc_t;
    if (unlikely(!io_metrics_enabled)) {
        return;
    }
#if LINUX_VERSION_CODE <= KERNEL_VERSION(6, 1, 0)
    atomic_set(&gc_t, background ? GC_BG : GC_FG);
#else
    atomic_set(&gc_t, no_bg_gc ? GC_FG : GC_BG);
#endif

    current_gc_t = atomic_read(&gc_t);
    atomic64_set(&f2fs_gc_metrics[current_gc_t].begin_time, ktime_get_ns());

    if (unlikely(f2fs_gc_begin_enabled)) {
        io_metrics_print("f2fs_gc_begin: gc_type:%d\n", current_gc_t);
    }
}

static void cb_f2fs_gc_end(void *ignore, struct super_block *sb, int ret,
           int seg_freed, int sec_freed, long long dirty_nodes,
           long long dirty_dents, long long dirty_imeta,
           unsigned int free_sec, unsigned int free_seg,
           int reserved_seg, unsigned int prefree_seg)
{
    int current_gc_t;
    u64 begin_time, current_time_ns, gc_elapse;
    if (unlikely(!io_metrics_enabled)) {
        return;
    }
    if (unlikely(!(sb->s_flags & SB_ACTIVE))) {
        return;
    }
    current_gc_t = atomic_read(&gc_t);
    if (unlikely(current_gc_t < 0 || current_gc_t >= TOTAL_GC_TYPES)) {
        io_metrics_print("gc_t(%d) is not a expected value\n", current_gc_t);
        return;
    }

    begin_time = atomic64_read(&f2fs_gc_metrics[current_gc_t].begin_time);
    if (unlikely(begin_time == 0))
        return;

    current_time_ns = ktime_get_ns();
    gc_elapse = current_time_ns - begin_time;

    gc_end_update_stats(current_gc_t, gc_elapse, free_seg);

    atomic64_set(&f2fs_gc_metrics[current_gc_t].begin_time, 0);

    if (unlikely(io_metrics_debug_enabled || f2fs_gc_end_enabled)) {
        const char *gc_type[] = {"Background", "Foreground"};
        u64 cnt = atomic64_read(&f2fs_gc_metrics[current_gc_t].cnt);
        io_metrics_print("%s gc elapse:%llu  count:%llu\n", gc_type[current_gc_t], gc_elapse, cnt);
    }
}

#if LINUX_VERSION_CODE <= KERNEL_VERSION(5, 15, 0)
static void cb_f2fs_write_checkpoint(void *ignore, struct super_block *sb,
                                                 int reason, char *msg)
#else
static void cb_f2fs_write_checkpoint(void *ignore, struct super_block *sb,
                                               int reason, const char *msg)
#endif /* LINUX_VERSION_CODE <= KERNEL_VERSION(5, 15, 0) */
{
    if (unlikely(!io_metrics_enabled)) {
        return;
    }
    if (!msg) {
        return;
    }

    if (strncmp(msg, "start block_ops", 16) == 0) {
        atomic64_set(&f2fs_cp_metrics.begin_time, ktime_get_ns());

#ifdef CONFIG_F2FS_STAT_FS
        atomic_set(&f2fs_cp_metrics.inplace_count, atomic_read(&F2FS_SB(sb)->inplace_count));
#endif
    }
    else if (!strncmp(msg, "finish checkpoint", 18)) {
        u64 begin_time = atomic64_read(&f2fs_cp_metrics.begin_time);
        if (likely(begin_time != 0)) {
            u64 cp_elapse = ktime_get_ns() - begin_time;
            u64 cnt, elapse_time, max_time;

            cnt = atomic64_inc_return(&f2fs_cp_metrics.cnt);
            elapse_time = atomic64_add_return(cp_elapse, &f2fs_cp_metrics.elapse_time);
            max_time = atomic64_read(&f2fs_cp_metrics.max_time);
            if (cp_elapse > max_time) {
                atomic64_set(&f2fs_cp_metrics.max_time, cp_elapse);
            }
            if (cnt > 0) {
                atomic64_set(&f2fs_cp_metrics.avg_time, elapse_time / cnt);
            } else {
                if (cnt != 0) {
                    atomic64_set(&f2fs_cp_metrics.cnt, 0);
                }
                atomic64_set(&f2fs_cp_metrics.avg_time, 0);
                atomic64_set(&f2fs_cp_metrics.elapse_time, 0);
                atomic64_set(&f2fs_cp_metrics.max_time, 0);
                atomic_set(&f2fs_cp_metrics.inplace_count, 0);
            }
            atomic64_set(&f2fs_cp_metrics.begin_time, 0);
            if (unlikely(io_metrics_debug_enabled || f2fs_write_checkpoint_enabled)) {
                io_metrics_print("checkpoint elapse:%llu  count:%lld\n", cp_elapse,
                                                atomic64_read(&f2fs_cp_metrics.cnt));
            }
        }
    }
}

static void cb_f2fs_sync_file_enter(void *ignore, struct inode *inode)
{
    if (unlikely(!io_metrics_enabled)) {
        return;
    }
    atomic64_inc(&f2fs_metrics.fsync_cnt);
    if (unlikely(io_metrics_debug_enabled || f2fs_sync_file_enter_enabled)) {
        io_metrics_print("count:%lld\n", atomic64_read(&f2fs_metrics.fsync_cnt));
    }
}

void f2fs_register_tracepoint_probes(void)
{
    int ret;
    ret = register_trace_f2fs_issue_discard(cb_f2fs_issue_discard, NULL);
    WARN_ON(ret);
    ret = register_trace_f2fs_gc_begin(cb_f2fs_gc_begin, NULL);
    WARN_ON(ret);
    ret = register_trace_f2fs_gc_end(cb_f2fs_gc_end, NULL);
    WARN_ON(ret);
    ret = register_trace_f2fs_write_checkpoint(cb_f2fs_write_checkpoint, NULL);
    WARN_ON(ret);
    ret = register_trace_f2fs_sync_file_enter(cb_f2fs_sync_file_enter, NULL);
    WARN_ON(ret);
}

void f2fs_unregister_tracepoint_probes(void)
{
    unregister_trace_f2fs_issue_discard(cb_f2fs_issue_discard, NULL);
    unregister_trace_f2fs_gc_begin(cb_f2fs_gc_begin, NULL);
    unregister_trace_f2fs_gc_end(cb_f2fs_gc_end, NULL);
    unregister_trace_f2fs_write_checkpoint(cb_f2fs_write_checkpoint, NULL);
    unregister_trace_f2fs_sync_file_enter(cb_f2fs_sync_file_enter, NULL);
}

static int f2fs_metrics_proc_show(struct seq_file *seq_filp, void *data)
{
    u64 value;
    struct file *file = (struct file *)seq_filp->private;
    if (unlikely(!io_metrics_enabled)) {
        seq_printf(seq_filp, "io_metrics_enabled not set to 1:%d\n", io_metrics_enabled);
        return 0;
    }

    if (proc_show_enabled || unlikely(io_metrics_debug_enabled)) {
        io_metrics_print("%s(%d) read %s/%s\n",
            current->comm, current->pid, file->f_path.dentry->d_parent->d_iname,
            file->f_path.dentry->d_iname);
    }
    value = get_metric_value(file->f_path.dentry->d_iname);
    seq_printf(seq_filp, "%llu\n", value);
    return 0;
}

int f2fs_metrics_proc_open(struct inode *inode, struct file *file)
{
    return single_open(file, f2fs_metrics_proc_show, file);
}

void f2fs_metrics_reset(void)
{
    int i;
    for (i = 0; i < TOTAL_GC_TYPES; i++) {
        atomic64_set(&f2fs_gc_metrics[i].elapse_time, 0);
        atomic64_set(&f2fs_gc_metrics[i].begin_time, 0);
        atomic64_set(&f2fs_gc_metrics[i].cnt, 0);
        atomic64_set(&f2fs_gc_metrics[i].avg_time, 0);
        atomic64_set(&f2fs_gc_metrics[i].segs, 0);
        atomic64_set(&f2fs_gc_metrics[i].avg_segs, 0);
        atomic64_set(&f2fs_gc_metrics[i].efficiency, 0);
    }

    atomic64_set(&f2fs_cp_metrics.cnt, 0);
    atomic64_set(&f2fs_cp_metrics.elapse_time, 0);
    atomic64_set(&f2fs_cp_metrics.begin_time, 0);
    atomic64_set(&f2fs_cp_metrics.avg_time, 0);
    atomic64_set(&f2fs_cp_metrics.max_time, 0);
    atomic_set(&f2fs_cp_metrics.inplace_count, 0);

    atomic64_set(&f2fs_metrics.discard_cnt, 0);
    atomic64_set(&f2fs_metrics.discard_len, 0);
    atomic64_set(&f2fs_metrics.fsync_cnt, 0);

    atomic_set(&gc_t, 0);
}

void f2fs_metrics_init(void)
{
    f2fs_metrics_reset();
}