# Linux 6.12.117 import decisions

Base: 8ea149a53695e05a6b7ebe9ed720a1b33a365813 (tested .116 CAKE).
Upstream interval: 5c40a1750f44dd6c7292d8c8d0b4ffff1734576e..
d3417611a3b40b7f04a29e558cebac2bf69889d7; 1,535 ordered commits.

## 0017 — 289f954b51ef34e42a10002f3bbf1db6930d8e92

exec: cancel io_uring requests before de_thread()

Context conflict with downstream DMA-BUF exec accounting between cancellation
and unshare_files(). Move cancellation before de_thread(), as upstream requires;
retain the existing POSIX timer teardown and all DMA-BUF accounting unchanged.
No fix is omitted or reversed. Upstream author, subject, body and SHA trailer
are retained by git am.

## 0049 — 9d225297088ab24a65a849f917ff2c100a9d65b8

spi: nxp-fspi: disable runtime PM on probe failures

ACK has reverted the upstream devm-controller/devm-mutex refactor and already
routes all post-PM failures through err_disable_runtime, with explicit mutex
and controller reference cleanup. Retain that ownership model. Use
pm_runtime_resume_and_get() (without the now-incorrect failed-get put_noidle)
and clear the autosuspend policy at the existing common PM cleanup label.
The existing register-controller failure cleanup remains covered as well.

## 0067 — 18b4a1566fcb0ec3040d5796226dcd57afb6b898

drm/panel-edp: Support MB116AS01'3 edp panel

Context-only conflict: ACK already contains an additional CSW panel entry.
Insert the new HKC entry at its sorted position, retaining the existing table.

## 0102 — 42ba1a68b06317154f1c4dd096673803b050a60b

PCI/ATS: Ensure pci_ats_supported() is PF-aware for VFs

The Android tree names upstream's untrusted security field
requires_dma_protection. Preserve that field/contract and add the upstream
physical-function recursive support check. No PCI structure or field rename.

## 0367 — 617668ddfbf0dad026fd768703468f4a95896726

virtio_balloon: warn on failed buffer add in tell_host()

Retain the Android post_page_relinquish_tlb_inv() before queue submission;
add the upstream failed-add WARN/early return without bypassing TLB handling.

## 0447 — 7999c8819f9991ed5c7f2c1f16efc902e32e4abc — ALREADY PRESENT

netfilter: nf_conntrack_expect: add and use nf_ct_expect_related_pair()

Already implemented in qualified commit 633ac4e5f56b6. The complete normalized
pair function matches the upstream implementation after passing the pinned
master_help into the private check/insert helpers required by the existing
lifetime/GC backport. Both H323/SIP already publish the pair atomically, with
no sequential RTP/RTCP expect_related calls. Accounting is already moved out
of insert() and into single/pair paths. A scripted source comparison against
the pre-import .116 source passed. Keep the stronger existing lifetime fix
and external prototype; skip this exact equivalent commit rather than double
applying it or creating a misleading empty upstream-fix commit.

## 0465–0471 — seven module ELF refactors — ALREADY PRESENT

The original OnePlus import 234073f08cd43 already includes all seven helpers.
For each commit below, the complete whitespace-normalized introduced helper
is identical to the pre-import .116 implementation; the existing validation
pipeline already calls it. Skip only those exact redundant refactors:

- 0942824a099f964eca7bb82dcaa395d0b89bc557: elf_validity_ehdr
- f19f1a1c5fe56274e95b342e8138f4b774febf97: elf_validity_cache_sechdrs
- 930088ffcbb346330d1c52f1405f9604408c1f35: elf_validity_cache_secstrings
- a1648f82ec44fa1ab8beb0923ca7c2038f93a5f2: elf_validity_cache_index_info
- ea97ea606363c681312120e05eb443e854b74d14: elf_validity_cache_index_mod
- a19215105674a425861ae1edc547e7dcfcc6ff38: elf_validity_cache_index_sym
- 3eafa6b2e38ec468edda97fd6f78f3619af9d898: elf_validity_cache_index_str

This does NOT skip db07e9b0320e4 (string-table type validation), whose new
SHT_STRTAB checks are genuinely absent and must be imported.

## 0575 — 7bbef48f0b91095be567ea1dda5f9aead1d61a9d

driver core: fix parent lock use-after-free on USB disconnect

Apply the per-device-type lock decision, USB-interface flag, and removal of
the USB-bus-wide flag. Retain the old bus_type.need_parent_lock member and its
reserves as unused legacy storage rather than shrinking the shipping object.
No new code reads that legacy field in this imported fix. The new device_type
field still requires a separate, truthful runtime/KABI preservation repair
before this tree can be considered stock-module compatible or flashable.

## 0636 — ec397edc4c8ca868865953926831b539698469e6

genirq: Defer kfree(irqaction) until irq_thread_dtor() finishes

Only the include context conflicts with the existing Android dtask trace-hook
include. Retain that include and add linux/wait_bit.h. All destructor/work-bit
and free_irq synchronization hunks apply unchanged.

## 0663 — 31d5d26130bf4b53684e662e3c3add03662b69d3

media: rc: Use after free in ir_raw_event_handle()

Retain ACK's NULL-device guard and existing unregister/free location. Add the
upstream task pin before stop, corresponding put in guarded free, and timer
quiescence. The extra guard is the conflicting context; no reference drop is
added to the live event handling path.

## Required infrared lifetime prerequisite — dccc0c3ddf8f16071736f98a7d6dd46a2d43e037

Reintroduce the coherent unregister-versus-free dependency from ACK/stable
75977f5f5c4974c72bbedefbf09ad33ebbaa36f1, which ACK had reverted in
5091711a35ceb solely for ABI reasons. It separates unregister/quiesce/free and
updates all 38 other affected in-tree caller files, including CEC. Retain
rc_dev.managed_alloc at its shipping location (and its devm assignment) as
unused legacy ABI storage; do not shrink or shift rc_dev. There are zero
stock module import records for the rc_* device allocation/unregister/free
interfaces in the preserved production oracle; all modified callers rebuild
together. New binary/module oracle validation remains mandatory.

Remove the immediate raw free from unregister; release raw state only in the
final device release. This is required to avoid both callback UAF and the
recursive raw-handler mutex deadlock that a context-only BPF backport would
introduce. Commit this prerequisite separately before completing the pending
BPF fix. The three official .117 IR fixes retain their relative order.

## 0873 — 23f917d95b88a73d1498dd00456cec418dd355c4

mptcp: hold MP_JOIN msk ref when cloning reqsk

Include-only context conflict with trace/hooks/net.h. Keep the Android hook,
add net/mptcp.h, and retain every upstream ownership/RCU/WRITE_ONCE hunk.

## 1151 — e47bfc7e40758e275b77a678325f835b3296fe67

fsverity: hoist statx reporting of fs-verity flag

Keep the existing SUSFS getattr hooks following idmap selection. Perform the
upstream verity attribute/mask handling before those hooks; apply all filesystem
specific removals unchanged. No encryption, verity verification or policy change.

## 1190 — e1e121346bc897f00254b0d6fb9e75faa9af5484 — ALREADY PRESENT

ext4: fix ABBA deadlock in ext4_rmdir()

The complete upstream patch reverse-checks cleanly. The same unlock-before-
invalidate ordering and success-only d_invalidate() guard were already added
by 8ec621757f19e (FROMLIST backport). Preserve that existing fix, skip duplicate.

## 1266 — 4b5e0313560025761693f4880d65a90df833585e

f2fs: remove mappedtodisk flag usage in mmap write path

Retain Android's cleancache read path between the affected statements. Remove
the mappedtodisk setter and page_mkwrite shortcut exactly as upstream requires.
No inline-crypto, fscrypt or filesystem policy changes are made.

## 1289 — 21d11290049549ab8f0fe93124d2067627d1069d

PCI: Set supported_speeds in set_pcie_port_type()

Keep Android's pci_set_requires_dma_protection() name/security behavior. Move
supported-speed initialization into set_pcie_port_type() as upstream requires;
the renamed adjacent security helper is the only context conflict.

## 1301 — 95312e7299fa1bc472014c0dcacd965e1d7da411

irqchip/gic-v3-its: Don't clamp nvecs to zero when id_bits is 32

Apply the exact u64/BIT_ULL arithmetic correction while retaining the existing
adjacent ITT allocation comment and platform code. No interrupt policy changes.

## 1435 — 3ab27ef1675e42a03b68c83bb251872811a44845

mm/madvise: add cond_resched() in THP branch of madvise_cold_or_pageout_pte_range()

Preserve OPlus __reclaim_pages(..., private->private) and add the upstream
reschedule point after unlocking/reclaiming. No reclaim-policy or ownership change.

## 1444 — bed2155d85a86568b4b867546f8fa60175c154af — ALREADY PRESENT

fscrypt: Fix missing barrier in fscrypt_find_mode_key()

The complete upstream patch reverse-checks cleanly against the current source.
The same acquire barrier was already added by 79c418a771a87. Preserve the
existing fix and all hardware-wrapped-key setup; skip the duplicate only.
