// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) 2023-2025 Qualcomm Innovation Center, Inc. All rights reserved.
 */


#include "rmnet_mem_priv.h"

MODULE_LICENSE("GPL v2");

DEFINE_SPINLOCK(rmnet_mem_lock);

unsigned int rmnet_mem_debug __read_mostly;
module_param(rmnet_mem_debug, uint, 0644);
MODULE_PARM_DESC(rmnet_mem_debug, "rmnet_mem debug status");

unsigned int rmnet_mem_pb_enable __read_mostly = 1;
module_param(rmnet_mem_pb_enable, uint, 0644);
MODULE_PARM_DESC(rmnet_mem_pb_enable, "rmnet_mem_pb_enable pb ind pool boosts");

int max_pool_size[POOL_LEN] = { 0, 0, MAX_POOL_O2, MAX_POOL_O3};
module_param_array(max_pool_size, int, NULL, 0644);
MODULE_PARM_DESC(max_pool_size, "Max Pool size per order");

int cache_pool_size[POOL_LEN];
module_param_array(cache_pool_size, int, NULL, 0444);
MODULE_PARM_DESC(cache_pool_size, "Pool size per order");

int static_pool_size[POOL_LEN];
module_param_array(static_pool_size, int, NULL, 0444);
MODULE_PARM_DESC(static_pool_size, "Pool size per order");

int target_pool_size[POOL_LEN] = { 0, 0, MID_POOL_O2, MID_POOL_O3};
module_param_array(target_pool_size, int, NULL, 0644);
MODULE_PARM_DESC(target_pool_size, "Pool size wq will adjust to on run");

unsigned int pool_unbound_feature[POOL_LEN] = { 0, 0, 1, 1};
module_param_array(pool_unbound_feature, uint, NULL, 0644);
MODULE_PARM_DESC(pool_unbound_featue, "Pool bound gate");

unsigned long long rmnet_mem_order_requests[POOL_LEN];
module_param_array(rmnet_mem_order_requests, ullong, NULL, 0644);
MODULE_PARM_DESC(rmnet_mem_order_requests, "Request per order");

unsigned long long rmnet_mem_id_req[POOL_LEN];
module_param_array(rmnet_mem_id_req, ullong, NULL, 0644);
MODULE_PARM_DESC(rmnet_mem_id_req, "Request per id");

unsigned long long rmnet_mem_id_recycled[POOL_LEN];
module_param_array(rmnet_mem_id_recycled, ullong, NULL, 0644);
MODULE_PARM_DESC(rmnet_mem_id_recycled, "Recycled per id");

unsigned long long rmnet_mem_order_recycled[POOL_LEN];
module_param_array(rmnet_mem_order_recycled, ullong, NULL, 0644);
MODULE_PARM_DESC(rmnet_mem_order_recycled, "Recycled per order");

unsigned long long rmnet_mem_id_gaveup[POOL_LEN];
module_param_array(rmnet_mem_id_gaveup, ullong, NULL, 0444);
MODULE_PARM_DESC(rmnet_mem_id_gaveup, "gaveup per id");

unsigned long long rmnet_mem_order_gaveup[POOL_LEN];
module_param_array(rmnet_mem_order_gaveup, ullong, NULL, 0444);
MODULE_PARM_DESC(rmnet_mem_order_gaveup, "gaveup per order");

unsigned long long rmnet_mem_stats[RMNET_MEM_STAT_MAX];
module_param_array(rmnet_mem_stats, ullong, NULL, 0644);
MODULE_PARM_DESC(rmnet_mem_stats, "Rmnet mem stats for modules");

unsigned long long rmnet_mem_err[ERR_MAX];
module_param_array(rmnet_mem_err, ullong, NULL, 0444);
MODULE_PARM_DESC(rmnet_mem_err, "Error counting");

unsigned long long rmnet_mem_pb_ind_max[POOL_LEN];
module_param_array(rmnet_mem_pb_ind_max, ullong, NULL, 0644);
MODULE_PARM_DESC(rmnet_mem_pb_ind_max, "Pool size vote that is active on PB ind");

unsigned long long rmnet_mem_cache_adds[POOL_LEN];
module_param_array(rmnet_mem_cache_adds, ullong, NULL, 0644);
MODULE_PARM_DESC(rmnet_mem_cache_adds, "add per cache pool");

unsigned long long rmnet_mem_cache_add_fails[POOL_LEN];
module_param_array(rmnet_mem_cache_add_fails, ullong, NULL, 0644);
MODULE_PARM_DESC(rmnet_mem_cache_add_fails, "fail to add per cache pool");

static char *verinfo[] = {"2003bae3"};
module_param_array(verinfo, charp, NULL, 0444);
MODULE_PARM_DESC(verinfo, "Version of the driver");

struct workqueue_struct *mem_wq;
struct delayed_work pool_adjust_work;
struct delayed_work pool_replenish_work;
int pb_ind_pending;
struct  hrtimer pb_timer;
struct list_head rmnet_mem_pool[POOL_LEN];
struct list_head rmnet_mem_cache[POOL_LEN];
uint32_t ipa_config = 0;

uint32_t rmnet_mem_config_query(unsigned int id)
{
	return ipa_config;
}
EXPORT_SYMBOL_GPL(rmnet_mem_config_query);

void rmnet_mem_info_ref_inc_entry(struct page *page, unsigned int id)
{
	page_ref_inc(page);
}
EXPORT_SYMBOL_GPL(rmnet_mem_info_ref_inc_entry);

struct rmnet_mem_notif_s {
	struct raw_notifier_head chain;
	spinlock_t lock;
};

struct rmnet_mem_notif_s rmnet_mem_notifier = {
	.chain = RAW_NOTIFIER_INIT(rmnet_mem_notifier.chain),
	.lock  = __SPIN_LOCK_UNLOCKED(rmnet_mem_notifier.lock),
};
EXPORT_SYMBOL_GPL(rmnet_mem_notifier);

int rmnet_mem_get_pool_size(unsigned int order)
{
	if (order >= POOL_LEN) {
		rmnet_mem_err[ERR_GET_ORDER_ERR]++;
		return 0;
	}
	/* Return actual size or configured amount if not grown yet.*/
	return (static_pool_size[order]) ? static_pool_size[order] : target_pool_size[order];
}
EXPORT_SYMBOL_GPL(rmnet_mem_get_pool_size);

int rmnet_mem_mode_notify(unsigned int pool_size)
{
	unsigned long flags;

	spin_lock_irqsave(&rmnet_mem_notifier.lock, flags);
	raw_notifier_call_chain(&rmnet_mem_notifier.chain, pool_size, NULL);
	spin_unlock_irqrestore(&rmnet_mem_notifier.lock, flags);
	return NOTIFY_OK;
}

int rmnet_mem_register_notifier(struct notifier_block *nb)
{
	unsigned long flags;
	int ret;

	spin_lock_irqsave(&rmnet_mem_notifier.lock, flags);
	ret = raw_notifier_chain_register(&rmnet_mem_notifier.chain, nb);
	spin_unlock_irqrestore(&rmnet_mem_notifier.lock, flags);
	return ret;
}
EXPORT_SYMBOL_GPL(rmnet_mem_register_notifier);

int rmnet_mem_unregister_notifier(struct notifier_block *nb)
{
	unsigned long flags;
	int ret;

	spin_lock_irqsave(&rmnet_mem_notifier.lock, flags);
	ret = raw_notifier_chain_unregister(&rmnet_mem_notifier.chain, nb);
	spin_unlock_irqrestore(&rmnet_mem_notifier.lock, flags);
	return ret;
}
EXPORT_SYMBOL_GPL(rmnet_mem_unregister_notifier);

/* Malloc by client so rem from to pool */
mem_info_s *rmnet_mem_add_page(struct page *page, u8 pageorder)
{
	mem_info_s *mem_info;

	mem_info = kzalloc(sizeof(*mem_info), GFP_ATOMIC);
	if (!mem_info) {
		rmnet_mem_err[ERR_MALLOC_FAIL1]++;
		return NULL;
	}

	static_pool_size[pageorder]++;

	mem_info->order = pageorder;
	mem_info->addr = (void *)page;
	INIT_LIST_HEAD(&mem_info->mem_head);
	INIT_LIST_HEAD(&mem_info->cache_head);

	if (pageorder < POOL_LEN)
		list_add(&mem_info->mem_head, &(rmnet_mem_pool[pageorder]));

	return mem_info;
}

void rmnet_mem_check_all(void)
{
	mem_info_s *mem_info;
	struct list_head *ptr = NULL, *next = NULL;
	int i, j;
	int free_stats[POOL_LEN] = {0, 0, 0, 0};
	int cache_stats[POOL_LEN] = {0, 0, 0, 0};

	for (i = 0, j = 0; i < POOL_LEN; i++) {
		list_for_each_safe(ptr, next, &rmnet_mem_pool[i]) {
			mem_info = list_entry(ptr, mem_info_s, mem_head);
			/* move free pages to end of stack and to free cache */
			if (page_ref_count(mem_info->addr) == 1)
				free_stats[i]++;

			if (!list_empty(&mem_info->cache_head))
				cache_stats[i]++;

			if (!list_empty(&mem_info->cache_head) &&
			    page_ref_count(mem_info->addr) != 1)	{
				pr_info("Invalid cache entry %p refcount %d",
					    mem_info->addr, page_ref_count(mem_info->addr));
			}
			j++;
		}
		/* Scale to have Percent of free*/
		if (j != static_pool_size[i]) {
			pr_info("Invalid static pool size %d i %d", j, static_pool_size[i]);
			BUG_ON(1);
		}
		j = 0;

	}
	pr_info("free stat order 2: %d  order 3: %d", free_stats[2], free_stats[3]);
	pr_info("cache status count order 2: %d  order 3: %d", cache_stats[2], cache_stats[3]);
	pr_info("static count order 2: %d  order 3: %d", static_pool_size[2], static_pool_size[3]);
}

void rmnet_mem_replenish_all(void)
{
	unsigned long flags;
	int i;

	spin_lock_irqsave(&rmnet_mem_lock, flags);
	for (i = 0; i < POOL_LEN; i++)
		rmnet_mem_cache_add(i, true);

	spin_unlock_irqrestore(&rmnet_mem_lock, flags);
}

/* Freed by client so added back to pool */
void rmnet_mem_free_all(void)
{
	unsigned long flags;
	mem_info_s *mem_info;
	struct list_head *ptr = NULL, *next = NULL;
	int i;

	spin_lock_irqsave(&rmnet_mem_lock, flags);
	for (i = 0; i < POOL_LEN; i++) {
		list_for_each_safe(ptr, next, &rmnet_mem_pool[i]) {
			mem_info = list_entry(ptr, mem_info_s, mem_head);

			list_del_init(&mem_info->mem_head);
			/* Remove from cache if present */
			if (!list_empty(&mem_info->cache_head)) {
				list_del_init(&mem_info->cache_head);
				cache_pool_size[mem_info->order]--;

			}
			put_page(mem_info->addr);
			static_pool_size[mem_info->order]--;
			kfree(mem_info);
		}
	}
	spin_unlock_irqrestore(&rmnet_mem_lock, flags);
}

void rmnet_mem_cache_add(unsigned int order, bool force)
{
	struct list_head *ptr = NULL, *next = NULL;
	mem_info_s *mem_info;
	int i = 0;
	int cacheadd = 0;

	if (static_pool_size[order] && (force || !cache_pool_size[order])) {
		list_for_each_safe(ptr, next, &rmnet_mem_pool[order]) {
			mem_info = list_entry(ptr, mem_info_s, mem_head);
			/* If node is not already in cache and free then add to cache list */
			if (list_empty(&mem_info->cache_head) &&
			    page_ref_count(mem_info->addr) == 1) {
				/* Add page to free list and move to end of pool stack */
				list_del_init(&mem_info->mem_head);
				list_add_tail(&mem_info->mem_head,
								&(rmnet_mem_pool[order]));
				cache_pool_size[order]++;
				cacheadd++;
				list_add(&mem_info->cache_head, &(rmnet_mem_cache[order]));
				rmnet_mem_cache_adds[order]++;
			}
			/* Stop if gone through half of pool or cache has grown past half */
			if (i++ > (static_pool_size[order] >> 3) ||
			    (cache_pool_size[order] > (static_pool_size[order] >> 3))) {
				break;
			}
		}
		/* If nothing found to add, move head of list */
		if (!cacheadd) {
			rmnet_mem_cache_add_fails[order]++;
			if (!list_is_first(&mem_info->mem_head, &(rmnet_mem_pool[order])))
				list_rotate_to_front(&mem_info->mem_head, &(rmnet_mem_pool[order]));
		}

	}
}

/* Freed by client so added back to pool */
struct page *rmnet_mem_get_pages_entry(gfp_t gfp_mask, unsigned int order, int *code,
				       int *pageorder, unsigned int id)
{
	unsigned long flags;
	mem_info_s *mem_info;
	struct page *page = NULL;
	int i = 0;
	int j = 0;
	int adding = 0;
	int default_mask =  GFP_ATOMIC | __GFP_NOMEMALLOC | __GFP_NOWARN;

	/* Failure condition never should be reached */
	if (order > POOL_LEN)
		goto end;

	spin_lock_irqsave(&rmnet_mem_lock, flags);

	rmnet_mem_id_req[id]++;
	rmnet_mem_order_requests[order]++;
	rmnet_mem_cache_add(order, false);

	mem_info = list_first_entry_or_null(&rmnet_mem_cache[order], mem_info_s, cache_head);

	if (mem_info) {
		rmnet_mem_id_recycled[id]++;
		rmnet_mem_order_recycled[order]++;
		cache_pool_size[order]--;
		page = mem_info->addr;
		/* Remove page from cache and move to end of pool stack */
		list_del_init(&mem_info->cache_head);
		list_del_init(&mem_info->mem_head);
		list_add_tail(&mem_info->mem_head, &(rmnet_mem_pool[order]));

		if (page_ref_count(mem_info->addr) != 1)
			BUG_ON(1);

		page_ref_inc(mem_info->addr);
	}
	/* Check high order for rmnet and lower order for IPA if matching order fails */
	for (j = order, i = 0; !page && j > 0 && j < POOL_LEN; j++) {
		do {
			mem_info = list_first_entry_or_null(&rmnet_mem_pool[j], mem_info_s, mem_head);
			if (!mem_info)
				break;

			if (page_ref_count(mem_info->addr) == 1) {
				rmnet_mem_id_recycled[id]++;
				rmnet_mem_order_recycled[j]++;
				page = mem_info->addr;
				page_ref_inc(mem_info->addr);
				list_rotate_left(&rmnet_mem_pool[j]);
				/* Could have gone up an order and skip cached check above,
				 * remove from cache if so. To avoid taken page in cache
				 */
				if (!list_empty(&mem_info->cache_head))
					list_del_init(&mem_info->cache_head);
				break;
			}
			list_rotate_left(&rmnet_mem_pool[j]);
			i++;
		} while (i <= 30);
		if (page && pageorder) {
			*pageorder = j;
			break;
		}
		i = 0;
	}

	if (static_pool_size[order] < max_pool_size[order] &&
	    pool_unbound_feature[order]) {
		adding = 1;
	}  else
		spin_unlock_irqrestore(&rmnet_mem_lock, flags);

	if (!page) {
		rmnet_mem_id_gaveup[id]++;
		rmnet_mem_order_gaveup[order]++;

		/* IPA doesn't want retry logic but pool will be empty for lower orders and those
		 * will fail too so that is akin to retry. So just hardcode to not retry for o3 page req
		 */
		if (order < 3) {
			page = __dev_alloc_pages((adding) ? default_mask : gfp_mask, order);

			if (page) {
				/* If below unbound limit then add page to static pool*/
				if (adding) {
					rmnet_mem_add_page(page, order);
					/* Incrementing since about to give out the page to client
					 * not because we are adding to bookkeeping.
					 */
					page_ref_inc(page);
				}

				if (pageorder)
					*pageorder = order;

			} else {
				rmnet_mem_stats[RMNET_MEM_ALLOC_FAILS]++;
			}

		} else {
			/* Only call get page if we will add page to static pool*/
			if (adding) {
				if (rmnet_mem_debug)
					rmnet_mem_check_all();

				page = __dev_alloc_pages((adding) ? default_mask : gfp_mask, order);
				if (page) {
					rmnet_mem_add_page(page, order);
					page_ref_inc(page);
				} else {
					rmnet_mem_stats[RMNET_MEM_ALLOC_FAILS]++;
				}

				if (pageorder)
					*pageorder = order;
			}
		}
	}
	/* If we had potential to add, this won't occur after we fill up to limit */
	if (adding)
		spin_unlock_irqrestore(&rmnet_mem_lock, flags);

end:
	if (pageorder && code && page) {
		if (*pageorder == order)
			*code = RMNET_MEM_SUCCESS;
		else if (*pageorder > order)
			*code = RMNET_MEM_UPGRADE;
		else if (*pageorder < order)
			*code = RMNET_MEM_DOWNGRADE;
	} else if (pageorder && code) {
		*code = RMNET_MEM_FAIL;
		*pageorder = 0;
	}

	return page;
}
EXPORT_SYMBOL_GPL(rmnet_mem_get_pages_entry);

/* Freed by client so added back to pool */
void rmnet_mem_put_page_entry(struct page *page)
{
	put_page(page);
}
EXPORT_SYMBOL_GPL(rmnet_mem_put_page_entry);

static void mem_replenish_work(struct work_struct *work)
{
	local_bh_disable();
	rmnet_mem_replenish_all();
	local_bh_enable();
}

static void mem_update_pool_work(struct work_struct *work)
{
	int i;
	int new_size;

	local_bh_disable();
	for (i = 0; i < POOL_LEN; i++) {
	/* If PB ind is active and max pool has been configured
	 * new pool size is max of the two.
	 */
		new_size = (pb_ind_pending && rmnet_mem_pb_ind_max[i]) ?
			    MAX_VOTE(rmnet_mem_pb_ind_max[i], target_pool_size[i]) :
			    target_pool_size[i];

		rmnet_mem_adjust(new_size, i);
	}
	local_bh_enable();

}

void rmnet_mem_cb(unsigned long event, void* data)
{
	switch (event) {
		case POWER_SAVE_NOTIF:
			queue_delayed_work(mem_wq, &pool_replenish_work, 0);
			break;
		case BUFF_ABOVE_HIGH_THRESHOLD_FOR_LL_PIPE:
			break;
		case BUFF_BELOW_LOW_THRESHOLD_FOR_LL_PIPE:
			rmnet_mem_stats[RMNET_MEM_LL_LOW]++;
				break;
		case FREE_PAGE_TASK_SCHEDULED:
			rmnet_mem_stats[RMNET_MEM_FREE_PAGE_SCHED]++;
			break;
		case FREE_PAGE_TASK_SCHEDULED_LL:
			rmnet_mem_stats[RMNET_MEM_FREE_PAGE_SCHED]++;
				break;
		case BUFF_ABOVE_HIGH_THRESHOLD_FOR_DEFAULT_PIPE:
			break;
		case BUFF_ABOVE_HIGH_THRESHOLD_FOR_COAL_PIPE:
			break;
		case BUFF_BELOW_LOW_THRESHOLD_FOR_DEFAULT_PIPE:
		case BUFF_BELOW_LOW_THRESHOLD_FOR_COAL_PIPE:
			rmnet_mem_stats[RMNET_MEM_LOW_MEM_NOTIF]++;
			queue_delayed_work(mem_wq, &pool_replenish_work, 0);
			break;
		default:
			break;
		}
}
EXPORT_SYMBOL_GPL(rmnet_mem_cb);

void rmnet_mem_adjust(unsigned int perm_size, u8 pageorder)
{
	struct list_head *entry, *next;
	mem_info_s *mem_info;
	int i = 0;
	struct page  *newpage = NULL;
	int adjustment = 0;
	unsigned long flags;
	int default_mask =  GFP_ATOMIC | __GFP_NOMEMALLOC | __GFP_NOWARN;

	if (pageorder >= POOL_LEN || perm_size > MAX_STATIC_POOL) {
		rmnet_mem_err[ERR_INV_ARGS]++;
		return;
	}

	spin_lock_irqsave(&rmnet_mem_lock, flags);
	adjustment = perm_size - static_pool_size[pageorder];
	if (perm_size == static_pool_size[pageorder]) {
		spin_unlock_irqrestore(&rmnet_mem_lock, flags);
		return;
	}

	rmnet_mem_cache_add(pageorder, false);
	if (perm_size > static_pool_size[pageorder]) {
		for (i = 0; i < (adjustment); i++) {
			newpage = __dev_alloc_pages(default_mask, pageorder);

			if (!newpage) {
				rmnet_mem_stats[RMNET_MEM_ALLOC_FAILS]++;
				continue;
			}

			mem_info = rmnet_mem_add_page(newpage, pageorder);

		}
	} else {
		/* Shrink static pool from cache first so mem actually goes down as
		 * opposed to staying in ipa's temp pool list.
		 */
		list_for_each_safe(entry, next, &(rmnet_mem_cache[pageorder])) {
			mem_info = list_entry(entry, mem_info_s, cache_head);
			if (static_pool_size[pageorder] == perm_size)
				break;
			/* Freeing temp pool memory Remove from ht and kfree*/
			/* Remove from cache if present */
			if (!list_empty(&mem_info->cache_head)) {
				list_del_init(&mem_info->cache_head);
				cache_pool_size[pageorder]--;
			}
			list_del_init(&mem_info->mem_head);
			put_page(mem_info->addr);
			kfree(mem_info);
			static_pool_size[pageorder]--;
		}
		list_for_each_safe(entry, next, &(rmnet_mem_pool[pageorder])) {
			mem_info = list_entry(entry, mem_info_s, mem_head);
			if (static_pool_size[pageorder] == perm_size)
				break;
			/* Freeing temp pool memory Remove from ht and kfree*/
			/* Remove from cache if present */
			if (!list_empty(&mem_info->cache_head)) {
				list_del_init(&mem_info->cache_head);
				cache_pool_size[pageorder]--;
			}
			list_del_init(&mem_info->mem_head);
			put_page(mem_info->addr);
			kfree(mem_info);
			static_pool_size[pageorder]--;
		}
	}
	spin_unlock_irqrestore(&rmnet_mem_lock, flags);
	if (pageorder == POOL_NOTIF)
		rmnet_mem_mode_notify(perm_size);

}

enum hrtimer_restart rmnet_mem_pb_timer_cb(struct hrtimer *t)
{
	unsigned int jiffies;

	pb_ind_pending = 0;
	jiffies = msecs_to_jiffies(RAMP_DOWN_DELAY);
	/* Ramping down can be done with a delay. Less urgent.*/
	queue_delayed_work(mem_wq, &pool_adjust_work, jiffies);

	return HRTIMER_NORESTART;
}

void rmnet_mem_pb_ind(void)
{
	/* Only listen to pb idn vote if configured*/
	if (!rmnet_mem_pb_enable && !rmnet_mem_pb_ind_max[POOL_NOTIF]) {
		return;
	}

	pb_ind_pending = 1;
	/* Trigger update to change pool size */
	if (hrtimer_active(&pb_timer)) {
		hrtimer_cancel(&pb_timer);
	} else {
		cancel_delayed_work(&pool_adjust_work);
		queue_delayed_work(mem_wq, &pool_adjust_work, 0);
	}
	rmnet_mem_stats[RMNET_MEM_PB_IND]++;
	hrtimer_start(&pb_timer, ns_to_ktime(PB_IND_DUR * NS_IN_MS),
					     HRTIMER_MODE_REL | HRTIMER_MODE_PINNED);
}
EXPORT_SYMBOL_GPL(rmnet_mem_pb_ind);

int __init rmnet_mem_module_init(void)
{
	int rc, i = 0;

	pr_info("%s(): Starting rmnet mem module\n", __func__);
	for (i = 0; i < POOL_LEN; i++) {
		INIT_LIST_HEAD(&(rmnet_mem_pool[i]));
		INIT_LIST_HEAD(&(rmnet_mem_cache[i]));
	}

	mem_wq = alloc_workqueue("mem_wq", WQ_HIGHPRI, 0);
	if (!mem_wq)
		return -ENOMEM;


	INIT_DELAYED_WORK(&pool_adjust_work, mem_update_pool_work);
	INIT_DELAYED_WORK(&pool_replenish_work, mem_replenish_work);

	rc = rmnet_mem_nl_register();
	if (rc) {
		pr_err("%s(): Failed to register generic netlink family\n", __func__);
		destroy_workqueue(mem_wq);
		mem_wq = NULL;
		return -ENOMEM;
	}
	hrtimer_init(&pb_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	pb_timer.function = rmnet_mem_pb_timer_cb;

	return 0;
}

void __exit rmnet_mem_module_exit(void)
{
	rmnet_mem_nl_unregister();
	if (mem_wq) {
		cancel_delayed_work_sync(&pool_adjust_work);
		drain_workqueue(mem_wq);
		destroy_workqueue(mem_wq);
		mem_wq = NULL;
	}
	rmnet_mem_free_all();
}
module_init(rmnet_mem_module_init);
module_exit(rmnet_mem_module_exit);
