#include "io_metrics_entry.h"
#include "procfs.h"
#if LINUX_VERSION_CODE <= KERNEL_VERSION(6, 1, 0)
#include "drivers/scsi/ufs/ufshcd.h"
#else
#include <scsi/scsi_cmnd.h>
#include <ufs/ufshcd.h>
#endif
#include "ufs_metrics.h"
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0)
#include <trace/hooks/ufshcd.h>
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 0)
#include <trace/hooks/oplus_ufs.h>
#endif

#define UFS_METRICS_LAT(op)   \
    atomic64_t ufs_metrics_lat_##op[LAT_500M_TO_MAX + 1] __cacheline_aligned = {0};

#ifdef CONFIG_OPLUS_FEATURE_STORAGE_IOLATENCY_STATS
/* Common latency range definitions */
#define LATENCY_DIST_SIZE 5        /* 5 latency ranges: 0-2ms, 2-20ms, 20-100ms, 100-500ms, >500ms */

/* Latency thresholds in nanoseconds */
#define LAT_THRESHOLD_2MS   (2ULL * 1000 * 1000)
#define LAT_THRESHOLD_20MS  (20ULL * 1000 * 1000)
#define LAT_THRESHOLD_100MS (100ULL * 1000 * 1000)
#define LAT_THRESHOLD_500MS (500ULL * 1000 * 1000)

/* Time unit definition */
#define NS_PER_100MS (100ULL * 1000 * 1000)

/*
 * Circular buffer IO latency metrics slot structure
 * Each slot records statistics for one time unit and uses epoch to track validity
 */
struct io_latency_slot {
    atomic64_t read_dist[LATENCY_DIST_SIZE];
    atomic64_t write_dist[LATENCY_DIST_SIZE];
    atomic64_t read_sz[LATENCY_DIST_SIZE];
    atomic64_t write_sz[LATENCY_DIST_SIZE];
    atomic64_t epoch;      /* Epoch number to track data validity in circular buffer */
    char padding[48];
};

/* 100ms granularity: 10 seconds circular buffer (100 slots)
 * This unified buffer serves both fine-grained (100ms) and coarse-grained (1s) statistics
 * by aggregating different numbers of slots during read operations.
 */
#define TEN_SECOND_100MS_SIZE 100  /* 10 seconds = 100 * 100ms slots */
struct io_latency_slot per_100ms_ufs_metrics[TEN_SECOND_100MS_SIZE] __cacheline_aligned;

/**
 * get_latency_index - Determine the latency range index
 * @elapsed_ns: IO latency in nanoseconds
 *
 * Returns: Index 0-4 for the corresponding latency range
 *   [0]: 0 ~ 2ms
 *   [1]: 2ms ~ 20ms
 *   [2]: 20ms ~ 100ms
 *   [3]: 100ms ~ 500ms
 *   [4]: > 500ms
 */
static inline int get_latency_index(ktime_t elapsed_ns)
{
    if (elapsed_ns <= LAT_THRESHOLD_2MS)
        return 0;
    if (elapsed_ns <= LAT_THRESHOLD_20MS)
        return 1;
    if (elapsed_ns <= LAT_THRESHOLD_100MS)
        return 2;
    if (elapsed_ns <= LAT_THRESHOLD_500MS)
        return 3;
    return 4;
}

/**
 * clear_latency_slot - Clear a latency slot's data
 * @slot: Pointer to the io_latency_slot to clear
 */
static inline void clear_latency_slot(struct io_latency_slot *slot)
{
    int i;
    for (i = 0; i < LATENCY_DIST_SIZE; i++) {
        atomic64_set(&slot->read_dist[i], 0);
        atomic64_set(&slot->write_dist[i], 0);
        atomic64_set(&slot->read_sz[i], 0);
        atomic64_set(&slot->write_sz[i], 0);
    }
}

/**
 * fill_latency_slot_circular - Record IO latency into a circular buffer slot
 * @slot: Pointer to the io_latency_slot
 * @current_epoch: Current epoch number for this slot
 * @elapsed_ns: IO latency in nanoseconds
 * @size: IO transfer size in bytes
 * @is_read: true for read, false for write
 *
 * This function handles circular buffer logic:
 * - If the slot's epoch differs from current_epoch, clear and update epoch
 * - Otherwise, accumulate statistics into the existing slot
 */
static inline void fill_latency_slot_circular(struct io_latency_slot *slot,
                                              u64 current_epoch,
                                              ktime_t elapsed_ns, u64 size, bool is_read)
{
    int lat_idx = get_latency_index(elapsed_ns);
    u64 slot_epoch;

    /* Atomically check and update epoch to avoid race condition */
    slot_epoch = atomic64_read(&slot->epoch);
    if (slot_epoch != current_epoch) {
        /* Use cmpxchg to atomically update epoch, only one thread will succeed */
        if (atomic64_cmpxchg(&slot->epoch, slot_epoch, current_epoch) == slot_epoch) {
            /* Only the thread that successfully updated epoch clears the slot */
            clear_latency_slot(slot);
        }
        /* Other threads skip clear and directly update stats */
    }

    /* Update statistics */
    if (is_read) {
        atomic64_inc(&slot->read_dist[lat_idx]);
        atomic64_add(size, &slot->read_sz[lat_idx]);
    } else {
        atomic64_inc(&slot->write_dist[lat_idx]);
        atomic64_add(size, &slot->write_sz[lat_idx]);
    }
}

/**
 * update_iolatency_stats - Update 100ms granularity IO latency statistics
 * @current_time_ns: Current timestamp in nanoseconds
 * @elapsed_ns: IO latency in nanoseconds
 * @size: IO transfer size in bytes
 * @is_read: true for read, false for write
 *
 * Uses circular buffer with epoch-based validity tracking:
 * - slot_index = (time / time_unit) % buffer_size
 * - epoch = (time / time_unit) / buffer_size
 *
 * The unified 100ms buffer (100 slots = 10 seconds) can serve both
 * fine-grained and coarse-grained statistics by aggregating different
 * numbers of slots during read operations.
 */
static void update_iolatency_stats(u64 current_time_ns, ktime_t elapsed_ns,
                                   u64 size, bool is_read)
{
    u64 time_unit_100ms;
    int slot_idx_100ms;
    u64 epoch_100ms;

    /* 100ms granularity circular buffer */
    time_unit_100ms = current_time_ns / NS_PER_100MS;
    slot_idx_100ms = time_unit_100ms % TEN_SECOND_100MS_SIZE;
    epoch_100ms = time_unit_100ms / TEN_SECOND_100MS_SIZE;
    fill_latency_slot_circular(&per_100ms_ufs_metrics[slot_idx_100ms],
                               epoch_100ms, elapsed_ns, size, is_read);
}
#endif /* CONFIG_OPLUS_FEATURE_STORAGE_IOLATENCY_STATS */

UFS_METRICS_LAT(write);
UFS_METRICS_LAT(read);

bool ufs_compl_command_enabled = false;
module_param(ufs_compl_command_enabled, bool, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(ufs_compl_command_enabled, " Debug android_vh_ufs_compl_command");

/* UFS performance statistics structure */
struct {
    atomic64_t read_size;      // Total read data size
    atomic64_t read_cnt;       // Number of read operations
    atomic64_t read_elapse;    // Total read time (ns)
    atomic64_t write_size;     // Total write data size
    atomic64_t write_cnt;      // Number of write operations
    atomic64_t write_elapse;   // Total write time (ns)
    char padding[16];          // Cache line alignment
} ufs_metrics __cacheline_aligned = {0};

/* Metric acquisition function implementations */
static inline u64 get_ufs_total_read_size_mb(void)
{
    return atomic64_read(&ufs_metrics.read_size) >> 20;
}

static inline u64 get_ufs_total_read_time_ms(void)
{
    return atomic64_read(&ufs_metrics.read_elapse) / NSEC_PER_MSEC;
}

static void get_ufs_read_lat_dist(struct seq_file *seq)
{
    int i;
    for (i = 0; i <= LAT_500M_TO_MAX; i++) {
        seq_printf(seq, "%llu,", atomic64_read(&ufs_metrics_lat_read[i]));
    }
    seq_putc(seq, '\n');
}

static inline u64 get_ufs_total_write_size_mb(void)
{
    return atomic64_read(&ufs_metrics.write_size) >> 20;
}

static inline u64 get_ufs_total_write_time_ms(void)
{
    return atomic64_read(&ufs_metrics.write_elapse) / NSEC_PER_MSEC;
}

static void get_ufs_write_lat_dist(struct seq_file *seq)
{
    int i;
    for (i = 0; i <= LAT_500M_TO_MAX; i++) {
        seq_printf(seq, "%llu,", atomic64_read(&ufs_metrics_lat_write[i]));
    }
    seq_putc(seq, '\n');
}

/* Define metric function types */
typedef u64 (*metric_value_func)(void);
typedef void (*metric_seq_func)(struct seq_file *seq);

/* Define metric mapping table using anonymous structure */
static const struct {
    const char *name;
    size_t name_len;
    enum { VALUE_FUNC, SEQ_FUNC } type;
    union {
        metric_value_func value_func;
        metric_seq_func seq_func;
    } func;
} ufs_metric_maps[] = {
    { "ufs_total_read_size_mb",  sizeof("ufs_total_read_size_mb") - 1,  VALUE_FUNC, { .value_func = get_ufs_total_read_size_mb }  },
    { "ufs_total_read_time_ms",  sizeof("ufs_total_read_time_ms") - 1,  VALUE_FUNC, { .value_func = get_ufs_total_read_time_ms }  },
    { "ufs_read_lat_dist",       sizeof("ufs_read_lat_dist") - 1,       SEQ_FUNC,   { .seq_func = get_ufs_read_lat_dist }         },
    { "ufs_total_write_size_mb", sizeof("ufs_total_write_size_mb") - 1, VALUE_FUNC, { .value_func = get_ufs_total_write_size_mb } },
    { "ufs_total_write_time_ms", sizeof("ufs_total_write_time_ms") - 1, VALUE_FUNC, { .value_func = get_ufs_total_write_time_ms } },
    { "ufs_write_lat_dist",      sizeof("ufs_write_lat_dist") - 1,      SEQ_FUNC,   { .seq_func = get_ufs_write_lat_dist }        },
    { NULL, 0, 0, { .value_func = NULL } } /* Terminator */
};

/* Encapsulate metric lookup and value acquisition logic */
static int get_metric_value(struct seq_file *seq, const char *metric_name)
{
    int i;
    int metric_name_len = strlen(metric_name);
    for (i = 0; ufs_metric_maps[i].name; i++) {
        if (metric_name_len != ufs_metric_maps[i].name_len)
            continue;

        if (strncmp(metric_name, ufs_metric_maps[i].name, metric_name_len) == 0) {
            if (ufs_metric_maps[i].type == VALUE_FUNC) {
                u64 value = ufs_metric_maps[i].func.value_func();
                seq_printf(seq, "%llu\n", value);
            } else {
                ufs_metric_maps[i].func.seq_func(seq);
            }
            return 0;
        }
    }

    io_metrics_print("unknown metric: %s\n", metric_name);
    return -EINVAL;
}

/* Generic function: Update UFS statistics */
static void update_ufs_metrics(bool is_read, int transfer_len, ktime_t elapsed_in_ufs)
{
    u64 ufs_lat_range = 0;

    if (is_read) {
        atomic64_inc(&ufs_metrics.read_cnt);
        atomic64_add(transfer_len, &ufs_metrics.read_size);
        atomic64_add(elapsed_in_ufs, &ufs_metrics.read_elapse);
    } else {
        atomic64_inc(&ufs_metrics.write_cnt);
        atomic64_add(transfer_len, &ufs_metrics.write_size);
        atomic64_add(elapsed_in_ufs, &ufs_metrics.write_elapse);
    }

    lat_range_check(elapsed_in_ufs, ufs_lat_range);
    if (is_read) {
        atomic64_inc(&ufs_metrics_lat_read[ufs_lat_range]);
    } else {
        atomic64_inc(&ufs_metrics_lat_write[ufs_lat_range]);
    }
}

/* UFS command completion callback function */
void cb_android_vh_ufs_compl_command(void *ignore, struct ufs_hba *hba,
                                     struct ufshcd_lrb *lrbp)
{
    ktime_t elapsed_in_ufs;
    int transfer_len = 0;

    if (unlikely(!io_metrics_enabled)) {
        return;
    }
    if (!lrbp->cmd) {
        return;
    }
    /*complete*/
    elapsed_in_ufs = lrbp->compl_time_stamp - lrbp->issue_time_stamp;
    switch (lrbp->cmd->cmnd[0]) {
        case READ_6:
        case READ_10:
        case READ_16:
        {
            transfer_len = be32_to_cpu(lrbp->ucd_req_ptr->sc.exp_data_transfer_len);
#ifdef CONFIG_OPLUS_FEATURE_STORAGE_IOLATENCY_STATS
            update_iolatency_stats(lrbp->compl_time_stamp, elapsed_in_ufs, transfer_len, true);
#endif /* CONFIG_OPLUS_FEATURE_STORAGE_IOLATENCY_STATS */
            update_ufs_metrics(true, transfer_len, elapsed_in_ufs);
            if (unlikely(ufs_compl_command_enabled || io_metrics_debug_enabled)) {
                io_metrics_print("read %d bytes cost %llu ns\n",
                                 transfer_len, elapsed_in_ufs);
            }
            return;
        }
        case WRITE_6:
        case WRITE_10:
        case WRITE_16:
        {
            transfer_len = be32_to_cpu(lrbp->ucd_req_ptr->sc.exp_data_transfer_len);
#ifdef CONFIG_OPLUS_FEATURE_STORAGE_IOLATENCY_STATS
            update_iolatency_stats(lrbp->compl_time_stamp, elapsed_in_ufs, transfer_len, false);
#endif /* CONFIG_OPLUS_FEATURE_STORAGE_IOLATENCY_STATS */
            update_ufs_metrics(false, transfer_len, elapsed_in_ufs);
            if (unlikely(ufs_compl_command_enabled || io_metrics_debug_enabled)) {
                io_metrics_print("write %d bytes cost %llu ns\n",
                                 transfer_len, elapsed_in_ufs);
            }
            return;
        }
        case UNMAP:
            if (unlikely(ufs_compl_command_enabled || io_metrics_debug_enabled)) {
                transfer_len = be32_to_cpu(lrbp->ucd_req_ptr->sc.exp_data_transfer_len);
                io_metrics_print("unmap %d bytes cost %llu ns\n",
                                 transfer_len, elapsed_in_ufs);
            }
            return;
        default:
            return;
    }
}

void ufs_register_tracepoint_probes(void)
{
    int ret = 0;
#if LINUX_VERSION_CODE > KERNEL_VERSION(5, 4, 0)
    ret = register_trace_android_vh_ufs_compl_command(cb_android_vh_ufs_compl_command, NULL);
    WARN_ON(ret);
#endif
    io_metrics_print("run:%d\n", ret);
}

void ufs_unregister_tracepoint_probes(void)
{
#if LINUX_VERSION_CODE > KERNEL_VERSION(5, 4, 0)
    unregister_trace_android_vh_ufs_compl_command(cb_android_vh_ufs_compl_command, NULL);
#endif
    return;
}

/* Handle metric queries from proc filesystem */
static int ufs_metrics_proc_show(struct seq_file *seq_filp, void *data)
{
    struct file *file = (struct file *)seq_filp->private;
    const char *file_name;
    if (unlikely(!io_metrics_enabled)) {
        seq_printf(seq_filp, "io_metrics_enabled not set to 1:%d\n", io_metrics_enabled);
        return 0;
    }

    if (!file)
        return -EINVAL;

    if (!file->f_path.dentry || !file->f_path.dentry->d_parent)
        return -ENOENT;

    if (proc_show_enabled || unlikely(io_metrics_debug_enabled)) {
        io_metrics_print("%s(%d) read %s/%s\n",
            current->comm, current->pid, file->f_path.dentry->d_parent->d_iname,
            file->f_path.dentry->d_iname);
    }
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 0)
    file_name = file->f_path.dentry->d_iname;
    return get_metric_value(seq_filp, file_name);
#else
    seq_printf(seq_filp, "%d\n", 0);
    return 0;
#endif
}

int ufs_metrics_proc_open(struct inode *inode, struct file *file)
{
    return single_open(file, ufs_metrics_proc_show, file);
}

#ifdef CONFIG_OPLUS_FEATURE_STORAGE_IOLATENCY_STATS
/**
 * get_100ms_dist - Get accumulated IO count from 100ms circular buffer
 * @lat_idx: Latency range index (0-4)
 * @rw: true for read, false for write
 * @window_count: Number of 100ms windows to accumulate
 *
 * Uses epoch-based validation to ensure only valid data is accumulated
 * Returns: Accumulated IO count
 */
static unsigned long long get_100ms_dist(int lat_idx, bool rw, int window_count)
{
    unsigned long long sum = 0;
    u64 current_time_ns = ktime_get_raw_ns();
    u64 current_unit = current_time_ns / NS_PER_100MS;
    u64 current_epoch = current_unit / TEN_SECOND_100MS_SIZE;
    int current_slot_idx = current_unit % TEN_SECOND_100MS_SIZE;
    int i;

    if (lat_idx < 0 || lat_idx >= LATENCY_DIST_SIZE)
        return 0;

    if (window_count > TEN_SECOND_100MS_SIZE)
        window_count = TEN_SECOND_100MS_SIZE;

    /* Accumulate data from recent windows (going backwards from current) */
    for (i = 0; i < window_count; i++) {
        int slot_idx = current_slot_idx - i;
        u64 expected_epoch = current_epoch;

        if (slot_idx < 0) {
            slot_idx += TEN_SECOND_100MS_SIZE;  /* Wrap around */
            expected_epoch--;  /* Previous epoch for wrapped slots */
        }

        /* Skip slots with stale epoch */
        if (atomic64_read(&per_100ms_ufs_metrics[slot_idx].epoch) != expected_epoch)
            continue;

        if (rw) {
            sum += atomic64_read(&per_100ms_ufs_metrics[slot_idx].read_dist[lat_idx]);
        } else {
            sum += atomic64_read(&per_100ms_ufs_metrics[slot_idx].write_dist[lat_idx]);
        }
    }
    return sum;
}

/**
 * get_100ms_size - Get accumulated IO size from 100ms circular buffer
 * @lat_idx: Latency range index (0-4)
 * @rw: true for read, false for write
 * @window_count: Number of 100ms windows to accumulate
 *
 * Uses epoch-based validation to ensure only valid data is accumulated
 * Returns: Accumulated IO size in MB
 */
static unsigned long long get_100ms_size(int lat_idx, bool rw, int window_count)
{
    unsigned long long sum = 0;
    u64 current_time_ns = ktime_get_raw_ns();
    u64 current_unit = current_time_ns / NS_PER_100MS;
    u64 current_epoch = current_unit / TEN_SECOND_100MS_SIZE;
    int current_slot_idx = current_unit % TEN_SECOND_100MS_SIZE;
    int i;

    if (lat_idx < 0 || lat_idx >= LATENCY_DIST_SIZE)
        return 0;

    if (window_count > TEN_SECOND_100MS_SIZE)
        window_count = TEN_SECOND_100MS_SIZE;

    /* Accumulate data from recent windows (going backwards from current) */
    for (i = 0; i < window_count; i++) {
        int slot_idx = current_slot_idx - i;
        u64 expected_epoch = current_epoch;

        if (slot_idx < 0) {
            slot_idx += TEN_SECOND_100MS_SIZE;  /* Wrap around */
            expected_epoch--;  /* Previous epoch for wrapped slots */
        }

        /* Skip slots with stale epoch */
        if (atomic64_read(&per_100ms_ufs_metrics[slot_idx].epoch) != expected_epoch)
            continue;

        if (rw) {
            sum += atomic64_read(&per_100ms_ufs_metrics[slot_idx].read_sz[lat_idx]);
        } else {
            sum += atomic64_read(&per_100ms_ufs_metrics[slot_idx].write_sz[lat_idx]);
        }
    }
    return sum / (1024 * 1024);  /* Convert to MB */
}

/**
 * io_dist_stats_show - Show IO latency distribution statistics
 * @seq_filp: seq_file pointer
 * @window_count: Number of 100ms windows to accumulate
 *
 * Output format: 6 comma-separated values on a single line
 *   Values 1-5: Read IO count for each latency range
 *   Value 6: Total read IO size in MB
 */
static void io_dist_stats_show(struct seq_file *seq_filp, int window_count)
{
    int i;
    unsigned long long total_size = 0;

    /* Print read IO counts for each latency range */
    for (i = 0; i < LATENCY_DIST_SIZE; i++) {
        seq_printf(seq_filp, "%llu,", get_100ms_dist(i, true, window_count));
        total_size += get_100ms_size(i, true, window_count);
    }

    /* Print total read IO size */
    seq_printf(seq_filp, "%llu\n", total_size);
}

/* 500ms statistics: 5 windows */
static int io_dist_stats_500ms_show(struct seq_file *seq_filp, void *data)
{
    io_dist_stats_show(seq_filp, 5);
    return 0;
}

int io_dist_stats_500ms_proc_open(struct inode *inode, struct file *file)
{
    return single_open(file, io_dist_stats_500ms_show, file);
}

/* 2s statistics: 20 windows */
static int io_dist_stats_2s_show(struct seq_file *seq_filp, void *data)
{
    io_dist_stats_show(seq_filp, 20);
    return 0;
}

int io_dist_stats_2s_proc_open(struct inode *inode, struct file *file)
{
    return single_open(file, io_dist_stats_2s_show, file);
}

/* 5s statistics: 50 windows */
static int io_dist_stats_5s_show(struct seq_file *seq_filp, void *data)
{
    io_dist_stats_show(seq_filp, 50);  /* 50 * 100ms = 5s */
    return 0;
}

int io_dist_stats_5s_proc_open(struct inode *inode, struct file *file)
{
    return single_open(file, io_dist_stats_5s_show, file);
}

/**
 * iolatencystatshow - Show IO latency statistics for 10 seconds window
 * @seq_filp: seq_file pointer
 * @data: unused
 *
 * This function replaces the old 1-second granularity implementation
 * by aggregating all 100 slots (10 seconds) of 100ms granularity data.
 * Output format: 10 lines, each line contains "count, size_mb"
 *   Lines 1-5: Read statistics for latency ranges 0-4
 *   Lines 6-10: Write statistics for latency ranges 0-4
 */
static int iolatencystatshow(struct seq_file *seq_filp, void *data)
{
    int i;

    /* Read statistics */
    for (i = 0; i < LATENCY_DIST_SIZE; i++) {
        seq_printf(seq_filp, "%llu, %llu\n",
                   get_100ms_dist(i, true, TEN_SECOND_100MS_SIZE),
                   get_100ms_size(i, true, TEN_SECOND_100MS_SIZE));
    }

    /* Write statistics */
    for (i = 0; i < LATENCY_DIST_SIZE; i++) {
        seq_printf(seq_filp, "%llu, %llu\n",
                   get_100ms_dist(i, false, TEN_SECOND_100MS_SIZE),
                   get_100ms_size(i, false, TEN_SECOND_100MS_SIZE));
    }

    return 0;
}

int ioLatencyStat_proc_open(struct inode *inode, struct file *file)
{
    return single_open(file, iolatencystatshow, file);
}
#endif /* CONFIG_OPLUS_FEATURE_STORAGE_IOLATENCY_STATS */

void ufs_metrics_reset(void)
{
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 0)
    int i;
    atomic64_set(&ufs_metrics.read_size, 0);
    atomic64_set(&ufs_metrics.read_cnt, 0);
    atomic64_set(&ufs_metrics.read_elapse, 0);
    atomic64_set(&ufs_metrics.write_size, 0);
    atomic64_set(&ufs_metrics.write_cnt, 0);
    atomic64_set(&ufs_metrics.write_elapse, 0);

    for (i = 0; i <= LAT_500M_TO_MAX; i++) {
        atomic64_set(&ufs_metrics_lat_write[i], 0);
        atomic64_set(&ufs_metrics_lat_read[i], 0);
    }
#endif
}
void ufs_metrics_init(void)
{
    ufs_metrics_reset();
#ifdef CONFIG_OPLUS_FEATURE_STORAGE_IOLATENCY_STATS
    /* Circular buffer slots are initialized with epoch=0, which will be
     * updated on first write. No explicit initialization needed. */
#endif /* CONFIG_OPLUS_FEATURE_STORAGE_IOLATENCY_STATS */
}
