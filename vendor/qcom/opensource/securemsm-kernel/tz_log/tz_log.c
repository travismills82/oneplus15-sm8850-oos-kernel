// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) 2016-2021, The Linux Foundation. All rights reserved.
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 */

#define pr_fmt(fmt) "tz_log :[%s]: " fmt, __func__

#include <linux/compiler.h>
#include <linux/debugfs.h>
#include <linux/delay.h>
#include <linux/dma-buf.h>
#include <linux/errno.h>
#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/ktime.h>
#include <linux/list.h>
#include <linux/module.h>
#include <linux/msm_ion.h>
#include <linux/mutex.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/poll.h>
#include <linux/proc_fs.h>
#include <linux/qtee_shmbridge.h>
#include <linux/rtc.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/time.h>
#include <linux/timekeeping.h>
#if IS_ENABLED(CONFIG_MSM_TMECOM_QMP)
#include <linux/tmelog.h>
#endif
#include <linux/types.h>
#include <linux/uaccess.h>
#include <linux/version.h>
#if (KERNEL_VERSION(6, 3, 0) <= LINUX_VERSION_CODE)
#include <linux/firmware/qcom/qcom_scm.h>
#else
#include <linux/qcom_scm.h>
#endif
#include <linux/wait.h>


/* QSEE_LOG_BUF_SIZE = 32K */
#define QSEE_LOG_BUF_SIZE 0x8000

/* enlarged qsee log buf size is 128K by default */
#define QSEE_LOG_BUF_SIZE_V2 0x20000

/* Tme log buffer size 20K */
#define TME_LOG_BUF_SIZE 0x5000

/* TZ Diagnostic Area legacy version number */
#define TZBSP_DIAG_MAJOR_VERSION_LEGACY	2

/* TZ Diagnostic Area version number */
#define TZBSP_FVER_MAJOR_MINOR_MASK     0x3FF  /* 10 bits */
#define TZBSP_FVER_MAJOR_SHIFT          22
#define TZBSP_FVER_MINOR_SHIFT          12
#define TZBSP_DIAG_MAJOR_VERSION_V9     9
#define TZBSP_DIAG_MINOR_VERSION_V2     2
#define TZBSP_DIAG_MINOR_VERSION_V21    3
#define TZBSP_DIAG_MINOR_VERSION_V22    4

/* TZ Diag Feature Version Id */
#define QCOM_SCM_FEAT_DIAG_ID           0x06

/*
 * Preprocessor Definitions and Constants
 */
#define TZBSP_MAX_CPU_COUNT 0x08
/*
 * Number of VMID Tables
 */
#define TZBSP_DIAG_NUM_OF_VMID 16
/*
 * VMID Description length
 */
#define TZBSP_DIAG_VMID_DESC_LEN 7
/*
 * Number of Interrupts
 */
#define TZBSP_DIAG_INT_NUM  32
/*
 * Length of descriptive name associated with Interrupt
 */
#define TZBSP_MAX_INT_DESC 16
/*
 * TZ 3.X version info
 */
#define QSEE_VERSION_TZ_3_X 0x800000
/*
 * TZ 4.X version info
 */
#define QSEE_VERSION_TZ_4_X 0x1000000

#define TZBSP_AES_256_ENCRYPTED_KEY_SIZE 256
#define TZBSP_NONCE_LEN 12
#define TZBSP_TAG_LEN 16

#define ENCRYPTED_TZ_LOG_ID 0
#define ENCRYPTED_QSEE_LOG_ID 1

/*
 * Directory for TZ DBG logs
 */
#define TZDBG_DIR_NAME "tzdbg"

#define TICKS_STR_LEN (16 + 1) // 16 for the length of uint64_t string and 1 for '\0'
#define TIMESTAMP_STR_LEN 128
#define BASE_16 16
#define NS_TO_10_US_BASE 10000
#define US_TO_10_NS_BASE 10000

/*
 * Mutex for logs read, since we have only one display buf
 */
DEFINE_MUTEX(tzdbg_mutex);

/*
 * List of clients to tzlog driver
 */
LIST_HEAD(clients_list);

/*
 * VMID Table
 */
struct tzdbg_vmid_t {
	uint8_t vmid; /* Virtual Machine Identifier */
	uint8_t desc[TZBSP_DIAG_VMID_DESC_LEN];	/* ASCII Text */
};
/*
 * Boot Info Table
 */
struct tzdbg_boot_info_t {
	uint32_t wb_entry_cnt;	/* Warmboot entry CPU Counter */
	uint32_t wb_exit_cnt;	/* Warmboot exit CPU Counter */
	uint32_t pc_entry_cnt;	/* Power Collapse entry CPU Counter */
	uint32_t pc_exit_cnt;	/* Power Collapse exit CPU counter */
	uint32_t warm_jmp_addr;	/* Last Warmboot Jump Address */
	uint32_t spare;	/* Reserved for future use. */
};
/*
 * Boot Info Table for 64-bit
 */
struct tzdbg_boot_info64_t {
	uint32_t wb_entry_cnt;  /* Warmboot entry CPU Counter */
	uint32_t wb_exit_cnt;   /* Warmboot exit CPU Counter */
	uint32_t pc_entry_cnt;  /* Power Collapse entry CPU Counter */
	uint32_t pc_exit_cnt;   /* Power Collapse exit CPU counter */
	uint32_t psci_entry_cnt;/* PSCI syscall entry CPU Counter */
	uint32_t psci_exit_cnt;   /* PSCI syscall exit CPU Counter */
	uint64_t warm_jmp_addr; /* Last Warmboot Jump Address */
	uint32_t warm_jmp_instr; /* Last Warmboot Jump Address Instruction */
};
/*
 * Reset Info Table
 */
struct tzdbg_reset_info_t {
	uint32_t reset_type;	/* Reset Reason */
	uint32_t reset_cnt;	/* Number of resets occurred/CPU */
};
/*
 * Interrupt Info Table
 */
struct tzdbg_int_t {
	/*
	 * Type of Interrupt/exception
	 */
	uint16_t int_info;
	/*
	 * Availability of the slot
	 */
	uint8_t avail;
	/*
	 * Reserved for future use
	 */
	uint8_t spare;
	/*
	 * Interrupt # for IRQ and FIQ
	 */
	uint32_t int_num;
	/*
	 * ASCII text describing type of interrupt e.g:
	 * Secure Timer, EBI XPU. This string is always null terminated,
	 * supporting at most TZBSP_MAX_INT_DESC characters.
	 * Any additional characters are truncated.
	 */
	uint8_t int_desc[TZBSP_MAX_INT_DESC];
	uint64_t int_count[TZBSP_MAX_CPU_COUNT]; /* # of times seen per CPU */
};

/*
 * Interrupt Info Table used in tz version >=4.X
 */
struct tzdbg_int_t_tz40 {
	uint16_t int_info;
	uint8_t avail;
	uint8_t spare;
	uint32_t int_num;
	uint8_t int_desc[TZBSP_MAX_INT_DESC];
	uint32_t int_count[TZBSP_MAX_CPU_COUNT]; /* uint32_t in TZ ver >= 4.x*/
};

/* warm boot reason for cores */
struct tzbsp_diag_wakeup_info_t {
	/* Wake source info : APCS_GICC_HPPIR */
	uint32_t HPPIR;
	/* Wake source info : APCS_GICC_AHPPIR */
	uint32_t AHPPIR;
};

/*
 * Log ring buffer position
 */
struct tzdbg_log_pos_t {
	uint16_t wrap;
	uint16_t offset;
};

struct tzdbg_log_pos_v2_t {
	uint32_t wrap;
	uint32_t offset;
};

/*
 * Client`s info
 */
struct clients_info_t {
	size_t display_offset;
	size_t display_len;
	struct file *file;
	struct list_head list;
	struct tzdbg_log_pos_t log_start;
	struct tzdbg_log_pos_v2_t log_start_v2;
};

 /*
  * Log ring buffer
  */
struct tzdbg_log_t {
	struct tzdbg_log_pos_t	log_pos;
	/* open ended array to the end of the 4K IMEM buffer */
	uint8_t					log_buf[];
};

struct tzdbg_log_v2_t {
	struct tzdbg_log_pos_v2_t	log_pos;
	/* open ended array to the end of the 4K IMEM buffer */
	uint8_t					log_buf[];
};

struct tzbsp_encr_info_for_log_chunk_t {
	uint32_t size_to_encr;
	uint8_t nonce[TZBSP_NONCE_LEN];
	uint8_t tag[TZBSP_TAG_LEN];
};

/*
 * Only `ENTIRE_LOG` will be used unless the
 * "OEM_tz_num_of_diag_log_chunks_to_encr" devcfg field >= 2.
 * If this is true, the diag log will be encrypted in two
 * separate chunks: a smaller chunk containing only error
 * fatal logs and a bigger "rest of the log" chunk. In this
 * case, `ERR_FATAL_LOG_CHUNK` and `BIG_LOG_CHUNK` will be
 * used instead of `ENTIRE_LOG`.
 */
enum tzbsp_encr_info_for_log_chunks_idx_t {
	BIG_LOG_CHUNK = 0,
	ENTIRE_LOG = 1,
	ERR_FATAL_LOG_CHUNK = 1,
	MAX_NUM_OF_CHUNKS,
};

struct tzbsp_encr_info_t {
	uint32_t num_of_chunks;
	struct tzbsp_encr_info_for_log_chunk_t chunks[MAX_NUM_OF_CHUNKS];
	uint8_t key[TZBSP_AES_256_ENCRYPTED_KEY_SIZE];
};

/*
 * Diagnostic Table
 * Note: This is the reference data structure for tz diagnostic table
 * supporting TZBSP_MAX_CPU_COUNT, the real diagnostic data is directly
 * copied into buffer from i/o memory.
 */
struct tzdbg_t {
	uint32_t magic_num;
	uint32_t version;
	/*
	 * Number of CPU's
	 */
	uint32_t cpu_count;
	/*
	 * Offset of VMID Table
	 */
	uint32_t vmid_info_off;
	/*
	 * Offset of Boot Table
	 */
	uint32_t boot_info_off;
	/*
	 * Offset of Reset info Table
	 */
	uint32_t reset_info_off;
	/*
	 * Offset of Interrupt info Table
	 */
	uint32_t int_info_off;
	/*
	 * Ring Buffer Offset
	 */
	uint32_t ring_off;
	/*
	 * Ring Buffer Length
	 */
	uint32_t ring_len;

	/* Offset for Wakeup info */
	uint32_t wakeup_info_off;

	union {
		/* The elements in below structure have to be used for TZ where
		 * diag version = TZBSP_DIAG_MINOR_VERSION_V2
		 */
		struct {

			/*
			 * VMID to EE Mapping
			 */
			struct tzdbg_vmid_t vmid_info[TZBSP_DIAG_NUM_OF_VMID];
			/*
			 * Boot Info
			 */
			struct tzdbg_boot_info_t  boot_info[TZBSP_MAX_CPU_COUNT];
			/*
			 * Reset Info
			 */
			struct tzdbg_reset_info_t reset_info[TZBSP_MAX_CPU_COUNT];
			uint32_t num_interrupts;
			struct tzdbg_int_t  int_info[TZBSP_DIAG_INT_NUM];
			/* Wake up info */
			struct tzbsp_diag_wakeup_info_t  wakeup_info[TZBSP_MAX_CPU_COUNT];

			uint8_t key[TZBSP_AES_256_ENCRYPTED_KEY_SIZE];

			uint8_t nonce[TZBSP_NONCE_LEN];

			uint8_t tag[TZBSP_TAG_LEN];
		};
		/* The elements in below structure have to be used for TZ where
		 * diag version = TZBSP_DIAG_MINOR_VERSION_V21
		 */
		struct {

			uint32_t encr_info_for_log_off;

			/*
			 * VMID to EE Mapping
			 */
			struct tzdbg_vmid_t vmid_info_v2[TZBSP_DIAG_NUM_OF_VMID];
			/*
			 * Boot Info
			 */
			struct tzdbg_boot_info_t  boot_info_v2[TZBSP_MAX_CPU_COUNT];
			/*
			 * Reset Info
			 */
			struct tzdbg_reset_info_t reset_info_v2[TZBSP_MAX_CPU_COUNT];
			uint32_t num_interrupts_v2;
			struct tzdbg_int_t  int_info_v2[TZBSP_DIAG_INT_NUM];

			/* Wake up info */
			struct tzbsp_diag_wakeup_info_t  wakeup_info_v2[TZBSP_MAX_CPU_COUNT];

			struct tzbsp_encr_info_t encr_info_for_log;
		};
	};

	/*
	 * We need at least 2K for the ring buffer
	 */
	struct tzdbg_log_t ring_buffer;	/* TZ Ring Buffer */
};

struct hypdbg_log_pos_t {
	uint16_t wrap;
	uint16_t offset;
};

struct rmdbg_log_hdr_t {
	uint32_t write_idx;
	uint32_t size;
};
struct rmdbg_log_pos_t {
	uint32_t read_idx;
	uint32_t size;
};
struct hypdbg_boot_info_t {
	uint32_t warm_entry_cnt;
	uint32_t warm_exit_cnt;
};

struct hypdbg_t {
	/* Magic Number */
	uint32_t magic_num;

	/* Number of CPU's */
	uint32_t cpu_count;

	/* Ring Buffer Offset */
	uint32_t ring_off;

	/* Ring buffer position mgmt */
	struct hypdbg_log_pos_t log_pos;
	uint32_t log_len;

	/* S2 fault numbers */
	uint32_t s2_fault_counter;

	/* Boot Info */
	struct hypdbg_boot_info_t boot_info[TZBSP_MAX_CPU_COUNT];

	/* Ring buffer pointer */
	uint8_t log_buf_p[];
};

struct tme_log_pos {
	uint32_t offset;
	size_t size;
};

/*
 * Enumeration order for VMID's
 */
enum tzdbg_stats_type {
	TZDBG_BOOT = 0,
	TZDBG_RESET,
	TZDBG_INTERRUPT,
	TZDBG_VMID,
	TZDBG_GENERAL,
	TZDBG_LOG,
	TZDBG_QSEE_LOG,
	TZDBG_HYP_GENERAL,
	TZDBG_HYP_LOG,
	TZDBG_RM_LOG,
	TZDBG_TME_LOG,
	TZDBG_STATS_MAX
};

struct tzdbg_stat {
	size_t display_len;
	size_t display_offset;
	char *name;
	char *data;
	bool avail;
};

struct tzdbg {
	void __iomem *virt_iobase;
	void __iomem *hyp_virt_iobase;
	void __iomem *rmlog_virt_iobase;
	void __iomem *tmelog_virt_iobase;
	struct tzdbg_t *diag_buf;
	struct hypdbg_t *hyp_diag_buf;
	uint8_t *rm_diag_buf;
	uint8_t *tme_buf;
	char *disp_buf;
	int debug_tz[TZDBG_STATS_MAX];
	struct tzdbg_stat stat[TZDBG_STATS_MAX];
	uint32_t hyp_debug_rw_buf_size;
	uint32_t rmlog_rw_buf_size;
	bool is_hyplog_enabled;
	uint32_t tz_version;
	bool is_encrypted_log_enabled;
	bool tz_qsee_plain_log_enabled;
	bool is_enlarged_buf;
	bool is_full_encrypted_tz_logs_supported;
	bool is_full_encrypted_tz_logs_enabled;
	int tz_diag_minor_version;
	int tz_diag_major_version;
	struct mutex lock;
};

struct tzbsp_encr_log_t {
	/* Magic Number */
	uint32_t magic_num;
	/* version NUMBER */
	uint32_t version;
	/* encrypted log size */
	uint32_t encr_log_buff_size;
	/* Wrap value*/
	uint16_t wrap_count;
	/* AES encryption key wrapped up with oem public key*/
	uint8_t key[TZBSP_AES_256_ENCRYPTED_KEY_SIZE];
	/* Nonce used for encryption*/
	uint8_t nonce[TZBSP_NONCE_LEN];
	/* Tag to be used for Validation */
	uint8_t tag[TZBSP_TAG_LEN];
	/* Encrypted log buffer */
	uint8_t log_buf[1];
};

struct encrypted_log_info {
	phys_addr_t paddr;
	void *vaddr;
	size_t size;
	uint64_t shmb_handle;
};

static struct tzdbg tzdbg = {
	.stat[TZDBG_BOOT].name = "boot",
	.stat[TZDBG_RESET].name = "reset",
	.stat[TZDBG_INTERRUPT].name = "interrupt",
	.stat[TZDBG_VMID].name = "vmid",
	.stat[TZDBG_GENERAL].name = "general",
	.stat[TZDBG_LOG].name = "log",
	.stat[TZDBG_QSEE_LOG].name = "qsee_log",
	.stat[TZDBG_HYP_GENERAL].name = "hyp_general",
	.stat[TZDBG_HYP_LOG].name = "hyp_log",
	.stat[TZDBG_RM_LOG].name = "rm_log",
	.stat[TZDBG_TME_LOG].name = "tme_log",
};

static struct tzdbg_log_t *g_qsee_log;
static struct tzdbg_log_v2_t *g_qsee_log_v2;
static dma_addr_t coh_pmem;
static uint32_t debug_rw_buf_size;
static uint32_t display_buf_size;
static uint32_t qseelog_buf_size;
static phys_addr_t disp_buf_paddr;
static uint32_t tmecrashdump_address_offset;

static uint64_t qseelog_shmbridge_handle;
static struct encrypted_log_info enc_qseelog_info;
static struct encrypted_log_info enc_tzlog_info;

#if (KERNEL_VERSION(6, 12, 0) <= LINUX_VERSION_CODE) && defined(CONFIG_TZLOG_TIME_CONSOLIDATE)
static bool g_realtime_consolidation_enable;
static uint64_t g_tz_ticks_baseline;
static uint64_t g_tz_ticks_frequency;
static ktime_t g_hlos_uptime_baseline;
#endif

static atomic_t is_rd_locked = ATOMIC_INIT(0);

/*
 * Debugfs data structure and functions
 */

static int _disp_tz_general_stats(void)
{
	int len = 0;

	mutex_lock(&tzdbg_mutex);
	atomic_set(&is_rd_locked, 1);
	len += scnprintf(tzdbg.disp_buf + len, debug_rw_buf_size - 1,
			"   Version        : 0x%x\n"
			"   Magic Number   : 0x%x\n"
			"   Number of CPU  : %d\n",
			tzdbg.diag_buf->version,
			tzdbg.diag_buf->magic_num,
			tzdbg.diag_buf->cpu_count);
	tzdbg.stat[TZDBG_GENERAL].data = tzdbg.disp_buf;
	return len;
}

static int _disp_tz_vmid_stats(void)
{
	int i, num_vmid;
	int len = 0;
	struct tzdbg_vmid_t *ptr;

	ptr = (struct tzdbg_vmid_t *)((unsigned char *)tzdbg.diag_buf +
					tzdbg.diag_buf->vmid_info_off);
	num_vmid = ((tzdbg.diag_buf->boot_info_off -
				tzdbg.diag_buf->vmid_info_off)/
					(sizeof(struct tzdbg_vmid_t)));

	mutex_lock(&tzdbg_mutex);
	atomic_set(&is_rd_locked, 1);

	for (i = 0; i < num_vmid; i++) {
		if (ptr->vmid < 0xFF) {
			len += scnprintf(tzdbg.disp_buf + len,
				(debug_rw_buf_size - 1) - len,
				"   0x%x        %s\n",
				(uint32_t)ptr->vmid, (uint8_t *)ptr->desc);
		}
		if (len > (debug_rw_buf_size - 1)) {
			pr_warn("%s: Cannot fit all info into the buffer\n",
								__func__);
			break;
		}
		ptr++;
	}

	tzdbg.stat[TZDBG_VMID].data = tzdbg.disp_buf;
	return len;
}

static int _disp_tz_boot_stats(void)
{
	int i;
	int len = 0;
	struct tzdbg_boot_info_t *ptr = NULL;
	struct tzdbg_boot_info64_t *ptr_64 = NULL;

	pr_info("qsee_version = 0x%x\n", tzdbg.tz_version);
	if (tzdbg.tz_version >= QSEE_VERSION_TZ_3_X) {
		ptr_64 = (struct tzdbg_boot_info64_t *)((unsigned char *)
			tzdbg.diag_buf + tzdbg.diag_buf->boot_info_off);
	} else {
		ptr = (struct tzdbg_boot_info_t *)((unsigned char *)
			tzdbg.diag_buf + tzdbg.diag_buf->boot_info_off);
	}

	mutex_lock(&tzdbg_mutex);
	atomic_set(&is_rd_locked, 1);
	for (i = 0; i < tzdbg.diag_buf->cpu_count; i++) {
		if (tzdbg.tz_version >= QSEE_VERSION_TZ_3_X) {
			len += scnprintf(tzdbg.disp_buf + len,
					(debug_rw_buf_size - 1) - len,
					"  CPU #: %d\n"
					"     Warmboot jump address : 0x%llx\n"
					"     Warmboot entry CPU counter : 0x%x\n"
					"     Warmboot exit CPU counter : 0x%x\n"
					"     Power Collapse entry CPU counter : 0x%x\n"
					"     Power Collapse exit CPU counter : 0x%x\n"
					"     Psci entry CPU counter : 0x%x\n"
					"     Psci exit CPU counter : 0x%x\n"
					"     Warmboot Jump Address Instruction : 0x%x\n",
					i, (uint64_t)ptr_64->warm_jmp_addr,
					ptr_64->wb_entry_cnt,
					ptr_64->wb_exit_cnt,
					ptr_64->pc_entry_cnt,
					ptr_64->pc_exit_cnt,
					ptr_64->psci_entry_cnt,
					ptr_64->psci_exit_cnt,
					ptr_64->warm_jmp_instr);

			if (len > (debug_rw_buf_size - 1)) {
				pr_warn("%s: Cannot fit all info into the buffer\n",
						__func__);
				break;
			}
			ptr_64++;
		} else {
			len += scnprintf(tzdbg.disp_buf + len,
					(debug_rw_buf_size - 1) - len,
					"  CPU #: %d\n"
					"     Warmboot jump address     : 0x%x\n"
					"     Warmboot entry CPU counter: 0x%x\n"
					"     Warmboot exit CPU counter : 0x%x\n"
					"     Power Collapse entry CPU counter: 0x%x\n"
					"     Power Collapse exit CPU counter : 0x%x\n",
					i, ptr->warm_jmp_addr,
					ptr->wb_entry_cnt,
					ptr->wb_exit_cnt,
					ptr->pc_entry_cnt,
					ptr->pc_exit_cnt);

			if (len > (debug_rw_buf_size - 1)) {
				pr_warn("%s: Cannot fit all info into the buffer\n",
						__func__);
				break;
			}
			ptr++;
		}
	}
	tzdbg.stat[TZDBG_BOOT].data = tzdbg.disp_buf;
	return len;
}

static int _disp_tz_reset_stats(void)
{
	int i;
	int len = 0;
	struct tzdbg_reset_info_t *ptr;

	ptr = (struct tzdbg_reset_info_t *)((unsigned char *)tzdbg.diag_buf +
					tzdbg.diag_buf->reset_info_off);

	mutex_lock(&tzdbg_mutex);
	atomic_set(&is_rd_locked, 1);
	for (i = 0; i < tzdbg.diag_buf->cpu_count; i++) {
		len += scnprintf(tzdbg.disp_buf + len,
				(debug_rw_buf_size - 1) - len,
				"  CPU #: %d\n"
				"     Reset Type (reason)       : 0x%x\n"
				"     Reset counter             : 0x%x\n",
				i, ptr->reset_type, ptr->reset_cnt);

		if (len > (debug_rw_buf_size - 1)) {
			pr_warn("%s: Cannot fit all info into the buffer\n",
								__func__);
			break;
		}

		ptr++;
	}
	tzdbg.stat[TZDBG_RESET].data = tzdbg.disp_buf;
	return len;
}

static int _disp_tz_interrupt_stats(void)
{
	int i, j;
	int len = 0;
	int *num_int;
	void *ptr;
	struct tzdbg_int_t *tzdbg_ptr;
	struct tzdbg_int_t_tz40 *tzdbg_ptr_tz40;

	num_int = (uint32_t *)((unsigned char *)tzdbg.diag_buf +
			(tzdbg.diag_buf->int_info_off - sizeof(uint32_t)));
	ptr = ((unsigned char *)tzdbg.diag_buf +
					tzdbg.diag_buf->int_info_off);

	pr_info("qsee_version = 0x%x\n", tzdbg.tz_version);

	mutex_lock(&tzdbg_mutex);
	atomic_set(&is_rd_locked, 1);
	if (tzdbg.tz_version < QSEE_VERSION_TZ_4_X) {
		tzdbg_ptr = ptr;
		for (i = 0; i < (*num_int); i++) {
			len += scnprintf(tzdbg.disp_buf + len,
				(debug_rw_buf_size - 1) - len,
				"     Interrupt Number          : 0x%x\n"
				"     Type of Interrupt         : 0x%x\n"
				"     Description of interrupt  : %s\n",
				tzdbg_ptr->int_num,
				(uint32_t)tzdbg_ptr->int_info,
				(uint8_t *)tzdbg_ptr->int_desc);
			for (j = 0; j < tzdbg.diag_buf->cpu_count; j++) {
				len += scnprintf(tzdbg.disp_buf + len,
				(debug_rw_buf_size - 1) - len,
				"     int_count on CPU # %d      : %u\n",
				(uint32_t)j,
				(uint32_t)tzdbg_ptr->int_count[j]);
			}
			len += scnprintf(tzdbg.disp_buf + len,
					debug_rw_buf_size - 1, "\n");

			if (len > (debug_rw_buf_size - 1)) {
				pr_warn("%s: Cannot fit all info into buf\n",
								__func__);
				break;
			}
			tzdbg_ptr++;
		}
	} else {
		tzdbg_ptr_tz40 = ptr;
		for (i = 0; i < (*num_int); i++) {
			len += scnprintf(tzdbg.disp_buf + len,
				(debug_rw_buf_size - 1) - len,
				"     Interrupt Number          : 0x%x\n"
				"     Type of Interrupt         : 0x%x\n"
				"     Description of interrupt  : %s\n",
				tzdbg_ptr_tz40->int_num,
				(uint32_t)tzdbg_ptr_tz40->int_info,
				(uint8_t *)tzdbg_ptr_tz40->int_desc);
			for (j = 0; j < tzdbg.diag_buf->cpu_count; j++) {
				len += scnprintf(tzdbg.disp_buf + len,
				(debug_rw_buf_size - 1) - len,
				"     int_count on CPU # %d      : %u\n",
				(uint32_t)j,
				(uint32_t)tzdbg_ptr_tz40->int_count[j]);
			}
			len += scnprintf(tzdbg.disp_buf + len,
					debug_rw_buf_size - 1, "\n");

			if (len > (debug_rw_buf_size - 1)) {
				pr_warn("%s: Cannot fit all info into buf\n",
								__func__);
				break;
			}
			tzdbg_ptr_tz40++;
		}
	}

	tzdbg.stat[TZDBG_INTERRUPT].data = tzdbg.disp_buf;
	return len;
}

static int _disp_tz_log_stats_legacy(void)
{
	int len = 0;
	unsigned char *ptr;

	ptr = (unsigned char *)tzdbg.diag_buf +
					tzdbg.diag_buf->ring_off;

	mutex_lock(&tzdbg_mutex);
	atomic_set(&is_rd_locked, 1);
	len += scnprintf(tzdbg.disp_buf, (debug_rw_buf_size - 1) - len,
							"%s\n", ptr);

	tzdbg.stat[TZDBG_LOG].data = tzdbg.disp_buf;
	return len;
}

static uint32_t _copy_to_dispbuf(struct tzdbg_log_t *log, struct tzdbg_log_pos_t *log_start,
	uint32_t round, uint32_t max_len, uint8_t *disp, uint32_t index)
{
	uint32_t len = 0;

	if (round == 0)
		return 0;

	/*
	 *  Read from ring buff while there is data and space in return buff
	 */
	while ((log_start->offset != log->log_pos.offset) && (len < max_len)) {
		disp[index++] = log->log_buf[log_start->offset];
		log_start->offset = (log_start->offset + 1) % round;
		if (log_start->offset == 0)
			++log_start->wrap;
		++len;
	}

	return len;
}

#if (KERNEL_VERSION(6, 12, 0) <= LINUX_VERSION_CODE) && defined(CONFIG_TZLOG_TIME_CONSOLIDATE)
/*
 * Calculate the length of log based on the end label "\r\n"
 *
 * Input  : buf : the initial log data buffer
 *          begin : the begin index of buffer
 *          end : the end index of buffer
 *          remain : the maxinum length of buffer that can be used to parse
 *          log_len : the length of the ring buffer
 *
 * Return : len for the length of log that is ended with "\r\n" or -EINVAL for failure
 */
static int _find_end_label(const uint8_t *buf, uint32_t begin, uint32_t end, uint32_t remain,
	uint32_t log_len)
{
	uint32_t len = 0;
	uint32_t next = (begin + 1) % log_len;

	while ((begin != end) && (next != end) && ((len + 2) <= remain)) {
		if ((char)buf[begin] == '\r' && (char)buf[next] == '\n')
			return len + strlen("\r\n");

		begin = next;
		next = (next + 1) % log_len;
		++len;
	}

	return -EINVAL;
}

/*
 * Parse the log with the format "[ticks]...\r\n" to extract the ticks information. Calculate the
 * corresponding real-time and generate a timestamp string in the format
 * [realtime][hlos uptime][tz uptime].
 *
 * Input  : buf : the initial log data buffer
 *          begin : the begin index of log data in the buffer
 *          len : the length of log data buffer
 *          round : the length of the ring buffer
 *          stamp_len : the length of the timestamp string buffer
 *
 * Output : timestamp : the timetamp string buffer
 *          next : the position next to ']'.
 *
 * Return : len for the size of timestamp string or -EINVAL for failure.
 */
static int _generate_realtime_timestamp(const char *buf, uint32_t begin, uint32_t len,
	uint32_t round,	char *timestamp, uint32_t stamp_len, uint32_t *next)
{
	uint32_t index = 0;
	uint64_t ticks = 0;
	char ticks_str[TICKS_STR_LEN] = {0};
	ktime_t hlos_t = g_hlos_uptime_baseline;
	ktime_t hlos_realtime_t = 0;
	ktime_t tz_t = 0;
	struct timespec64 hlos_ts = {};
	struct timespec64 hlos_realtime_ts = {};
	struct timespec64 tz_ts = {};
	struct rtc_time hlos_rtc_t = {};
	uint32_t size = 0;

	/*
	 * Considering three situations about ticks.
	 * 1. Normal log : [ticks]... --> parse the ticks.
	 * 2. Abnormal log : [invalid ticks]...--> Not parse the ticks.
	 * 3. Abnormal log : XXX]... --> Not parse the ticks.
	 */
	if (buf[begin] != '[')
		return -EINVAL;

	for (index = 1; index < len && index < sizeof(ticks_str); ++index) {
		if (buf[(begin + index) % round] != ']') {
			ticks_str[index - 1] = buf[(begin + index) % round];
		} else {
			ticks_str[index - 1] = '\0';
			if (kstrtoull(ticks_str, BASE_16, &ticks))
				return -EINVAL;

			*next = index + 1;
			break;
		}
	}

	if (index == len || index == sizeof(ticks_str))
		return -EINVAL;

	/* Calculate the relevant hlos uptime based on hlos uptime baseline and tz time interval. */
	if (likely(ticks > g_tz_ticks_baseline))
		hlos_t += (ticks - g_tz_ticks_baseline) * US_TO_10_NS_BASE / g_tz_ticks_frequency;
	else
		hlos_t -= (g_tz_ticks_baseline - ticks) * US_TO_10_NS_BASE / g_tz_ticks_frequency;

	hlos_ts = ktime_to_timespec64(hlos_t);
	hlos_realtime_t = ktime_mono_to_real(hlos_t);
	hlos_realtime_ts = ktime_to_timespec64(hlos_realtime_t);
	hlos_rtc_t = rtc_ktime_to_tm(hlos_realtime_t);
	tz_t = ticks * US_TO_10_NS_BASE / g_tz_ticks_frequency;
	tz_ts = ktime_to_timespec64(tz_t);

	size = scnprintf(timestamp, stamp_len,
			"[%02d-%02d %02d:%02d:%02d.%05ld][%lld.%05ld][%lld.%05ld]",
			hlos_rtc_t.tm_mon + 1, hlos_rtc_t.tm_mday,
			hlos_rtc_t.tm_hour, hlos_rtc_t.tm_min,
			hlos_rtc_t.tm_sec, hlos_realtime_ts.tv_nsec / NS_TO_10_US_BASE,
			hlos_ts.tv_sec, hlos_ts.tv_nsec / NS_TO_10_US_BASE,
			tz_ts.tv_sec, tz_ts.tv_nsec / NS_TO_10_US_BASE);

	return size;
}

/*
 * Parse log data, enhance it with additional time information and copy it into disp buffer.
 *
 * Original log format : [ticks]log data\r\n
 * New log format with real-time : [ticks][hlos real-time][hlos uptime][tz uptime]log data\r\n
 *
 * Input  : log : the initial log data information including log data and end index of log buffer
 *          log_start : information prepared for log reading including begin index
 *          round : the length of ring buffer
 *          max_len : the maxinum length of disp buffer that is used to store log data
 *          index : the start index of disp
 *
 * Output : disp : the log buffer with real-time information
 *
 * Return : the length of the written log buffer
 */
static uint32_t _copy_to_dispbuf_with_realtime(struct tzdbg_log_t *log,
	struct tzdbg_log_pos_t *log_start, uint32_t round, uint32_t max_len, uint8_t *disp,
	uint32_t index)
{
	uint32_t len = 0;
	int each_log_len = 0;
	char timestamp[TIMESTAMP_STR_LEN] = {0};
	int stamp_len = 0;
	uint32_t next = 0;
	uint32_t begin = log_start->offset;
	uint32_t end = log->log_pos.offset;
	uint32_t remain = max_len;
	uint32_t copy_len = 0;

	if (round == 0)
		return 0;

	pr_debug("tzdbg_log begin = %u, end = %u, round = %u, disp_buf max_len = %u, index = %u\n",
		begin, end, round, max_len, index);

	while ((begin != end) && (len < max_len)) {
		/*
		 * There will be three main type of log buffer.
		 * 1. Nomal log ends with \r\n. It will add the realtime information.
		 * 2. Abnormal log doesn't end with \r\n since reading occurs before writing.
		 * 3. Abnormal log doesn't end with \r\n because of insufficient buffer.
		 */
		each_log_len = _find_end_label(log->log_buf, begin, end, remain, round);
		if (each_log_len == -EINVAL) {
			if (len == 0) {
				/*
				 * "0" indicates that it is the first time real-time information is
				 * being added. Due to an error caused by insufficient buffer space
				 * passed from user space, there is not enough buffer space to add
				 * any real-time information. Consequently, tz_log can only copy the
				 * initial buffer into user space.
				 */
				len += _copy_to_dispbuf(log, log_start, round, remain, disp, index);
				pr_warn("Read an incomplete log and copy to disp. len = %u\n", len);
			}
			break;
		}

		if ((uint32_t)each_log_len > remain)
			break;

		copy_len = (uint32_t)each_log_len;
		stamp_len = _generate_realtime_timestamp(log->log_buf, begin, each_log_len, round,
				timestamp, sizeof(timestamp), &next);
		if (stamp_len != -EINVAL) {
			if (stamp_len <= (remain - each_log_len)) {
				/* copy the ticks and timestamp into disp buffer. */
				index += _copy_to_dispbuf(log, log_start, round, next, disp, index);
				copy_len -= next;
				len += next;
				memcpy(disp + index, timestamp, stamp_len);
				index += (uint32_t)stamp_len;
				len += (uint32_t)stamp_len;
			} else {
				pr_debug("Insufficient buffer for the log with real-time!Remain size is %u, expected size is %u\n",
					remain, each_log_len + stamp_len);
				/*
				 * There are two main reasons for the insufficient buffer. One is
				 * that there is already some log data in it when the length is not
				 * zero. The other is due to insufficient space provided by the
				 * user. For the former reason, the data will not be copied until
				 * the next round of reading from user space. For the latter reason,
				 * the initial log data will be copied to user space.
				 */
				if (len != 0)
					break;
			}
		}

		/*
		 * 1. Update the information about disp, including the starting index, the length
		 *    that has been copied and remainning length.
		 * 2. Update the next data reading index of log.
		 */
		index += _copy_to_dispbuf(log, log_start, round, copy_len, disp, index);
		len += copy_len;
		remain = max_len - len;
		begin = (begin + each_log_len) % round;
		pr_debug("log len = %d, timestamp len = %d, total copy len = %u, disp buf index = %u, remain = %u, tzdbg_log begin = %u.\n",
			each_log_len, stamp_len, len, index, remain, begin);
	}

	return len;
}
#endif

static int _disp_log_stats(struct tzdbg_log_t *log,
			struct tzdbg_log_pos_t *log_start, uint32_t log_len,
			size_t count, uint32_t buf_idx)
{
	uint32_t wrap_start = 0;
	uint32_t wrap_end = 0;
	uint32_t wrap_cnt = 0;
	uint32_t max_len = 0;
	uint32_t len = 0;

	wrap_start = log_start->wrap;
	wrap_end = log->log_pos.wrap;

	/* Calculate difference in # of buffer wrap-arounds */
	if (wrap_end >= wrap_start)
		wrap_cnt = wrap_end - wrap_start;
	else {
		/* wrap counter has wrapped around, invalidate start position */
		wrap_cnt = 2;
	}

	if (wrap_cnt > 1) {
		/* end position has wrapped around more than once, */
		/* current start no longer valid                   */
		log_start->wrap = log->log_pos.wrap - 1;
		log_start->offset = (log->log_pos.offset + 1) % log_len;
	} else if ((wrap_cnt == 1) &&
		(log->log_pos.offset > log_start->offset)) {
		/* end position has overwritten start */
		log_start->offset = (log->log_pos.offset + 1) % log_len;
	}

	pr_debug("diag_buf wrap = %u, offset = %u\n",
		log->log_pos.wrap, log->log_pos.offset);
	while (log_start->offset == log->log_pos.offset) {
		/*
		 * No data in ring buffer,
		 * so we'll hang around until something happens
		 */
		unsigned long t = msleep_interruptible(50);

		if (t != 0) {
			/* Some event woke us up, so let's quit */
			return 0;
}

		if (buf_idx == TZDBG_LOG) {
			mutex_lock(&tzdbg_mutex);
			memcpy_fromio((void *)tzdbg.diag_buf, tzdbg.virt_iobase,
						debug_rw_buf_size);
			mutex_unlock(&tzdbg_mutex);
		}

	}

	max_len = (count > debug_rw_buf_size) ? debug_rw_buf_size : count;

	pr_debug("diag_buf wrap = %u, offset = %u\n",
		log->log_pos.wrap, log->log_pos.offset);

#if (KERNEL_VERSION(6, 12, 0) <= LINUX_VERSION_CODE) && defined(CONFIG_TZLOG_TIME_CONSOLIDATE)
	if (g_realtime_consolidation_enable)
		len = _copy_to_dispbuf_with_realtime(log, log_start, log_len, max_len,
			tzdbg.disp_buf, 0);
	else
		len = _copy_to_dispbuf(log, log_start, log_len, max_len, tzdbg.disp_buf, 0);
#else
	len = _copy_to_dispbuf(log, log_start, log_len, max_len, tzdbg.disp_buf, 0);
#endif
	/*
	 * return buffer to caller
	 */
	tzdbg.stat[buf_idx].data = tzdbg.disp_buf;
	return len;
}

static uint32_t _copy_to_dispbuf_v2(struct tzdbg_log_v2_t *log,
	struct tzdbg_log_pos_v2_t *log_start, uint32_t round, uint32_t max_len, uint8_t *disp,
	uint32_t index)
{
	uint32_t len = 0;

	if (round == 0)
		return 0;

	/*
	 *  Read from ring buff while there is data and space in return buff
	 */
	while ((log_start->offset != log->log_pos.offset) && (len < max_len)) {
		disp[index++] = log->log_buf[log_start->offset];
		log_start->offset = (log_start->offset + 1) % round;
		if (log_start->offset == 0)
			++log_start->wrap;
		++len;
	}

	return len;
}

#if (KERNEL_VERSION(6, 12, 0) <= LINUX_VERSION_CODE) && defined(CONFIG_TZLOG_TIME_CONSOLIDATE)
/*
 * Parse log data, enhance it with additional time information and copy it into disp buffer.
 *
 * Original log format : [ticks]log data\r\n
 * New log format with real-time : [ticks][hlos real-time][hlos uptime][tz uptime]log data\r\n
 *
 * Input  : log : the initial log data information including log data and end index of log buffer
 *          log_start : information prepared for log reading including begin index
 *          round : the length of ring buffer
 *          max_len : the maxinum length of disp buffer that is used to store log data
 *          index : the start index of disp
 *
 * Output : disp : the log buffer with real-time information
 *
 * Return : the length of the written log buffer
 */
static uint32_t _copy_to_dispbuf_with_realtime_v2(struct tzdbg_log_v2_t *log,
	struct tzdbg_log_pos_v2_t *log_start, uint32_t round, uint32_t max_len, uint8_t *disp,
	uint32_t index)
{
	uint32_t len = 0;
	int each_log_len = 0;
	char timestamp[TIMESTAMP_STR_LEN] = {0};
	int stamp_len = 0;
	uint32_t next = 0;
	uint32_t begin = log_start->offset;
	uint32_t end = log->log_pos.offset;
	uint32_t remain = max_len;
	uint32_t copy_len = 0;

	if (round == 0)
		return 0;

	pr_debug("tzdbg_log begin = %u, end = %u, round = %u, disp_buf max_len = %u, index = %u\n",
		begin, end, round, max_len, index);

	while ((begin != end) && (len < max_len)) {
		/*
		 * There will be three main type of log buffer.
		 * 1. Nomal log ends with \r\n. It will add the realtime information.
		 * 2. Abnormal log doesn't end with \r\n since reading occurs before writing.
		 * 3. Abnormal log doesn't end with \r\n because of insufficient buffer.
		 */
		each_log_len = _find_end_label(log->log_buf, begin, end, remain, round);
		if (each_log_len == -EINVAL) {
			if (len == 0) {
				/*
				 * "0" indicates that it is the first time real-time information is
				 * being added. Due to an error caused by insufficient buffer space
				 * passed from user space, there is not enough buffer space to add
				 * any real-time information. Consequently, tz_log can only copy the
				 * initial buffer into user space.
				 */
				len += _copy_to_dispbuf_v2(log, log_start, round, remain, disp,
					index);
				pr_warn("Read an incomplete log and copy to disp. len = %u\n", len);
			}
			break;
		}

		if ((uint32_t)each_log_len > remain)
			break;

		copy_len = (uint32_t)each_log_len;
		stamp_len = _generate_realtime_timestamp(log->log_buf, begin, each_log_len, round,
				timestamp, sizeof(timestamp), &next);
		if (stamp_len != -EINVAL) {
			if (stamp_len <= (remain - each_log_len)) {
				/* copy the ticks and timestamp into disp buffer. */
				index += _copy_to_dispbuf_v2(log, log_start, round, next, disp,
					index);
				copy_len -= next;
				len += next;
				memcpy(disp + index, timestamp, stamp_len);
				index += (uint32_t)stamp_len;
				len += (uint32_t)stamp_len;
			} else {
				pr_debug("Insufficient buffer for the log with real-time!Remain size is %u, expected size is %u\n",
					remain, each_log_len + stamp_len);
				/*
				 * There are two main reasons for the insufficient buffer. One is
				 * that there is already some log data in it when the length is not
				 * zero. The other is due to insufficient space provided by the
				 * user. For the former reason, the data will not be copied until
				 * the next round of reading from user space. For the latter reason,
				 * the initial log data will be copied to user space.
				 */
				if (len != 0)
					break;
			}
		}

		/*
		 * 1. Update the information about disp, including the starting index, the length
		 *    that has been copied and remainning length.
		 * 2. Update the next data reading index of log.
		 */
		index += _copy_to_dispbuf_v2(log, log_start, round, copy_len, disp, index);
		len += copy_len;
		remain = max_len - len;
		begin = (begin + each_log_len) % round;
		pr_debug("log len = %d, timestamp len = %d, total copy len = %u, disp buf index = %u, remain = %u, tzdbg_log begin = %u.\n",
			each_log_len, stamp_len, len, index, remain, begin);
	}

	return len;
}
#endif

static int _disp_log_stats_v2(struct tzdbg_log_v2_t *log,
			struct tzdbg_log_pos_v2_t *log_start, uint32_t log_len,
			size_t count, uint32_t buf_idx)
{
	uint32_t wrap_start = 0;
	uint32_t wrap_end = 0;
	uint32_t wrap_cnt = 0;
	uint32_t max_len = 0;
	uint32_t len = 0;

	wrap_start = log_start->wrap;
	wrap_end = log->log_pos.wrap;

	/* Calculate difference in # of buffer wrap-arounds */
	if (wrap_end >= wrap_start)
		wrap_cnt = wrap_end - wrap_start;
	else {
		/* wrap counter has wrapped around, invalidate start position */
		wrap_cnt = 2;
}

	if (wrap_cnt > 1) {
		/* end position has wrapped around more than once, */
		/* current start no longer valid                   */
		log_start->wrap = log->log_pos.wrap - 1;
		log_start->offset = (log->log_pos.offset + 1) % log_len;
	} else if ((wrap_cnt == 1) &&
		(log->log_pos.offset > log_start->offset)) {
		/* end position has overwritten start */
		log_start->offset = (log->log_pos.offset + 1) % log_len;
	}
	pr_debug("diag_buf wrap = %u, offset = %u\n",
		log->log_pos.wrap, log->log_pos.offset);

	while (log_start->offset == log->log_pos.offset) {
		/*
		 * No data in ring buffer,
		 * so we'll hang around until something happens
		 */
		unsigned long t = msleep_interruptible(50);

		if (t != 0) {
			/* Some event woke us up, so let's quit */
			return 0;
		}

		if (buf_idx == TZDBG_LOG) {
			mutex_lock(&tzdbg_mutex);
			memcpy_fromio((void *)tzdbg.diag_buf, tzdbg.virt_iobase,
						debug_rw_buf_size);
			mutex_unlock(&tzdbg_mutex);
		}
	}

	max_len = (count > debug_rw_buf_size) ? debug_rw_buf_size : count;

	pr_debug("diag_buf wrap = %u, offset = %u\n",
		log->log_pos.wrap, log->log_pos.offset);
	mutex_lock(&tzdbg_mutex);
	atomic_set(&is_rd_locked, 1);
#if (KERNEL_VERSION(6, 12, 0) <= LINUX_VERSION_CODE) && defined(CONFIG_TZLOG_TIME_CONSOLIDATE)
	if (g_realtime_consolidation_enable)
		len = _copy_to_dispbuf_with_realtime_v2(log, log_start, log_len, max_len,
			tzdbg.disp_buf, 0);
	else
		len = _copy_to_dispbuf_v2(log, log_start, log_len, max_len, tzdbg.disp_buf, 0);
#else
	len = _copy_to_dispbuf_v2(log, log_start, log_len, max_len, tzdbg.disp_buf, 0);
#endif

	/*
	 * return buffer to caller
	 */
	tzdbg.stat[buf_idx].data = tzdbg.disp_buf;

	return len;
}

static int __disp_hyp_log_stats(uint8_t *log,
			struct hypdbg_log_pos_t *log_start, uint32_t log_len,
			size_t count, uint32_t buf_idx)
{
	struct hypdbg_t *hyp = tzdbg.hyp_diag_buf;
	unsigned long t = 0;
	uint32_t wrap_start;
	uint32_t wrap_end;
	uint32_t wrap_cnt;
	int max_len;
	int len = 0;
	int i = 0;

	wrap_start = log_start->wrap;
	wrap_end = hyp->log_pos.wrap;

	/* Calculate difference in # of buffer wrap-arounds */
	if (wrap_end >= wrap_start)
		wrap_cnt = wrap_end - wrap_start;
	else {
		/* wrap counter has wrapped around, invalidate start position */
		wrap_cnt = 2;
	}

	if (wrap_cnt > 1) {
		/* end position has wrapped around more than once, */
		/* current start no longer valid                   */
		log_start->wrap = hyp->log_pos.wrap - 1;
		log_start->offset = (hyp->log_pos.offset + 1) % log_len;
	} else if ((wrap_cnt == 1) &&
		(hyp->log_pos.offset > log_start->offset)) {
		/* end position has overwritten start */
		log_start->offset = (hyp->log_pos.offset + 1) % log_len;
	}

	while (log_start->offset == hyp->log_pos.offset) {
		/*
		 * No data in ring buffer,
		 * so we'll hang around until something happens
		 */
		t = msleep_interruptible(50);
		if (t != 0) {
			/* Some event woke us up, so let's quit */
			return 0;
		}

		/* TZDBG_HYP_LOG */
		mutex_lock(&tzdbg_mutex);
		memcpy_fromio((void *)tzdbg.hyp_diag_buf, tzdbg.hyp_virt_iobase,
						tzdbg.hyp_debug_rw_buf_size);
		mutex_unlock(&tzdbg_mutex);
	}

	max_len = (count > tzdbg.hyp_debug_rw_buf_size) ?
				tzdbg.hyp_debug_rw_buf_size : count;

	/*
	 *  Read from ring buff while there is data and space in return buff
	 */
	mutex_lock(&tzdbg_mutex);
	atomic_set(&is_rd_locked, 1);
	while ((log_start->offset != hyp->log_pos.offset) && (len < max_len)) {
		tzdbg.disp_buf[i++] = log[log_start->offset];
		log_start->offset = (log_start->offset + 1) % log_len;
		if (log_start->offset == 0)
			++log_start->wrap;
		++len;
	}

	/*
	 * return buffer to caller
	 */
	tzdbg.stat[buf_idx].data = tzdbg.disp_buf;
	return len;
}
static int __disp_rm_log_stats(uint8_t *log_ptr, uint32_t max_len)
{
	uint32_t i = 0;
	/*
	 *  Transfer data from rm dialog buff to display buffer in user space
	 */
	mutex_lock(&tzdbg_mutex);
	atomic_set(&is_rd_locked, 1);
	while ((i < max_len) && (i < display_buf_size)) {
		tzdbg.disp_buf[i] = log_ptr[i];
		i++;
	}
	if (i != max_len)
		pr_err("Dropping RM log message, max_len:%d display_buf_size:%d\n",
			i, display_buf_size);
	tzdbg.stat[TZDBG_RM_LOG].data = tzdbg.disp_buf;
	return i;
}

static int print_text(char *intro_message,
			unsigned char *text_addr,
			unsigned int size,
			char *buf, uint32_t buf_len)
{
	unsigned int   i;
	int len = 0;

	pr_debug("begin address %p, size %d\n", text_addr, size);
	len += scnprintf(buf + len, buf_len - len, "%s\n", intro_message);
	for (i = 0;  i < size;  i++) {
		if (buf_len <= len + 6) {
			pr_err("buffer not enough, buf_len %d, len %d\n",
				buf_len, len);
			return buf_len;
		}
		len += scnprintf(buf + len, buf_len - len, "%02hhx ",
					text_addr[i]);
		if ((i & 0x1f) == 0x1f)
			len += scnprintf(buf + len, buf_len - len, "%c", '\n');
	}
	len += scnprintf(buf + len, buf_len - len, "%c", '\n');
	return len;
}

static int _disp_encrpted_log_stats(struct encrypted_log_info *enc_log_info,
				enum tzdbg_stats_type type, uint32_t log_id)
{
	int ret = 0, len = 0;
	struct tzbsp_encr_log_t *encr_log_head;
	uint32_t size = 0;

	if ((!tzdbg.is_full_encrypted_tz_logs_supported) &&
		(tzdbg.is_full_encrypted_tz_logs_enabled))
		pr_info("TZ not supporting full encrypted log functionality\n");

	pr_debug("enc_log_info: paddr: 0x%llx, size: %zu\n",
		 (uint64_t)enc_log_info->paddr, enc_log_info->size);
	ret = qcom_scm_request_encrypted_log(enc_log_info->paddr,
		enc_log_info->size, log_id, tzdbg.is_full_encrypted_tz_logs_supported,
		tzdbg.is_full_encrypted_tz_logs_enabled);
	if (ret) {
		pr_debug("request_encrypted_log scm failed, ret: %d\n", ret);
		return 0;
	}
	encr_log_head = (struct tzbsp_encr_log_t *)(enc_log_info->vaddr);
	pr_debug("display_buf_size = %d, encr_log_buff_size = %d\n",
		display_buf_size, encr_log_head->encr_log_buff_size);
	size = encr_log_head->encr_log_buff_size;

	len += scnprintf(tzdbg.disp_buf + len,
			(display_buf_size - 1) - len,
			"\n-------- New Encrypted %s --------\n",
			((log_id == ENCRYPTED_QSEE_LOG_ID) ?
				"QSEE Log" : "TZ Dialog"));

	len += scnprintf(tzdbg.disp_buf + len,
			(display_buf_size - 1) - len,
			"\nMagic_Num :\n0x%x\n"
			"\nVerion :\n%d\n"
			"\nEncr_Log_Buff_Size :\n%d\n"
			"\nWrap_Count :\n%d\n",
			encr_log_head->magic_num,
			encr_log_head->version,
			encr_log_head->encr_log_buff_size,
			encr_log_head->wrap_count);

	len += print_text("\nKey : ", encr_log_head->key,
			TZBSP_AES_256_ENCRYPTED_KEY_SIZE,
			tzdbg.disp_buf + len, display_buf_size);
	len += print_text("\nNonce : ", encr_log_head->nonce,
			TZBSP_NONCE_LEN,
			tzdbg.disp_buf + len, display_buf_size - len);
	len += print_text("\nTag : ", encr_log_head->tag,
			TZBSP_TAG_LEN,
			tzdbg.disp_buf + len, display_buf_size - len);

#if (KERNEL_VERSION(6, 12, 0) <= LINUX_VERSION_CODE) && defined(CONFIG_TZLOG_TIME_CONSOLIDATE)
	len += scnprintf(tzdbg.disp_buf + len, (display_buf_size - 1) - len,
			"\nHlos_Real_Time_Baseline :\n%llx\n\nHlos_Uptime_Baseline :\n%llx\n\n"
			"\nTZ_Uptime_Ticks_Baseline :\n%llx\n\nTZ_Tick_Frequency :\n%llx\n",
			ktime_mono_to_real(g_hlos_uptime_baseline), g_hlos_uptime_baseline,
			g_tz_ticks_baseline, g_tz_ticks_frequency);
#endif

	if (len > display_buf_size - size)
		pr_warn("Cannot fit all info into the buffer\n");

	pr_debug("encrypted log size %d, disply buffer size %d, used len %d\n",
			size, display_buf_size, len);

	len += print_text("\nLog : ", encr_log_head->log_buf, size,
				tzdbg.disp_buf + len, display_buf_size - len);

	memset(enc_log_info->vaddr, 0, enc_log_info->size);
	tzdbg.stat[type].data = tzdbg.disp_buf;
	return len;
}

static int check_tz_qsee_log_state(struct tzdbg_log_t *log, struct tzdbg_log_pos_t *log_start,
		struct tzdbg_log_v2_t *log_v2, struct tzdbg_log_pos_v2_t *log_start_v2)
{
	int ret = 0;

	if (!tzdbg.is_enlarged_buf) {
		if (!(log_start->offset == log->log_pos.offset &&
			log_start->wrap == log->log_pos.wrap))
			ret = 1;
	} else {
		if (!(log_start_v2->offset == log_v2->log_pos.offset &&
			log_start_v2->wrap == log_v2->log_pos.wrap))
			ret = 1;
	}

	return ret;
}

static int _disp_tz_log_stats(struct clients_info_t *clients_info,
		size_t count, bool check_log_state)
{
	struct tzdbg_log_v2_t *log_v2_ptr;
	struct tzdbg_log_t *log_ptr;

	log_ptr = (struct tzdbg_log_t *)((unsigned char *)tzdbg.diag_buf +
			tzdbg.diag_buf->ring_off -
			offsetof(struct tzdbg_log_t, log_buf));

	log_v2_ptr = (struct tzdbg_log_v2_t *)((unsigned char *)tzdbg.diag_buf +
			tzdbg.diag_buf->ring_off -
			offsetof(struct tzdbg_log_v2_t, log_buf));

	pr_debug("log_start: [wrap,offset]:[0x%x, 0x%x], log_start_v2: [wrap,offset]: [0x%x, 0x%x]\n",
		clients_info->log_start.wrap, clients_info->log_start.offset,
		clients_info->log_start_v2.wrap, clients_info->log_start_v2.offset);

	pr_debug("log_ptr: [wrap,offset]:[0x%x, 0x%x], log_v2_ptr: [wrap,offset]:[0x%x, 0x%x]\n",
		log_ptr->log_pos.wrap, log_ptr->log_pos.offset,
		log_v2_ptr->log_pos.wrap, log_v2_ptr->log_pos.offset);

	if (check_log_state)
		return check_tz_qsee_log_state(log_ptr, &clients_info->log_start,
				log_v2_ptr, &clients_info->log_start_v2);

	if (!tzdbg.is_enlarged_buf)
		return _disp_log_stats(log_ptr, &clients_info->log_start,
				tzdbg.diag_buf->ring_len, count, TZDBG_LOG);

	return _disp_log_stats_v2(log_v2_ptr, &clients_info->log_start_v2,
			tzdbg.diag_buf->ring_len, count, TZDBG_LOG);
}

static int _disp_hyp_log_stats(size_t count)
{
	static struct hypdbg_log_pos_t log_start = {0};
	uint8_t *log_ptr;
	uint32_t log_len;

	log_ptr = (uint8_t *)((unsigned char *)tzdbg.hyp_diag_buf +
				tzdbg.hyp_diag_buf->ring_off);
	log_len = tzdbg.hyp_debug_rw_buf_size - tzdbg.hyp_diag_buf->ring_off;

	return __disp_hyp_log_stats(log_ptr, &log_start,
			log_len, count, TZDBG_HYP_LOG);
}

static int _disp_rm_log_stats(size_t count)
{
	static struct rmdbg_log_pos_t log_start = { 0 };
	struct rmdbg_log_hdr_t *p_log_hdr = NULL;
	uint8_t *log_ptr = NULL;
	uint32_t log_len = 0;
	static bool wrap_around = { false };

	/* Return 0 to close the display file,if there is nothing else to do */
	if ((log_start.size == 0x0) && wrap_around) {
		wrap_around = false;
		return 0;
	}
	/* Copy RM log data to tzdbg diag buffer for the first time */
	/* Initialize the tracking data structure */
	if (tzdbg.rmlog_rw_buf_size != 0) {
		if (!wrap_around) {
			mutex_lock(&tzdbg_mutex);
			memcpy_fromio((void *)tzdbg.rm_diag_buf,
					tzdbg.rmlog_virt_iobase,
					tzdbg.rmlog_rw_buf_size);
			mutex_unlock(&tzdbg_mutex);
			/* get RM header info first */
			p_log_hdr = (struct rmdbg_log_hdr_t *)tzdbg.rm_diag_buf;
			/* Update RM log buffer index tracker and its size */
			log_start.read_idx = 0x0;
			log_start.size = p_log_hdr->size;
			/* Add size sanity check so that it doesn't exceed total size */
			if (tzdbg.rmlog_rw_buf_size < log_start.size) {
				pr_err("RM log size cannot be greater than buffer size, Exiting..\n");
				return 0;
			}
		}
		/* Update RM log buffer starting ptr */
		log_ptr =
			(uint8_t *) ((unsigned char *)tzdbg.rm_diag_buf +
				 sizeof(struct rmdbg_log_hdr_t));
		pr_debug("log_start.read_idx: %#x, log_start.size: %#x\n",
				log_start.read_idx, log_start.size);
	} else {
	/* Return 0 to close the display file,if there is nothing else to do */
		pr_err("There is no RM log to read, size is %d!\n",
			tzdbg.rmlog_rw_buf_size);
		return 0;
	}
	log_len = log_start.size;
	log_ptr += log_start.read_idx;
	/* Check if we exceed the max length provided by user space */
	log_len = (count > log_len) ? log_len : count;
	/* Update tracking data structure */
	log_start.size -= log_len;
	log_start.read_idx += log_len;
	pr_debug("log_len: %d, log_start.read_idx: %#x, log_start.size: %#x\n",
			log_len, log_start.read_idx, log_start.size);

	if (log_start.size)
		wrap_around =  true;
	return __disp_rm_log_stats(log_ptr, log_len);
}

static int _disp_qsee_log_stats(struct clients_info_t *clients_info,
		size_t count, bool check_log_state)
{
	if (!tzdbg.tz_qsee_plain_log_enabled)
		return 0;

	pr_debug("log_start: [wrap,offset]:[0x%x, 0x%x], log_start_v2: [wrap,offset]: [0x%x, 0x%x]\n",
		clients_info->log_start.wrap, clients_info->log_start.offset,
		clients_info->log_start_v2.wrap, clients_info->log_start_v2.offset);

	pr_debug("g_qsee_log: [wrap,offset]:[0x%x, 0x%x], g_qsee_log_v2: [wrap,offset]:[0x%x, 0x%x]\n",
		g_qsee_log->log_pos.wrap, g_qsee_log->log_pos.offset,
		g_qsee_log_v2->log_pos.wrap, g_qsee_log_v2->log_pos.offset);
	int ret = 0;
	mutex_lock(&(tzdbg.lock));
	if (check_log_state)
		ret = check_tz_qsee_log_state(g_qsee_log, &clients_info->log_start,
			g_qsee_log_v2, &clients_info->log_start_v2);

	if (!tzdbg.is_enlarged_buf){
		ret = _disp_log_stats(g_qsee_log, &clients_info->log_start,
			QSEE_LOG_BUF_SIZE - sizeof(struct tzdbg_log_pos_t),
			count, TZDBG_QSEE_LOG);
        }else{

	        ret = _disp_log_stats_v2(g_qsee_log_v2, &clients_info->log_start_v2,
		QSEE_LOG_BUF_SIZE_V2 - sizeof(struct tzdbg_log_pos_v2_t),
		count, TZDBG_QSEE_LOG);
        }
	mutex_unlock(&(tzdbg.lock));
	return ret;
}

static int _disp_hyp_general_stats(size_t count)
{
	int len = 0;
	int i;
	struct hypdbg_boot_info_t *ptr = NULL;

	mutex_lock(&tzdbg_mutex);
	atomic_set(&is_rd_locked, 1);
	len += scnprintf((unsigned char *)tzdbg.disp_buf + len,
			tzdbg.hyp_debug_rw_buf_size - 1,
			"   Magic Number    : 0x%x\n"
			"   CPU Count       : 0x%x\n"
			"   S2 Fault Counter: 0x%x\n",
			tzdbg.hyp_diag_buf->magic_num,
			tzdbg.hyp_diag_buf->cpu_count,
			tzdbg.hyp_diag_buf->s2_fault_counter);

	ptr = tzdbg.hyp_diag_buf->boot_info;
	for (i = 0; i < tzdbg.hyp_diag_buf->cpu_count; i++) {
		len += scnprintf((unsigned char *)tzdbg.disp_buf + len,
				(tzdbg.hyp_debug_rw_buf_size - 1) - len,
				"  CPU #: %d\n"
				"     Warmboot entry CPU counter: 0x%x\n"
				"     Warmboot exit CPU counter : 0x%x\n",
				i, ptr->warm_entry_cnt, ptr->warm_exit_cnt);

		if (len > (tzdbg.hyp_debug_rw_buf_size - 1)) {
			pr_warn("%s: Cannot fit all info into the buffer\n",
								__func__);
			break;
		}
		ptr++;
	}

	tzdbg.stat[TZDBG_HYP_GENERAL].data = (char *)tzdbg.disp_buf;
	return len;
}

#if IS_ENABLED(CONFIG_MSM_TMECOM_QMP)
static int _disp_tme_log_stats(size_t count)
{
	static struct tme_log_pos log_start = { 0 };
	static bool wrap_around = { false };
	uint32_t buf_size;
	uint8_t *log_ptr = NULL;
	uint32_t log_len = 0;
	int ret = 0;

	/* Return 0 to close the display file */
	if ((log_start.size == 0x0) && wrap_around) {
		wrap_around = false;
		return 0;
	}

	/* Copy TME log data to tzdbg diag buffer for the first time */
	if (!wrap_around) {
		if (tmelog_process_request(tmecrashdump_address_offset,
					   TME_LOG_BUF_SIZE, &buf_size)) {
			pr_err("Read tme log failed, ret=%d, buf_size: %#x\n", ret, buf_size);
			return 0;
		}
		log_start.offset = 0x0;
		log_start.size = buf_size;
	}

	log_ptr = tzdbg.tmelog_virt_iobase;
	log_len = log_start.size;
	log_ptr += log_start.offset;

	/* Check if we exceed the max length provided by user space */
	log_len = min(min((uint32_t)count, log_len), display_buf_size);

	log_start.size -= log_len;
	log_start.offset += log_len;
	pr_debug("log_len: %d, log_start.offset: %#x, log_start.size: %zu\n",
			log_len, log_start.offset, log_start.size);

	if (log_start.size)
		wrap_around =  true;

	/* Copy TME log data to display buffer */
	mutex_lock(&tzdbg_mutex);
	atomic_set(&is_rd_locked, 1);
	memcpy_fromio(tzdbg.disp_buf, log_ptr, log_len);

	tzdbg.stat[TZDBG_TME_LOG].data = tzdbg.disp_buf;
	return log_len;
}
#else
static int _disp_tme_log_stats(size_t count)
{
	return 0;
}
#endif

static ssize_t tzdbg_fs_read_unencrypted(struct clients_info_t *clients_info, int tz_id,
	char __user *buf, size_t count, loff_t *offp)
{
	int len = 0;
	size_t len_out = 0;

	if (tz_id == TZDBG_BOOT || tz_id == TZDBG_RESET ||
		tz_id == TZDBG_INTERRUPT || tz_id == TZDBG_GENERAL ||
		tz_id == TZDBG_VMID || tz_id == TZDBG_LOG) {
		if (!tzdbg.tz_qsee_plain_log_enabled)
			return 0;

		pr_debug("TZ diag region is directly accessible, copy data now.\n");
		mutex_lock(&tzdbg_mutex);
		memcpy_fromio((void *)tzdbg.diag_buf, tzdbg.virt_iobase, debug_rw_buf_size);
		mutex_unlock(&tzdbg_mutex);
	}

	if (tz_id == TZDBG_HYP_GENERAL || tz_id == TZDBG_HYP_LOG) {
		mutex_lock(&tzdbg_mutex);
		memcpy_fromio((void *)tzdbg.hyp_diag_buf,
				tzdbg.hyp_virt_iobase,
				tzdbg.hyp_debug_rw_buf_size);
		mutex_unlock(&tzdbg_mutex);
	}
	switch (tz_id) {
	case TZDBG_BOOT:
		len = _disp_tz_boot_stats();
		break;
	case TZDBG_RESET:
		len = _disp_tz_reset_stats();
		break;
	case TZDBG_INTERRUPT:
		len = _disp_tz_interrupt_stats();
		break;
	case TZDBG_GENERAL:
		len = _disp_tz_general_stats();
		break;
	case TZDBG_VMID:
		len = _disp_tz_vmid_stats();
		break;
	case TZDBG_LOG:
		if (TZBSP_DIAG_MAJOR_VERSION_LEGACY <
				(tzdbg.diag_buf->version >> 16)) {
			len = _disp_tz_log_stats(clients_info, count, false);
			*offp = 0;
		} else {
			len = _disp_tz_log_stats_legacy();
		}
		break;
	case TZDBG_QSEE_LOG:
		len = _disp_qsee_log_stats(clients_info, count, false);
		*offp = 0;
		break;
	case TZDBG_HYP_GENERAL:
		len = _disp_hyp_general_stats(count);
		break;
	case TZDBG_HYP_LOG:
		len = _disp_hyp_log_stats(count);
		*offp = 0;
		break;
	case TZDBG_RM_LOG:
		len = _disp_rm_log_stats(count);
		*offp = 0;
		break;
	case TZDBG_TME_LOG:
		len = _disp_tme_log_stats(count);
		*offp = 0;
		break;
	default:
		break;
	}

	if (len > count)
		len = count;

	len_out = simple_read_from_buffer(buf, len, offp,
				tzdbg.stat[tz_id].data, len);

	if (atomic_read(&is_rd_locked)) {
		atomic_set(&is_rd_locked, 0);
		mutex_unlock(&tzdbg_mutex);
	}

	return len_out;
}

static int is_log_ready(int tz_id, struct clients_info_t *clients_info)
{
	int ret = 0;
	struct tzbsp_encr_log_t *encr_log_head = NULL;
	struct encrypted_log_info *enc_log_info = NULL;
	uint32_t size = 0;
	uint32_t log_id = 0;

	if (tzdbg.is_encrypted_log_enabled) {
		if ((!tzdbg.is_full_encrypted_tz_logs_supported)
				&& (tzdbg.is_full_encrypted_tz_logs_enabled))
			pr_info("TZ not supporting full encrypted log functionality\n");
		if (tz_id == TZDBG_LOG) {
			enc_log_info = &enc_tzlog_info;
			log_id = ENCRYPTED_TZ_LOG_ID;
		} else { // Can be qsee log only
			enc_log_info = &enc_qseelog_info;
			log_id = ENCRYPTED_QSEE_LOG_ID;
		}
		pr_debug("enc_log_info: paddr: 0x%llx, size: %zu\n",
			 (uint64_t)enc_log_info->paddr, enc_log_info->size);
		ret = qcom_scm_request_encrypted_log(enc_log_info->paddr,
			enc_log_info->size, log_id, tzdbg.is_full_encrypted_tz_logs_supported,
			tzdbg.is_full_encrypted_tz_logs_enabled);
		if (ret) {
			pr_debug("request_encrypted_log scm failed, ret: %d\n", ret);
			return 0;
		}
		encr_log_head = (struct tzbsp_encr_log_t *)(enc_log_info->vaddr);
		size = encr_log_head->encr_log_buff_size;

		/*
		 * 2nd time scm call always fail due to current QTEE behavior.
		 * Make another call here so that read system call from userspace get correct logs.
		 * Do nothing for this scm failure.
		 *
		 * This scm might require removal or updation as per QTEE behavior later.
		 */
		ret = qcom_scm_request_encrypted_log(enc_log_info->paddr,
			enc_log_info->size, log_id, tzdbg.is_full_encrypted_tz_logs_supported,
			tzdbg.is_full_encrypted_tz_logs_enabled);
		if (ret)
			pr_debug("2nd request_encrypted_log scm expected to fail, ret: %d\n", ret);
		ret = size;
	} else {
		if (tz_id == TZDBG_LOG) {
			// update tz_log buffer
			memcpy_fromio((void *)tzdbg.diag_buf, tzdbg.virt_iobase, debug_rw_buf_size);
			ret = _disp_tz_log_stats(clients_info, 0, true);
		} else {
			ret = _disp_qsee_log_stats(clients_info, 0, true);
		}
	}

	pr_debug("log_id: %d, ret: %d\n", log_id, ret);
	return ret;
}

static ssize_t tzdbg_fs_read_encrypted(struct clients_info_t *clients_info,
	int tz_id, char __user *buf, size_t count, loff_t *offp)
{
	int len = 0, ret = 0;

	pr_debug("%s: tz_id = %d\n", __func__, tz_id);

	if (tz_id >= TZDBG_STATS_MAX) {
		pr_err("invalid encrypted log id %d\n", tz_id);
		return ret;
	}

	mutex_lock(&tzdbg_mutex);
	if (!clients_info->display_len) {

		if (tz_id == TZDBG_QSEE_LOG)
			clients_info->display_len = _disp_encrpted_log_stats(
					&enc_qseelog_info,
					tz_id, ENCRYPTED_QSEE_LOG_ID);
		else
			clients_info->display_len = _disp_encrpted_log_stats(
					&enc_tzlog_info,
					tz_id, ENCRYPTED_TZ_LOG_ID);
		clients_info->display_offset = 0;
	}
	len = clients_info->display_len;
	if (len > count)
		len = count;

	*offp = 0;
	ret = simple_read_from_buffer(buf, len, offp,
				tzdbg.stat[tz_id].data + clients_info->display_offset,
				count);
	clients_info->display_offset += ret;
	clients_info->display_len -= ret;
	mutex_unlock(&tzdbg_mutex);

	pr_debug("ret = %d, offset = %d\n", ret, (int)(*offp));
	pr_debug("display_len = %lu, offset = %lu\n",
			clients_info->display_len, clients_info->display_offset);
	return ret;
}

static ssize_t tzdbg_fs_read(struct file *file, char __user *buf,
	size_t count, loff_t *offp)
{
	struct seq_file *seq = file->private_data;
	int tz_id = TZDBG_STATS_MAX;
	ssize_t len = 0;
	struct clients_info_t *clients_info = NULL;

	mutex_lock(&tzdbg_mutex);
	list_for_each_entry(clients_info, &clients_list, list) {
		if (clients_info->file == file)
			break;
	}
	mutex_unlock(&tzdbg_mutex);
	if (!clients_info)
		return -ENODATA;

	if (seq)
		tz_id = *(int *)(seq->private);
	else {
		pr_err("%s: Seq data null unable to proceed\n", __func__);
		return 0;
	}

	if (!tzdbg.is_encrypted_log_enabled ||
	    (tz_id == TZDBG_HYP_GENERAL || tz_id == TZDBG_HYP_LOG)
	    || tz_id == TZDBG_RM_LOG || tz_id == TZDBG_TME_LOG)
		len = tzdbg_fs_read_unencrypted(clients_info, tz_id, buf, count, offp);
	else
		len = tzdbg_fs_read_encrypted(clients_info, tz_id, buf, count, offp);

	return len;
}

static int tzdbg_procfs_open(struct inode *inode, struct file *file)
{
	struct clients_info_t *clients_info = NULL;
	int ret = 0;

	clients_info = kzalloc(sizeof(*clients_info), GFP_KERNEL);
	if (!clients_info)
		return -ENOMEM;

	clients_info->file = file;
	mutex_lock(&tzdbg_mutex);
	list_add(&clients_info->list, &clients_list);
	mutex_unlock(&tzdbg_mutex);

#if (LINUX_VERSION_CODE <= KERNEL_VERSION(6,0,0))
	ret = single_open(file, NULL, PDE_DATA(inode));
#else
	ret = single_open(file, NULL, pde_data(inode));
#endif
	if (ret) {
		mutex_lock(&tzdbg_mutex);
		list_del(&clients_info->list);
		mutex_unlock(&tzdbg_mutex);
		kfree(clients_info);
	}

	return ret;
}

static int tzdbg_procfs_release(struct inode *inode, struct file *file)
{
	struct clients_info_t *clients_info = NULL;

	mutex_lock(&tzdbg_mutex);
	list_for_each_entry(clients_info, &clients_list, list) {
		if (clients_info->file == file) {
			list_del(&clients_info->list);
			kfree(clients_info);
			break;
		}
	}
	mutex_unlock(&tzdbg_mutex);

	return single_release(inode, file);
}

static loff_t tzdbg_procfs_lseek(struct file *file, loff_t offset, int whence)
{
	pr_warn("%s: Operation not supported\n", __func__);
	return -EOPNOTSUPP;
}

static __poll_t tzdbg_procfs_poll(struct file *file, struct poll_table_struct *wait)
{
	struct seq_file *seq = file->private_data;
	int tz_id = *(int *)(seq->private);
	struct clients_info_t *clients_info = NULL;

	/*
	 * Bail out early if below conditions are met
	 * 1. Polling available for tz/qsee logs only
	 * 2. If encryption and plain text logs both are not enabled.
	 *
	 * Return POLLERR so that userspace doesn't poll again in this case.
	 */
	if (!((tz_id == TZDBG_LOG || tz_id == TZDBG_QSEE_LOG) &&
		(tzdbg.is_encrypted_log_enabled || tzdbg.tz_qsee_plain_log_enabled)))
		return POLLERR;

	mutex_lock(&tzdbg_mutex);
	list_for_each_entry(clients_info, &clients_list, list) {
		if (clients_info->file == file)
			break;
	}
	mutex_unlock(&tzdbg_mutex);

	if (is_log_ready(tz_id, clients_info)) {
		pr_debug("Setting polling flag true, tzid: %d!\n", tz_id);
		return EPOLLIN | EPOLLRDNORM;
	}

	return 0;
}

struct proc_ops tzdbg_fops = {
	.proc_flags   = PROC_ENTRY_PERMANENT,
	.proc_read    = tzdbg_fs_read,
	.proc_open    = tzdbg_procfs_open,
	.proc_release = tzdbg_procfs_release,
/* mandatory unless nonseekable_open() or equivalent is used */
	.proc_lseek   = tzdbg_procfs_lseek,
	.proc_poll    = tzdbg_procfs_poll,
};

static int tzdbg_init_tme_log(struct platform_device *pdev, void __iomem *virt_iobase)
{
	/*
	 * Tme logs are dumped in tme log ddr region but that region is not
	 * accessible to hlos. Instead, collect logs at tme crashdump ddr
	 * region with tmecom interface and then display logs reading from
	 * crashdump region.
	 */
	if (of_property_read_u32((&pdev->dev)->of_node, "tmecrashdump-address-offset",
				&tmecrashdump_address_offset)) {
		pr_err("Tme Crashdump address offset need to be defined!\n");
		return -EINVAL;
	}

	tzdbg.tmelog_virt_iobase =
		devm_ioremap(&pdev->dev, tmecrashdump_address_offset, TME_LOG_BUF_SIZE);
	if (!tzdbg.tmelog_virt_iobase) {
		pr_err("ERROR: Could not ioremap: start=%#x, len=%u\n",
				tmecrashdump_address_offset, TME_LOG_BUF_SIZE);
		return -ENXIO;
	}

	return 0;
}

/*
 * Allocates log buffer in HLOS and register with QTEE.
 */
static int tzdbg_register_qsee_log_buf(struct platform_device *pdev)
{
	int ret = 0;
	void *buf = NULL;
	uint32_t ns_vmids[] = {VMID_HLOS};
	uint32_t ns_vm_perms[] = {PERM_READ | PERM_WRITE};
	uint32_t ns_vm_nums = 1;

	if (tzdbg.is_enlarged_buf) {
		if (of_property_read_u32((&pdev->dev)->of_node,
			"qseelog-buf-size-v2", &qseelog_buf_size)) {
			pr_debug("Enlarged qseelog buf size isn't defined\n");
			qseelog_buf_size = QSEE_LOG_BUF_SIZE_V2;
		}
	}  else {
		qseelog_buf_size = QSEE_LOG_BUF_SIZE;
	}
	pr_debug("qseelog buf size is 0x%x\n", qseelog_buf_size);

	buf = dma_alloc_coherent(&pdev->dev,
			qseelog_buf_size, &coh_pmem, GFP_KERNEL);
	if (buf == NULL)
		return -ENOMEM;

	if (!tzdbg.is_encrypted_log_enabled) {
		ret = qtee_shmbridge_register(coh_pmem,
			qseelog_buf_size, ns_vmids, ns_vm_perms, ns_vm_nums,
			PERM_READ | PERM_WRITE,
			&qseelog_shmbridge_handle);
		if (ret) {
			pr_err("failed to create bridge for qsee_log buf\n");
			goto exit_free_mem;
		}
	}

	g_qsee_log = (struct tzdbg_log_t *)buf;
	g_qsee_log->log_pos.wrap = g_qsee_log->log_pos.offset = 0;

	g_qsee_log_v2 = (struct tzdbg_log_v2_t *)buf;
	g_qsee_log_v2->log_pos.wrap = g_qsee_log_v2->log_pos.offset = 0;

	/* Always register qsee log buffer */
	ret = qcom_scm_register_qsee_log_buf(coh_pmem, qseelog_buf_size);
	if (ret) {
		pr_err("scm_call to register log buf failed, resp result =%d\n", ret);
		goto exit_dereg_bridge;
	}

	return ret;

exit_dereg_bridge:
	if (!tzdbg.is_encrypted_log_enabled)
		qtee_shmbridge_deregister(qseelog_shmbridge_handle);
exit_free_mem:
	dma_free_coherent(&pdev->dev, qseelog_buf_size,
			(void *)g_qsee_log, coh_pmem);
	return ret;
}

static void tzdbg_free_qsee_log_buf(struct platform_device *pdev)
{
	if (!tzdbg.is_encrypted_log_enabled)
		qtee_shmbridge_deregister(qseelog_shmbridge_handle);
	dma_free_coherent(&pdev->dev, qseelog_buf_size,
				(void *)g_qsee_log, coh_pmem);
}

static int tzdbg_allocate_encrypted_log_buf(struct platform_device *pdev)
{
	int ret = 0;
	uint32_t ns_vmids[] = {VMID_HLOS};
	uint32_t ns_vm_perms[] = {PERM_READ | PERM_WRITE};
	uint32_t ns_vm_nums = 1;

	if (!tzdbg.is_encrypted_log_enabled)
		return 0;

	pr_debug("Register tz/qsee encrypted logs buffer\n");
	/* max encrypted qsee log buf zize (include header, and page align) */
	enc_qseelog_info.size = qseelog_buf_size + PAGE_SIZE;

	enc_qseelog_info.vaddr = dma_alloc_coherent(&pdev->dev,
					enc_qseelog_info.size,
					&enc_qseelog_info.paddr, GFP_KERNEL);
	if (enc_qseelog_info.vaddr == NULL)
		return -ENOMEM;

	ret = qtee_shmbridge_register(enc_qseelog_info.paddr,
			enc_qseelog_info.size, ns_vmids,
			ns_vm_perms, ns_vm_nums,
			PERM_READ | PERM_WRITE, &enc_qseelog_info.shmb_handle);
	if (ret) {
		pr_err("failed to create encr_qsee_log bridge, ret %d\n", ret);
		goto exit_free_qseelog;
	}
	pr_debug("Alloc memory for encr_qsee_log, size = %zu\n",
			enc_qseelog_info.size);

	enc_tzlog_info.size = debug_rw_buf_size;
	enc_tzlog_info.vaddr = dma_alloc_coherent(&pdev->dev,
					enc_tzlog_info.size,
					&enc_tzlog_info.paddr, GFP_KERNEL);
	if (enc_tzlog_info.vaddr == NULL)
		goto exit_unreg_qseelog;

	ret = qtee_shmbridge_register(enc_tzlog_info.paddr,
			enc_tzlog_info.size, ns_vmids, ns_vm_perms, ns_vm_nums,
			PERM_READ | PERM_WRITE, &enc_tzlog_info.shmb_handle);
	if (ret) {
		pr_err("failed to create encr_tz_log bridge, ret = %d\n", ret);
		goto exit_free_tzlog;
	}
	pr_debug("Alloc memory for encr_tz_log, size %zu\n",
		enc_qseelog_info.size);

	return 0;

exit_free_tzlog:
	dma_free_coherent(&pdev->dev, enc_tzlog_info.size,
			enc_tzlog_info.vaddr, enc_tzlog_info.paddr);
exit_unreg_qseelog:
	qtee_shmbridge_deregister(enc_qseelog_info.shmb_handle);
exit_free_qseelog:
	dma_free_coherent(&pdev->dev, enc_qseelog_info.size,
			enc_qseelog_info.vaddr, enc_qseelog_info.paddr);
	return -ENOMEM;
}

static void tzdbg_free_encrypted_log_buf(struct platform_device *pdev)
{
	qtee_shmbridge_deregister(enc_tzlog_info.shmb_handle);
	dma_free_coherent(&pdev->dev, enc_tzlog_info.size,
			enc_tzlog_info.vaddr, enc_tzlog_info.paddr);
	qtee_shmbridge_deregister(enc_qseelog_info.shmb_handle);
	dma_free_coherent(&pdev->dev, enc_qseelog_info.size,
			enc_qseelog_info.vaddr, enc_qseelog_info.paddr);
}

static int  tzdbg_fs_init(struct platform_device *pdev)
{
	int rc = 0;
	int i;
	struct proc_dir_entry *dent_dir;
	struct proc_dir_entry *dent;

	dent_dir = proc_mkdir(TZDBG_DIR_NAME, NULL);
	if (dent_dir == NULL) {
		dev_err(&pdev->dev, "tzdbg proc_mkdir failed\n");
		return -ENOMEM;
	}

	for (i = 0; i < TZDBG_STATS_MAX; i++) {
		tzdbg.debug_tz[i] = i;
		if (!tzdbg.stat[i].avail)
			continue;

		dent = proc_create_data(tzdbg.stat[i].name,
				0444, dent_dir,
				&tzdbg_fops, &tzdbg.debug_tz[i]);
		if (dent == NULL) {
			dev_err(&pdev->dev, "TZ proc_create_data failed\n");
			rc = -ENOMEM;
			goto err;
		}
	}
	platform_set_drvdata(pdev, dent_dir);
	return 0;
err:
	remove_proc_subtree(TZDBG_DIR_NAME, NULL);

	return rc;
}

static void tzdbg_fs_exit(struct platform_device *pdev)
{
	struct proc_dir_entry *dent_dir;

	dent_dir = platform_get_drvdata(pdev);
	if (dent_dir)
		remove_proc_subtree(TZDBG_DIR_NAME, NULL);
}

static int __update_hypdbg_base(struct platform_device *pdev,
			void __iomem *virt_iobase)
{
	phys_addr_t hypdiag_phy_iobase;
	uint32_t hyp_address_offset;
	uint32_t hyp_size_offset;
	struct hypdbg_t *hyp;
	uint32_t *ptr = NULL;

	if (of_property_read_u32((&pdev->dev)->of_node, "hyplog-address-offset",
							&hyp_address_offset)) {
		dev_err(&pdev->dev, "hyplog address offset is not defined\n");
		return -EINVAL;
	}
	if (of_property_read_u32((&pdev->dev)->of_node, "hyplog-size-offset",
							&hyp_size_offset)) {
		dev_err(&pdev->dev, "hyplog size offset is not defined\n");
		return -EINVAL;
	}

	hypdiag_phy_iobase = readl_relaxed(virt_iobase + hyp_address_offset);
	tzdbg.hyp_debug_rw_buf_size = readl_relaxed(virt_iobase +
					hyp_size_offset);

	tzdbg.hyp_virt_iobase = devm_ioremap(&pdev->dev,
					hypdiag_phy_iobase,
					tzdbg.hyp_debug_rw_buf_size);
	if (!tzdbg.hyp_virt_iobase) {
		dev_err(&pdev->dev, "ERROR could not ioremap: start=%pr, len=%u\n",
			&hypdiag_phy_iobase, tzdbg.hyp_debug_rw_buf_size);
		return -ENXIO;
	}

	ptr = kzalloc(tzdbg.hyp_debug_rw_buf_size, GFP_KERNEL);
	if (!ptr)
		return -ENOMEM;

	tzdbg.hyp_diag_buf = (struct hypdbg_t *)ptr;
	hyp = tzdbg.hyp_diag_buf;
	hyp->log_pos.wrap = hyp->log_pos.offset = 0;
	return 0;
}

static int __update_rmlog_base(struct platform_device *pdev,
			       void __iomem *virt_iobase)
{
	uint32_t rmlog_address;
	uint32_t rmlog_size;
	uint32_t *ptr = NULL;

	/* if we don't get the node just ignore it */
	if (of_property_read_u32((&pdev->dev)->of_node, "rmlog-address",
							&rmlog_address)) {
		dev_err(&pdev->dev, "RM log address is not defined\n");
		tzdbg.rmlog_rw_buf_size = 0;
		return 0;
	}
	/* if we don't get the node just ignore it */
	if (of_property_read_u32((&pdev->dev)->of_node, "rmlog-size",
							&rmlog_size)) {
		dev_err(&pdev->dev, "RM log size is not defined\n");
		tzdbg.rmlog_rw_buf_size = 0;
		return 0;
	}

	tzdbg.rmlog_rw_buf_size = rmlog_size;

	/* Check if there is RM log to read */
	if (!tzdbg.rmlog_rw_buf_size) {
		tzdbg.rmlog_virt_iobase = NULL;
		tzdbg.rm_diag_buf = NULL;
		dev_err(&pdev->dev, "RM log size is %d\n",
			tzdbg.rmlog_rw_buf_size);
		return 0;
	}

	tzdbg.rmlog_virt_iobase = devm_ioremap(&pdev->dev,
					rmlog_address,
					rmlog_size);
	if (!tzdbg.rmlog_virt_iobase) {
		dev_err(&pdev->dev, "ERROR could not ioremap: start=%u, len=%u\n",
			rmlog_address, tzdbg.rmlog_rw_buf_size);
		return -ENXIO;
	}

	ptr = kzalloc(tzdbg.rmlog_rw_buf_size, GFP_KERNEL);
	if (!ptr)
		return -ENOMEM;

	tzdbg.rm_diag_buf = (uint8_t *)ptr;
	return 0;
}
static int tzdbg_get_tz_version(void)
{
	u64 version;
	int ret = 0;

	ret = qcom_scm_get_tz_log_feat_id(&version);

	if (ret) {
		pr_err("%s: scm_call to get tz version failed\n",
				__func__);
		return ret;
	}
	tzdbg.tz_version = version;

	ret = qcom_scm_get_tz_feat_id_version(QCOM_SCM_FEAT_DIAG_ID, &version);
	if (ret) {
		pr_err("%s: scm_call to get tz diag version failed, ret = %d\n",
				__func__, ret);
		return ret;
	}
	pr_warn("tz diag version is %llu\n", version);
	tzdbg.tz_diag_major_version =
		((version >> TZBSP_FVER_MAJOR_SHIFT) & TZBSP_FVER_MAJOR_MINOR_MASK);
	tzdbg.tz_diag_minor_version =
		((version >> TZBSP_FVER_MINOR_SHIFT) & TZBSP_FVER_MAJOR_MINOR_MASK);
	if (tzdbg.tz_diag_major_version == TZBSP_DIAG_MAJOR_VERSION_V9) {
		switch (tzdbg.tz_diag_minor_version) {
		case TZBSP_DIAG_MINOR_VERSION_V2:
		case TZBSP_DIAG_MINOR_VERSION_V21:
		case TZBSP_DIAG_MINOR_VERSION_V22:
			tzdbg.is_enlarged_buf = true;
			break;
		default:
			tzdbg.is_enlarged_buf = false;
		}
	} else {
		tzdbg.is_enlarged_buf = false;
	}
	return ret;
}

static void tzdbg_query_log_status(void)
{
	int ret_encrypt_log = 0;
	int ret_query_log = 0;
	u64 status = 0;
	bool entry = false;

#if (KERNEL_VERSION(6, 12, 0) <= LINUX_VERSION_CODE)
	entry = true;
	ret_query_log = qcom_scm_query_log_status(&status);
	if (!ret_query_log) {
		/* status:
		 * Bit 0: encryption status
		 * Bit 1: tz/qsee plain text logging status
		 * --------------------------------------------------------------------
		 * |Bit 0|Bit 1| Comments                                             |
		 * --------------------------------------------------------------------
		 * |  1  |  0  | Possible combn, no direct access to tz/qsee buffer   |
		 * |  0  |  0  | Possible combn, no direct access to tz/qsee buffer.  |
		 * |  1  |  1  | Combn not possible                                   |
		 * |  0  |  1  | Possible combn, tz/qsee direct buffer access allowed |
		 * --------------------------------------------------------------------
		 */
		tzdbg.is_encrypted_log_enabled = status & 1;
		tzdbg.tz_qsee_plain_log_enabled = (status >> 1) & 1;
	} else
		pr_err("scm_call query_log_status failed, ret: %d\n", ret_query_log);
#endif
	if (!entry || ret_query_log == -EIO) {
		ret_encrypt_log = qcom_scm_query_encrypted_log_feature(&status);
		if (!ret_encrypt_log)
			tzdbg.is_encrypted_log_enabled = status;
		else
			pr_err("scm_call query_encr_log_feature failed ret: %d\n", ret_encrypt_log);

		/*
		 * Enable plain logging for below cases:
		 * 1. If query_encrypted_log scm isn't present in TZ.
		 * 2. If scm is success and encryption is not enabled.
		 */
		if (ret_encrypt_log == -EIO ||
			(!ret_encrypt_log && !tzdbg.is_encrypted_log_enabled))
			tzdbg.tz_qsee_plain_log_enabled = true;
	}

	pr_info("encryption: %d, plain log: %d, status: 0x%llx, entry: %d\n",
		tzdbg.is_encrypted_log_enabled, tzdbg.tz_qsee_plain_log_enabled, status, entry);
}

#if (KERNEL_VERSION(6, 12, 0) <= LINUX_VERSION_CODE) && defined(CONFIG_TZLOG_TIME_CONSOLIDATE)
static bool tzdbg_query_tz_time(void)
{
	int ret = 0;
	uint64_t ticks = 0;
	uint32_t frequency = 0;
	ktime_t begin_time = 0;
	ktime_t end_time = 0;

	begin_time = ktime_get_boottime_ns();

	ret = qcom_scm_query_tz_time(&ticks, &frequency);
	if (ret) {
		pr_err("QUERY_TZ_TIME_FEATURE failed ret %d\n", ret);
		return false;
	}

	end_time = ktime_get_boottime_ns();
	g_tz_ticks_baseline = ticks;
	g_tz_ticks_frequency = frequency;
	g_hlos_uptime_baseline = begin_time + (end_time - begin_time) / 2;

	return true;
}
#endif

/*
 * Driver functions
 */
static int tz_log_probe(struct platform_device *pdev)
{
	struct resource *resource;
	void __iomem *virt_iobase;
	phys_addr_t tzdiag_phy_iobase;
	uint32_t *ptr = NULL;
	int ret = 0, i;

	/*
	 * By default all nodes will be created.
	 * Mark avail as false later selectively if there's need to skip proc node creation.
	 */
	for (i = 0; i < TZDBG_STATS_MAX; i++)
		tzdbg.stat[i].avail = true;

	ret = tzdbg_get_tz_version();
	if (ret)
		return ret;

	/* Get address that stores the physical location diagnostic data */
	resource = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!resource) {
		dev_err(&pdev->dev,
				"%s: ERROR Missing MEM resource\n", __func__);
		return -ENXIO;
	}

	/* Get the debug buffer size */
	debug_rw_buf_size = resource_size(resource);

	/* Map address that stores the physical location diagnostic data */
	virt_iobase = devm_ioremap(&pdev->dev, resource->start,
				debug_rw_buf_size);
	if (!virt_iobase) {
		dev_err(&pdev->dev,
			"%s: ERROR could not ioremap: start=%pr, len=%u\n",
			__func__, &resource->start,
			(unsigned int)(debug_rw_buf_size));
		return -ENXIO;
	}

	if (pdev->dev.of_node) {
		tzdbg.is_hyplog_enabled = of_property_read_bool(
			(&pdev->dev)->of_node, "qcom,hyplog-enabled");
		if (tzdbg.is_hyplog_enabled) {
			ret = __update_hypdbg_base(pdev, virt_iobase);
			if (ret) {
				dev_err(&pdev->dev,
					"%s: fail to get hypdbg_base ret %d\n",
					__func__, ret);
				return -EINVAL;
			}
			ret = __update_rmlog_base(pdev, virt_iobase);
			if (ret) {
				dev_err(&pdev->dev,
					"%s: fail to get rmlog_base ret %d\n",
					__func__, ret);
				return -EINVAL;
			}
		} else {
			tzdbg.stat[TZDBG_HYP_LOG].avail = false;
			tzdbg.stat[TZDBG_HYP_GENERAL].avail = false;
			tzdbg.stat[TZDBG_RM_LOG].avail = false;
			dev_info(&pdev->dev, "Hyp log service not support\n");
		}
	} else {
		dev_dbg(&pdev->dev, "Device tree data is not found\n");
	}

	/* Retrieve the address of diagnostic data */
	tzdiag_phy_iobase = readl_relaxed(virt_iobase);

	tzdbg_query_log_status();

	/*
	 * TZ diag region is directly accessible if plain text logging is
	 * enabled.
	 *
	 * Map region only in this case otherwise skip it. The data mapped here
	 * will be displayed via tzdbg_fs_read_unencrypted func.
	 */
	if (tzdbg.tz_qsee_plain_log_enabled) {
		pr_debug("Create buffer for tz logs direct access\n");
		tzdbg.virt_iobase = devm_ioremap(&pdev->dev,
				tzdiag_phy_iobase, debug_rw_buf_size);

		if (!tzdbg.virt_iobase) {
			dev_err(&pdev->dev,
				"%s: could not ioremap: start=%pr, len=%u\n",
				__func__, &tzdiag_phy_iobase,
				debug_rw_buf_size);
			return -ENXIO;
		}
		/* allocate diag_buf */
		ptr = kzalloc(debug_rw_buf_size, GFP_KERNEL);
		if (ptr == NULL)
			return -ENOMEM;
		tzdbg.diag_buf = (struct tzdbg_t *)ptr;
	}

	if (tzdbg.is_encrypted_log_enabled) {
		if ((tzdbg.tz_diag_major_version == TZBSP_DIAG_MAJOR_VERSION_V9) &&
			(tzdbg.tz_diag_minor_version >= TZBSP_DIAG_MINOR_VERSION_V22))
			tzdbg.is_full_encrypted_tz_logs_supported = true;
		if (pdev->dev.of_node) {
			tzdbg.is_full_encrypted_tz_logs_enabled = of_property_read_bool(
				(&pdev->dev)->of_node, "qcom,full-encrypted-tz-logs-enabled");
		}
	}

	/* Init for tme log */
	ret = tzdbg_init_tme_log(pdev, virt_iobase);
	if (ret < 0) {
		tzdbg.stat[TZDBG_TME_LOG].avail = false;
		pr_warn("Tme log initialization failed!\n");
	}

	/*
	 * QTEE expects HLOS to always register a buffer so that it can log
	 * data to it. QTEE doesn't own any internal buffer for qsee logs.
	 *
	 * The buffer should be registered even if later, HLOS isn't allowed to
	 * access contents directly.
	 */
	ret = tzdbg_register_qsee_log_buf(pdev);
	if (ret) {
		pr_warn("Failure with plain qsee log buffer, Skipping qsee_log node creation..\n");
		tzdbg.stat[TZDBG_QSEE_LOG].avail = false;
	}

	/* Allocate encrypted qsee and tz log buffer if encryption is enabled */
	ret = tzdbg_allocate_encrypted_log_buf(pdev);
	if (ret) {
		dev_err(&pdev->dev,
			" %s: Failed to allocate encrypted log buffer\n",
			__func__);
		goto exit_free_qsee_log_buf;
	}

	/* allocate display_buf */
	if (UINT_MAX/4 < qseelog_buf_size) {
		pr_err("display_buf_size integer overflow\n");
		goto exit_free_qsee_log_buf;
	}
	display_buf_size = qseelog_buf_size * 4;
	tzdbg.disp_buf = dma_alloc_coherent(&pdev->dev, display_buf_size,
		&disp_buf_paddr, GFP_KERNEL);
	if (tzdbg.disp_buf == NULL) {
		ret = -ENOMEM;
		goto exit_free_encr_log_buf;
	}

#if (KERNEL_VERSION(6, 12, 0) <= LINUX_VERSION_CODE) && defined(CONFIG_TZLOG_TIME_CONSOLIDATE)
	g_realtime_consolidation_enable = tzdbg_query_tz_time();
	if (g_realtime_consolidation_enable)
		pr_info("Timestamp consolidation is enabled. Ticks is %lld, Frequency is %lld, Hlos time is %lld\n",
			g_tz_ticks_baseline, g_tz_ticks_frequency, g_hlos_uptime_baseline);
	else
		pr_info("Timestamp consolidation is not supported!\n");
#endif

	/* init tzdbg lock */
	mutex_init(&(tzdbg.lock));

	if (tzdbg_fs_init(pdev))
		goto exit_free_disp_buf;
	return 0;

exit_free_disp_buf:
	dma_free_coherent(&pdev->dev, display_buf_size,
			(void *)tzdbg.disp_buf, disp_buf_paddr);
exit_free_encr_log_buf:
	tzdbg_free_encrypted_log_buf(pdev);
exit_free_qsee_log_buf:
	tzdbg_free_qsee_log_buf(pdev);
	if (tzdbg.tz_qsee_plain_log_enabled)
		kfree(tzdbg.diag_buf);
	return -ENXIO;
}

#if KERNEL_VERSION(6, 10, 0) > LINUX_VERSION_CODE
static int tz_log_remove(struct platform_device *pdev)
#else
static void tz_log_remove(struct platform_device *pdev)
#endif
{
	tzdbg_fs_exit(pdev);
	dma_free_coherent(&pdev->dev, display_buf_size,
			(void *)tzdbg.disp_buf, disp_buf_paddr);
	tzdbg_free_encrypted_log_buf(pdev);
	tzdbg_free_qsee_log_buf(pdev);
	if (!tzdbg.is_encrypted_log_enabled)
		kfree(tzdbg.diag_buf);

#if KERNEL_VERSION(6, 10, 0) > LINUX_VERSION_CODE
	return 0;
#endif
}

static const struct of_device_id tzlog_match[] = {
	{.compatible = "qcom,tz-log"},
	{}
};

static struct platform_driver tz_log_driver = {
	.probe		= tz_log_probe,
	.remove		= tz_log_remove,
	.driver		= {
		.name = "tz_log",
		.of_match_table = tzlog_match,
		.probe_type = PROBE_PREFER_ASYNCHRONOUS,
	},
};

module_platform_driver(tz_log_driver);

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("TZ Log driver");
