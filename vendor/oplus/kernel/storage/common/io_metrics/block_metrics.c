#include "io_metrics_entry.h"
#include "procfs.h"
#include "block_metrics.h"
#include <trace/events/block.h>
#include <linux/string.h>
#include <linux/seq_file.h>
#include <linux/spinlock.h>
#include <linux/ktime.h>
#include <linux/moduleparam.h>
#include <linux/blkdev.h>
#include <linux/cache.h>

#define BLK_METRICS_LAT(op, size, layer)   \
    atomic64_t blk_metrics_lat_##op##_##size##_##layer[LAT_500M_TO_MAX + 1] = {0};

BLK_METRICS_LAT(read,    4k, in_blk);
BLK_METRICS_LAT(read,    4k, in_drv);
BLK_METRICS_LAT(write,   4k, in_blk);
BLK_METRICS_LAT(write,   4k, in_drv);
BLK_METRICS_LAT(read,  512k, in_blk);
BLK_METRICS_LAT(read,  512k, in_drv);
BLK_METRICS_LAT(write, 512k, in_blk);
BLK_METRICS_LAT(write, 512k, in_drv);

bool block_rq_issue_enabled = false;
bool block_rq_complete_enabled = false;
module_param(block_rq_issue_enabled, bool, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(block_rq_issue_enabled, "Enable block_rq_issue debug (default: false)");
module_param(block_rq_complete_enabled, bool, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(block_rq_complete_enabled, "Enable block_rq_complete debug (default: false)");

struct blk_metrics_struct blk_metrics[OP_MAX][IO_SIZE_MAX] __cacheline_aligned = {0};
spinlock_t blk_metrics_lock[OP_MAX][IO_SIZE_MAX];
// Update latency distribution statistical metrics
static void update_lat_metrics(enum io_op_type op_type, enum io_range io_range,
                             u64 in_block, u64 in_driver) {
    int i;
    struct lat_metrics_map {
        enum io_op_type op;
        enum io_range range;
        atomic64_t *in_blk;
        atomic64_t *in_drv;
    } maps[] = {
        {OP_READ,  IO_SIZE_0_TO_4K,       blk_metrics_lat_read_4k_in_blk,   blk_metrics_lat_read_4k_in_drv},
        {OP_READ,  IO_SIZE_512K_TO_MAX,   blk_metrics_lat_read_512k_in_blk, blk_metrics_lat_read_512k_in_drv},
        {OP_WRITE, IO_SIZE_0_TO_4K,       blk_metrics_lat_write_4k_in_blk,  blk_metrics_lat_write_4k_in_drv},
        {OP_WRITE, IO_SIZE_512K_TO_MAX,   blk_metrics_lat_write_512k_in_blk,blk_metrics_lat_write_512k_in_drv},
    };

    u64 blk_range = LAT_500M_TO_MAX, drv_range = LAT_500M_TO_MAX;
    lat_range_check(in_block, blk_range);
    lat_range_check(in_driver, drv_range);

    for (i = 0; i < ARRAY_SIZE(maps); i++) {
        if (maps[i].op != op_type || maps[i].range != io_range)
            continue;

        atomic64_inc(&maps[i].in_blk[blk_range]);
        atomic64_inc(&maps[i].in_drv[drv_range]);
        break;
    }
}

static void update_metrics(struct blk_metrics_struct *metrics, u32 nr_bytes,
                          u64 in_block, u64 in_driver) {
    metrics->total_cnt++;
    metrics->total_size += nr_bytes;
    metrics->layer[IN_BLOCK].elapse_time += in_block;
    metrics->layer[IN_BLOCK].max_time = max(metrics->layer[IN_BLOCK].max_time, in_block);
    metrics->layer[IN_DRIVER].elapse_time += in_driver;
    metrics->layer[IN_DRIVER].max_time = max(metrics->layer[IN_DRIVER].max_time, in_driver);
    metrics->max_time = max(metrics->max_time, in_block + in_driver);
}

static void block_stat_update(struct request *rq, enum io_op_type op_type,
                     u64 io_complete_time_ns, u64 in_block, u64 in_driver)
{
    struct blk_metrics_struct *metrics;
    unsigned long flags;
    enum io_range io_range = IO_SIZE_MAX;
    u32 nr_bytes = blk_rq_bytes(rq);
    if (nr_bytes >= IO_SIZE_512K_TO_MAX_MASK) {/* [512K, +∞) */
        io_range = IO_SIZE_512K_TO_MAX;
    } else if (nr_bytes > IO_SIZE_128K_TO_512K_MASK) {/* (128K, 512K) */
        io_range = IO_SIZE_128K_TO_512K;
    } else if (nr_bytes > IO_SIZE_32K_TO_128K_MASK) {/* (32K, 128K] */
        io_range = IO_SIZE_32K_TO_128K;
    } else if (nr_bytes > IO_SIZE_4K_TO_32K_MASK) {/* (4K, 32K] */
        io_range = IO_SIZE_4K_TO_32K;
    } else {/* (0, 4K] */
        io_range = IO_SIZE_0_TO_4K;
    }

    metrics = &blk_metrics[op_type][io_range];

    spin_lock_irqsave(&blk_metrics_lock[op_type][io_range], flags);
    update_metrics(metrics, nr_bytes, in_block, in_driver);
    spin_unlock_irqrestore(&blk_metrics_lock[op_type][io_range], flags);

    // Update latency statistics (only focus on read/write of 4K and above 512K)
    if ((op_type == OP_READ || op_type == OP_WRITE) &&
        (io_range == IO_SIZE_0_TO_4K || io_range == IO_SIZE_512K_TO_MAX)) {
        update_lat_metrics(op_type, io_range, in_block, in_driver);
    }
}

#if LINUX_VERSION_CODE <= KERNEL_VERSION(5, 11, 0)
static char *blk_get_disk_name(struct gendisk *hd, int partno, char *buf)
{
    if (!partno)
        snprintf(buf, BDEVNAME_SIZE, "%s", hd->disk_name);
    else if (isdigit(hd->disk_name[strlen(hd->disk_name)-1]))
        snprintf(buf, BDEVNAME_SIZE, "%sp%d", hd->disk_name, partno);
    else
        snprintf(buf, BDEVNAME_SIZE, "%s%d", hd->disk_name, partno);

    return buf;
}
#endif

static void append_char(char *rwbs, int *i, size_t max_len, char c) {
    if (*i < max_len) {
        rwbs[*i] = c;
        (*i)++;
    }
}

static void blk_fill_rwbs_private(char *rwbs, unsigned int op, int bytes) {
    int i = 0;
    const size_t max_len = RWBS_LEN - 1;

    if (op & REQ_PREFLUSH)
        append_char(rwbs, &i, max_len, 'F');

    switch (op & REQ_OP_MASK) {
        case REQ_OP_WRITE:
#if LINUX_VERSION_CODE <= KERNEL_VERSION(6, 1, 0)
        case REQ_OP_WRITE_SAME:
#endif
            append_char(rwbs, &i, max_len, 'W');
            break;

        case REQ_OP_DISCARD:
            append_char(rwbs, &i, max_len, 'D');
            break;

        case REQ_OP_SECURE_ERASE:
            append_char(rwbs, &i, max_len, 'D');
            append_char(rwbs, &i, max_len, 'E');
            break;

        case REQ_OP_FLUSH:
            append_char(rwbs, &i, max_len, 'F');
            break;

        case REQ_OP_READ:
            append_char(rwbs, &i, max_len, 'R');
            break;

        default:
            append_char(rwbs, &i, max_len, 'N');
    }
    if (op & REQ_FUA)
        append_char(rwbs, &i, max_len, 'F');
    if (op & REQ_RAHEAD)
        append_char(rwbs, &i, max_len, 'A');
    if (op & REQ_SYNC)
        append_char(rwbs, &i, max_len, 'S');
    if (op & REQ_META)
        append_char(rwbs, &i, max_len, 'M');

    rwbs[i] = '\0';
}

#if LINUX_VERSION_CODE <= KERNEL_VERSION(5, 15, 0)
static void cb_block_rq_issue(void *ignore, struct request_queue *q,
                             struct request *rq)
#else
static void cb_block_rq_issue(void *ignore, struct request *rq)
#endif
{
    if (unlikely(!io_metrics_enabled)) {
        return ;
    }
    rq->io_start_time_ns = ktime_get_ns();  // Record driver layer start time

    // Debug information printing
    if (unlikely(io_metrics_debug_enabled || block_rq_issue_enabled)) {
#if LINUX_VERSION_CODE <= KERNEL_VERSION(6, 1, 0)
        char *devname = rq->rq_disk ? rq->rq_disk->disk_name : "";
#else
        char *devname = rq->part ? (rq->part->bd_disk ? rq->part->bd_disk->disk_name : "") : "";
#endif
        char rwbs[RWBS_LEN]={};
        unsigned int nr_bytes = blk_rq_bytes(rq);

        blk_fill_rwbs_private(rwbs, rq->cmd_flags, nr_bytes);

        io_metrics_print("dev:%-6s rwbs:%-4s nr_bytes:%-10d " \
          "start_time_ns:%-16llu io_start_time_ns:%-16llu \n",
          devname, rwbs, nr_bytes, rq->start_time_ns, rq->io_start_time_ns);
    }
}

static void print_completion_debug(struct request *rq, unsigned int nr_bytes,
                                  blk_status_t error, u64 complete_time,
                                  u64 in_driver, u64 in_block) {
    char devname[BDEVNAME_SIZE] = {0};
    char rwbs[RWBS_LEN] = {0};
    blk_fill_rwbs_private(rwbs, rq->cmd_flags, nr_bytes);

#if LINUX_VERSION_CODE <= KERNEL_VERSION(5, 11, 0)
    if (rq->bio && rq->bio->bi_disk) {
        blk_get_disk_name(rq->bio->bi_disk, rq->bio->bi_partno, devname);
    }
#endif
    io_metrics_print("dev:%-6s rwbs:%-4s nr_bytes:%-10d error:%-3d "
                    "start_time_ns:%-16llu io_start_time_ns:%-16llu "
                    "io_complete_time_ns:%-16llu in_driver:%-10llu in_block:%-10llu\n",
                    devname, rwbs, nr_bytes, error,
                    rq->start_time_ns, rq->io_start_time_ns, complete_time,
                    in_driver, in_block);
}

#if LINUX_VERSION_CODE <= KERNEL_VERSION(6, 1, 0)
static void cb_block_rq_complete(void *ignore, struct request *rq,
                      int error, unsigned int nr_bytes)
#else
static void cb_block_rq_complete(void *ignore, struct request *rq,
                      blk_status_t error, unsigned int nr_bytes)
#endif
{
    u64 io_complete_time_ns = ktime_get_ns();
    u64 in_driver = ((io_complete_time_ns > rq->io_start_time_ns) && rq->io_start_time_ns) ?
                   (io_complete_time_ns - rq->io_start_time_ns) : 0;
    u64 in_block = ((rq->io_start_time_ns > rq->start_time_ns) && rq->start_time_ns) ?
                  (rq->io_start_time_ns - rq->start_time_ns) : 0;

    if (!nr_bytes)
        return;

    if (unlikely(!io_metrics_enabled)) {
        return;
    }

    switch (rq->cmd_flags & REQ_OP_MASK) {
        case REQ_OP_WRITE:
#if LINUX_VERSION_CODE <= KERNEL_VERSION(6, 1, 0)
        case REQ_OP_WRITE_SAME:
#endif
            if (!error && nr_bytes)
                block_stat_update(rq, OP_WRITE, io_complete_time_ns, in_block, in_driver);
            break;
        case REQ_OP_READ:
            if (!error && nr_bytes)
                block_stat_update(rq, OP_READ, io_complete_time_ns, in_block, in_driver);
            break;
        default:
            break;
    }

    // Debug printing
    if (unlikely(io_metrics_debug_enabled || block_rq_complete_enabled))
        print_completion_debug(rq, nr_bytes, error, io_complete_time_ns, in_driver, in_block);
}

void block_register_tracepoint_probes(void)
{
    int ret;
    ret = register_trace_block_rq_issue(cb_block_rq_issue, NULL);
    WARN_ON(ret);
    ret = register_trace_block_rq_complete(cb_block_rq_complete, NULL);
    WARN_ON(ret);
}

void block_unregister_tracepoint_probes(void)
{
    unregister_trace_block_rq_issue(cb_block_rq_issue, NULL);
    unregister_trace_block_rq_complete(cb_block_rq_complete, NULL);
}

static int handle_cnt(struct seq_file *m, enum io_op_type op)
{
    u64 cnt = 0;
    unsigned long flags;
    int i;
    for (i = 0; i < IO_SIZE_MAX; i++) {
        spin_lock_irqsave(&blk_metrics_lock[op][i], flags);
        cnt += blk_metrics[op][i].total_cnt;
        spin_unlock_irqrestore(&blk_metrics_lock[op][i], flags);
    }
    seq_printf(m, "%llu\n", cnt);
    return 0;
}

static int handle_avg_size(struct seq_file *m, enum io_op_type op)
{
    u64 total_size = 0, total_cnt = 0;
    unsigned long flags;
    int i;
    for (i = 0; i < IO_SIZE_MAX; i++) {
        spin_lock_irqsave(&blk_metrics_lock[op][i], flags);
        total_size += blk_metrics[op][i].total_size;
        total_cnt += blk_metrics[op][i].total_cnt;
        spin_unlock_irqrestore(&blk_metrics_lock[op][i], flags);
    }
    seq_printf(m, "%llu\n", total_cnt ? (total_size / total_cnt) : 0);
    return 0;
}

static int handle_size_dist(struct seq_file *m, enum io_op_type op)
{
    unsigned long flags;
    int i;
    for (i = 0; i < IO_SIZE_MAX; i++) {
        spin_lock_irqsave(&blk_metrics_lock[op][i], flags);
        seq_printf(m, "%llu,", blk_metrics[op][i].total_cnt);
        spin_unlock_irqrestore(&blk_metrics_lock[op][i], flags);
    }
    seq_printf(m, "\n");
    return 0;
}

static int handle_avg_time(struct seq_file *m, enum io_op_type op)
{
    u64 total_time = 0, total_cnt = 0;
    unsigned long flags;
    int i;
    for (i = 0; i < IO_SIZE_MAX; i++) {
        spin_lock_irqsave(&blk_metrics_lock[op][i], flags);
        total_time += blk_metrics[op][i].layer[IN_BLOCK].elapse_time;
        total_time += blk_metrics[op][i].layer[IN_DRIVER].elapse_time;
        total_cnt += blk_metrics[op][i].total_cnt;
        spin_unlock_irqrestore(&blk_metrics_lock[op][i], flags);
    }
    seq_printf(m, "%llu\n", total_cnt ? (total_time / total_cnt) : 0);
    return 0;
}

static int handle_max_time(struct seq_file *m, enum io_op_type op)
{
    u64 max_time = 0;
    unsigned long flags;
    int i;
    for (i = 0; i < IO_SIZE_MAX; i++) {
        spin_lock_irqsave(&blk_metrics_lock[op][i], flags);
        max_time = max(max_time, blk_metrics[op][i].max_time);
        spin_unlock_irqrestore(&blk_metrics_lock[op][i], flags);
    }
    seq_printf(m, "%llu\n", max_time);
    return 0;
}

static int handle_4k_blk_avg_time(struct seq_file *m, enum io_op_type op)
{
    u64 cnt = 0, time = 0;
    unsigned long flags;
    enum io_range range = IO_SIZE_0_TO_4K;
    spin_lock_irqsave(&blk_metrics_lock[op][range], flags);
    cnt = blk_metrics[op][range].total_cnt;
    time = blk_metrics[op][range].layer[IN_BLOCK].elapse_time;
    spin_unlock_irqrestore(&blk_metrics_lock[op][range], flags);
    seq_printf(m, "%llu\n", cnt ? (time / cnt) : 0);
    return 0;
}

static int handle_4k_blk_max_time(struct seq_file *m, enum io_op_type op)
{
    u64 max_time = 0;
    unsigned long flags;
    enum io_range range = IO_SIZE_0_TO_4K;
    spin_lock_irqsave(&blk_metrics_lock[op][range], flags);
    max_time = blk_metrics[op][range].layer[IN_BLOCK].max_time;
    spin_unlock_irqrestore(&blk_metrics_lock[op][range], flags);
    seq_printf(m, "%llu\n", max_time);
    return 0;
}

static int handle_4k_blk_lat_dist(struct seq_file *m, enum io_op_type op)
{
    int i;
    atomic64_t *lat_array = (op == OP_READ) ? blk_metrics_lat_read_4k_in_blk :
                                           blk_metrics_lat_write_4k_in_blk;
    for (i = 0; i <= LAT_500M_TO_MAX; i++)
        seq_printf(m, "%lld,", atomic64_read(&lat_array[i]));
    seq_putc(m, '\n');
    return 0;
}

static int handle_4k_drv_avg_time(struct seq_file *m, enum io_op_type op)
{
    u64 cnt = 0, time = 0;
    unsigned long flags;
    enum io_range range = IO_SIZE_0_TO_4K;
    spin_lock_irqsave(&blk_metrics_lock[op][range], flags);
    cnt = blk_metrics[op][range].total_cnt;
    time = blk_metrics[op][range].layer[IN_DRIVER].elapse_time;
    spin_unlock_irqrestore(&blk_metrics_lock[op][range], flags);
    seq_printf(m, "%llu\n", cnt ? (time / cnt) : 0);
    return 0;
}

static int handle_4k_drv_max_time(struct seq_file *m, enum io_op_type op)
{
    u64 max_time = 0;
    unsigned long flags;
    enum io_range range = IO_SIZE_0_TO_4K;
    spin_lock_irqsave(&blk_metrics_lock[op][range], flags);
    max_time = blk_metrics[op][range].layer[IN_DRIVER].max_time;
    spin_unlock_irqrestore(&blk_metrics_lock[op][range], flags);
    seq_printf(m, "%llu\n", max_time);
    return 0;
}

static int handle_4k_drv_lat_dist(struct seq_file *m, enum io_op_type op)
{
    int i;
    atomic64_t *lat_array = (op == OP_READ) ? blk_metrics_lat_read_4k_in_drv :
                                           blk_metrics_lat_write_4k_in_drv;
    for (i = 0; i <= LAT_500M_TO_MAX; i++)
        seq_printf(m, "%lld,", atomic64_read(&lat_array[i]));
    seq_putc(m, '\n');
    return 0;
}

static int handle_512k_blk_avg_time(struct seq_file *m, enum io_op_type op)
{
    u64 cnt = 0, time = 0;
    unsigned long flags;
    enum io_range range = IO_SIZE_512K_TO_MAX;
    spin_lock_irqsave(&blk_metrics_lock[op][range], flags);
    cnt = blk_metrics[op][range].total_cnt;
    time = blk_metrics[op][range].layer[IN_BLOCK].elapse_time;
    spin_unlock_irqrestore(&blk_metrics_lock[op][range], flags);
    seq_printf(m, "%llu\n", cnt ? (time / cnt) : 0);
    return 0;
}

static int handle_512k_blk_max_time(struct seq_file *m, enum io_op_type op)
{
    u64 max_time = 0;
    unsigned long flags;
    enum io_range range = IO_SIZE_512K_TO_MAX;
    spin_lock_irqsave(&blk_metrics_lock[op][range], flags);
    max_time = blk_metrics[op][range].layer[IN_BLOCK].max_time;
    spin_unlock_irqrestore(&blk_metrics_lock[op][range], flags);
    seq_printf(m, "%llu\n", max_time);
    return 0;
}

static int handle_512k_blk_lat_dist(struct seq_file *m, enum io_op_type op) {
    int i;
    atomic64_t *lat_array = (op == OP_READ) ? blk_metrics_lat_read_512k_in_blk :
                                           blk_metrics_lat_write_512k_in_blk;
    for (i = 0; i <= LAT_500M_TO_MAX; i++)
        seq_printf(m, "%lld,", atomic64_read(&lat_array[i]));
    seq_putc(m, '\n');
    return 0;
}

static int handle_512k_drv_avg_time(struct seq_file *m, enum io_op_type op)
{
    u64 cnt = 0, time = 0;
    unsigned long flags;
    enum io_range range = IO_SIZE_512K_TO_MAX;
    spin_lock_irqsave(&blk_metrics_lock[op][range], flags);
    cnt = blk_metrics[op][range].total_cnt;
    time = blk_metrics[op][range].layer[IN_DRIVER].elapse_time;
    spin_unlock_irqrestore(&blk_metrics_lock[op][range], flags);
    seq_printf(m, "%llu\n", cnt ? (time / cnt) : 0);
    return 0;
}

static int handle_512k_drv_max_time(struct seq_file *m, enum io_op_type op)
{
    u64 max_time = 0;
    unsigned long flags;
    enum io_range range = IO_SIZE_512K_TO_MAX;
    spin_lock_irqsave(&blk_metrics_lock[op][range], flags);
    max_time = blk_metrics[op][range].layer[IN_DRIVER].max_time;
    spin_unlock_irqrestore(&blk_metrics_lock[op][range], flags);
    seq_printf(m, "%llu\n", max_time);
    return 0;
}

static int handle_512k_drv_lat_dist(struct seq_file *m, enum io_op_type op) {
    int i;
    atomic64_t *lat_array = (op == OP_READ) ? blk_metrics_lat_read_512k_in_drv :
                                           blk_metrics_lat_write_512k_in_drv;
    for (i = 0; i <= LAT_500M_TO_MAX; i++)
        seq_printf(m, "%lld,", atomic64_read(&lat_array[i]));
    seq_putc(m, '\n');
    return 0;
}

typedef int (*metric_handler_func)(struct seq_file *, enum io_op_type);

/* Metric mapping table - using the same style as ufs_metric_maps */
static const struct {
    const char *name;
    size_t name_len;
    enum io_op_type op;
    metric_handler_func handler;
} blk_metric_maps[] = {
    {"bio_read_cnt",               sizeof("bio_read_cnt")-1,                 OP_READ,  handle_cnt,             },
    {"bio_read_avg_size",          sizeof("bio_read_avg_size")-1,            OP_READ,  handle_avg_size,        },
    {"bio_read_size_dist",         sizeof("bio_read_size_dist")-1,           OP_READ,  handle_size_dist,       },
    {"bio_read_avg_time",          sizeof("bio_read_avg_time")-1,            OP_READ,  handle_avg_time,        },
    {"bio_read_max_time",          sizeof("bio_read_max_time")-1,            OP_READ,  handle_max_time,        },
    {"bio_read_4k_blk_avg_time",   sizeof("bio_read_4k_blk_avg_time")-1,     OP_READ,  handle_4k_blk_avg_time, },
    {"bio_read_4k_blk_max_time",   sizeof("bio_read_4k_blk_max_time")-1,     OP_READ,  handle_4k_blk_max_time, },
    {"bio_read_4k_blk_lat_dist",   sizeof("bio_read_4k_blk_lat_dist")-1,     OP_READ,  handle_4k_blk_lat_dist, },
    {"bio_read_4k_drv_avg_time",   sizeof("bio_read_4k_drv_avg_time")-1,     OP_READ,  handle_4k_drv_avg_time, },
    {"bio_read_4k_drv_max_time",   sizeof("bio_read_4k_drv_max_time")-1,     OP_READ,  handle_4k_drv_max_time, },
    {"bio_read_4k_drv_lat_dist",   sizeof("bio_read_4k_drv_lat_dist")-1,     OP_READ,  handle_4k_drv_lat_dist, },
    {"bio_read_512k_blk_avg_time", sizeof("bio_read_512k_blk_avg_time")-1,   OP_READ,  handle_512k_blk_avg_time},
    {"bio_read_512k_blk_max_time", sizeof("bio_read_512k_blk_max_time")-1,   OP_READ,  handle_512k_blk_max_time},
    {"bio_read_512k_blk_lat_dist", sizeof("bio_read_512k_blk_lat_dist")-1,   OP_READ,  handle_512k_blk_lat_dist},
    {"bio_read_512k_drv_avg_time", sizeof("bio_read_512k_drv_avg_time")-1,   OP_READ,  handle_512k_drv_avg_time},
    {"bio_read_512k_drv_max_time", sizeof("bio_read_512k_drv_max_time")-1,   OP_READ,  handle_512k_drv_max_time},
    {"bio_read_512k_drv_lat_dist", sizeof("bio_read_512k_drv_lat_dist")-1,   OP_READ,  handle_512k_drv_lat_dist},

    {"bio_write_cnt",               sizeof("bio_write_cnt")-1,               OP_WRITE, handle_cnt,             },
    {"bio_write_avg_size",          sizeof("bio_write_avg_size")-1,          OP_WRITE, handle_avg_size,        },
    {"bio_write_size_dist",         sizeof("bio_write_size_dist")-1,         OP_WRITE, handle_size_dist,       },
    {"bio_write_avg_time",          sizeof("bio_write_avg_time")-1,          OP_WRITE, handle_avg_time,        },
    {"bio_write_max_time",          sizeof("bio_write_max_time")-1,          OP_WRITE, handle_max_time,        },
    {"bio_write_4k_blk_avg_time",   sizeof("bio_write_4k_blk_avg_time")-1,   OP_WRITE, handle_4k_blk_avg_time, },
    {"bio_write_4k_blk_max_time",   sizeof("bio_write_4k_blk_max_time")-1,   OP_WRITE, handle_4k_blk_max_time, },
    {"bio_write_4k_blk_lat_dist",   sizeof("bio_write_4k_blk_lat_dist")-1,   OP_WRITE, handle_4k_blk_lat_dist, },
    {"bio_write_4k_drv_avg_time",   sizeof("bio_write_4k_drv_avg_time")-1,   OP_WRITE, handle_4k_drv_avg_time, },
    {"bio_write_4k_drv_max_time",   sizeof("bio_write_4k_drv_max_time")-1,   OP_WRITE, handle_4k_drv_max_time, },
    {"bio_write_4k_drv_lat_dist",   sizeof("bio_write_4k_drv_lat_dist")-1,   OP_WRITE, handle_4k_drv_lat_dist, },
    {"bio_write_512k_blk_avg_time", sizeof("bio_write_512k_blk_avg_time")-1, OP_WRITE, handle_512k_blk_avg_time},
    {"bio_write_512k_blk_max_time", sizeof("bio_write_512k_blk_max_time")-1, OP_WRITE, handle_512k_blk_max_time},
    {"bio_write_512k_blk_lat_dist", sizeof("bio_write_512k_blk_lat_dist")-1, OP_WRITE, handle_512k_blk_lat_dist},
    {"bio_write_512k_drv_avg_time", sizeof("bio_write_512k_drv_avg_time")-1, OP_WRITE, handle_512k_drv_avg_time},
    {"bio_write_512k_drv_max_time", sizeof("bio_write_512k_drv_max_time")-1, OP_WRITE, handle_512k_drv_max_time},
    {"bio_write_512k_drv_lat_dist", sizeof("bio_write_512k_drv_lat_dist")-1, OP_WRITE, handle_512k_drv_lat_dist},

    {NULL, 0, OP_MAX, NULL}  // Terminator
};

static int get_metric_value(struct seq_file *seq, const char *metric_name)
{
    int i;
    int metric_name_len = strlen(metric_name);
    for (i = 0; blk_metric_maps[i].name; i++) {
        if (metric_name_len != blk_metric_maps[i].name_len)
            continue;

        if (strncmp(metric_name, blk_metric_maps[i].name, metric_name_len) == 0) {
            return blk_metric_maps[i].handler(seq, blk_metric_maps[i].op);
        }
    }
    io_metrics_print("unknown metric: %s\n", metric_name);
    return -EINVAL;
}

static int block_metrics_proc_show(struct seq_file *m, void *data)
{
    struct file *file = m->private;
    const char *node_name;

    if (unlikely(!io_metrics_enabled)) {
        seq_printf(m, "io_metrics_enabled not set to 1:%d\n", io_metrics_enabled);
        return 0;
    }

    if (!file)
        return -EINVAL;

    if (!file->f_path.dentry || !file->f_path.dentry->d_parent)
        return -ENOENT;

    node_name = file->f_path.dentry->d_iname;
    if (proc_show_enabled || unlikely(io_metrics_debug_enabled)) {
        io_metrics_print("%s(%d) read %s/%s\n",
            current->comm, current->pid,
            file->f_path.dentry->d_parent->d_iname,
            file->f_path.dentry->d_iname);
    }
    return get_metric_value(m, node_name);
}

int block_metrics_proc_open(struct inode *inode, struct file *file)
{
    return single_open(file, block_metrics_proc_show, file);
}

void block_metrics_reset(void)
{
    unsigned long flags;
    int op, size;

    for (op = 0; op < OP_MAX; op++) {
        for (size = 0; size < IO_SIZE_MAX; size++) {
            spin_lock_irqsave(&blk_metrics_lock[op][size], flags);
            memset(&blk_metrics[op][size], 0, sizeof(struct blk_metrics_struct));
            spin_unlock_irqrestore(&blk_metrics_lock[op][size], flags);
        }
    }

    io_metrics_print("Reset block metrics (size: %lu bytes)\n",
                   (unsigned long)sizeof(blk_metrics));

    memset(blk_metrics_lat_read_4k_in_blk, 0, sizeof(blk_metrics_lat_read_4k_in_blk));
    memset(blk_metrics_lat_read_4k_in_drv, 0, sizeof(blk_metrics_lat_read_4k_in_drv));
    memset(blk_metrics_lat_write_4k_in_blk, 0, sizeof(blk_metrics_lat_write_4k_in_blk));
    memset(blk_metrics_lat_write_4k_in_drv, 0, sizeof(blk_metrics_lat_write_4k_in_drv));
    memset(blk_metrics_lat_read_512k_in_blk, 0, sizeof(blk_metrics_lat_read_512k_in_blk));
    memset(blk_metrics_lat_read_512k_in_drv, 0, sizeof(blk_metrics_lat_read_512k_in_drv));
    memset(blk_metrics_lat_write_512k_in_blk, 0, sizeof(blk_metrics_lat_write_512k_in_blk));
    memset(blk_metrics_lat_write_512k_in_drv, 0, sizeof(blk_metrics_lat_write_512k_in_drv));
}


void block_metrics_init(void)
{
    int op, size;
    for (op = 0; op < OP_MAX; op++) {
        for (size = 0; size < IO_SIZE_MAX; size++) {
            spin_lock_init(&blk_metrics_lock[op][size]);
        }
    }
    block_metrics_reset();
}