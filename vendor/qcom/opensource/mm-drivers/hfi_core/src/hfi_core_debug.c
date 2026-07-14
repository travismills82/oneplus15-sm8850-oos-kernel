// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 */

#include <linux/debugfs.h>
#include "hfi_core_debug.h"
#include "hfi_core.h"
#include "hfi_interface.h"
#include "hfi_if_abstraction.h"
#include "hfi_dbg_packet.h"
#include <linux/kthread.h>
#if IS_ENABLED(CONFIG_DEBUG_FS)
#include "hfi_queue_controller.h"
#include "hfi_ipc.h"
#endif // CONFIG_DEBUG_FS

u32 msm_hfi_core_debug_level = HFI_CORE_PRINTK;
bool msm_hfi_fail_client_0_reg;
u32 msm_hfi_packet_cmd_id = 0x01000004;
#if IS_ENABLED(CONFIG_DEBUG_FS)
bool hfi_core_loop_back_mode_enable;
bool hfi_core_lb_start_event_thread;

#define HFI_COMMAND_DEVICE_INIT                                      0x01000001
#define HFI_COMMAND_DEVICE_INIT_DEVICE_CAPS                          0x01000002
#define HFI_COMMAND_DEVICE_INIT_VIG_CAPS                             0x01000003
#define HFI_COMMAND_DEVICE_INIT_DMA_CAPS                             0x01000004
#define HFI_COMMAND_DEVICE_INIT_COMMON_LAYER_CAPS                    0x01000005
#define HFI_COMMAND_DEVICE_INIT_DISPLAY_CAPS                         0x01000006
#define HFI_COMMAND_DEVICE_INIT_DISPLAY_WB_CAPS                      0x01000007
#define HFI_COMMAND_DISPLAY_FRAME_TRIGGER                            0x02000003
#define HFI_COMMAND_DISPLAY_EVENT_REGISTER                           0x02000004
#define HFI_COMMAND_DISPLAY_EVENT_DEREGISTER                         0x02000005
#define HFI_COMMAND_DISPLAY_EVENT_FRAME_SCAN_START                   0x04000002
#define HFI_COMMAND_DISPLAY_EVENT_VSYNC                              0x04000001
#define HFI_COMMAND_DEVICE_INIT_VIG_R1_CAPS                          0x0100000A
#define HFI_COMMAND_DEVICE_INIT_DMA_R1_CAPS                          0x0100000B
#define HFI_COMMAND_DEBUG_INIT                                       0xFF000007
#define HFI_COMMAND_DEBUG_PANIC_EVENT                                0xFF00000A
#define HFI_COMMAND_DEBUG_PANIC_SUBSCRIBE                            0xFF000009
#define HFI_COMMAND_DISPLAY_DISABLE                                  0x02000008
#define HFI_DEBUG_EVENT_UNDERRUN                                     (1 << 0)
#define FPS                                                          120

/*
 * hfi_display_event_id - HFI event ID
 * @HFI_EVENT_VSYNC             : Event ID for vsync
 * @HFI_EVENT_SCAN_START        : Event ID for frame scan start
 * @HFI_EVENT_SCAN_COMPLETE     : Event ID for frame scan complete
 * @HFI_EVENT_IDLE              : Event ID for frame idle
 * @HFI_EVENT_POWER             : Event ID for display power
 * @HFI_EVENT_MAX               : Max enum of display Event ID
 */
enum hfi_display_event_id {
	HFI_EVENT_VSYNC               = 0x1,
	HFI_EVENT_SCAN_START          = 0x2,
	HFI_EVENT_SCAN_COMPLETE       = 0x3,
	HFI_EVENT_IDLE                = 0x4,
	HFI_EVENT_POWER               = 0x5,
	HFI_EVENT_PANIC               = 0x6,
	HFI_EVENT_MAX                 = 0x7,
};

/**
 * struct event_data - Structure holding the event data of the registered events.
 *
 * @cmd_buff_type: command buffer type.
 * @object_id: object id.
 * @header_id: header id.
 * @cmd: u32 command id for respond response packet.
 * @id: per-type id(e.g display-id/layer-id, etc)
 * @packet_id: packet id
 */
struct event_data {
	u32 cmd_buff_type;
	u32 object_id;
	u32 header_id;
	u32 cmd;
	u32 id;
	u32 packet_id;
};

/**
 * struct hfi_dbg_events - Structure holding state of registered events.
 *
 * @event_enabled: flag for enabled event.
 * @evt_data: structure holding the event data.
 * @commit_response_flag: flag for response required at commit.
 * @frame_count: count of frame sice the event registered.
 * @frame_evt_reg: flag to trigger the scan_start thread
 *                 for the first frame after event register.
 */
struct hfi_dbg_events {
	bool event_enabled;
	struct event_data evt_data;
	bool commit_response_flag;
	u32 frame_count;
	bool frame_evt_reg;
};

struct hfi_dbg_events g_debug_events[HFI_EVENT_MAX];

/**
 * struct hfi_lb_mem_cache - define loopback cache item
 *
 * @hfi_cmd: HFI command
 * @payload: payload of response for HFI command
 * @payload_size: size of the payload
 * @flags: HFI command flag
 * @list: list head pointer
 */
struct hfi_lb_mem_cache {
	u32 hfi_cmd;
	u32 *payload;
	u32 payload_size;
	u32 flags;
	struct list_head list;
};

#endif // CONFIG_DEBUG_FS

/*
 * struct hfi_display_mode_info - hfi dcp mode info
 * @size            :  Size of hfi_dcs_mode_info structure.
 * @h_active        :  Active width of one frame in pixels.
 * @h_back_porch    :  Horizontal back porch in pixels.
 * @h_sync_width    :  HSYNC width in pixels.
 * @h_front_porch   :  Horizontal front porch in pixels.
 * @h_skew          :  Horizontal sync skew value
 * @h_sync_polarity :  Polarity of HSYNC (false is active low).
 * @v_active        :  Active height of one frame in lines.
 * @v_back_porch    :  Vertical back porch in lines.
 * @v_sync_width    :  VSYNC width in lines.
 * @v_front_porch   :  Vertical front porch in lines.
 * @v_sync_polarity :  Polarity of VSYNC (false is active low).
 * @clk_rate_hz_lo  :  Lower address value DSI bit clock rate per lane in Hz.
 * @clk_rate_hz_hi  :  Upper address value of DSI bit clock rate per lane in Hz.
 * @flags_lo        :  Lower address value of flags.
 * @flags_hi        :  Upper address value of flags.
 * @reserved1       :  Reserved for future use.
 * @reserved2       :  Reserved for future use.
 */
struct hfi_display_mode_info {
	u32 size;
	u32 h_active;
	u32 h_back_porch;
	u32 h_sync_width;
	u32 h_front_porch;
	u32 h_skew;
	u32 h_sync_polarity;
	u32 v_active;
	u32 v_back_porch;
	u32 v_sync_width;
	u32 v_front_porch;
	u32 v_sync_polarity;
	u32 refresh_rate;
	u32 clk_rate_hz_lo;
	u32 clk_rate_hz_hi;
	u32 flags_lo;
	u32 flags_hi;
	u32 reserved1;
	u32 reserved2;
};

/**
 * struct dbg_client_data - Structure holding the data of the debug clients.
 *
 * @list: client node.
 * @client_id: client id.
 * @client_handle: handle for the client, this is returned by the HFI core
 *                 driver after a successful registration of the client.
 * @open_params: client hfi core open parameters
 */
struct dbg_client_data {
	struct list_head list;
	void *client_handle;
	struct hfi_core_open_params open_params;
	struct hfi_core_cmds_buf_desc *buf_desc;
	bool lb_dcp_client_swi_configured;
	bool lb_dcp_client_resource_ready;
};

/**
 * struct hfi_core_dbg_data - structure holding debugfs data.
 *
 * @root: debugfs root
 * @drv_data: pointer to hfi core drv data structure
 * @clients_list: list of debug clients registered
 * @clients_list_lock: lock to synchronize access to the clients list
 * @signaled_clients_mask: Clients mask for which callback is received
 * @wait_queue: wait queue for clients that are waiting for callback
 *                  signal
 * @listener_thread: Thread to process callback signals received for
 *                   clients in "wait queue".
 * @lb_mem_cache: loopback cache list.
 * @worker: worker which executes the kthread_worker
 * @thread_priority_work: delayed worker to schedule
 *                   work at scan start of commit
 * @scan_start_thread: handle to scan start thread
 */
struct hfi_core_dbg_data {
	struct dentry *root;
	struct hfi_core_drv_data *drv_data;
	struct list_head clients_list;
	struct mutex clients_list_lock;
	atomic_t signaled_clients_mask;
	wait_queue_head_t wait_queue;
	struct task_struct *listener_thread;
	struct list_head lb_mem_cache;

	struct kthread_worker worker;
	struct kthread_delayed_work thread_priority_work;
	struct task_struct *scan_start_thread;
};

#if IS_ENABLED(CONFIG_DEBUG_FS)

/* version in the keys of the Panel Init HFI */
#define PANEL_KEYS_VERSION 0
/* Maximum number of Key, Value pairs for Panel Init HFI,
 * this number must be greater than the max number of elements
 * in panel_gen_keys, panel_tm_keys and panel_caps_keys
 */
#define PANEL_INIT_KV_PAIRS_MAX 30
#define UINT_BASE 16
#define PANIC_COMMIT_CNT_TIMEOUT 10

#define PACK_KV_PAIR(_kv_, _i_, _key_, _prop_) ({                             \
	_kv_[_i_].key = HFI_PACK_KEY(_key_, 0, (sizeof(_prop_)/sizeof(u32))); \
	_kv_[_i_].value_ptr = &_prop_;                                        \
})

//static u32 commit_add_packets[1] = {HFI_COMMAND_DISPLAY_FRAME_TRIGGER};

static u32 panel_init_add_packets[2] = {HFI_COMMAND_PANEL_INIT_TIMING_MODE_CAPS,
	HFI_COMMAND_PANEL_INIT_GENERIC_CAPS};

// HFI_COMMAND_PANEL_INIT_PANEL_CAPS 0x03000001
static u32 panel_caps_keys[1] = {HFI_PROPERTY_PANEL_TIMING_MODE_COUNT};
static u32 panel_caps_sizes[1] = {1};
static u32 panel_caps_payload[1] = {1};

// HFI_COMMAND_PANEL_INIT_GENERIC_CAPS 0x03000003
static u32 panel_gen_keys[23] = {
	HFI_PROPERTY_PANEL_PHYSICAL_TYPE,
	HFI_PROPERTY_PANEL_COLOR_ORDER,
	HFI_PROPERTY_PANEL_DMA_TRIGGER,
	HFI_PROPERTY_PANEL_TRAFFIC_MODE,
	HFI_PROPERTY_PANEL_VIRTUAL_CHANNEL_ID,
	HFI_PROPERTY_PANEL_WR_MEM_START,
	HFI_PROPERTY_PANEL_WR_MEM_CONTINUE,
	HFI_PROPERTY_PANEL_TE_DCS_COMMAND,
	HFI_PROPERTY_PANEL_OPERATING_MODE,
	HFI_PROPERTY_PANEL_BL_MIN_LEVEL,
	HFI_PROPERTY_PANEL_BL_MAX_LEVEL,
	HFI_PROPERTY_PANEL_BRIGHTNESS_MAX_LEVEL,
	HFI_PROPERTY_PANEL_NAME,
	HFI_PROPERTY_PANEL_BPP,
	HFI_PROPERTY_PANEL_LANES_STATE,
	HFI_PROPERTY_PANEL_LANE_MAP,
	HFI_PROPERTY_PANEL_TX_EOT_APPEND,
	HFI_PROPERTY_PANEL_BLLP_EOF_POWER_MODE,
	HFI_PROPERTY_PANEL_BLLP_POWER_MODE,
	HFI_PROPERTY_PANEL_BL_PMIC_CONTROL_TYPE,
	HFI_PROPERTY_PANEL_SEC_BL_PMIC_CONTROL_TYPE,
	HFI_PROPERTY_PANEL_CTRL_NUM,
	HFI_PROPERTY_PANEL_PHY_NUM
	};
static u32 panel_gen_sizes[23] = {
	1, 1, 1, 1, 1,
	1, 1, 1, 1, 1,
	1, 1, 1, 1, 1,
	1, 1, 1, 1, 1,
	1, 2, 2
};

static u32 panel_gen_payload[25] = {
	1,      1,      3,              3,      0,
	0,      0,      0,              1,      0xA,
	0xFFF,  0,      0x3733746E,     6,      0xF,
	1,      1,      1,              1,      3,
	0,      1,      0,              1,      0
};

// HFI_COMMAND_PANEL_INIT_TIMING_MODE_CAPS 0x03000002
static u32 panel_tm_keys[8] = {
	HFI_PROPERTY_PANEL_INDEX,
	HFI_PROPERTY_PANEL_CLOCKRATE,
	HFI_PROPERTY_PANEL_FRAMERATE,
	HFI_PROPERTY_PANEL_JITTER,
	HFI_PROPERTY_PANEL_RESOLUTION_DATA,
	HFI_PROPERTY_PANEL_COMPRESSION_DATA,
	HFI_PROPERTY_PANEL_DISPLAY_TOPOLOGY,
	HFI_PROPERTY_PANEL_DEFAULT_TOPOLOGY_INDEX
	};
static u32 panel_tm_sizes[8] = {1, 2, 1, 2, 0xe, 0xc, 4, 1};
static u32 panel_tm_payload[37] = {
	0,
	0, 0,
	0x78,
	2, 1,
	0x5A0, 0xC80, 0x64, 0x14, 0, 0, 0, 0, 0x14, 0x2C, 0x14, 0, 0, 0x2,
	1, 0x11, 0, 0x28, 0x2D0, 1, 8, 0, 0x80, 0, 0, 1,
	2, 2, 1, 0,
	0
};

static int _get_debugfs_input_client(struct file *file,
	const char __user *user_buf, size_t count, loff_t *ppos,
	struct hfi_core_drv_data **drv_data)
{
	char buf[10];
	int client_id;

	HFI_CORE_DBG_H("+\n");

	if (!file || !file->private_data) {
		HFI_CORE_ERR("invalid params\n");
		return -EINVAL;
	}

	if (!user_buf) {
		HFI_CORE_ERR("user buffer is null\n");
		return -EINVAL;
	}
	*drv_data = file->private_data;

	if (count >= sizeof(buf))
		return -EFAULT;

	if (copy_from_user(buf, user_buf, count))
		return -EFAULT;

	buf[count] = 0; /* end of string */

	if (kstrtouint(buf, 0, &client_id))
		return -EFAULT;

	if (client_id < HFI_CORE_CLIENT_ID_0 ||
		client_id >= HFI_CORE_CLIENT_ID_MAX) {
		HFI_CORE_ERR("invalid client_id:%d min:%d max:%d\n", client_id,
			HFI_CORE_CLIENT_ID_0, HFI_CORE_CLIENT_ID_MAX);
		return -EINVAL;
	}

	HFI_CORE_DBG_H("-\n");
	return client_id;
}

struct dbg_client_data *_get_client_node(struct hfi_core_drv_data *drv_data,
	u32 client_id)
{
	struct dbg_client_data *node = NULL;
	bool found = false;
	struct hfi_core_dbg_data *debugfs_data;

	HFI_CORE_DBG_H("+\n");

	if (!drv_data || !drv_data->debug_info.data) {
		HFI_CORE_ERR("invalid params\n");
		return NULL;
	}
	debugfs_data = (struct hfi_core_dbg_data *)drv_data->debug_info.data;

	mutex_lock(&debugfs_data->clients_list_lock);
	list_for_each_entry(node, &debugfs_data->clients_list, list) {
		if (node && node->open_params.client_id == client_id) {
			found = true;
			break;
		}
	}
	mutex_unlock(&debugfs_data->clients_list_lock);

	HFI_CORE_DBG_H("-\n");
	return found ? node : NULL;
}

static int print_hfi_header_info(struct hfi_header_info *header_info)
{
	char *cmd_buff_type_str;

	if (!header_info) {
		HFI_CORE_ERR("invalid header to print\n");
		return -EINVAL;
	}

	switch (header_info->cmd_buff_type) {
	case HFI_CMD_BUFF_SYSTEM:
		cmd_buff_type_str = "SYSTEM";
		break;
	case HFI_CMD_BUFF_DEVICE:
		cmd_buff_type_str = "DEVICE";
		break;
	case HFI_CMD_BUFF_DISPLAY:
		cmd_buff_type_str = "DISPLAY";
		break;
	case HFI_CMD_BUFF_DEBUG:
		cmd_buff_type_str = "DEBUG";
		break;
	case HFI_CMD_BUFF_VIRTUALIZATION:
		cmd_buff_type_str = "VIRTZ";
		break;
	default:
		cmd_buff_type_str = "Unknown";
		break;
	}

	HFI_CORE_DBG_H(
		"num_packets: %u cmd_buff_type: %s object_id: %u header_id: %u",
		header_info->num_packets, cmd_buff_type_str,
		header_info->object_id, header_info->header_id);

	return 0;
}

static int print_u32_payload(void *payload_ptr, u32 payload_size)
{
	u32 *payload_u32_ptr = (u32 *)payload_ptr;
	u32 array_size = 0;

	if (!payload_u32_ptr || !payload_size || (payload_size < 4)) {
		HFI_CORE_ERR("invalid payload to print, payload_sz: %u\n",
			payload_size);
		return -EINVAL;
	}

	array_size = payload_size / sizeof(u32);
	for (int i = 0; i < array_size; i++) {
		HFI_CORE_DBG_H("payload dword[%d]: 0x%x\n", i, *payload_u32_ptr);
		payload_u32_ptr++;
	}
	return 0;
}

static int print_hfi_packet_info(struct hfi_packet_info *packet_info)
{
	char *payload_type_str;

	if (!packet_info) {
		HFI_CORE_ERR("invalid packet to print\n");
		return -EINVAL;
	}

	switch (packet_info->payload_type) {
	case HFI_PAYLOAD_NONE:
		payload_type_str = "NONE";
		break;
	case HFI_PAYLOAD_U32:
		payload_type_str = "U32";
		break;
	case HFI_PAYLOAD_U32_ARRAY:
		payload_type_str = "U32_ARRAY";
		break;
	case HFI_PAYLOAD_U64:
		payload_type_str = "U64";
		break;
	case HFI_PAYLOAD_U64_ARRAY:
		payload_type_str = "U64_ARRAY";
		break;
	case HFI_PAYLOAD_BLOB:
		payload_type_str = "BLOB";
		break;
	default:
		payload_type_str = "Unknown";
		break;
	}

	HFI_CORE_DBG_H(
		"CMD: 0x%x id: 0x%x flags: 0x%x packet_id: 0x%x payload_type: %s payload_size: %u payload_ptr: 0x%llx",
		packet_info->cmd, packet_info->id,
		packet_info->flags, packet_info->packet_id,
		payload_type_str, packet_info->payload_size,
		(u64)packet_info->payload_ptr);

	/* print payload */
	switch (packet_info->payload_type) {
	case HFI_PAYLOAD_NONE:
		HFI_CORE_DBG_H("no payload\n");
		break;
	case HFI_PAYLOAD_U32:
	case HFI_PAYLOAD_U32_ARRAY:
		print_u32_payload(packet_info->payload_ptr,
			packet_info->payload_size);
		break;
	default:
		HFI_CORE_ERR("not support payload type to print\n");
		return -EINVAL;
	}

	return 0;
}

static struct hfi_lb_mem_cache *hfi_core_lb_cmd_get_payload(
	struct list_head *lb_head, u32 hfi_cmd)
{
	struct hfi_lb_mem_cache *lb_cache = NULL;

	if (!lb_head || list_empty(lb_head))
		return NULL;

	HFI_CORE_DBG_H("hfi_cmd: 0x%x ", hfi_cmd);
	list_for_each_entry(lb_cache, lb_head, list) {
		if (lb_cache) {
			if (lb_cache->hfi_cmd == hfi_cmd)
				return lb_cache;
		}
	}

	return NULL;
}

static int hfi_core_lb_append_packet(struct hfi_core_cmds_buf_desc *buf_desc,
	struct hfi_core_drv_data *drv_data, u32 cmd, u32 packet_id, u32 id)
{
	int ret = 0;
	struct hfi_cmd_buff_hdl pkt_buff_hdl;
	struct hfi_packet_info packet_info;
	struct hfi_lb_mem_cache *lb_cache = NULL;
	struct hfi_core_dbg_data *debugfs_data;

	if (!buf_desc) {
		HFI_CORE_ERR("invalid command buffer\n");
		return -EINVAL;
	}

	debugfs_data = (struct hfi_core_dbg_data *)drv_data->debug_info.data;
	if (!debugfs_data) {
		HFI_CORE_ERR("debugfs_data is null\n");
		return -EINVAL;
	}
	/*lookup the node in loopback cache*/
	lb_cache = hfi_core_lb_cmd_get_payload(&debugfs_data->lb_mem_cache, cmd);
	if (!lb_cache) {
		HFI_CORE_ERR("no response packet found for cmd: 0x%x\n", cmd);
		return -EINVAL;
	}

	/* fill tx buffer */
	pkt_buff_hdl.cmd_buffer = buf_desc->pbuf_vaddr;
	pkt_buff_hdl.size = buf_desc->size;

	packet_info.cmd = cmd;
	packet_info.id = id;
	packet_info.flags = lb_cache->flags;
	packet_info.packet_id = packet_id;
	if (lb_cache->payload_size == 0)
		packet_info.payload_type = HFI_PAYLOAD_NONE;
	else if (lb_cache->payload_size == 1)
		packet_info.payload_type = HFI_PAYLOAD_U32;
	else
		packet_info.payload_type = HFI_PAYLOAD_U32_ARRAY;

	packet_info.payload_size = lb_cache->payload_size * sizeof(u32);
	packet_info.payload_ptr = lb_cache->payload;
	ret = hfi_create_full_packet(&pkt_buff_hdl, &packet_info);
	if (ret) {
		HFI_CORE_ERR("failed to create hfi packet\n");
		return ret;
	}

	return ret;
}

static struct hfi_core_cmds_buf_desc *loopback_create_response_pkt(
	struct hfi_core_drv_data *drv_data, int client_id, uint32_t prio_info,
	struct hfi_header_info *header_info_rx)
{
	struct hfi_core_cmds_buf_desc *tx_buff_desc;
	struct hfi_cmd_buff_hdl pkt_buff_hdl;
	struct hfi_header_info header_info_tx;
	int ret = 0;

	/* get tx buffer */
	/* Allocate buf desc memory */
	tx_buff_desc = kzalloc(sizeof(struct hfi_core_cmds_buf_desc),
		GFP_KERNEL);
	if (!tx_buff_desc) {
		HFI_CORE_ERR(
			"failed to allocate buffer memory for client: %d\n",
			client_id);
		return NULL;
	}

	tx_buff_desc->prio_info = prio_info;
	ret = get_device_tx_buffer(drv_data, client_id, tx_buff_desc);
	if (ret || !tx_buff_desc->pbuf_vaddr || !tx_buff_desc->size) {
		HFI_CORE_ERR(
			"failed to get device tx buffer for client: %d ret: %d\n",
			client_id, ret);
		return NULL;
	}

	HFI_CORE_DBG_H(
		"got tx buffer for client: %d with pbuf_vaddr: 0x%llx size: %lu dva: 0x%llx\n",
		client_id, (u64)tx_buff_desc->pbuf_vaddr,
		tx_buff_desc->size, tx_buff_desc->priv_dva);

	/* fill tx buffer header from rx buffer header info */
	pkt_buff_hdl.cmd_buffer = tx_buff_desc->pbuf_vaddr;
	pkt_buff_hdl.size = tx_buff_desc->size;

	header_info_tx.cmd_buff_type = header_info_rx->cmd_buff_type;
	header_info_tx.object_id = header_info_rx->object_id;
	header_info_tx.header_id = header_info_rx->header_id;

	ret = hfi_create_header(&pkt_buff_hdl, &header_info_tx);
	if (ret) {
		HFI_CORE_ERR("failed to create hfi header\n");
		return NULL;
	}

	return tx_buff_desc;
}

static void update_global_event_data(u32 cmd_idx, struct hfi_header_info *header_info,
				struct hfi_packet_info *packet_info, bool reset)
{
	if (reset) {
		g_debug_events[cmd_idx].event_enabled = false;
		g_debug_events[cmd_idx].evt_data.cmd_buff_type = 0;
		g_debug_events[cmd_idx].evt_data.object_id = 0;
		g_debug_events[cmd_idx].evt_data.header_id = 0;
		g_debug_events[cmd_idx].evt_data.cmd = 0;
		g_debug_events[cmd_idx].evt_data.id = 0;
		g_debug_events[cmd_idx].evt_data.packet_id = 0;
		g_debug_events[cmd_idx].frame_count = 0;
		g_debug_events[cmd_idx].frame_evt_reg = false;

		return;
	}

	if (!packet_info || !header_info) {
		HFI_CORE_ERR("invalid params\n");
		return;
	}

	if (cmd_idx == HFI_EVENT_SCAN_START || cmd_idx == HFI_EVENT_VSYNC ||
		cmd_idx == HFI_EVENT_PANIC)
		g_debug_events[cmd_idx].event_enabled = true;
	else
		g_debug_events[cmd_idx].event_enabled = false;
	g_debug_events[cmd_idx].evt_data.cmd_buff_type = header_info->cmd_buff_type;
	g_debug_events[cmd_idx].evt_data.object_id = header_info->object_id;
	g_debug_events[cmd_idx].evt_data.header_id = header_info->header_id;
	g_debug_events[cmd_idx].evt_data.id = packet_info->id;
	g_debug_events[cmd_idx].evt_data.packet_id = packet_info->packet_id;
	g_debug_events[cmd_idx].frame_count = 0;
	if (hfi_core_lb_start_event_thread)
		g_debug_events[cmd_idx].frame_evt_reg = true;
	else
		g_debug_events[cmd_idx].frame_evt_reg = false;

	switch (cmd_idx) {
	case HFI_EVENT_SCAN_START:
		g_debug_events[cmd_idx].evt_data.cmd =
			HFI_COMMAND_DISPLAY_EVENT_FRAME_SCAN_START;
		break;
	case HFI_EVENT_VSYNC:
		g_debug_events[cmd_idx].evt_data.cmd =
			HFI_COMMAND_DISPLAY_EVENT_VSYNC;
		g_debug_events[cmd_idx].commit_response_flag = true;
		break;
	case HFI_EVENT_PANIC:
		g_debug_events[cmd_idx].evt_data.cmd =
			HFI_COMMAND_DEBUG_PANIC_EVENT;
		g_debug_events[cmd_idx].commit_response_flag = true;
		break;
	default:
		break;
	}
}

void hfi_core_event_callback(struct kthread_work *work)
{
	u64 qtmr_counter, frametime_ms;
	u32 payload_size, cmd_idx;
	u32 payload[3];
	u32 panic_payload[2];
	struct hfi_core_cmds_buf_desc *tx_buff_desc = NULL;
	struct hfi_cmd_buff_hdl pkt_buff_hdl;
	struct hfi_header_info header_info_tx;
	struct hfi_packet_info packet_info;
	int rc = 0, client_id = HFI_CORE_CLIENT_ID_LOOPBACK_DCP;
	struct hfi_core_drv_data *drv_data;
	struct hfi_core_dbg_data *debugfs_data = container_of(work, struct hfi_core_dbg_data,
				thread_priority_work.work);

	frametime_ms = DIV_ROUND_UP(1000, FPS);
	if (!debugfs_data) {
		HFI_CORE_ERR("debugfs_data not found, frame count is reset\n");
		goto exit;
	}

	drv_data = debugfs_data->drv_data;
	if (!drv_data) {
		HFI_CORE_ERR("drv_data not found, frame count is reset\n");
		goto exit;
	}

	/* The loop  runs from HFI_EVENT_VSYNC(0x1) to HFI_EVENT_DISPLAY_MAX(0x7) */
	for (int i = HFI_EVENT_VSYNC; i < HFI_EVENT_MAX; i++) {
		cmd_idx = i;
		if (!g_debug_events[cmd_idx].commit_response_flag)
			continue;
		/*Trigger panic in PANIC_COMMIT_CNT_TIMEOUT Frames*/
		if (cmd_idx == HFI_EVENT_PANIC &&
			g_debug_events[cmd_idx].frame_count != PANIC_COMMIT_CNT_TIMEOUT)
			continue;

		/* get tx buffer */
		/* Allocate buf desc memory */
		tx_buff_desc = kzalloc(sizeof(struct hfi_core_cmds_buf_desc), GFP_KERNEL);
		if (!tx_buff_desc) {
			HFI_CORE_ERR(
				"failed to allocate buffer for client: %d, frame count is reset\n",
				client_id);
			goto exit;
		}

		tx_buff_desc->prio_info = HFI_CORE_PRIO_1;
		rc = get_device_tx_buffer(drv_data, client_id, tx_buff_desc);
		if (rc || !tx_buff_desc->pbuf_vaddr || !tx_buff_desc->size) {
			HFI_CORE_ERR(
				"failed to get device tx buffer for client: %d rc: %d, frame count is reset\n",
				client_id, rc);
			goto exit;
		}

		HFI_CORE_DBG_H(
			"got tx buffer for client: %d with pbuf_vaddr: 0x%llx size: %lu dva: 0x%llx\n",
			client_id, (u64)tx_buff_desc->pbuf_vaddr,
			tx_buff_desc->size, tx_buff_desc->priv_dva);

		/* fill tx buffer header from rx buffer header info */
		pkt_buff_hdl.cmd_buffer = tx_buff_desc->pbuf_vaddr;
		pkt_buff_hdl.size = tx_buff_desc->size;

		header_info_tx.cmd_buff_type =
			g_debug_events[cmd_idx].evt_data.cmd_buff_type;
		header_info_tx.object_id = g_debug_events[cmd_idx].evt_data.object_id;
		header_info_tx.header_id = g_debug_events[cmd_idx].evt_data.header_id;

		rc = hfi_create_header(&pkt_buff_hdl, &header_info_tx);
		if (rc) {
			HFI_CORE_ERR("failed to create hfi header\n");
			goto fail;
		}

		packet_info.cmd = g_debug_events[cmd_idx].evt_data.cmd;
		packet_info.id = g_debug_events[cmd_idx].evt_data.id;
		packet_info.flags = 0x0;
		packet_info.packet_id = g_debug_events[cmd_idx].evt_data.packet_id;
		packet_info.payload_type = HFI_PAYLOAD_U32_ARRAY;
		if (cmd_idx == HFI_EVENT_PANIC) {
			panic_payload[0] = 0;
			panic_payload[1] = HFI_DEBUG_EVENT_UNDERRUN;
			payload_size = 2;
			packet_info.payload_ptr = panic_payload;
		} else {
			/* Convert Qtimer into u32 timestamp_hi & timestamp_lo values*/
			qtmr_counter = arch_timer_read_counter();
			payload[1] = (qtmr_counter & 0xFFFFFFFF);
			payload[0] = (qtmr_counter >> 32);
			payload[2] = g_debug_events[cmd_idx].frame_count;
			payload_size = 3;
			packet_info.payload_ptr = payload;
		}
		packet_info.payload_size = payload_size * sizeof(u32);
		rc = hfi_create_full_packet(&pkt_buff_hdl, &packet_info);
		if (rc) {
			HFI_CORE_ERR("failed to create hfi packet\n");
			goto fail;
		}

		/* set tx buffer for ipc signalling
		 * supports to send only one buf desc at a time
		 */
		rc = set_device_tx_buffer(drv_data, client_id, &tx_buff_desc, 1);
		if (rc) {
			HFI_CORE_ERR("failed to set device tx buff for signal\n");
			goto fail;
		}

		/* trigger the ipc now after setting the tx-buff */
		trigger_ipc(client_id, drv_data, HFI_IPC_EVENT_QUEUE_NOTIFY);
		kfree(tx_buff_desc);

		if (cmd_idx == HFI_EVENT_PANIC) {
			update_global_event_data(cmd_idx, NULL, NULL, true);
			continue;
		}

		if (cmd_idx == HFI_EVENT_VSYNC) {
			g_debug_events[cmd_idx].frame_count++;
			continue;
		}

		g_debug_events[cmd_idx].commit_response_flag = false;

	}

	kthread_queue_delayed_work(&debugfs_data->worker,
		&debugfs_data->thread_priority_work, msecs_to_jiffies(frametime_ms));

	return;

fail:
	kfree(tx_buff_desc);
exit:
	kthread_queue_delayed_work(&debugfs_data->worker,
		&debugfs_data->thread_priority_work, msecs_to_jiffies(frametime_ms));
	return;

}

static int process_lb_panic_subscribe(void *payload_ptr,
	struct hfi_header_info *header_info, struct hfi_packet_info *packet_info,
	struct hfi_core_dbg_data *debugfs_data)
{
	u32 *payload_u32_ptr;
	u32 enable, cmd_idx;
	u64 frametime_ms;

	if (!payload_ptr) {
		HFI_CORE_ERR("%s: invalid payload\n", __func__);
		return -EINVAL;
	}

	payload_u32_ptr = (u32 *)payload_ptr;
	enable = payload_u32_ptr[2];
	cmd_idx = HFI_EVENT_PANIC;

	if (enable) {
		update_global_event_data(cmd_idx, header_info, packet_info, false);
		frametime_ms = DIV_ROUND_UP(1000, FPS);
		if (hfi_core_lb_start_event_thread)
			kthread_queue_delayed_work(&debugfs_data->worker,
						&debugfs_data->thread_priority_work,
						msecs_to_jiffies(frametime_ms));
		hfi_core_lb_start_event_thread = false;
	} else {
		update_global_event_data(cmd_idx, NULL, NULL, true);
	}

	return 0;
}

static int process_lb_event_deregister(void *payload_ptr)
{
	u32 cmd_idx;
	u32 *payload_u32_ptr;

	if (!payload_ptr) {
		HFI_CORE_ERR("invalid payload\n");
		return -EINVAL;
	}

	payload_u32_ptr = (u32 *)payload_ptr;
	cmd_idx = *payload_u32_ptr;
	if (cmd_idx >= HFI_EVENT_VSYNC && cmd_idx < HFI_EVENT_MAX)
		update_global_event_data(cmd_idx, NULL, NULL, true);
	return 0;
}

static void process_lb_frame_trigger(struct hfi_core_dbg_data *debugfs_data)
{
	u64 frametime_ms;

	if (g_debug_events[HFI_EVENT_SCAN_START].event_enabled) {
		g_debug_events[HFI_EVENT_SCAN_START].commit_response_flag = true;
		if (g_debug_events[HFI_EVENT_SCAN_START].frame_evt_reg
				&& hfi_core_lb_start_event_thread) {
			frametime_ms = DIV_ROUND_UP(1000, FPS);
			kthread_queue_delayed_work(&debugfs_data->worker,
				&debugfs_data->thread_priority_work,
				msecs_to_jiffies(frametime_ms));
			g_debug_events[HFI_EVENT_SCAN_START].frame_evt_reg = false;
			hfi_core_lb_start_event_thread = false;
		}
		g_debug_events[HFI_EVENT_SCAN_START].frame_count++;
	}
	if (g_debug_events[HFI_EVENT_PANIC].event_enabled)
		g_debug_events[HFI_EVENT_PANIC].frame_count++;
}

static int process_lb_event_register(void *payload_ptr,
	struct hfi_header_info *header_info, struct hfi_packet_info *packet_info,
	struct hfi_core_dbg_data *debugfs_data)
{
	u32 cmd_idx;
	u64 frametime_ms;
	u32 *payload_u32_ptr;

	if (!payload_ptr) {
		HFI_CORE_ERR("invalid payload\n");
		return -EINVAL;
	}

	payload_u32_ptr = (u32 *)payload_ptr;
	cmd_idx = *payload_u32_ptr;
	if (cmd_idx >= HFI_EVENT_VSYNC && cmd_idx < HFI_EVENT_MAX) {
		if (cmd_idx == HFI_EVENT_SCAN_START || cmd_idx == HFI_EVENT_VSYNC) {
			update_global_event_data(cmd_idx, header_info, packet_info, false);
			if (cmd_idx == HFI_EVENT_VSYNC) {
				frametime_ms = DIV_ROUND_UP(1000, FPS);
				if (hfi_core_lb_start_event_thread)
					kthread_queue_delayed_work(&debugfs_data->worker,
						&debugfs_data->thread_priority_work,
						msecs_to_jiffies(frametime_ms));
				g_debug_events[cmd_idx].frame_evt_reg = false;
				hfi_core_lb_start_event_thread = false;
			}
		} else {
			HFI_CORE_ERR(
				"event register not enabled for event id: %d\n", cmd_idx);
		}
	}

	return 0;
}

static int process_lb_device_init_response(struct hfi_core_cmds_buf_desc *tx_buff_desc,
		struct hfi_core_drv_data *drv_data, u32 cmd, u32 packet_id, u32 id)
{
	int num_packets = 0, ret = 0;
	u32 mdss_init[] = {HFI_COMMAND_DEVICE_INIT_DEVICE_CAPS, HFI_COMMAND_DEVICE_INIT_VIG_CAPS,
		HFI_COMMAND_DEVICE_INIT_DMA_CAPS, HFI_COMMAND_DEVICE_INIT_COMMON_LAYER_CAPS,
		HFI_COMMAND_DEVICE_INIT_DISPLAY_CAPS, HFI_COMMAND_DEVICE_INIT_DISPLAY_WB_CAPS,
		HFI_COMMAND_DEVICE_INIT_VIG_R1_CAPS, HFI_COMMAND_DEVICE_INIT_DMA_R1_CAPS};

	/* fill tx buffer packet info from loopback cache */
	ret = hfi_core_lb_append_packet(tx_buff_desc, drv_data,
		cmd, packet_id, id);
	if (ret) {
		HFI_CORE_ERR(
			"failed to append packet info for buff desc: 0x%llx pkt id: %d\n",
			(u64)tx_buff_desc->pbuf_vaddr, packet_id);
	}
	num_packets = ARRAY_SIZE(mdss_init);
	for (int j = 0; j < num_packets; j++) {
		/* fill tx buffer packet info from loopback cache */
		ret = hfi_core_lb_append_packet(tx_buff_desc, drv_data, mdss_init[j],
			packet_id, id);
		if (ret) {
			HFI_CORE_ERR(
				"failed to append init packet for buff desc: 0x%llx packet: %d\n",
				(u64)tx_buff_desc->pbuf_vaddr, j);
		}
	}

	return ret;
}

static int process_loop_back_response(struct hfi_core_drv_data *drv_data,
	struct hfi_core_cmds_buf_desc *rx_buff_desc)
{
	int ret = 0, client_id = HFI_CORE_CLIENT_ID_LOOPBACK_DCP;
	struct hfi_cmd_buff_hdl cmd_buf_hdl;
	struct hfi_header_info header_info;
	struct hfi_packet_info packet_info;
	struct hfi_core_dbg_data *debugfs_data;
	struct hfi_core_cmds_buf_desc *tx_buff_desc = NULL;
	bool hfi_header_setup = false;

	HFI_CORE_DBG_H("+\n");

	if (!rx_buff_desc) {
		HFI_CORE_ERR("invalid params\n");
		return -EINVAL;
	}

	debugfs_data = (struct hfi_core_dbg_data *)drv_data->debug_info.data;
	if (list_empty(&debugfs_data->lb_mem_cache)) {
		HFI_CORE_ERR("list empty\n");
		return -EINVAL;
	}

	/*unpack rx buffer header*/
	cmd_buf_hdl.cmd_buffer = rx_buff_desc->pbuf_vaddr;
	cmd_buf_hdl.size = (u32)rx_buff_desc->size;

	ret = hfi_unpacker_get_header_info(&cmd_buf_hdl, &header_info);
	if (ret) {
		HFI_CORE_ERR("failed to get header info for buff desc: 0x%llx\n",
			(u64)rx_buff_desc->pbuf_vaddr);
		return ret;
	}

	if (!header_info.num_packets) {
		HFI_CORE_DBG_H("buff desc 0x%llx has no packets\n",
			(u64)rx_buff_desc->pbuf_vaddr);
		return 0;
	}

	print_hfi_header_info(&header_info);
	for (int i = 1; i <= header_info.num_packets; i++) {
		ret = hfi_unpacker_get_packet_info(&cmd_buf_hdl, i, &packet_info);
		if (ret) {
			HFI_CORE_ERR(
				"failed to get packet info for buff desc: 0x%llx packet: %d\n",
				(u64)rx_buff_desc->pbuf_vaddr, i);
			return ret;
		}
		print_hfi_packet_info(&packet_info);
		switch (packet_info.cmd) {
		case HFI_COMMAND_DEVICE_INIT:
			if (hfi_core_lb_cmd_get_payload(&debugfs_data->lb_mem_cache,
				packet_info.cmd)) {
				if (!hfi_header_setup) {
					tx_buff_desc = loopback_create_response_pkt(drv_data,
						client_id, HFI_CORE_PRIO_1, &header_info);
					if (!tx_buff_desc) {
						HFI_CORE_ERR(
							"failed to create tx buffer for client: %d\n",
							client_id);
						return -EINVAL;
					}
					hfi_header_setup = true;
				}
				ret = process_lb_device_init_response(tx_buff_desc, drv_data,
					packet_info.cmd, packet_info.packet_id, packet_info.id);
			}
			break;

		case HFI_COMMAND_DISPLAY_EVENT_REGISTER:
			ret = process_lb_event_register(packet_info.payload_ptr, &header_info,
				&packet_info, debugfs_data);
			if (ret)
				return ret;
			break;

		case HFI_COMMAND_DISPLAY_FRAME_TRIGGER:
			process_lb_frame_trigger(debugfs_data);
			break;

		case HFI_COMMAND_DISPLAY_EVENT_DEREGISTER:
			ret = process_lb_event_deregister(packet_info.payload_ptr);
			if (ret)
				return ret;
			break;
		case HFI_COMMAND_DEBUG_INIT:
			if (hfi_core_lb_cmd_get_payload(&debugfs_data->lb_mem_cache,
				packet_info.cmd)) {
				if (!hfi_header_setup) {
					tx_buff_desc = loopback_create_response_pkt(drv_data,
						client_id, HFI_CORE_PRIO_1, &header_info);
					if (!tx_buff_desc) {
						HFI_CORE_ERR(
							"failed to create tx buffer for client: %d\n",
							client_id);
						return -EINVAL;
					}
					hfi_header_setup = true;
				}
				ret = hfi_core_lb_append_packet(tx_buff_desc, drv_data,
					packet_info.cmd, packet_info.packet_id,
					packet_info.id);
				if (ret) {
					HFI_CORE_ERR(
						"failed to append packet info for buff desc: 0x%llx packet: %d\n",
						(u64)tx_buff_desc->pbuf_vaddr, i);
				}
			}
			break;

		case HFI_COMMAND_DEBUG_PANIC_SUBSCRIBE:
			process_lb_panic_subscribe(packet_info.payload_ptr, &header_info,
				&packet_info, debugfs_data);
			break;

		case HFI_COMMAND_DISPLAY_DISABLE:
			if (!hfi_header_setup) {
				tx_buff_desc = loopback_create_response_pkt(drv_data,
					client_id, HFI_CORE_PRIO_1, &header_info);
				if (!tx_buff_desc) {
					HFI_CORE_ERR(
						"failed to create tx buffer for client: %d\n",
						client_id);
					return -EINVAL;
				}
				hfi_header_setup = true;
			}
			ret = hfi_core_lb_append_packet(tx_buff_desc, drv_data,
					packet_info.cmd, packet_info.packet_id, packet_info.id);
			break;
		default:
			HFI_CORE_ERR(
				"No response found/supported for cmd id: %x\n",
				packet_info.cmd);
			break;
		}

	}

	if (tx_buff_desc) {
		/*
		 * Set tx buffer for ipc signalling
		 * supports to send only one buf desc at a time
		 */
		ret = set_device_tx_buffer(drv_data, client_id,
			&tx_buff_desc, 1);
		if (ret) {
			HFI_CORE_ERR(
				"failed to set device tx buff for signal\n");
			return ret;
		}

		/* trigger the ipc now after setting the tx-buff */
		trigger_ipc(client_id, drv_data, HFI_IPC_EVENT_QUEUE_NOTIFY);
		kfree(tx_buff_desc);
	}

	HFI_CORE_DBG_H("-\n");
	return ret;

}

static int dump_buffer(struct hfi_core_drv_data *drv_data,
	struct hfi_core_cmds_buf_desc *buff_desc)
{
	int ret = 0;
	struct hfi_cmd_buff_hdl cmd_buf_hdl;
	struct hfi_header_info header_info;
	struct hfi_packet_info packet_info;

	HFI_CORE_DBG_H("+\n");

	if (!drv_data || !buff_desc) {
		HFI_CORE_ERR("invalid params\n");
		return -EINVAL;
	}

	cmd_buf_hdl.cmd_buffer = buff_desc->pbuf_vaddr;
	cmd_buf_hdl.size = (u32)buff_desc->size;
	ret = hfi_unpacker_get_header_info(&cmd_buf_hdl, &header_info);
	if (ret) {
		HFI_CORE_ERR(
			"failed to get header info for buff desc: 0x%llx\n",
			(u64)buff_desc->pbuf_vaddr);
		return ret;
	}

	if (!header_info.num_packets) {
		HFI_CORE_DBG_H("buff desc 0x%llx has no packets\n",
			(u64)buff_desc->pbuf_vaddr);
		return 0;
	}

	print_hfi_header_info(&header_info);

	for (int i = 1; i <= header_info.num_packets; i++) {
		ret = hfi_unpacker_get_packet_info(&cmd_buf_hdl, i, &packet_info);
		if (ret) {
			HFI_CORE_ERR(
				"failed to get packet info for buff desc: 0x%llx packet: %d\n",
				(u64)buff_desc->pbuf_vaddr, i);
			return ret;
		}
		print_hfi_packet_info(&packet_info);
	}

	HFI_CORE_DBG_H("-\n");
	return ret;
}

static int get_rx_buffers(struct hfi_core_drv_data *drv_data, int client_id)
{
	int ret = 0;
	struct hfi_core_cmds_buf_desc buff_desc;
	struct dbg_client_data *client;
	struct hfi_core_cmds_buf_desc *buff_desc_ptr_array[1];

	HFI_CORE_DBG_H("+\n");

	client = _get_client_node(drv_data, client_id);
	if (!client || IS_ERR_OR_NULL(client->client_handle)) {
		HFI_CORE_ERR("client with id: %d is not found\n", client_id);
		return -EINVAL;
	}

	while (1) {
		memset(&buff_desc, 0, sizeof(buff_desc));
		ret = hfi_core_cmds_rx_buf_get(client->client_handle,
			&buff_desc);
		if (ret == -ENOBUFS) {
			HFI_CORE_DBG_H(
				"no more rx buffers to process for client: %d\n",
				client_id);
			break;
		}

		if (ret || !buff_desc.pbuf_vaddr || !buff_desc.size) {
			HFI_CORE_ERR(
				"failed to get rx buffer for client: %d ret: %d\n",
				client_id, ret);
			return -EINVAL;
		}
		HFI_CORE_DBG_H("-- printing RX buffer --\n");
		dump_buffer(drv_data, &buff_desc);
		HFI_CORE_DBG_H("-- printing RX buffer DONE --\n");

		buff_desc_ptr_array[0] = &buff_desc;
		ret = hfi_core_release_rx_buffer(client->client_handle,
			buff_desc_ptr_array, 1);
		if (ret) {
			HFI_CORE_DBG_H("failed to release rx buffer for client: %d\n",
				client_id);
			return ret;
		}
	}

	HFI_CORE_DBG_H("-\n");
	return ret;
}

static int init_loop_back_client(struct hfi_core_drv_data *drv_data,
	struct dbg_client_data *client_data, int client_id)
{
	int ret = 0;

	HFI_CORE_DBG_H("+\n");

	if (!client_data->lb_dcp_client_swi_configured) {
		client_data->lb_dcp_client_swi_configured = true;
		ret = trigger_ipc(client_id, drv_data,
			HFI_IPC_EVENT_QUEUE_NOTIFY);
		if (ret) {
			HFI_CORE_ERR("failed to trigger IPC power notification\n");
			return ret;
		}
		return 0;
	}

	if (!client_data->lb_dcp_client_resource_ready) {
		client_data->lb_dcp_client_resource_ready = true;
		ret = trigger_ipc(client_id, drv_data,
			HFI_IPC_EVENT_QUEUE_NOTIFY);
		if (ret) {
			HFI_CORE_ERR("failed to trigger IPC event notification\n");
			return ret;
		}
	}

	HFI_CORE_DBG_H("loopback client resource ready with id: %d\n",
		client_id);
	HFI_CORE_DBG_H("-\n");
	return ret;
}

static int process_loop_back_dcp_client(struct hfi_core_drv_data *drv_data,
	int client_id)
{
	int ret = 0;
	struct hfi_core_cmds_buf_desc *buff_desc;
	struct dbg_client_data *client;

	HFI_CORE_DBG_H("+\n");

	if (client_id != HFI_CORE_CLIENT_ID_LOOPBACK_DCP) {
		HFI_CORE_ERR("client :%d invalid\n", client_id);
		return -EINVAL;
	}

	/* we cannot create same debug client twice */
	client = _get_client_node(drv_data, client_id);
	if (!client) {
		HFI_CORE_ERR("client :%d not registered as debug client\n",
			client_id);
		return -EINVAL;
	}

	if (!client->lb_dcp_client_swi_configured ||
		!client->lb_dcp_client_resource_ready) {
		ret = init_loop_back_client(drv_data, client, client_id);
		if (ret) {
			HFI_CORE_ERR(
				"loopback client init failed with id: %d\n",
				client_id);
			return ret;
		}
		return 0;
	}
	/* loopback resource is ready */

	/* Allocate buf desc memory */
	buff_desc = kzalloc(sizeof(struct hfi_core_cmds_buf_desc), GFP_KERNEL);
	if (!buff_desc) {
		HFI_CORE_ERR(
			"failed to allocate buffer memory for client: %d\n",
			client_id);
		return -EINVAL;
	}

	while (1) {
		memset(buff_desc, 0, sizeof(struct hfi_core_cmds_buf_desc));
		ret = get_device_rx_buffer(drv_data, client_id, buff_desc);
		if (ret == -ENOBUFS)
			break;

		if (ret || !buff_desc->pbuf_vaddr || !buff_desc->size) {
			HFI_CORE_ERR(
				"failed to get device rx buffer for client: %d ret: %d\n",
				client_id, ret);
			return -EINVAL;
		}

		ret = process_loop_back_response(drv_data, buff_desc);
		if (ret) {
			HFI_CORE_ERR(
				"failed to process rx buffer for client: %d pbuf_vaddr: 0x%pK\n",
				client_id, buff_desc->pbuf_vaddr);
			return ret;
		}

		// put rx buffer
		ret = put_device_rx_buffer(drv_data, client_id, &buff_desc, 1);
		if (ret) {
			HFI_CORE_ERR(
				"failed to send tx buffer for client: %d pbuf_vaddr: 0x%pK\n",
				client_id, buff_desc->pbuf_vaddr);
			return ret;
		}
	}
	kfree(buff_desc);

	HFI_CORE_DBG_H("-\n");
	return ret;
}

int hfi_core_dgb_client_cb(struct hfi_core_session *hfi_session,
			const void *cb_data, u32 flags)
{
	HFI_CORE_DBG_H("+\n");

	if (!hfi_session || !cb_data) {
		HFI_CORE_ERR("invalid params\n");
		return -EINVAL;
	}

	struct hfi_core_dbg_data *dbg_data =
		(struct hfi_core_dbg_data *)cb_data;
	atomic_or(BIT(hfi_session->client_id),
		&dbg_data->signaled_clients_mask);
	wake_up_all(&dbg_data->wait_queue);

	HFI_CORE_DBG_H("-\n");
	return 0;
}

static int hfi_core_dbg_listener(void *data)
{
	u32 mask;
	struct hfi_core_dbg_data *dbg_data;
	struct hfi_core_drv_data *drv_data = (struct hfi_core_drv_data *)data;

	HFI_CORE_DBG_H("+\n");

	if (!drv_data || !drv_data->debug_info.data) {
		HFI_CORE_ERR("invalid params\n");
		return -EINVAL;
	}
	dbg_data = (struct hfi_core_dbg_data *)drv_data->debug_info.data;

	while (1) {
		wait_event(dbg_data->wait_queue,
			atomic_read(&dbg_data->signaled_clients_mask) != 0);
		mask = atomic_xchg(&dbg_data->signaled_clients_mask, 0);
		HFI_CORE_DBG_H("mask: %u\n", mask);
		if (!mask)
			continue;
		for (int client_id = HFI_CORE_CLIENT_ID_0;
			client_id < HFI_CORE_CLIENT_ID_MAX; client_id++) {
			if (BIT(client_id) & mask) {
				if (hfi_core_loop_back_mode_enable) {
					if (client_id ==
						HFI_CORE_CLIENT_ID_LOOPBACK_DCP)
						process_loop_back_dcp_client(
							drv_data, client_id);
					else
						get_rx_buffers(drv_data,
							client_id);
				} else {
					get_rx_buffers(drv_data, client_id);
				}
			}
		}
	}

	HFI_CORE_DBG_H("-\n");

	return 0;
}

static ssize_t hfi_core_dbg_reg_client(struct file *file,
	const char __user *user_buf, size_t count, loff_t *ppos)
{
	int client_id;
	struct hfi_core_drv_data *drv_data;
	struct dbg_client_data *client;
	struct hfi_core_cb_ops cb_ops;
	struct hfi_core_dbg_data *debugfs_data;

	HFI_CORE_DBG_H("+\n");

	client_id = _get_debugfs_input_client(file, user_buf, count, ppos,
		&drv_data);
	if (client_id < 0 || !drv_data || !drv_data->debug_info.data) {
		HFI_CORE_ERR(
			"failed to get client id: %d or drv data / dgb data\n",
			client_id);
		return -EINVAL;
	}

	/* we cannot create same debug client twice */
	if (_get_client_node(drv_data, client_id)) {
		HFI_CORE_ERR("client:%d already registered as debug client\n",
			client_id);
		return -EINVAL;
	}

	client = kzalloc(sizeof(*client), GFP_KERNEL);
	if (!client)
		return -ENOMEM;

	HFI_CORE_DBG_H("register client %d\n", client_id);

	debugfs_data = (struct hfi_core_dbg_data *)drv_data->debug_info.data;

	client->open_params.client_id = client_id;
	cb_ops.hfi_cb_fn = hfi_core_dgb_client_cb;
	cb_ops.cb_data = debugfs_data;
	client->open_params.ops = &cb_ops;

	client->client_handle = hfi_core_open_session(&client->open_params);
	if (IS_ERR_OR_NULL(client->client_handle)) {
		HFI_CORE_ERR("error registering as debug client:%d\n",
			client_id);
		client->client_handle = NULL;
		return -EFAULT;
	}

	mutex_lock(&debugfs_data->clients_list_lock);
	list_add(&client->list, &debugfs_data->clients_list);
	mutex_unlock(&debugfs_data->clients_list_lock);

	if (client_id == HFI_CORE_CLIENT_ID_LOOPBACK_DCP) {
		kthread_init_worker(&debugfs_data->worker);
		debugfs_data->scan_start_thread = kthread_run(
				kthread_worker_fn, &debugfs_data->worker, "event_scan_start");
		kthread_init_delayed_work(&debugfs_data->thread_priority_work,
					hfi_core_event_callback);
		hfi_core_lb_start_event_thread = true;
	}

	HFI_CORE_DBG_H("-\n");
	return count;
}

static ssize_t hfi_core_dbg_get_buf(struct file *file,
	const char __user *user_buf, size_t count, loff_t *ppos)
{
	int client_id, ret = 0;
	struct hfi_core_drv_data *drv_data;
	struct dbg_client_data *client;
	struct hfi_core_cmds_buf_desc *buff_desc;

	HFI_CORE_DBG_H("+\n");

	client_id = _get_debugfs_input_client(file, user_buf, count, ppos,
		&drv_data);
	if (client_id < 0 || !drv_data || !drv_data->debug_info.data) {
		HFI_CORE_ERR(
			"failed to get client id: %d or drv data / dgb data\n",
			client_id);
		return -EINVAL;
	}

	client = _get_client_node(drv_data, client_id);
	if (!client || IS_ERR_OR_NULL(client->client_handle)) {
		HFI_CORE_ERR("client with id: %d is not found\n", client_id);
		return -EINVAL;
	}

	/* Allocate buf desc memory */
	buff_desc = kzalloc(sizeof(struct hfi_core_cmds_buf_desc), GFP_KERNEL);
	if (!buff_desc) {
		HFI_CORE_ERR(
			"failed to allocate buffer memory for client: %d\n",
			client_id);
		return -EINVAL;
	}

	buff_desc->prio_info = HFI_CORE_PRIO_1;
	ret = hfi_core_cmds_tx_buf_get(client->client_handle, buff_desc);
	if (ret || !buff_desc->pbuf_vaddr || !buff_desc->size) {
		HFI_CORE_ERR(
			"failed to get tx buffer for client: %d ret: %d\n",
			client_id, ret);
		return -EINVAL;
	}
	client->buf_desc = buff_desc;

	HFI_CORE_DBG_H(
		"got tx buffer for client: %d with pbuf_vaddr: 0x%pK size: %lu\n",
		client_id, client->buf_desc->pbuf_vaddr,
		client->buf_desc->size);

	HFI_CORE_DBG_H("-\n");
	return count;
}

static ssize_t hfi_core_dbg_send_buf(struct file *file,
	const char __user *user_buf, size_t count, loff_t *ppos)
{
	int client_id;
	struct hfi_core_drv_data *drv_data;
	struct dbg_client_data *client;
	int ret = 0;

	HFI_CORE_DBG_H("+\n");

	client_id = _get_debugfs_input_client(file, user_buf, count, ppos,
		&drv_data);
	if (client_id < 0 || !drv_data || !drv_data->debug_info.data) {
		HFI_CORE_ERR(
			"failed to get client id: %d or drv data / dgb data\n",
			client_id);
		return -EINVAL;
	}

	client = _get_client_node(drv_data, client_id);
	if (!client || IS_ERR_OR_NULL(client->client_handle)) {
		HFI_CORE_ERR("client with id: %d is not found\n", client_id);
		return -EINVAL;
	}

	if (client && client->client_handle) {
		HFI_CORE_DBG_H("client with id: %d\n", client_id);
	} else {
		HFI_CORE_ERR("client is null\n");
		return -EINVAL;
	}

	if (client->buf_desc) {
		HFI_CORE_DBG_H(
			"sending tx buffer with pbuf_vaddr: 0x%pK size: %lu\n",
			client->buf_desc->pbuf_vaddr,
			client->buf_desc->size);
	} else {
		HFI_CORE_ERR("client buffer desc is null\n");
		return -EINVAL;
	}
	/* supports to send only one buf desc at a time */
	ret = hfi_core_cmds_tx_buf_send(client->client_handle,
		&client->buf_desc, 1, HFI_CORE_SET_FLAGS_TRIGGER_IPC);
	if (ret) {
		HFI_CORE_ERR(
			"failed to send tx buffer for client: %d pbuf_vaddr: 0x%pK\n",
			client_id, client->buf_desc->pbuf_vaddr);
		return ret;
	}

	HFI_CORE_ERR("sent tx buffer for client: %d with pbuf_vaddr: 0x%pK size: %lu\n",
		client_id, client->buf_desc->pbuf_vaddr,
		client->buf_desc->size);

	HFI_CORE_DBG_H("-\n");
	return count;
}

bool hfi_core_lb_cmd_update_payload(struct list_head *lb_head, u32 hfi_cmd,
	u32 flags, u32 payload_size, u32 *payload)
{
	bool found = false;
	struct hfi_lb_mem_cache *lb_cache = NULL;
	u32 *temp_data;

	if (!list_empty(lb_head)) {
		list_for_each_entry(lb_cache, lb_head, list) {
			if (lb_cache && lb_cache->hfi_cmd == hfi_cmd) {
				found = true;
				temp_data = lb_cache->payload;
				lb_cache->payload = payload;
				lb_cache->flags = flags;
				lb_cache->payload_size = payload_size;
				kfree(temp_data);
			}
		}
	}
	return found;
}

static int hfi_core_create_lb_cmd_packets(
	struct hfi_core_dbg_data *debugfs_data, u32 hfi_cmd, u32 flags,
	u32 payload_size, const int *cmd_buf)
{
	struct hfi_lb_mem_cache *lb_cache;
	u32 *payload = NULL;
	int ret = 0, size = 0;

	if (!debugfs_data || !cmd_buf) {
		HFI_CORE_ERR("invalid params\n");
		return -EINVAL;
	}

	if (payload_size > 0) {
		size = payload_size * sizeof(u32);
		payload = kmemdup(cmd_buf, size, GFP_KERNEL);
		if (!payload) {
			ret = -ENOMEM;
			HFI_CORE_ERR("payload creation failed\n");
			goto err;
		}
	}

	ret = hfi_core_lb_cmd_update_payload(&debugfs_data->lb_mem_cache,
		hfi_cmd, flags, payload_size, payload);
	if (ret) {
		HFI_CORE_DBG_INFO("node updated");
		return 0;
	}
	lb_cache = kzalloc(sizeof(struct hfi_lb_mem_cache), GFP_KERNEL);
	if (!lb_cache) {
		ret = -ENOMEM;
		HFI_CORE_ERR("adding node to cache failed\n");
		goto fail;
	}

	lb_cache->hfi_cmd = hfi_cmd;
	lb_cache->payload = payload;
	lb_cache->flags = flags;
	lb_cache->payload_size = payload_size;

	INIT_LIST_HEAD(&lb_cache->list);
	list_add(&lb_cache->list, &debugfs_data->lb_mem_cache);
	HFI_CORE_DBG_INFO("node added");

	return ret;

fail:
	kfree(payload);
err:
	return ret;
}

static ssize_t hfi_core_dbg_lb_cmd_buf_wr(struct file *file,
	const char __user *user_buf, size_t count, loff_t *ppos)
{
	struct hfi_core_drv_data *drv_data;
	struct hfi_core_dbg_data *debugfs_data;
	char *input, *token, *input_copy;
	const char *delim = " ";
	int ret = 0, strtoint = 0,  max_size = SZ_4K;
	int *buffer;
	u32 buf_size = 0, payload_size = 0;


	if (!file || !file->private_data) {
		HFI_CORE_ERR("unexpected data 0x%llx\n", (u64)file);
		return -EINVAL;
	}

	drv_data = file->private_data;
	debugfs_data = (struct hfi_core_dbg_data *)drv_data->debug_info.data;
	if (!debugfs_data) {
		HFI_CORE_ERR("debugfs_data is null\n");
		return -EINVAL;
	}

	input = kzalloc(count + 1, GFP_KERNEL);
	if (!input)
		return -ENOMEM;

	if (copy_from_user(input, user_buf, count)) {
		HFI_CORE_ERR("copy from user failed\n");
		ret = -EFAULT;
		goto end;
	}
	input[count] = '\0';

	input_copy = input;

	/* Parse payload size */
	token = strsep(&input_copy, delim);
	ret = kstrtouint(token, UINT_BASE, &payload_size);
	if (ret) {
		HFI_CORE_ERR("payload size parsing failed\n");
		goto end;
	}

	token = strsep(&input_copy, delim);

	/* dynamically create buffer for payload size */
	buffer = kzalloc((payload_size + 2) * sizeof(u32), GFP_KERNEL);
	if (!buffer) {
		ret = -ENOMEM;
		HFI_CORE_ERR("buffer creation failed\n");
		goto end;
	}

	/* Parse payload buffer */
	while (token) {
		ret = kstrtouint(token, 16, &strtoint);
		if (ret) {
			HFI_CORE_ERR("input buffer conversion failed\n");
			goto end1;
		}

		buffer[buf_size++] = strtoint;
		if (buf_size >= max_size) {
			HFI_CORE_ERR("buffer size exceeding the limit %d\n",
					max_size);
			ret = -EFAULT;
			goto end1;
		}
		token = strsep(&input_copy, delim);
	}

	HFI_CORE_DBG_INFO("command packet size in bytes: %u\n", buf_size);
	if (!buf_size) {
		ret = -EFAULT;
		goto end1;
	}

	if (payload_size == 0)
		ret = hfi_core_create_lb_cmd_packets(debugfs_data, buffer[0], buffer[1],
						payload_size, NULL);
	else
		ret = hfi_core_create_lb_cmd_packets(debugfs_data, buffer[0], buffer[1],
						payload_size, &buffer[2]);
	if (ret) {
		HFI_CORE_ERR("packet creation failed\n");
		goto end1;
	}

	ret = count;

end1:
	kfree(buffer);
end:
	kfree(input);
	return ret;
}

static ssize_t hfi_core_dbg_lb_cmd_buf_rd(struct file *file,
	char __user *user_buf, size_t count, loff_t *ppos)
{
	struct hfi_core_drv_data *drv_data;
	struct hfi_core_dbg_data *debugfs_data;
	struct hfi_lb_mem_cache *lb_cache;
	char *strs = NULL;
	char *strs_temp = NULL;
	char *buf = NULL;
	u32 len = 0, blen = 0, n = 0;
	int i = 0, left_size = 0, max_size = SZ_4K;

	if (*ppos)
		return 0;

	if (!user_buf) {
		HFI_CORE_ERR("unexpected user data\n");
		return -EINVAL;
	}

	if (!file || !file->private_data) {
		HFI_CORE_ERR("unexpected data 0x%llx\n", (u64)file);
		return -EINVAL;
	}

	drv_data = file->private_data;
	debugfs_data = (struct hfi_core_dbg_data *)drv_data->debug_info.data;
	if (!debugfs_data) {
		HFI_CORE_ERR("debugfs_data is null\n");
		return -EINVAL;
	}

	buf = kzalloc(max_size, GFP_KERNEL);
	if (!buf)
		return -ENOMEM;

	if (list_empty(&debugfs_data->lb_mem_cache)) {
		HFI_CORE_DBG_INFO("List Empty\n");
		return 0;
	}

	list_for_each_entry(lb_cache, &debugfs_data->lb_mem_cache, list) {
		if (lb_cache->payload_size > max_size) {
			HFI_CORE_ERR("no valid data from payload\n");
			return -EINVAL;
		}

		len += scnprintf(buf + len, max_size - len,
				"hfi_cmd: 0x%x flags: 0x%x payload_size: 0x%x\npayload:",
				lb_cache->hfi_cmd, lb_cache->flags,
				lb_cache->payload_size);

		if (lb_cache->payload_size > 0) {
			/* max digits in largest int value(2^32) = 10 */
			left_size = lb_cache->payload_size * 10 + 1;
			strs = kzalloc(left_size, GFP_KERNEL);
			if (!strs)
				return -ENOMEM;

			strs_temp = strs;
			for (i = 0; i < lb_cache->payload_size; i++) {
				n = scnprintf(strs_temp, left_size, "0x%x ",
					lb_cache->payload[i]);
				strs_temp += n;
				left_size -= n;
			}

			blen = strlen(strs);
			if (blen == 0) {
				HFI_CORE_ERR("snprintf failed, blen %d\n",
					blen);
				blen = -EFAULT;
				goto err;
			}
			len += scnprintf(buf + len, max_size - len, "%s\n", strs);

			kfree(strs);
		}
	}

	if (copy_to_user(user_buf, buf, len)) {
		HFI_CORE_ERR("copy to user buffer failed\n");
		len = -EFAULT;
		goto err;
	}

	*ppos += len;

err:
	kfree(buf);
	return len;
}

static ssize_t hfi_core_dbg_put_tx_buf(struct file *file,
	const char __user *user_buf, size_t count, loff_t *ppos)
{
	int client_id;
	struct hfi_core_drv_data *drv_data;
	struct dbg_client_data *client;
	int ret = 0;

	HFI_CORE_DBG_H("+\n");

	client_id = _get_debugfs_input_client(file, user_buf, count, ppos,
		&drv_data);
	if (client_id < 0 || !drv_data || !drv_data->debug_info.data) {
		HFI_CORE_ERR(
			"failed to get client id: %d or drv data / dgb data\n",
			client_id);
		return -EINVAL;
	}

	client = _get_client_node(drv_data, client_id);
	if (!client || IS_ERR_OR_NULL(client->client_handle)) {
		HFI_CORE_ERR("client with id: %d is not found\n", client_id);
		return -EINVAL;
	}

	if (client && client->client_handle) {
		HFI_CORE_DBG_H("client with id: %d\n", client_id);
	} else {
		HFI_CORE_ERR("client is null\n");
		return -EINVAL;
	}

	if (client->buf_desc) {
		HFI_CORE_DBG_H("put tx buffer with pbuf_vaddr: 0x%pK size: %lu\n",
			client->buf_desc->pbuf_vaddr, client->buf_desc->size);
	} else {
		HFI_CORE_ERR("client buffer desc is null\n");
		return count;
	}

	HFI_CORE_ERR(
		"put tx buffer for client: %d with pbuf_vaddr: 0x%pK size: %lu\n",
		client_id, client->buf_desc->pbuf_vaddr,
		client->buf_desc->size);
	// supports to send only one buf desc at a time
	ret = hfi_core_release_tx_buffer(client->client_handle,
		&client->buf_desc, 1);
	if (ret) {
		HFI_CORE_ERR(
			"failed to send tx buffer for client: %d pbuf_vaddr: 0x%pK\n",
			client_id, client->buf_desc->pbuf_vaddr);
		return ret;
	}
	kfree(client->buf_desc);

	HFI_CORE_DBG_H("-\n");
	return count;
}

static ssize_t hfi_core_dbg_unreg_client(struct file *file,
	const char __user *user_buf, size_t count, loff_t *ppos)
{
	int client_id;
	struct hfi_core_drv_data *drv_data;
	struct dbg_client_data *client;
	struct hfi_core_dbg_data *debugfs_data;
	int ret = 0;

	HFI_CORE_DBG_H("+\n");

	client_id = _get_debugfs_input_client(file, user_buf, count, ppos,
		&drv_data);
	if (client_id < 0 || !drv_data || !drv_data->debug_info.data) {
		HFI_CORE_ERR(
			"failed to get client id: %d or drv data / dgb data\n",
			client_id);
		return -EINVAL;
	}

	client = _get_client_node(drv_data, client_id);
	if (!client || IS_ERR_OR_NULL(client->client_handle)) {
		HFI_CORE_ERR("client with id: %d is not found\n", client_id);
		return -EINVAL;
	}
	debugfs_data = (struct hfi_core_dbg_data *)drv_data->debug_info.data;

	if (client && client->client_handle) {
		HFI_CORE_DBG_H("client with id: %d\n", client_id);
	} else {
		HFI_CORE_ERR("client is null\n");
		return -EINVAL;
	}

	// supports to send only one buf desc at a time
	ret = hfi_core_close_session(client->client_handle);
	if (ret) {
		HFI_CORE_ERR("failed to close client: %d\n", client_id);
		return ret;
	}

	mutex_lock(&debugfs_data->clients_list_lock);
	list_del_init(&client->list);
	mutex_unlock(&debugfs_data->clients_list_lock);
	kfree(client);

	if (client_id == HFI_CORE_CLIENT_ID_LOOPBACK_DCP) {
		if (debugfs_data->scan_start_thread) {
			kthread_flush_worker(&debugfs_data->worker);
			kthread_stop(debugfs_data->scan_start_thread);

			for (int i = HFI_EVENT_VSYNC; i < HFI_EVENT_MAX; i++)
				update_global_event_data(i, NULL, NULL, true);
		}
	}

	HFI_CORE_ERR("closed client: %d\n", client_id);

	HFI_CORE_DBG_H("-\n");
	return count;
}

static ssize_t hfi_core_dbg_print_res_tbl(struct file *file,
	const char __user *user_buf, size_t count, loff_t *ppos)
{
	int client_id;
	struct hfi_core_drv_data *drv_data;
	struct hfi_resource_data *res_data = NULL;
	struct hfi_core_resource_table_hdr *tbl_hdr;
	struct hfi_core_resource_hdr *res_hdr;
	struct hfi_channel_virtio_virtq *virtq_chan;
	struct hfi_virtio_virtq *virtq_res_hdr;

	HFI_CORE_DBG_H("+\n");

	client_id = _get_debugfs_input_client(file, user_buf, count, ppos,
		&drv_data);
	if (client_id < 0 || !drv_data || !drv_data->debug_info.data ||
		!drv_data->client_data[client_id].resource_info.res_data_mem) {
		HFI_CORE_ERR(
			"failed to get client id: %d or drv data / dgb data / res data\n",
			client_id);
		return -EINVAL;
	}

	res_data = (struct hfi_resource_data *)
		drv_data->client_data[client_id].resource_info.res_data_mem;

	/* print res tbl */
	if (!res_data->tbl_res_hdr_mem.cpu_va) {
		HFI_CORE_ERR("table header cpu va is null\n");
		return -EINVAL;
	}
	if (res_data->tbl_res_hdr_mem.size_wr <
		sizeof(struct hfi_core_resource_table_hdr)) {
		HFI_CORE_ERR("resource table size wr: %zu is incorrect\n",
			res_data->tbl_res_hdr_mem.size_wr);
		return -EINVAL;
	}
	tbl_hdr = (struct hfi_core_resource_table_hdr *)
		res_data->tbl_res_hdr_mem.cpu_va;
	HFI_CORE_DBG_H(
		"TBL HDR: ver: %x size: %x hdr_off: %x hdr_size: %x hdr_num: %u\n",
		tbl_hdr->version, tbl_hdr->size, tbl_hdr->res_hdr_offset,
		tbl_hdr->res_hdr_size, tbl_hdr->res_hdrs_num);

	/* print res header */
	if (res_data->tbl_res_hdr_mem.size_wr <
		(sizeof(struct hfi_core_resource_table_hdr) +
		(tbl_hdr->res_hdrs_num *
			sizeof(struct hfi_core_resource_hdr)))) {
		HFI_CORE_ERR("resource hdr size wr: %zu is incorrect\n",
			res_data->tbl_res_hdr_mem.size_wr);
		return -EINVAL;
	}
	res_hdr = (struct hfi_core_resource_hdr *)
		((u8 *)tbl_hdr + sizeof(struct hfi_core_resource_table_hdr));
	for (int i = 0; i < tbl_hdr->res_hdrs_num ; i++) {
		HFI_CORE_DBG_H(
			"RES HDR[%d]: ver: %x type: %u status: %u addrh: %x addrl: %u size: %x\n",
			i, res_hdr->version, res_hdr->type, res_hdr->status,
			res_hdr->start_addr_high, res_hdr->start_addr_low,
			res_hdr->size);
		res_hdr++;
	}

	/* print virtq res hr channel */
	if (!res_data->vitq_res.q_hdr_mem.mem.cpu_va) {
		HFI_CORE_ERR("virtq res hdr cpu va is null\n");
		return -EINVAL;
	}
	if (res_data->vitq_res.q_hdr_mem.mem.size_wr <
		sizeof(struct hfi_channel_virtio_virtq)) {
		HFI_CORE_ERR("virtq res hdr chan size wr: %zu is incorrect\n",
			res_data->vitq_res.q_hdr_mem.mem.size_wr);
		return -EINVAL;
	}
	virtq_chan = (struct hfi_channel_virtio_virtq *)
		res_data->vitq_res.q_hdr_mem.mem.cpu_va;
	HFI_CORE_DBG_H(
		"VIRTQ RES HDR CHAN: dev id: %u flags: %x res num: %u\n",
		virtq_chan->dcp_device_id, virtq_chan->flags, virtq_chan->num);

	/* print virtq res hrs */
	if (res_data->vitq_res.q_hdr_mem.mem.size_wr <
		(sizeof(struct hfi_channel_virtio_virtq) +
		(virtq_chan->num * sizeof(struct hfi_virtio_virtq)))) {
		HFI_CORE_ERR("virtq res hdr size wr: %zu is incorrect\n",
			res_data->vitq_res.q_hdr_mem.mem.size_wr);
		return -EINVAL;
	}
	virtq_res_hdr = (struct hfi_virtio_virtq *)
		((u8 *)virtq_chan + sizeof(struct hfi_channel_virtio_virtq));
	for (int i = 0; i < virtq_chan->num ; i++) {
		HFI_CORE_DBG_H(
			"VIRTQ RES HDR[%d]: queue id: %u prio: %u type: %u queue size: 0x%x addrl: 0x%x addrh: 0x%x align: %u size: 0x%x\n",
			i, virtq_res_hdr->queue_id,
			virtq_res_hdr->queue_priority, virtq_res_hdr->type,
			virtq_res_hdr->queue_size, virtq_res_hdr->addr_lower,
			virtq_res_hdr->addr_higher, virtq_res_hdr->alignment,
			virtq_res_hdr->size);
		virtq_res_hdr++;
	}

	HFI_CORE_DBG_H("-\n");
	return count;
}
#define INVAID_INDEX                                0xff
#define MAX_HFI_CMDS 8

static u32 *allocate_payload(u32 size)
{
	u32 *payload_ptr;

	HFI_CORE_DBG_H("+\n");

	payload_ptr = kzalloc(size, GFP_KERNEL);
	if (!payload_ptr) {
		HFI_CORE_ERR(
			"failed to allocate payload memory\n");
		return NULL;
	}

	HFI_CORE_DBG_H("-\n");
	return payload_ptr;
}

static int fill_header(struct hfi_header_info *header_info)
{
	u32 cmd_idx = INVAID_INDEX;
	static u32 types[MAX_HFI_CMDS] = {HFI_COMMAND_DEBUG_LOOPBACK_U32,
		HFI_COMMAND_DEVICE_INIT,
		HFI_COMMAND_PANEL_INIT_PANEL_CAPS,
		HFI_COMMAND_PANEL_INIT_TIMING_MODE_CAPS,
		HFI_COMMAND_PANEL_INIT_GENERIC_CAPS,
		HFI_COMMAND_DISPLAY_SET_MODE,
		HFI_COMMAND_DISPLAY_SET_PROPERTY,
		HFI_COMMAND_DISPLAY_FRAME_TRIGGER};

	if (!header_info) {
		HFI_CORE_ERR("invalid params\n");
		return -EINVAL;
	}

	for (int i = 0; i < MAX_HFI_CMDS; i++) {
		if (types[i] == msm_hfi_packet_cmd_id) {
			cmd_idx = i;
			break;
		}
	}

	if (cmd_idx == INVAID_INDEX) {
		HFI_CORE_ERR("cmd: 0x%x is not supported\n",
			msm_hfi_packet_cmd_id);
		return -EINVAL;
	}

	switch (cmd_idx) {
	case 0:
		header_info->cmd_buff_type = HFI_CMD_BUFF_DEBUG;
		header_info->object_id = 0;
		header_info->header_id = 16;
		return 0;
	case 1:
		header_info->cmd_buff_type = HFI_CMD_BUFF_DEVICE;
		header_info->object_id = 0;
		header_info->header_id = 1;
		return 0;
	case 2:
	case 3:
	case 4:
	case 5:
	case 6:
	case 7:
		header_info->cmd_buff_type = HFI_CMD_BUFF_DISPLAY;
		header_info->object_id = 0;
		header_info->header_id = 1;
		return 0;
	default:
		HFI_CORE_ERR("cmd idx: %d is not supported\n", cmd_idx);
		return -EINVAL;
	}
}

static int fill_packet(struct hfi_packet_info *packet_info)
{
	u32 cmd_idx = INVAID_INDEX;
	u32 *payload_ptr = NULL;
	static u32 types[MAX_HFI_CMDS] = {HFI_COMMAND_DEBUG_LOOPBACK_U32,
		HFI_COMMAND_DEVICE_INIT,
		HFI_COMMAND_PANEL_INIT_PANEL_CAPS,
		HFI_COMMAND_PANEL_INIT_TIMING_MODE_CAPS,
		HFI_COMMAND_PANEL_INIT_GENERIC_CAPS,
		HFI_COMMAND_DISPLAY_SET_MODE,
		HFI_COMMAND_DISPLAY_SET_PROPERTY,
		HFI_COMMAND_DISPLAY_FRAME_TRIGGER};
	static u32 loopback_payload[1] = {9680};
	static struct hfi_display_mode_info mode_info_payload = {
		.size = 18, /* TODO: Is this on bytes or Dwords? */
		.h_active = 1440,
		.h_back_porch = 20,
		.h_sync_width = 4,
		.h_front_porch = 20,
		.h_skew = 0,
		.h_sync_polarity = 0,
		.v_active = 3200,
		.v_back_porch = 18,
		.v_sync_width = 2,
		.v_front_porch = 20,
		.v_sync_polarity = 0,
		.refresh_rate = 120,
		.clk_rate_hz_lo = 0,
		.clk_rate_hz_hi = 0,
		.flags_lo = 0,
		.flags_hi = 0,
		.reserved1 = 0,
		.reserved2 = 0,
	};
	static u32 frame_trigger_payload[1] = {0x2}; // COMMIT

	if (!packet_info) {
		HFI_CORE_ERR("invalid params\n");
		return -EINVAL;
	}

	HFI_CORE_DBG_H("filling packet_info->cmd:0x%x\n", packet_info->cmd);
	for (int i = 0; i < MAX_HFI_CMDS; i++) {
		if (types[i] == packet_info->cmd) {
			cmd_idx = i;
			break;
		}
	}

	if (cmd_idx == INVAID_INDEX) {
		HFI_CORE_ERR("cmd: 0x%x is not supported\n",
			packet_info->cmd);
		return -EINVAL;
	}

	HFI_CORE_DBG_H("filling cmd_idx:%d\n", cmd_idx);
	switch (cmd_idx) {
	case 0: // HFI_COMMAND_DEBUG_LOOPBACK_U32
		packet_info->id = 0;
		packet_info->flags = HFI_TX_FLAGS_INTR_REQUIRED |
			HFI_TX_FLAGS_RESPONSE_REQUIRED;
		packet_info->packet_id = 16;
		packet_info->payload_type = HFI_PAYLOAD_U32;
		packet_info->payload_size = sizeof(loopback_payload);
		payload_ptr = allocate_payload(packet_info->payload_size);
		if (!payload_ptr) {
			HFI_CORE_ERR("payload allocate failed for %d\n",
				cmd_idx);
			return -EINVAL;
		}
		memcpy(payload_ptr, &loopback_payload[0],
			sizeof(loopback_payload));
		packet_info->payload_ptr = payload_ptr;
		return 0;
	case 1: // HFI_COMMAND_DEVICE_INIT
		packet_info->id = 0;
		packet_info->flags = HFI_TX_FLAGS_INTR_REQUIRED |
			HFI_TX_FLAGS_RESPONSE_REQUIRED;
		packet_info->packet_id = 1;
		packet_info->payload_type = HFI_PAYLOAD_NONE;
		packet_info->payload_size = 0;
		packet_info->payload_ptr = NULL;
		return 0;
	case 2: // HFI_COMMAND_PANEL_INIT_PANEL_CAPS
		packet_info->id = 0;
		packet_info->flags = HFI_TX_FLAGS_INTR_REQUIRED |
			HFI_TX_FLAGS_RESPONSE_REQUIRED;
		packet_info->packet_id = 2;
		packet_info->payload_type = HFI_PAYLOAD_U32_ARRAY;
		packet_info->payload_size = 0;
		packet_info->payload_ptr = NULL;
		return 0;
	case 3: // HFI_COMMAND_PANEL_INIT_TIMING_MODE_CAPS
		packet_info->id = 0;
		packet_info->flags = HFI_TX_FLAGS_INTR_REQUIRED |
			HFI_TX_FLAGS_RESPONSE_REQUIRED;
		packet_info->packet_id = 3;
		packet_info->payload_type = HFI_PAYLOAD_U32_ARRAY;
		packet_info->payload_size = 0;
		packet_info->payload_ptr = NULL;
		return 0;
	case 4: // HFI_COMMAND_PANEL_INIT_GENERIC_CAPS
		packet_info->id = 0;
		packet_info->flags = HFI_TX_FLAGS_INTR_REQUIRED |
			HFI_TX_FLAGS_RESPONSE_REQUIRED;
		packet_info->packet_id = 4;
		packet_info->payload_type = HFI_PAYLOAD_U32_ARRAY;
		packet_info->payload_size = 0;
		packet_info->payload_ptr = NULL;
		return 0;
	case 5: // HFI_COMMAND_DISPLAY_SET_MODE
		packet_info->id = 0;
		packet_info->flags = HFI_TX_FLAGS_INTR_REQUIRED |
			HFI_TX_FLAGS_RESPONSE_REQUIRED;
		packet_info->packet_id = 5;
		packet_info->payload_type = HFI_PAYLOAD_U32_ARRAY;
		packet_info->payload_size = sizeof(mode_info_payload);
		packet_info->payload_ptr = &mode_info_payload;
		return 0;
	case 6: // HFI_COMMAND_DISPLAY_SET_PROPERTY
		packet_info->id = 0;
		packet_info->flags = HFI_TX_FLAGS_INTR_REQUIRED |
			HFI_TX_FLAGS_RESPONSE_REQUIRED;
		packet_info->packet_id = 6;
		packet_info->payload_type = HFI_PAYLOAD_U32_ARRAY;
		packet_info->payload_size = 0;
		packet_info->payload_ptr = NULL;
		HFI_CORE_DBG_H("fill CMD:DISPLAY_SET_PROPERTY: cmd:0x%x type:0x%x\n",
			packet_info->cmd, packet_info->payload_type);
		return 0;
	case 7: // HFI_COMMAND_DISPLAY_FRAME_TRIGGER
		packet_info->id = 0;
		packet_info->flags = HFI_TX_FLAGS_INTR_REQUIRED |
			HFI_TX_FLAGS_RESPONSE_REQUIRED;
		packet_info->packet_id = 6;
		packet_info->payload_type = HFI_PAYLOAD_U32;
		packet_info->payload_size = sizeof(frame_trigger_payload);
		packet_info->payload_ptr = &frame_trigger_payload;
		HFI_CORE_DBG_H("fill CMD:DISPLAY_FRAME_TRIGGER: cmd:0x%x type:0x%x sz:%d\n",
			packet_info->cmd, packet_info->payload_type,
			packet_info->payload_size);
		return 0;
	default:
		HFI_CORE_ERR("cmd idx: %d is not supported\n", cmd_idx);
		return -EINVAL;
	}

}

enum hfi_color_formats {
	/* Interleaved RGB */
	HFI_COLOR_FORMAT_INTERLEAVED_RGB_MIN        = 0x01000000,
	HFI_COLOR_FORMAT_RGB565                     = 0x01000001,
	HFI_COLOR_FORMAT_RGB888                     = 0x01000002,
	HFI_COLOR_FORMAT_ARGB8888                   = 0x01000003,
	HFI_COLOR_FORMAT_RGBA8888                   = 0x01000004,
	HFI_COLOR_FORMAT_XRGB8888                   = 0x01000005,
	HFI_COLOR_FORMAT_RGBX8888                   = 0x01000006,
	HFI_COLOR_FORMAT_ARGB1555                   = 0x01000007,
	HFI_COLOR_FORMAT_RGBA5551                   = 0x01000008,
	HFI_COLOR_FORMAT_XRGB1555                   = 0x01000009,
	HFI_COLOR_FORMAT_RGBX5551                   = 0x0100000A,
	HFI_COLOR_FORMAT_ARGB4444                   = 0x0100000B,
	HFI_COLOR_FORMAT_RGBA4444                   = 0x0100000C,
	HFI_COLOR_FORMAT_RGBX4444                   = 0x0100000D,
	HFI_COLOR_FORMAT_XRGB4444                   = 0x0100000E,
	HFI_COLOR_FORMAT_ARGB2_10_10_10             = 0x0100000F,
	HFI_COLOR_FORMAT_XRGB2_10_10_10             = 0x01000010,
	HFI_COLOR_FORMAT_RGBA10_10_10_2             = 0x01000011,
	HFI_COLOR_FORMAT_RGBX10_10_10_2             = 0x01000012,
	HFI_COLOR_FORMAT_ARGB_FP_16                 = 0x01000013,
	HFI_COLOR_FORMAT_RGBA_FP_16                 = 0x01000014,
	HFI_COLOR_FORMAT_INTERLEAVED_RGB_MAX        = 0x01FFFFFF,
};

struct layer_prop_u32 {
	u32 id;
	u32 prop;
};

struct layer_prop_roi {
	u32 id;
	u32 x;
	u32 y;
	u32 w;
	u32 h;
};

struct layer_props {
	struct layer_prop_u32 blend_type;
	struct layer_prop_u32 alpha;
	struct layer_prop_u32 zpos;
	struct layer_prop_roi src_roi;
	struct layer_prop_roi dest_roi;
//	layer_prop_roi blend_roi;
	struct layer_prop_u32 src_img_w;
	struct layer_prop_u32 src_img_h;
	struct layer_prop_u32 src_addr;
	struct layer_prop_u32 src_format;
};

int append_kv_pairs_if_needed_commit(struct hfi_cmd_buff_hdl *cmd_buf_hdl,
	struct hfi_packet_info *packet_info)
{
	int i, rc = 0;
	struct hfi_kv_info kv_pairs[20];
	u32 kv_cnt;
	u32 kv_size = 0;
	u32 LAYER_1 = 1;
	u32 LAYER_2 = 2;
	u32 num_layers;

	struct layer_props layers[2] = {
		{
			.blend_type = {LAYER_1, 0},
			.alpha = {LAYER_1, 1023},
			.zpos = {LAYER_1, 1},
			.src_roi = {LAYER_1, 0, 0, 960, 1080},
			.dest_roi = {LAYER_1, 0, 0, 960, 1080},
			.src_img_w = {LAYER_1, 1920},
			.src_img_h = {LAYER_1, 1080},
			.src_addr = {LAYER_1, 0xdeadbeef},
			.src_format = {LAYER_1, HFI_COLOR_FORMAT_XRGB8888}
		},
		{
			.blend_type = {LAYER_2, 1},
			.alpha = {LAYER_2, 1023},
			.zpos = {LAYER_2, 1},
			.src_roi = {LAYER_2, 960, 0, 960, 1080},
			.dest_roi = {LAYER_2, 960, 0, 960, 1080},
			.src_img_w = {LAYER_2, 1920},
			.src_img_h = {LAYER_2, 1080},
			.src_addr = {LAYER_2, 0xdeadbeef},
			.src_format = {LAYER_2, HFI_COLOR_FORMAT_XRGB8888}
		}
	};
	HFI_CORE_DBG_H("+\n");

	if (!cmd_buf_hdl || !packet_info) {
		HFI_CORE_ERR("invalid params\n");
		return -EINVAL;
	}

	if (packet_info->cmd != HFI_COMMAND_DISPLAY_SET_PROPERTY) {
		HFI_CORE_ERR("not needed for cmd: 0x%x\n", packet_info->cmd);
		return 0;
	}

	//// Pack HFI_PROPERTY_DISPLAY_ATTACH_LAYER /////////////////
	kv_cnt = 2;
	memset(kv_pairs, 0, sizeof(kv_pairs));

	PACK_KV_PAIR(kv_pairs, 0, HFI_PROPERTY_DISPLAY_ATTACH_LAYER, LAYER_1);
	PACK_KV_PAIR(kv_pairs, 1, HFI_PROPERTY_DISPLAY_ATTACH_LAYER, LAYER_2);

	kv_size = sizeof(u32) * kv_cnt + sizeof(u32) * kv_cnt;
	//Append commit packets pairs
	HFI_CORE_DBG_H("cmd: 0x%x kv_cnt: %d kv_size: %u\n",
		packet_info->cmd, kv_cnt, kv_size);
	rc = hfi_append_packet_with_kv_pairs(cmd_buf_hdl, packet_info->cmd,
		HFI_PAYLOAD_U32_ARRAY, 0, &kv_pairs[0], kv_cnt, kv_size);
	if (rc) {
		HFI_CORE_ERR("Error in creating kv pair for commit\n");
		return rc;
	}

	//// Pack LAYER PROPERTIES /////////////////////////////////

	num_layers = sizeof(layers) / sizeof(struct layer_props);

	for (i = 0; i < num_layers; i++) {
		memset(kv_pairs, 0, sizeof(kv_pairs));
		kv_cnt = 9;
		PACK_KV_PAIR(kv_pairs, 0, HFI_PROPERTY_LAYER_BLEND_TYPE,
			layers[i].blend_type);
		PACK_KV_PAIR(kv_pairs, 1, HFI_PROPERTY_LAYER_ALPHA,
			layers[i].alpha);
		PACK_KV_PAIR(kv_pairs, 2, HFI_PROPERTY_LAYER_ZPOS,
			layers[i].zpos);
		PACK_KV_PAIR(kv_pairs, 3, HFI_PROPERTY_LAYER_SRC_ROI,
			layers[i].src_roi);
		PACK_KV_PAIR(kv_pairs, 4, HFI_PROPERTY_LAYER_DEST_ROI,
			layers[i].dest_roi);
		PACK_KV_PAIR(kv_pairs, 5, HFI_PROPERTY_LAYER_SRC_IMG_SIZE_W,
			layers[i].src_img_w);
		PACK_KV_PAIR(kv_pairs, 6, HFI_PROPERTY_LAYER_SRC_IMG_SIZE_H,
			layers[i].src_img_h);
		PACK_KV_PAIR(kv_pairs, 7, HFI_PROPERTY_LAYER_SRC_ADDR,
			layers[i].src_addr);
		PACK_KV_PAIR(kv_pairs, 8, HFI_PROPERTY_LAYER_SRC_FORMAT,
			layers[i].src_format);

		kv_size = sizeof(u32) * kv_cnt + sizeof(struct layer_props);
		//Append commit packets pairs
		HFI_CORE_DBG_H("cmd: 0x%x kv_cnt: %d kv_size: %u\n",
			packet_info->cmd, kv_cnt, kv_size);
		rc = hfi_append_packet_with_kv_pairs(cmd_buf_hdl,
			packet_info->cmd, HFI_PAYLOAD_U32_ARRAY, 0,
			&kv_pairs[0], kv_cnt, kv_size);
		if (rc) {
			HFI_CORE_ERR("Error in creating kv pair for commit\n");
			return rc;
		}
	}

	HFI_CORE_DBG_H("-\n");
	return rc;
}

int append_kv_pairs_if_needed(struct hfi_cmd_buff_hdl *cmd_buf_hdl,
	struct hfi_packet_info *packet_info)
{
	int ret = 0;
	struct hfi_kv_info kv_pairs[PANEL_INIT_KV_PAIRS_MAX];
	u32 num_props = 0;
	u32 append_size = 0;

	HFI_CORE_DBG_H("+\n");

	if (!cmd_buf_hdl || !packet_info) {
		HFI_CORE_ERR("invalid params\n");
		return -EINVAL;
	}

	if (packet_info->cmd != HFI_COMMAND_PANEL_INIT_PANEL_CAPS &&
		packet_info->cmd != HFI_COMMAND_PANEL_INIT_TIMING_MODE_CAPS &&
		packet_info->cmd != HFI_COMMAND_PANEL_INIT_GENERIC_CAPS) {
		HFI_CORE_ERR("not needed for cmd: 0x%x\n", packet_info->cmd);
		return 0;
	}

	if (packet_info->cmd == HFI_COMMAND_PANEL_INIT_PANEL_CAPS) {
		num_props = sizeof(panel_caps_keys) / sizeof(u32);
		append_size = (num_props * sizeof(u32)) +
			sizeof(panel_caps_payload);
		for (int i = 0; i < num_props; i++) {
			kv_pairs[i].key = HFI_PACK_KEY(panel_caps_keys[i],
				PANEL_KEYS_VERSION, panel_caps_sizes[i]);
			if (i == 0)
				kv_pairs[i].value_ptr = panel_caps_payload;
			else
				kv_pairs[i].value_ptr = (void *)
					((u32 *)kv_pairs[i - 1].value_ptr +
					panel_caps_sizes[i - 1]);
		}
	} else if (packet_info->cmd == HFI_COMMAND_PANEL_INIT_TIMING_MODE_CAPS) {
		num_props = sizeof(panel_tm_keys) / sizeof(u32);
		append_size = (num_props * sizeof(u32)) +
			sizeof(panel_tm_payload);
		for (int i = 0; i < num_props; i++) {
			kv_pairs[i].key = HFI_PACK_KEY(panel_tm_keys[i],
				PANEL_KEYS_VERSION, panel_tm_sizes[i]);
			if (i == 0)
				kv_pairs[i].value_ptr = panel_tm_payload;
			else
				kv_pairs[i].value_ptr = (void *)
					((u32 *)kv_pairs[i - 1].value_ptr +
					panel_tm_sizes[i - 1]);
		}
	} else if (packet_info->cmd == HFI_COMMAND_PANEL_INIT_GENERIC_CAPS) {
		num_props = sizeof(panel_gen_keys) / sizeof(u32);
		append_size = (num_props * sizeof(u32)) +
			sizeof(panel_gen_payload);
		for (int i = 0; i < num_props; i++) {
			kv_pairs[i].key = HFI_PACK_KEY(panel_gen_keys[i],
				PANEL_KEYS_VERSION, panel_gen_sizes[i]);

			if (i == 0)
				kv_pairs[i].value_ptr = panel_gen_payload;
			else
				kv_pairs[i].value_ptr = (void *)
					((u32 *)kv_pairs[i - 1].value_ptr +
					panel_gen_sizes[i - 1]);
		}
	} else {
		HFI_CORE_ERR("unknown command:0x%x\n", packet_info->cmd);
	}

	HFI_CORE_DBG_H("cmd: 0x%x num_props: %d append_size: %u\n",
		packet_info->cmd, num_props, append_size);
	ret = hfi_append_packet_with_kv_pairs(cmd_buf_hdl,
		packet_info->cmd, HFI_PAYLOAD_U32_ARRAY, 0,
		&kv_pairs[0], num_props, append_size);
	if (ret) {
		HFI_CORE_ERR("failed for cmd: 0x%x\n", packet_info->cmd);
		return ret;
	}

	HFI_CORE_DBG_H("-\n");
	return ret;
}
static ssize_t hfi_core_dbg_test_packet(struct file *file,
	const char __user *user_buf, size_t count, loff_t *ppos)
{
	int client_id;
	struct hfi_core_drv_data *drv_data;
	struct dbg_client_data *client;
	int ret = 0;
	struct hfi_cmd_buff_hdl pkt_buff_hdl;
	struct hfi_header_info header_info;
	struct hfi_packet_info packet_info;
	struct hfi_core_cmds_buf_desc *buff_desc;
	u32 num_packets = 1;

	HFI_CORE_DBG_H("+\n");

	client_id = _get_debugfs_input_client(file, user_buf, count, ppos,
		&drv_data);
	if (client_id < 0 || !drv_data || !drv_data->debug_info.data) {
		HFI_CORE_ERR(
			"failed to get client id: %d or drv data / dgb data\n",
			client_id);
		return -EINVAL;
	}

	client = _get_client_node(drv_data, client_id);
	if (!client || IS_ERR_OR_NULL(client->client_handle)) {
		HFI_CORE_ERR("client with id: %d is not found\n", client_id);
		return -EINVAL;
	}

	if (client && client->client_handle) {
		HFI_CORE_DBG_H("client with id: %d\n", client_id);
	} else {
		HFI_CORE_ERR("client is null\n");
		return -EINVAL;
	}

	/* get tx buffer */
	/* Allocate buf desc memory */
	buff_desc = kzalloc(sizeof(struct hfi_core_cmds_buf_desc), GFP_KERNEL);
	if (!buff_desc) {
		HFI_CORE_ERR(
			"failed to allocate buffer memory for client: %d\n",
			client_id);
		return -EINVAL;
	}

	buff_desc->prio_info = HFI_CORE_PRIO_1;
	ret = hfi_core_cmds_tx_buf_get(client->client_handle, buff_desc);
	if (ret || !buff_desc->pbuf_vaddr ||
		!buff_desc->size) {
		HFI_CORE_ERR("failed to get tx buffer for client: %d ret: %d\n",
			client_id, ret);
		return -EINVAL;
	}
	client->buf_desc = buff_desc;

	HFI_CORE_DBG_H(
		"got tx buffer for client: %d with pbuf_vaddr: 0x%llx size: %lu dva: 0x%llx\n",
		client_id, (u64)client->buf_desc->pbuf_vaddr,
		client->buf_desc->size, client->buf_desc->priv_dva);

	/* fill tx buffer */
	pkt_buff_hdl.cmd_buffer = client->buf_desc->pbuf_vaddr;
	pkt_buff_hdl.size = client->buf_desc->size;
	ret = fill_header(&header_info);
	if (ret) {
		HFI_CORE_ERR("failed to fill hfi header\n");
		return ret;
	}
	ret = hfi_create_header(&pkt_buff_hdl, &header_info);
	if (ret) {
		HFI_CORE_ERR("failed to create hfi header\n");
		return ret;
	}

	packet_info.cmd = msm_hfi_packet_cmd_id;
	if (packet_info.cmd == HFI_COMMAND_PANEL_INIT_PANEL_CAPS)
		num_packets = 3;
	if (packet_info.cmd == HFI_COMMAND_DISPLAY_SET_PROPERTY)
		num_packets = 2;

	for (int i = 0; i < num_packets; i++) {
		if (i > 0) {
			if (msm_hfi_packet_cmd_id ==
				HFI_COMMAND_PANEL_INIT_PANEL_CAPS) {
				memset(&packet_info, 0,
					sizeof(struct hfi_packet_info));
				packet_info.cmd =
					panel_init_add_packets[i - 1];
			}
			if (msm_hfi_packet_cmd_id ==
				HFI_COMMAND_DISPLAY_SET_PROPERTY) {
				HFI_CORE_DBG_H("clearing packet_info for frame trigger\n");
				memset(&packet_info, 0,
					sizeof(struct hfi_packet_info));
				packet_info.cmd =
					HFI_COMMAND_DISPLAY_FRAME_TRIGGER;
			}
		}

		HFI_CORE_DBG_H("packet_info.cmd:0x%x\n", packet_info.cmd);
		ret = fill_packet(&packet_info);
		if (ret) {
			HFI_CORE_ERR("failed to fill hfi packet\n");
			return ret;
		}
		ret = hfi_create_full_packet(&pkt_buff_hdl, &packet_info);
		if (ret) {
			HFI_CORE_ERR("failed to create hfi header\n");
			return ret;
		}

		ret = append_kv_pairs_if_needed(&pkt_buff_hdl, &packet_info);
		if (ret) {
			HFI_CORE_ERR("failed to append kv pairs\n");
			return ret;
		}
		ret = append_kv_pairs_if_needed_commit(&pkt_buff_hdl,
			&packet_info);
		if (ret) {
			HFI_CORE_ERR("failed to append kv pairs for commit\n");
			return ret;
		}
	}

	HFI_CORE_DBG_H("sending header_info.cmd_buff_type:0x%x  packet_info.cmd:0x%x type:0x%x\n",
		header_info.cmd_buff_type, packet_info.cmd,
		packet_info.payload_type);
	HFI_CORE_DBG_H("-- printing TX buffer --\n");
	dump_buffer(drv_data, client->buf_desc);
	HFI_CORE_DBG_H("-- printing TX buffer DONE --\n");

	/* send tx buffer */
	// supports to send only one buf desc at a time
	ret = hfi_core_cmds_tx_buf_send(client->client_handle,
		&client->buf_desc, 1, HFI_CORE_SET_FLAGS_TRIGGER_IPC);
	if (ret) {
		HFI_CORE_ERR("failed to send tx buffer for client: %d pbuf_vaddr: 0x%pK\n",
			client_id, client->buf_desc->pbuf_vaddr);
		return ret;
	}

	HFI_CORE_DBG_H("-\n");
	return count;
}

char *get_dump_event_str(u32 val)
{
	char *str;

	switch (val) {
	case 0xbeef:
		str = "init trace dumps. [max trace events][trace mem ptr]";
		break;
	default:
		str = NULL;
	}

	return str;
}

static inline int _dump_event(struct hfi_core_trace_event *event, char *buf,
	int len, int max_size, u32 index)
{
	char data[HFI_CORE_MAX_DATA_PER_EVENT_DUMP];
	u32 data_cnt;
	int i, tmp_len = 0, ret = 0;
	char *dump_info;

	/* no data for this event */
	if (!event->data_cnt)
		return 0;

	memset(&data, 0, sizeof(data));
	if (event->data_cnt > HFI_CORE_EVENT_MAX_DATA) {
		HFI_CORE_ERR(
			"event[%d] has invalid data_cnt:%d greater than max_data_cnt: %d\n",
			index, event->data_cnt, HFI_CORE_EVENT_MAX_DATA);
		data_cnt = HFI_CORE_EVENT_MAX_DATA;
	} else {
		data_cnt = event->data_cnt;
	}

	for (i = 0; i < data_cnt; i++) {
		if (i == 0 && event->data[i]) {
			dump_info = get_dump_event_str(event->data[i]);
			if (dump_info) {
				tmp_len += scnprintf(data + tmp_len,
					(HFI_CORE_MAX_DATA_PER_EVENT_DUMP -
					tmp_len), "%s: ", dump_info);
				continue;
			}
		}
		tmp_len += scnprintf(data + tmp_len,
			HFI_CORE_MAX_DATA_PER_EVENT_DUMP - tmp_len,
			"0x%lx ", (unsigned long)event->data[i]);
	}

	ret = scnprintf(buf + len, max_size - len, HFI_CORE_EVT_MSG, index,
		event->time, (u64)event, event->data_cnt, data);

	HFI_CORE_DBG_H(
		HFI_CORE_EVT_MSG, index, event->time, (u64)event,
		event->data_cnt, data);

	return ret;
}

static ssize_t hfi_core_dbg_dump_events_rd(struct file *file,
	char __user *user_buf, size_t user_buf_size, loff_t *ppos)
{
	struct hfi_core_drv_data *drv_data;
	u32 entry_size = sizeof(struct hfi_core_trace_event), max_size = SZ_4K;
	char *buf = NULL;
	int len = 0;
	static u64 start_time;
	static int index, start_index;
	static bool wraparound;
	struct hfi_core_trace_event *event;

	if (!file || !file->private_data) {
		HFI_CORE_ERR("unexpected data 0x%llx\n", (u64)file);
		return -EINVAL;
	}
	drv_data = file->private_data;
	if (!drv_data) {
		HFI_CORE_ERR("drv data is null\n");
		return -EINVAL;
	}

	if (!drv_data->fw_trace_mem) {
		HFI_CORE_ERR("fw trace events not supported\n");
		return -EINVAL;
	}

	if (wraparound && index >= start_index) {
		HFI_CORE_DBG_H("no more data index: %d total_events: %d\n",
			index, HFI_CORE_MAX_TRACE_EVENTS);
		start_time = 0;
		index = 0;
		wraparound = false;
		return 0;
	}

	if (user_buf_size < entry_size) {
		HFI_CORE_ERR("not enough buff size: %zu to dump entries: %d\n",
			user_buf_size, entry_size);
		return -EINVAL;
	}

	buf = kzalloc(max_size, GFP_KERNEL);
	if (!buf)
		return -ENOMEM;

	event = (struct hfi_core_trace_event *)drv_data->fw_trace_mem->cpu_va;
	HFI_CORE_DBG_H("events:0x%pK start_index:%d", event, start_index);
	while ((!wraparound || index < start_index) &&
		len < (max_size - entry_size)) {
		len += _dump_event(&event[index], buf, len, max_size, index);
		//event++;
		index++;
		if (index >= HFI_CORE_MAX_TRACE_EVENTS) {
			index = 0;
			wraparound = true;
		}
	}
	HFI_CORE_DBG_H("-- dump_events: index:%d\n", index);

	if (len <= 0 || len > user_buf_size) {
		HFI_CORE_ERR("len: %d invalid buff size: %zu\n",
			len, user_buf_size);
		len = 0;
		goto exit;
	}

	if (copy_to_user(user_buf, buf, len)) {
		HFI_CORE_ERR("failed to copy to user!\n");
		len = -EFAULT;
		goto exit;
	}
	*ppos += len;
exit:
	kfree(buf);
	return len;
}

static const struct file_operations hfi_core_register_clients_fops = {
	.open = simple_open,
	.write = hfi_core_dbg_reg_client,
};

static const struct file_operations hfi_core_dbg_get_buf_fops = {
	.open = simple_open,
	.write = hfi_core_dbg_get_buf,
};

static const struct file_operations hfi_core_dbg_send_buf_fops = {
	.open = simple_open,
	.write = hfi_core_dbg_send_buf,
};

static const struct file_operations hfi_core_dbg_put_buf_fops = {
	.open = simple_open,
	.write = hfi_core_dbg_put_tx_buf,
};

static const struct file_operations hfi_core_unregister_clients_fops = {
	.open = simple_open,
	.write = hfi_core_dbg_unreg_client,
};

static const struct file_operations hfi_core_print_res_table_fops = {
	.open = simple_open,
	.write = hfi_core_dbg_print_res_tbl,
};

static const struct file_operations hfi_core_dbg_test_pkt_fops = {
	.open = simple_open,
	.write = hfi_core_dbg_test_packet,
};

static const struct file_operations hfi_core_dbg_dump_events_fops = {
	.open = simple_open,
	.read = hfi_core_dbg_dump_events_rd,
};

static const struct file_operations hfi_core_dbg_lb_cmd_fops = {
	.open = simple_open,
	.write = hfi_core_dbg_lb_cmd_buf_wr,
	.read = hfi_core_dbg_lb_cmd_buf_rd,
};

int hfi_core_dbg_debugfs_register(struct hfi_core_drv_data *drv_data)
{
	int ret = 0;
	struct dentry *debugfs_root;
	struct task_struct *thread;
	struct hfi_core_dbg_data *debugfs_data;

	HFI_CORE_DBG_H("+\n");

	if (!drv_data) {
		HFI_CORE_ERR("invalid params\n");
		return -EINVAL;
	}

	debugfs_data = kzalloc(sizeof(*debugfs_data), GFP_KERNEL);
	if (!debugfs_data)
		return -ENOMEM;

	debugfs_root = debugfs_create_dir("hfi_core", NULL);
	if (IS_ERR_OR_NULL(debugfs_root)) {
		HFI_CORE_ERR("debugfs_root create_dir fail, error %ld\n",
			PTR_ERR(debugfs_root));
		ret = PTR_ERR(debugfs_root);
		goto failed_create_dir;
	}

	drv_data->debug_info.data = (void *)debugfs_data;
	debugfs_data->drv_data = drv_data;
	mutex_init(&debugfs_data->clients_list_lock);
	INIT_LIST_HEAD(&debugfs_data->clients_list);
	INIT_LIST_HEAD(&debugfs_data->lb_mem_cache);

	debugfs_create_file("hfi_core_register_client", 0600, debugfs_root,
		drv_data, &hfi_core_register_clients_fops);
	debugfs_create_file("hfi_core_get_buf", 0600, debugfs_root,
		drv_data, &hfi_core_dbg_get_buf_fops);
	debugfs_create_file("hfi_core_trigger_ipc", 0600, debugfs_root,
		drv_data, &hfi_core_dbg_send_buf_fops);
	debugfs_create_file("hfi_core_put_buf", 0600, debugfs_root,
		drv_data, &hfi_core_dbg_put_buf_fops);
	debugfs_create_file("hfi_core_unregister_client", 0600, debugfs_root,
		drv_data, &hfi_core_unregister_clients_fops);
	debugfs_create_file("hfi_core_print_res_tbl", 0600, debugfs_root,
		drv_data, &hfi_core_print_res_table_fops);
	debugfs_create_file("hfi_core_dbg_test_pkt_send", 0600, debugfs_root,
		drv_data, &hfi_core_dbg_test_pkt_fops);
	debugfs_create_file("hfi_core_dump_events", 0600, debugfs_root,
		drv_data, &hfi_core_dbg_dump_events_fops);
	debugfs_create_bool("hfi_core_fail_client0_reg", 0600, debugfs_root,
		&msm_hfi_fail_client_0_reg);
	debugfs_create_u32("hfi_core_pkt_cmd_id", 0600, debugfs_root,
		&msm_hfi_packet_cmd_id);
	debugfs_create_file("hfi_core_lb_cmd_data", 0600, debugfs_root,
		drv_data, &hfi_core_dbg_lb_cmd_fops);
	debugfs_create_u32("hfi_core_debug_level", 0600, debugfs_root,
		&msm_hfi_core_debug_level);

	debugfs_data->root = debugfs_root;

	// NOTE: This wait-object has to be initialized before the thread runs
	init_waitqueue_head(&debugfs_data->wait_queue);
	thread = kthread_run(hfi_core_dbg_listener, (void *)drv_data,
		"hfi_core_dbg_client_listener");
	if (IS_ERR(thread)) {
		HFI_CORE_ERR("listener thread create failed\n");
		ret = PTR_ERR(thread);
		goto failed_thread;
	}
	debugfs_data->listener_thread = thread;

	HFI_CORE_DBG_H("-\n");
	return 0;

failed_thread:
	if (debugfs_root)
		debugfs_remove_recursive(debugfs_root);
failed_create_dir:
	kfree(debugfs_data);
	drv_data->debug_info.data = NULL;
	return ret;
}

void hfi_core_dbg_debugfs_unregister(struct hfi_core_drv_data *drv_data)
{
	struct hfi_core_dbg_data *debugfs_data;
	struct hfi_lb_mem_cache *lb_cache, *temp;

	HFI_CORE_DBG_H("+\n");

	if (!drv_data || !drv_data->debug_info.data) {
		HFI_CORE_ERR("invalid params\n");
		return;
	}
	debugfs_data = (struct hfi_core_dbg_data *)drv_data->debug_info.data;

	if (debugfs_data->listener_thread)
		kthread_stop(debugfs_data->listener_thread);

	if (!list_empty(&debugfs_data->lb_mem_cache)) {
		list_for_each_entry_safe(lb_cache, temp, &debugfs_data->lb_mem_cache, list) {
			kfree(lb_cache->payload);
			list_del(&lb_cache->list);
			kfree(lb_cache);
		}
	}

	if (debugfs_data->root)
		debugfs_remove_recursive(debugfs_data->root);
	kfree(debugfs_data);
	drv_data->debug_info.data = NULL;

	HFI_CORE_DBG_H("-\n");
}

#else

int hfi_core_dbg_debugfs_register(struct hfi_core_drv_data *drv_data)
{
	return 0;
}

void hfi_core_dbg_debugfs_unregister(struct hfi_core_drv_data *drv_data)
{
}

#endif /* CONFIG_DEBUG_FS */
