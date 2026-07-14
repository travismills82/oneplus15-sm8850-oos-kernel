// SPDX-License-Identifier: GPL-2.0
/* Copyright (c) 2011-2018, The Linux Foundation. All rights reserved.
 * Copyright (c) 2018, Linaro Limited
 * Copyright (c) 2022-2023 Qualcomm Innovation Center, Inc. All rights reserved.
 */

#ifndef __FASTRPC_SHARED_H__
#define __FASTRPC_SHARED_H__

#include <linux/rpmsg.h>
#include <linux/uaccess.h>
#include <linux/qrtr.h>
#include <net/sock.h>
#include <linux/workqueue.h>
#include <linux/miscdevice.h>
#include <linux/debugfs.h>
#include <linux/seq_file.h>
#include <linux/version.h>
#include <linux/soc/qcom/pdr.h>
#include <linux/kobject.h>
#include <linux/hashtable.h>
#include <linux/iosys-map.h>
#include "../include/uapi/misc/fastrpc.h"

#if (KERNEL_VERSION(6, 3, 0) <= LINUX_VERSION_CODE)
#include <linux/cpu.h>
#include <linux/firmware/qcom/qcom_scm.h>
#else
#include <linux/qcom_scm.h>
#endif

#define ADSP_DOMAIN_ID (0)
#define MDSP_DOMAIN_ID (1)
#define SDSP_DOMAIN_ID (2)
#define CDSP_DOMAIN_ID (3)
#define CDSP1_DOMAIN_ID (4)
#define NUM_LEGACY_ID_MAX	5 /* adsp, mdsp, slpi, cdsp, cdsp1 */
#define FASTRPC_MAX_SESSIONS	50
#define FASTRPC_MAX_SESSIONS_PER_PROCESS	4

/* Check if given domain id is valid */
#define IS_LEGACY_DOMAIN_ID(domain) (domain < NUM_LEGACY_ID_MAX)

/* Check if given session id is valid */
#define IS_VALID_SESSION_ID(sess) (sess < FASTRPC_MAX_SESSIONS_PER_PROCESS)

/* Max number of SMMU context banks in a pool */
#define FASTRPC_MAX_CB_POOL	7
#define FASTRPC_MAX_SPD		4
#define FASTRPC_MAX_VMIDS	16
#define FASTRPC_ALIGN		128
#define FASTRPC_MAX_FDLIST	16
#define FASTRPC_MAX_CRCLIST	64
#define FASTRPC_KERNEL_PERF_LIST (PERF_KEY_MAX)
#define FASTRPC_DSP_PERF_LIST 12
#define FASTRPC_MAX_STATIC_HANDLE (20)
#define INIT_FILELEN_MAX (5 * 1024 * 1024)
#define INIT_FILE_NAMELEN_MAX (128)
#define FASTRPC_DEVICE_NAME	"fastrpc"
#define SESSION_ID_INDEX (30)
#define SESSION_ID_MASK (1 << SESSION_ID_INDEX)
#define MAX_FRPC_TGID 64
#define COPY_BUF_WARN_LIMIT (512*1024)
#define SMMU_4K 0x1000
#define SMMU_1M 0x100000ULL
#define SMMU_2M 0x200000ULL
#define SMMU_1G 0x40000000ULL

/* Max length of domain name */
#define MAX_DOMAIN_NAMELEN 30

/*
 * Align the size to next IOMMU page size
 * for example 1MB gets aligned to 2MB, as
 * IOMMU has only 3 page sizes 4K, 2M and 1G
 */
#define SMMU_ALIGN(size) ({		\
	u64 align_size = 0;		\
	if (size > SMMU_1G)		\
		align_size = SMMU_1G;	\
	else if (size > SMMU_2M)	\
		align_size = SMMU_2M;	\
	else				\
		align_size = SMMU_4K;	\
	ALIGN(size, align_size);	\
})

/*
 * Default SMMU CB device index
 * Used to log messages on this SMMU device
 */
#define DEFAULT_SMMU_IDX	0

/*
 * Fastrpc context ID bit-map:
 *
 * bits 0-3   : type of remote PD
 * bit  4     : type of job (sync/async)
 * bit  5     : reserved
 * bits 6-15  : IDR id
 * bits 16-63 : job id counter
 */
/* Starting position of idr in context id */
#define FASTRPC_CTXID_IDR_POS  (6)

/* Number of idr bits in context id */
#define FASTRPC_CTXID_IDR_BITS (10)

/* Max idr value */
#define FASTRPC_CTX_MAX (1 << FASTRPC_CTXID_IDR_BITS)

/* Bit-mask for idr */
#define FASTRPC_CTXID_IDR_MASK (FASTRPC_CTX_MAX - 1)

/* Macro to pack idr into context id  */
#define FASTRPC_PACK_IDR_IN_CTXID(ctxid, idr) (ctxid | ((idr & \
	FASTRPC_CTXID_IDR_MASK) << FASTRPC_CTXID_IDR_POS))

/* Macro to extract idr from context id */
#define FASTRPC_GET_IDR_FROM_CTXID(ctxid) ((ctxid >> FASTRPC_CTXID_IDR_POS) & \
	FASTRPC_CTXID_IDR_MASK)

/* Number of pd bits in context id (starting pos 0) */
#define FASTRPC_CTXID_PD_BITS (4)

/* Bit-mask for pd type */
#define FASTRPC_CTXID_PD_MASK ((1 << FASTRPC_CTXID_PD_BITS) - 1)

/* Macro to pack pd type into context id  */
#define FASTRPC_PACK_PD_IN_CTXID(ctxid, pd) (ctxid | (pd & \
		FASTRPC_CTXID_PD_MASK))

/* Starting position of job id counter in context id */
#define FASTRPC_CTXID_JOBID_POS (16)

/* Macro to pack job id counter into context id  */
#define FASTRPC_PACK_JOBID_IN_CTXID(ctxid, jobid) (ctxid | \
		(jobid << FASTRPC_CTXID_JOBID_POS))

/* Macro to extract ctxid (mask pd type) from response context */
#define FASTRPC_GET_CTXID_FROM_RSP_CTX(rsp_ctx) (rsp_ctx & \
		~FASTRPC_CTXID_PD_MASK)

/* Maximum buffers cached in cached buffer list */
#define FASTRPC_MAX_CACHED_BUFS (32)
#define FASTRPC_MAX_CACHE_BUF_SIZE (8*1024*1024)
/* Max no. of persistent headers pre-allocated per user process */
#define FASTRPC_MAX_PERSISTENT_HEADERS    (8)
/* Process status notifications from DSP will be sent with this unique context */
#define FASTRPC_NOTIF_CTX_RESERVED 0xABCDABCD
#define FASTRPC_UNIQUE_ID_CONST 1000

/* Add memory to static PD pool, protection thru XPU */
#define ADSP_MMAP_HEAP_ADDR  4
/* MAP static DMA buffer on DSP User PD */
#define ADSP_MMAP_DMA_BUFFER  6
/* Add memory to static PD pool protection thru hypervisor */
#define ADSP_MMAP_REMOTE_HEAP_ADDR  8
/* Add memory to userPD pool, for user heap */
#define ADSP_MMAP_ADD_PAGES 0x1000
/* Add memory to userPD pool, for LLC heap */
#define ADSP_MMAP_ADD_PAGES_LLC 0x3000
/* Map persistent header buffer on DSP */
#define ADSP_MMAP_PERSIST_HDR  0x4000
/* Size of dbglogbuf to log map/unmap calls on DSP*/
#define DBGLOGBUF_SIZE (1*1024*1024)


/* Fastrpc attribute for no mapping of fd  */
#define FASTRPC_ATTR_NOMAP (16)

/* This flag is used to skip CPU mapping  */
#define  FASTRPC_MAP_FD_NOMAP (16)

/* Map the DMA handle in the invoke call for backward compatibility */
#define FASTRPC_MAP_LEGACY_DMA_HANDLE  0x20000

#define DSP_UNSUPPORTED_API (0x80000414)
/* MAX NUMBER of DSP ATTRIBUTES SUPPORTED */
#define FASTRPC_MAX_DSP_ATTRIBUTES (256)
#define FASTRPC_MAX_DSP_ATTRIBUTES_LEN (sizeof(u32) * FASTRPC_MAX_DSP_ATTRIBUTES)

/* Retrives number of input buffers from the scalars parameter */
#define REMOTE_SCALARS_INBUFS(sc)	(((sc) >> 16) & 0x0ff)

/* Retrives number of output buffers from the scalars parameter */
#define REMOTE_SCALARS_OUTBUFS(sc)	(((sc) >> 8) & 0x0ff)

/* Retrives number of input handles from the scalars parameter */
#define REMOTE_SCALARS_INHANDLES(sc)	(((sc) >> 4) & 0x0f)

/* Retrives number of output handles from the scalars parameter */
#define REMOTE_SCALARS_OUTHANDLES(sc)	((sc) & 0x0f)

#define REMOTE_SCALARS_LENGTH(sc)	(REMOTE_SCALARS_INBUFS(sc) +   \
					 REMOTE_SCALARS_OUTBUFS(sc) +  \
					 REMOTE_SCALARS_INHANDLES(sc)+ \
					 REMOTE_SCALARS_OUTHANDLES(sc))
#define FASTRPC_BUILD_SCALARS(attr, method, in, out, oin, oout)  \
				(((attr & 0x07) << 29) |		\
				((method & 0x1f) << 24) |	\
				((in & 0xff) << 16) |		\
				((out & 0xff) <<  8) |		\
				((oin & 0x0f) <<  4) |		\
				(oout & 0x0f))

#define FASTRPC_SCALARS(method, in, out) \
		FASTRPC_BUILD_SCALARS(0, method, in, out, 0, 0)

#define FASTRPC_CREATE_PROCESS_NARGS	6
#define FASTRPC_CREATE_STATIC_PROCESS_NARGS	3

/* DSP status macros */
#define DSP_STATUS_UP true
#define DSP_STATUS_DOWN false

/*
 * Num of pages shared with process spawn call.
 *     Page 1 : init-mem buf
 *     Page 2 : proc attrs debug buf
 *     Page 3 : rootheap buf
 *     Page 4 : proc_init shared buf
 *     Page 5 : map debug log buf
 */
#define NUM_PAGES_WITH_SHARED_BUF 2
#define NUM_PAGES_WITH_ROOTHEAP_BUF 3
#define NUM_PAGES_WITH_PROC_INIT_SHAREDBUF 4
#define NUM_PAGES_WITH_MAP_DEBUG_BUF 5

#define miscdev_to_fdevice(d) container_of(d, struct fastrpc_device_node, miscdev)

/* Length of glink transaction history to store */
#define GLINK_MSG_HISTORY_LEN	(128)

#define FASTRPC_RSP_VERSION2 2
/* Early wake up poll completion number received from remoteproc */
#define FASTRPC_EARLY_WAKEUP_POLL (0xabbccdde)
/* Poll response number from remote processor for call completion */
#define FASTRPC_POLL_RESPONSE (0xdecaf)
/* timeout in us for polling until memory barrier */
#define FASTRPC_POLL_TIME_MEM_UPDATE (500)
/* timeout in us for busy polling after early response from remoteproc */
#define FASTRPC_POLL_TIME (4000)
/* timeout in us for polling completion signal after user early hint */
#define FASTRPC_USER_EARLY_HINT_TIMEOUT (500)
/* CPU feature information to DSP */
#define FASTRPC_CPUINFO_DEFAULT (0)
#define FASTRPC_CPUINFO_EARLY_WAKEUP (1)

/* Default root heap buffer size and count */
#define FASTRPC_DEFAULT_ROOTHEAP_BUF_SIZE (0x140000)
#define FASTRPC_DEFAULT_ROOTHEAP_BUF_COUNT (3)

/* Position of priority in frpc tid for glink msg packet */
#define PRIORITY_POS_IN_FRPC_TID 26

/* Bit-mask to retain only priority bits of tid  */
#define FRPC_TID_PRIO_MASK (~((1UL << PRIORITY_POS_IN_FRPC_TID) - 1))

/**
 * Macro to generate frpc thread id based on priority and hlos thread id
 *
 * TID breakdown:
 *		bits 0-25  : actual HLOS thread id
 *		bits 26-31 : priority of rpc call
 */
#define GENERATE_FRPC_TID_WITH_PRIORITY(tid, priority) \
			(tid | (priority << PRIORITY_POS_IN_FRPC_TID))

/**
 * Macro to validate if bits in priority positions of original HLOS tid
 * aren't already non-zero.
 * Returns:
 *      false if the any of the bits are set
 *      true otherwise
 */
#define VALIDATE_PRIORITY_BITS_IN_TID(tid) \
		(((tid & FRPC_TID_PRIO_MASK) == 0) ? true : false)

/* Maximum PM timeout that can be voted through fastrpc */
#define FASTRPC_MAX_PM_TIMEOUT_MS 50
#define FASTRPC_NON_SECURE_WAKE_SOURCE_CLIENT_NAME	"fastrpc-non_secure"
#define FASTRPC_SECURE_WAKE_SOURCE_CLIENT_NAME		"fastrpc-secure"

#ifndef topology_cluster_id
#define topology_cluster_id(cpu) topology_physical_package_id(cpu)
#endif


#define FASTRPC_DSPSIGNAL_TIMEOUT_NONE 0xffffffff
#define FASTRPC_DSPSIGNAL_NUM_SIGNALS 1024
#define FASTRPC_DSPSIGNAL_GROUP_SIZE 256
/* Macro to return PDR status */
#define IS_PDR(fl) (fl->spd && fl->spd->pdrcount != fl->spd->prevpdrcount)
/* Macro to return SSR status */
#define IS_SSR(fl) (fl && fl->cctx && atomic_read(&fl->cctx->teardown))

#define AUDIO_PDR_SERVICE_LOCATION_CLIENT_NAME   "audio_pdr_adsp"
#define AUDIO_PDR_ADSP_SERVICE_NAME              "avs/audio"
#define ADSP_AUDIOPD_NAME                        "msm/adsp/audio_pd"

#define SENSORS_PDR_ADSP_SERVICE_LOCATION_CLIENT_NAME   "sensors_pdr_adsp"
#define SENSORS_PDR_ADSP_SERVICE_NAME              "tms/servreg"
#define ADSP_SENSORPD_NAME                       "msm/adsp/sensor_pd"

#define SENSORS_PDR_SLPI_SERVICE_LOCATION_CLIENT_NAME "sensors_pdr_slpi"
#define SENSORS_PDR_SLPI_SERVICE_NAME            SENSORS_PDR_ADSP_SERVICE_NAME
#define SLPI_SENSORPD_NAME                       "msm/slpi/sensor_pd"

#define OIS_PDR_ADSP_SERVICE_LOCATION_CLIENT_NAME   "ois_pdr_adsprpc"
#define OIS_PDR_ADSP_SERVICE_NAME              "tms/servreg"
#define ADSP_OISPD_NAME                        "msm/adsp/ois_pd"

#define DBG_FS_SIZE (200*1024)
#define NUM_DUMPED (128)

#define PERF_END ((void)0)

#define PERF(enb, cnt, ff) \
	{\
		struct timespec64 startT = {0};\
		uint64_t *counter = cnt;\
		if (enb && counter) {\
			ktime_get_real_ts64(&startT);\
		} \
		ff ;\
		if (enb && counter) {\
			*counter += getnstimediff(&startT);\
		} \
	}

#define GET_COUNTER(perf_ptr, offset)  \
	(perf_ptr != NULL ?\
		(((offset >= 0) && (offset < PERF_KEY_MAX)) ?\
			(uint64_t *)(perf_ptr + offset)\
				: (uint64_t *)NULL) : (uint64_t *)NULL)

/* Registered QRTR service ID */
#define FASTRPC_REMOTE_SERVER_SERVICE_ID 5012
/*
 * Fastrpc remote server instance ID bit-map:
 *
 * bits 0-1   : channel ID
 * bits 2-7   : reserved
 * bits 8-9   : remote domains (SECURE_PD, GUEST_OS)
 * bits 10-31 : reserved
 */
#define REMOTE_DOMAIN_INSTANCE_INDEX (8)
#define GET_SERVER_INSTANCE(remote_domain, cid) \
	((remote_domain << REMOTE_DOMAIN_INSTANCE_INDEX) | cid)
#define GET_CID_FROM_SERVER_INSTANCE(remote_server_instance) \
	(remote_server_instance & 0x3)
/* Maximun received fastprc packet size */
#define FASTRPC_SOCKET_RECV_SIZE sizeof(union rsp)
#define FIND_DIGITS(number) ({ \
		unsigned int count = 0, i= number; \
		while(i != 0) { \
			i /= 10; \
			count++; \
		} \
	count; \
	})
#define COUNT_OF(number) (number == 0 ? 1 : FIND_DIGITS(number))

/*
 * By default, the sid will be prepended adjacent to smmu pa before sending
 * to DSP. But if the chipset's dtsi specifies the new addressing format to
 * handle pa's of longer widths, then the sid will be prepended at the
 * position specified in this macro.
 */
#define SID_POS_IN_IOVA 56

/* Default width of pa bus from dsp */
#define DSP_DEFAULT_BUS_WIDTH 32

/* Extract smmu pa from consolidated iova */
#define IOVA_TO_PHYSADDR(iova, sid_pos) (iova & ((1ULL << sid_pos) - 1ULL))

/*
 * Prepare the consolidated iova to send to dsp by prepending the sid
 * to smmu pa at the appropriate position
 */
#define RECONSTRUCT_IOVA_FROM_SID_PA(sid, phys, sid_pos) \
	(phys += sid << sid_pos)

/* Check if the given flag is used for extended UDMA mapping */
#define IS_EXTENDED_MAP_FLAG(flag) \
	(flag == FASTRPC_MAP_FD_EXTENDED || \
	flag == FASTRPC_MAP_FD_DELAYED_EXTENDED)

/*
 * Generates a physical ID for a DSP (Digital Signal Processor) device.
 *
 * The resulting physical ID is a composite value consisting of:
 *   Type identifier multiplied by 1000, plus the instance identifier
 *
 * @param type        : Type identifier for the DSP device
 * @param instance_id : Instance identifier for the DSP device
 *
 * @return The generated physical ID for the DSP device
 */
#define GENERATE_DSP_PHYSICAL_ID(type, instance_id) \
	((type * 1000) + instance_id)

/*
 * Generates a unique logical domain ID by combining a type and counter.
 *
 * @param type    : The type of domain (e.g. NSP, LPASS)
 * @param counter : Global count of DSPs discovered for specific type
 * @return A unique logical domain ID for DSP
 */
#define GENERATE_LOGICAL_DOMAIN_ID(type, counter) \
	((type * 1000) + counter)

/*
 * Process types on remote subsystem
 * Always add new PD types at the end, before MAX_PD_TYPE
 */
enum fastrpc_cb_pd_types {
	DEFAULT_UNUSED            = 0,  /* PD type not configured for context banks */
	ROOT_PD                   = 1,  /* Root PD */
	AUDIO_STATICPD            = 2,  /* ADSP Audio Static PD */
	SENSORS_STATICPD          = 3,  /* ADSP Sensors Static PD */
	SECURE_STATICPD           = 4,  /* CDSP Secure Static PD */
	OIS_STATICPD              = 5,  /* ADSP OIS Static PD */
	CPZ_USERPD                = 6,  /* CDSP CPZ USER PD */
	USERPD                    = 7,  /* DSP User Dynamic PD */
	GUEST_OS_SHARED           = 8,  /* Legacy Guest OS Shared */
	USER_UNSIGNEDPD_POOL      = 9,  /* DSP User Dynamic Unsigned PD pool */
	EXT_MAP_PD_TYPE           = 10, /* DSP extended mapping */
	MAX_PD_TYPE,                    /* Max PD type */
};

/* List of const remote handles used by framework only */
enum fastrpc_internal_remote_handles {
	FASTRPC_INIT_HANDLE = 1,
	FASTRPC_DSP_UTILITIES_HANDLE = 2,
};

/* List of method ids associated with process group const handle*/
enum fastrpc_process_method_ids {
	FASTRPC_RMID_INIT_ATTACH        = 0,
	FASTRPC_RMID_INIT_RELEASE       = 1,
	FASTRPC_RMID_INIT_MMAP          = 4,
	FASTRPC_RMID_INIT_MUNMAP        = 5,
	FASTRPC_RMID_INIT_CREATE        = 6,
	FASTRPC_RMID_INIT_CREATE_ATTR   = 7,
	FASTRPC_RMID_INIT_CREATE_STATIC = 8,
	FASTRPC_RMID_INIT_MEM_MAP       = 10,
	FASTRPC_RMID_INIT_MEM_UNMAP     = 11,
	FASTRPC_RMID_INIT_MDCTX_MANAGE  = 12,
	FASTRPC_RMID_INIT_PROCESS_DUMP  = 13,
	FASTRPC_RMID_INIT_MAX,
};

/* Attributes for internal purposes. Clients cannot query these */
enum fastrpc_internal_attributes {
	/* DMA handle reverse RPC support */
	DMA_HANDLE_REVERSE_RPC_CAP = 129,
	ROOTPD_RPC_HEAP_SUPPORT = 132,
	DBGLOGBUF_SUPPORT = 134,
};

enum fastrpc_remote_domains_id {
	SECURE_PD = 0,
	GUEST_OS = 1,
	MAX_REMOTE_ID = SECURE_PD + 1,
};

 /* Types of fastrpc DMA bufs sent to DSP */
 enum fastrpc_buf_type {
	METADATA_BUF,
	COPYDATA_BUF,
	INITMEM_BUF,
	USER_BUF,
	REMOTEHEAP_BUF,
	ROOTHEAP_BUF,
	/* Buffer to log DSP map/unmap debug info*/
	MAP_DEBUG_BUF,
};

/* Types of RPC calls to DSP */
enum fastrpc_msg_type {
	USER_MSG = 0,
	KERNEL_MSG_WITH_ZERO_PID,
	KERNEL_MSG_WITH_NONZERO_PID,
};

enum fastrpc_response_flags {
	NORMAL_RESPONSE = 0,
	EARLY_RESPONSE = 1,
	USER_EARLY_SIGNAL = 2,
	COMPLETE_SIGNAL = 3,
	STATUS_RESPONSE = 4,
	POLL_MODE = 5,
};

/* To maintain the dsp map current state */
enum fastrpc_map_state {
	/* Default smmu/global mapping */
	FD_MAP_DEFAULT = 0,
	/* Initiated DSP mapping */
	FD_DSP_MAP_IN_PROGRESS,
	/* Completed DSP mapping */
	FD_DSP_MAP_COMPLETE,
	/* Initiated DSP unmapping */
	FD_DSP_UNMAP_IN_PROGRESS,
};

enum fastrpc_dump_type {
	CMA = 0,
	DEBUGFS = 1,
	INIT_MEM = 2,
};

struct fastrpc_dump_info{
	/* Type of memory dumped */
	enum fastrpc_dump_type type;
	/* Offset at which is a particular memory dumped*/
	u64 offset;
	/* Length of memory dumped */
	u64 size;
	/*ipa of memory dumped */
	u64 phys;
};

/* Legacy domain names of DSP */
static const char *legacy_domains[NUM_LEGACY_ID_MAX] =
{
	"adsp",
	"mdsp",
	"sdsp",
	"cdsp",
	"cdsp1"
};

/* DSP labels defined in device tree */
static const char *fastrpc_dsp_labels[FASTRPC_MAX_DSP_TYPE] =
{
	NULL,
	"nsp",
	"lpass",
	"sdsp"
};

struct fastrpc_tvm_dma_heap {
	const char *name;          // Name of the dma heap
	void *mem_pool;            // dma heap memory pool
	struct dma_heap *dmaheap;  // dma heap
	bool in_use;               // Flag to indicate if heap is being used
};

struct fastrpc_socket {
	struct socket *sock;                   // Socket used to communicate with remote domain
	struct sockaddr_qrtr local_sock_addr;  // Local socket address on kernel side
	struct sockaddr_qrtr remote_sock_addr; // Remote socket address on remote domain side
	struct mutex socket_mutex;             // Mutex for socket synchronization
	void *recv_buf;                        // Received packet buffer
};

struct frpc_transport_session_control {
	struct fastrpc_socket frpc_socket;     // Fastrpc socket data structure
	u32 remote_server_instance;       // Unique remote server instance ID
	bool remote_server_online;             // Flag to indicate remote server status
	struct work_struct work;               // work for handling incoming messages
	struct workqueue_struct *wq;           // workqueue to post @work on
};

struct fastrpc_phy_page {
	u64 addr;		/* physical address */
	u64 size;		/* size of contiguous region */
};

struct fastrpc_invoke_buf {
	u32 num;		/* number of contiguous regions */
	u32 pgidx;		/* index to start of contiguous region */
};

struct fastrpc_remote_dmahandle {
	s32 fd;		/* dma handle fd */
	u32 offset;	/* dma handle offset */
	u32 len;	/* dma handle length */
};

struct fastrpc_remote_buf {
	u64 pv;		/* buffer pointer */
	u64 len;	/* length of buffer */
};

union fastrpc_remote_arg {
	struct fastrpc_remote_buf buf;
	struct fastrpc_remote_dmahandle dma;
};

struct fastrpc_mmap_rsp_msg {
	u64 vaddr;
};

struct fastrpc_mmap_req_msg {
	s32 pgid;
	u32 flags;
	u64 vaddr;
	s32 num;
};

struct fastrpc_mem_map_req_msg {
	s32 pgid;
	s32 fd;
	s32 offset;
	u32 flags;
	u64 vaddrin;
	s32 num;
	s32 data_len;
};

struct fastrpc_munmap_req_msg {
	s32 pgid;
	u64 vaddr;
	u64 size;
};

struct fastrpc_mem_unmap_req_msg {
	s32 pgid;
	s32 fd;
	u64 vaddrin;
	u64 len;
};

struct gid_list {
	u32 *gids;
	u32 gidcount;
};

struct fastrpc_msg {
	int pid;		/* process group id */
	int tid;		/* thread id */
	u64 ctx;		/* invoke caller context */
	u32 handle;	/* handle to invoke */
	u32 sc;		/* scalars structure describing the data */
	u64 addr;		/* physical address */
	u64 size;		/* size of contiguous region */
};

struct fastrpc_invoke_rsp {
	u64 ctx;		/* invoke caller context */
	int retval;		/* invoke return value */
};

struct fastrpc_invoke_rspv2 {
	u64 ctx;		/* invoke caller context */
	int retval;		/* invoke return value */
	u32 flags;		/* early response flags */
	u32 early_wake_time;	/* user hint in us */
	u32 version;		/* version number */
};

struct fastrpc_tx_msg {
	struct fastrpc_msg msg;	/* Msg sent to remote subsystem */
	int rpmsg_send_err;	/* rpmsg error */
	s64 ns;			/* Timestamp (in ns) of msg */
};

struct fastrpc_rx_msg {
	struct fastrpc_invoke_rspv2 rsp; /* Response from remote subsystem */
	s64 ns;		/* Timestamp (in ns) of response */
};

struct fastrpc_rpmsg_log {
	u32 tx_index;	/* Current index of 'tx_msgs' array */
	u32 rx_index;	/* Current index of 'rx_msgs' array */
	/* Rolling history of messages sent to remote subsystem */
	struct fastrpc_tx_msg tx_msgs[GLINK_MSG_HISTORY_LEN];
	/* Rolling history of responses from remote subsystem */
	struct fastrpc_rx_msg rx_msgs[GLINK_MSG_HISTORY_LEN];
	spinlock_t tx_lock;
	spinlock_t rx_lock;
};

struct dsp_notif_rsp {
	u64 ctx;		  /* response context */
	u32 type;        /* Notification type */
	int pid;		      /* user process pid */
	u32 status;	  /* userpd status notification */
};

union rsp {
	struct fastrpc_invoke_rsp rsp;
	struct fastrpc_invoke_rspv2 rsp2;
	struct dsp_notif_rsp rsp3;
};

struct fastrpc_buf_overlap {
	u64 start;
	u64 end;
	int raix;
	u64 mstart;
	u64 mend;
	u64 offset;
};

struct fastrpc_buf {
	/* Node for adding to buffer lists */
	struct list_head node;
	struct fastrpc_user *fl;
	struct dma_buf *dmabuf;
	struct device *dev;
	/* Context bank with which DMA buffer was allocated */
	struct fastrpc_smmu *smmucb;
	void *virt;
	u32 type;
	u64 phys;
	u64 size;
	/* Lock for dma buf attachments */
	struct mutex lock;
	struct list_head attachments;
	uintptr_t raddr;
	bool in_use;
	u32 domain_id;
	/* time counter to trace buffer allocation latency */
	struct timespec64 alloc_time;
	/* time counter to trace scm assign latency */
	struct timespec64 scm_assign_time;
	/* sg table for TVM's dma heap mapping */
	struct sg_table *table;
	/* dma buffer attach for TVM's dma heap mapping */
	struct dma_buf_attachment *attach;
	/* dma buffer virtual map on kernel */
	struct iosys_map virt_map;
};

struct fastrpc_dma_buf_attachment {
	struct device *dev;
	struct sg_table sgt;
	struct list_head node;
};

struct fastrpc_map {
	struct list_head node;
	struct fastrpc_user *fl;
	int fd;
	struct dma_buf *buf;
	struct sg_table *table;
	struct dma_buf_attachment *attach;
	/* Context bank with which buffer was mapped on SMMU */
	struct fastrpc_smmu *smmucb;
	u64 phys;
	u64 size;
	void *va;
	u64 len;
	u64 raddr;
	u32 attr;
	u32 flags;
	struct kref refcount;
	int secure;
	atomic_t state;
};

struct fastrpc_perf {
	u64 count;
	u64 flush;
	u64 map;
	u64 copy;
	u64 link;
	u64 getargs;
	u64 putargs;
	u64 invargs;
	u64 invoke;
	u64 tid;
};

struct fastrpc_smmu {
	struct device *dev;
	int sid;
	bool valid;
	struct mutex map_mutex;
	/* gen pool for QRTR */
	struct gen_pool *frpc_genpool;
	/* fastrpc gen pool buffer */
	struct fastrpc_buf *frpc_genpool_buf;
	/* fastrpc gen pool buffer fixed IOVA */
	unsigned long genpool_iova;
	/* fastrpc gen pool buffer size */
	size_t genpool_size;
	/* Total bytes allocated using this CB */
	u64 allocatedbytes;
	/* Total size of the context bank */
	u64 totalbytes;
	/* Min alloc size for which CB can be used */
	u64 minallocsize;
	/* Max alloc size for which CB can be used */
	u64 maxallocsize;
	/* To indentify the parent session this SMMU CB belomngs to */
	struct fastrpc_pool_ctx *sess;
	/* Number of PA bits in IOVA */
	u32 pa_bits;
	/* Position of SID in IOVA */
	u32 sid_pos;
};

struct fastrpc_pool_ctx {
	/* Context bank pool */
	struct fastrpc_smmu smmucb[FASTRPC_MAX_CB_POOL];
	u32 pd_type;
	bool secure;
	bool sharedcb;
	/* Number of context banks in the pool */
	u32 smmucount;
	/* Number of applications using the pool */
	int usecount;
};

struct fastrpc_static_pd {
	char *servloc_name;
	char *spdname;
	void *pdrhandle;
	u64 pdrcount;
	u64 prevpdrcount;
	atomic_t ispdup;
	atomic_t is_attached;
	struct fastrpc_channel_ctx *cctx;
};

struct heap_bufs {
	/* List of bufs */
	struct list_head list;
	/* Number of bufs */
	unsigned int num;
};

struct fastrpc_domain;

struct fastrpc_channel_ctx {
	int domain_id;
	int sesscount;
	int vmcount;
	u64 perms;
	/* Structure holding info on domain associated with channel */
	struct fastrpc_domain *domain;
	struct qcom_scm_vmperm vmperms[FASTRPC_MAX_VMIDS];
#if !IS_ENABLED(CONFIG_QCOM_FASTRPC_TRUSTED)
	struct rpmsg_device *rpdev;
#else
	struct frpc_transport_session_control session_control;
#endif
	struct device *dev;
	struct fastrpc_pool_ctx session[FASTRPC_MAX_SESSIONS];
	struct fastrpc_static_pd spd[FASTRPC_MAX_SPD];
	spinlock_t lock;
	struct idr ctx_idr;
	struct ida tgid_frpc_ida;
	struct list_head users;
	struct kref refcount;
	/* Flag if dsp attributes are cached */
	bool valid_attributes;
	bool cpuinfo_status;
	bool staticpd_status;
	u32 dsp_attributes[FASTRPC_MAX_DSP_ATTRIBUTES];
	u32 lowest_capacity_core_count;
	u32 qos_latency;
	/* Device node of channel using dynamic name */
	struct fastrpc_device_node *fdevice;
	/* Non secure device node using legacy device name */
	struct fastrpc_device_node *legacy_fdevice;
	/* Secure device node using legacy device name */
	struct fastrpc_device_node *legacy_secure_fdevice;
	struct gid_list gidlist;
	struct list_head gmaps;
	struct fastrpc_rpmsg_log gmsg_log;
	/* Secure subsystems like ADSP/SLPI will use secure client */
	struct wakeup_source *wake_source_secure;
	/* Non-secure subsystem like CDSP will use regular client */
	struct wakeup_source *wake_source;
	struct mutex wake_mutex;
	/* Set when ssr is force-triggered on a kernel rpc call timeout */
	bool startshutdown;
	bool secure;
	bool unsigned_support;
	u64 dma_mask;
	u64 cpuinfo_todsp;
	int max_sess_per_proc;
	bool pd_type;
	/* Set teardown flag when remoteproc is shutting down */
	atomic_t teardown;
	/* Buffers donated to grow rootheap on DSP */
	struct heap_bufs rootheap_bufs;
	/* jobid counter to prepend into ctxid */
	u64 jobid;
	/* Flag to indicate CB pooling is enabled for channel */
	bool smmucb_pool;
	/* Number of active ongoing invocations (device ioctl / release) */
	u32 invoke_cnt;
	/* Completion object for threads to wait for SSR handling to finish */
	struct completion ssr_complete;
	/* Wait queue to block/resume SSR until all invocations are complete */
	wait_queue_head_t ssr_wait_queue;
	/* Format to control where sid is prepended to iova */
	u32 iova_format;
	/* Default user object for making kernel-to-rootpd rpc calls */
	struct fastrpc_user *default_user;
	/* Root heap buffer size */
	unsigned int rootheap_buf_size;
	/* Root heap buffer count */
	unsigned int rootheap_buf_count;
	/* Completion object for the threads to wait for device to crash */
	struct completion rpmsg_remove_start;
};

struct fastrpc_ssr_handler {
	/* Worker thread to trigger SSR based on timeout */
	struct work_struct ssr_work;
	/* Remote-proc handle to trigger ssr */
	void *rphandle;
	int domain_id;
};

struct fastrpc_domain {
	/* Node for adding to global domains hash-table */
	struct hlist_node node;
	/* Logical domain ID returned to users */
	u32 id;
	/* Name of the dsp domain */
	char name[MAX_DOMAIN_NAMELEN];
	/* Flag to indicate domain up or down */
	bool status;
	/* Flag to indicate if configured as legacy node */
	bool legacy;
	/* Instance ID configured in dtsi */
	u32 instance_id;
	/* Unique physical ID - the key for the kernel hash-table */
	u32 phy_id;
	/* Type of DSP */
	enum fastrpc_dsp_type type;
	/*
	 * Legacy name - This will be assigned to the dsp with the instance id '0'
	 * for types LPASS, SDSP
	 * for NSP, instance id '0' would be assigned legacy name 'cdsp'
	 *          instance id '1' would be assigned legacy name 'cdsp1'
	 * This will be used to handle all the rpc calls made by clients
	 * using old legacy domain names
	 */
	char *legacy_name;
	/*
	 * Legacy id - This will be assigned to the dsp with the instance id '0'
	 * for types LPASS, SDSP
	 * for NSP, instance id '0' would be assigned CDSP_DOMAIN_ID
	 *          instance id '1' would be assigned CDSP1_DOMAIN_ID
	 * This will be used to handle all the rpc calls made by clients
	 * using old legacy domain ids
	 */
	u32 legacy_id;
	/* Sysfs object for domain */
	struct kobject kobj_sysfs;
	/* Channel context for domain */
	struct fastrpc_channel_ctx *cctx;
	/* structure for handling SSR, when fastrpc framework hangs */
	struct fastrpc_ssr_handler ssr_handler;
};

struct fastrpc_invoke_ctx {
	/* Node for adding to context list */
	struct list_head node;
	int nscalars;
	int nbufs;
	int retval;
	int pid;
	int tgid;
	u32 sc;
	u32 handle;
	u32 *crc;
	/* user hint of completion time in us */
	u32 early_wake_time;
	u64 *perf_kernel;
	u64 *perf_dsp;
	u64 ctxid;
	u64 msg_sz;
	/* work done status flag */
	bool is_work_done;
	/* response flags from remote processor */
	enum fastrpc_response_flags rsp_flags;
	struct kref refcount;
	struct completion work;
	// struct work_struct put_work;
	struct fastrpc_msg msg;
	struct fastrpc_user *fl;
	union fastrpc_remote_arg *rpra;
	union fastrpc_remote_arg *outbufs;
	struct fastrpc_map **maps;
	struct fastrpc_buf *buf;
	struct fastrpc_invoke_args *args;
	struct fastrpc_buf_overlap *olaps;
	struct fastrpc_channel_ctx *cctx;
	struct fastrpc_perf *perf;
	/* Timer to trigger ssr callback on a kernel rpc call timeout */
	struct timer_list ssr_timer;
};

struct fastrpc_device_node {
	struct fastrpc_channel_ctx *cctx;
	struct miscdevice miscdev;
	bool secure;
};

struct fastrpc_mdctx_info {
	/* Node to add to process multidomain context list */
	struct list_head node;
	/* List of domains on which context was created */
	uint32_t *domains;
	/* List of physical ids of the domains */
	uint32_t *phy_ids;
	/* List of HW instance ids of the domains */
	uint32_t *instance_ids;
	/* List of session ids on each domain */
	uint32_t *session_ids;
	/* List of fastrpc-assigned tgids of each session */
	int32_t *tgids_frpc;
	/* Number of domains */
	uint32_t num_domains;
	/* User-obj using which context was created */
	struct fastrpc_user *fl;
	/* Kernel generated context id */
	uint64_t ctx;
};

struct fastrpc_internal_config {
	int user_fd;
	int user_size;
	u64 root_addr;
	u32 root_size;
};

/* FastRPC ioctl structure to set session related info */
struct fastrpc_internal_sessinfo {
	uint32_t domain_id;  /* Set the remote subsystem, Domain ID of the session  */
	uint32_t session_id; /* Unused, Set the Session ID on remote subsystem */
	uint32_t pd;    /* Set the process type on remote subsystem */
	uint32_t sharedcb;   /* Unused, Session can share context bank with other sessions */
};

struct fastrpc_notif_queue {
	/* Number of pending status notifications in queue */
	atomic_t notif_queue_count;
	/* Wait queue to synchronize notifier thread and response */
	wait_queue_head_t notif_wait_queue;
	/* IRQ safe spin lock for protecting notif queue */
	spinlock_t nqlock;
};

struct fastrpc_internal_notif_rsp {
	u32 domain;					/* Domain of User PD */
	u32 session;				/* Session ID of User PD */
	u32 status;			/* Status of the process */
};

struct fastrpc_notif_rsp {
	struct list_head notifn;
	u32 domain;
	u32 session;
	enum fastrpc_status_flags status;
};

enum fastrpc_process_state {
	/* Default state */
	DEFAULT_PROC_STATE = 0,
	/* Process create on DSP initiated */
	DSP_CREATE_START,
	/* Process create on DSP complete */
	DSP_CREATE_COMPLETE,
	/* Process exit on DSP initiated */
	DSP_EXIT_START,
	/* Process exit on DSP complete */
	DSP_EXIT_COMPLETE,
};

struct fastrpc_user {
	struct list_head user;
	struct list_head maps;
	struct list_head pending;
	struct list_head interrupted;
	struct list_head mmaps;
	struct list_head cached_bufs;
	/* list of client drivers registered to fastrpc driver*/
	struct list_head fastrpc_drivers;

	/* List of multidomain contexts created using this user */
	struct list_head mdctxs;

	struct fastrpc_channel_ctx *cctx;

	/* Context bank(s) used for all regular buffer mappings */
	struct fastrpc_pool_ctx *sctx;

	/*
	 * Context bank(s) used for secure buffer mappings after remote process
	 * migrates to cpz
	 */
	struct fastrpc_pool_ctx *secsctx;

	/* Context bank(s) used for extended addr space mappings */
	struct fastrpc_pool_ctx *extctx;

	struct fastrpc_buf *init_mem;
	/* Pre-allocated header buffer */
	struct fastrpc_buf *pers_hdr_buf;
	/* proc_init shared buffer */
	struct fastrpc_buf *proc_init_sharedbuf;
	struct fastrpc_static_pd *spd;
	/* Pre-allocated buffer divided into N chunks */
	struct fastrpc_buf *hdr_bufs;
	/* dbglogbuf to log DSP map/unmap debug info */
	struct fastrpc_buf *dbglogbuf;
	/*
	 * Unique device struct for each process, shared with
	 * client drivers when attached to fastrpc driver.
	 */
	struct fastrpc_device *device;
#ifdef CONFIG_DEBUG_FS
	atomic_t debugfs_file_create;
	struct dentry *debugfs_file;
	char *debugfs_buf;
#endif
	/*
	 * Hlos pid of process stored during device open. For untrusted apps,
	 * this will be the pid of DSP HAL service. For trusted apps, this
	 * will be the pid of the process itself.
	 */
	int tgid;
	/* Unique fastrpc pid sent to dsp */
	int tgid_frpc;
	/* Actual hlos pid of process offloading to dsp */
	int tgid_app;
	/* Process name of process offloading to dsp */
	char name[TASK_COMM_LEN];
	/* PD type of remote subsystem process */
	u32 pd_type;
	/* total cached buffers */
	u32 num_cached_buf;
	/* total persistent headers */
	u32 num_pers_hdrs;
	u32 profile;
	int sessionid;
	/* Threads poll for specified timeout and fall back to glink wait */
	u32 poll_timeout;
	u32 ws_timeout;
	u32 qos_request;
	/* Flag to enable PM wake/relax voting for invoke */
	u32 wake_enable;
	bool is_secure_dev;
	/* If set, threads will poll for DSP response instead of glink wait */
	bool poll_mode;
	bool is_unsigned_pd;
	/* Variable to identify if client driver dma operation are pending*/
	bool is_dma_invoke_pend;
	bool sharedcb;
	char *servloc_name;;
	/* Lock for lists */
	spinlock_t lock;
	/* lock for dsp signals */
	spinlock_t dspsignals_lock;
	/*mutex for process maps synchronization*/
	struct mutex map_mutex;
	struct mutex signal_create_mutex;
	/* mutex for qos request synchronization */
	struct mutex pm_qos_mutex;
	/* Compleation object for dma invocations by client driver*/
	struct completion dma_invoke;
	/* Completion objects and state for dspsignals */
	struct fastrpc_dspsignal *signal_groups[FASTRPC_DSPSIGNAL_NUM_SIGNALS /FASTRPC_DSPSIGNAL_GROUP_SIZE];
	struct dev_pm_qos_request *dev_pm_qos_req;
	/* Process status notification queue */
	struct fastrpc_notif_queue proc_state_notif;
	struct list_head notif_queue;
	struct fastrpc_internal_config config;
	bool multi_session_support;
	bool untrusted_process;
	bool set_session_info;
	/* Various states throughout process life cycle */
	atomic_t state;
	/* Timeout in ms */
	uint32_t timeout;
	/* Flag to check if dsp timeout recovery is enabled */
	bool dsp_recovery;
	/* dma heap pool for TVM's dma memory allocations */
	struct fastrpc_tvm_dma_heap *tvm_dma_heap;

	/* Node for adding this user-object to active-users list during ssr */
	struct list_head active_user_ssr;
	struct kref refcount;
	struct work_struct put_work;
};

struct fastrpc_ctrl_latency {
	u32 enable;	/* latency control enable */
	u32 latency;	/* latency request in us */
};

struct fastrpc_ctrl_smmu {
	u32 sharedcb;	/* Set to SMMU share context bank */
};

struct fastrpc_ctrl_wakelock {
	u32 enable;	/* wakelock control enable */
};

struct fastrpc_ctrl_pm {
	u32 timeout;	/* timeout(in ms) for PM to keep system awake */
};

struct fastrpc_internal_control {
	u32 req;
	union {
		struct fastrpc_ctrl_latency lp;
		struct fastrpc_ctrl_smmu smmu;
		struct fastrpc_ctrl_wakelock wp;
		struct fastrpc_ctrl_pm pm;
	};
};

enum fastrpc_dspsignal_state {
	DSPSIGNAL_STATE_UNUSED = 0,
	DSPSIGNAL_STATE_PENDING,
	DSPSIGNAL_STATE_SIGNALED,
	DSPSIGNAL_STATE_CANCELED,
};

struct fastrpc_internal_dspsignal {
	u32 req;
	u32 signal_id;
	union {
		u32 flags;
		u32 timeout_usec;
	};
};

struct fastrpc_dspsignal {
	struct completion comp;
	int state;
};

int fastrpc_transport_send(struct fastrpc_channel_ctx *cctx, void *rpc_msg, uint32_t rpc_msg_size);
int fastrpc_transport_init(void);
void fastrpc_transport_deinit(void);
int fastrpc_handle_rpc_response(struct fastrpc_channel_ctx *cctx, void *data,
				int len, bool is_glink_wakeup);
void ssr_timer_callback(struct timer_list *timer);

/*
 * Registers a device with the FastRPC framework.
 *
 * @param dev        The device to register.
 * @param cctx       FastRPC channel context.
 * @param is_secured flag for secure device node
 * @param legacy     flag for legacy device node.
 * @param domain     Name of the domain the device is associated
 *
 * @return 0 on success, negative error code on failure.
 */
int fastrpc_device_register(struct device *dev, struct fastrpc_channel_ctx *cctx,
				bool is_secured, bool legacy, const char *domain);

struct fastrpc_channel_ctx* get_current_channel_ctx(struct device *dev);
void fastrpc_notify_users(struct fastrpc_user *user);

/* Create default user object for remote channel */
int fastrpc_channel_default_user_create(struct fastrpc_channel_ctx *cctx);

/* Remove default user object for remote channel */
int fastrpc_channel_default_user_delete(struct fastrpc_channel_ctx *cctx);

/* Function to clean all SMMU mappings associated with a fastrpc user obj */
void fastrpc_free_user(struct fastrpc_user *fl);

/*
 * Creates a sysfs interface for the given fastrpc channel context.
 *
 * @param domain pointer to the domain info struct.
 *
 * @return 0 on success, a negative error code on failure.
 */
int fastrpc_sysfs_domain_create(struct fastrpc_domain *domain);

/*
 * Removes sysfs directory of a channel.
 *
 * This function is responsible for deleting the sysfs directory
 * associated with a specific channel context.
 * It takes a pointer to the channel context as an argument.
 *
 * @param domain Pointer to the domain info struct
 */
void fastrpc_sysfs_domain_remove(struct fastrpc_domain *domain);

/*
 * Populate fastrpc_domain from device tree node.
 *
 * @param rdev   Device structure to extract info from.
 * @param domain Pointer to fastrpc_domain pointer to be populated.
 *
 * @return 0 on success, negative error code on failure.
 */
int fastrpc_populate_domain_from_dt(struct device *rdev,
	struct fastrpc_domain **domain);

/*
 * fastrpc_sysfs_register_kset - Register the fastrpc kset
 *
 * Creates a kset to create a parent directory "fastrpc" under /sys/kernel.
 *
 * Return: 0 on success, -ENOMEM on failure
 */
int fastrpc_sysfs_register_kset(void);

/*
 * fastrpc_sysfs_deregister_kset - Deregister the fastrpc kset from sysfs
 *
 * This function deregisters the fastrpc kset from the sysfs file system.
 *
 * @return: None
 */
void fastrpc_sysfs_deregister_kset(void);

/*
 * Reserve one of the TVM dma heap.
 * Reserved is returned on tvm_dma_heap.
 *
 * @param tvm_dma_heap  TVM dma heap structure
 *
 * @return 0 on success, negative error code on failure.
 */
int fastrpc_reserve_dma_heap(struct fastrpc_tvm_dma_heap **tvm_dma_heap);

/*
 * Unreserve TVM dma heap.
 *
 * @param tvm_dma_heap  TVM dma heap structure to unreserve
 *
 * @return: None
 */
void fastrpc_unreserve_dma_heap(struct fastrpc_tvm_dma_heap *tvm_dma_heap);

/*
 * Allocate dma memory. Memory will be mapped to SMMU
 * and to kernel address space.
 *
 * @param buf  fastrpc buffer structure
 *
 * @return 0 on success, negative error code on failure.
 */
inline int __fastrpc_dma_alloc(struct fastrpc_buf *buf);

/*
 * Free dma buffer and unmap from SMMU and kernel.
 *
 * @param buf  fastrpc buffer structure
 *
 * @return: None
 */
void __fastrpc_dma_buf_free(struct fastrpc_buf *buf);

/*
 * Map dma buffer to SMMU
 *
 * @param attach  smmu device
 *
 * @return 0 on success, negative error code on failure.
 */
struct sg_table *__dma_buf_map_attachment_wrap(struct dma_buf_attachment *attach);

/*
 * Unmap buffer from SMMU
 *
 * @param attach  smmu device
 * @param table  sg table
 *
 * @return: None
 */
void __dma_buf_unmap_attachment_wrap(struct dma_buf_attachment *attach,
	struct sg_table *table);

/*
 * fastrpc_file_get - Take a reference on the fastrpc user object
 *
 * This function increments the reference count for the specified
 * fastrpc user object if it is non-zero, ensuring safe access.
 *
 * @fl: Pointer to the fastrpc_user structure.
 *
 * @return: 0 on success, or a negative error code on failure.
 */
int fastrpc_file_get(struct fastrpc_user *fl);

/*
 * fastrpc_file_put - Release reference to the fastrpc user object
 *
 * This function decrements the reference count for the specified
 * fastrpc user object. If the reference count drops to zero, the
 * corresponding resources are released.
 *
 * @fl: Pointer to the fastrpc_user structure.
 * @worker: If true, schedules the release callback in the worker thread,
 * 			otherwise, decreases the reference count.
 *
 * @return: None
 */
void fastrpc_file_put(struct fastrpc_user *fl, bool worker);

/*
 * fastrpc_is_device_crashing - Determine if the device is about to crash
 *
 * This function obtains the remoteproc handle, checks its status, and
 * determines whether the device is in a crashing state.
 *
 * @cctx: Pointer to the fastrpc_channel_ctx structure.
 *
 * @return: True if the remoteproc state indicates a crash with recovery
 *			disabled, otherwise false.
 */
bool fastrpc_is_device_crashing(struct fastrpc_channel_ctx *cctx);

#endif /* __FASTRPC_SHARED_H__ */
