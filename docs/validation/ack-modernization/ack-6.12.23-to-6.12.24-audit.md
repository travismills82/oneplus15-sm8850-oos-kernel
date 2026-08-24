# ACK 6.12.23 to 6.12.24 audit

## Scope and provenance

This experiment starts from canonical lineage
`39bfc1db04fe323c1a6c75ee089d8eb4817002d4`, whose common-kernel tree is
identical to the physically tested runtime source HEAD
`36f40a44d700422969dc7debb6519c0f9ab977d0`.

The OnePlus manifest identifies its Android Common Kernel import as:

- project: `kernel/common`
- revision: `b2a876903b495c444a94b16f50d1463ffe953957`
- tag: `android16-6.12-2025-06_r53`
- reported sublevel: 6.12.23

The point-release boundary was taken from the official Android Common Kernel
`android16-6.12` history:

- 6.12.23 release commit: `83b4161a63b87ce40d9f24f09b5b006f63d95b7c`
- 6.12.24 release commit: `b6efa8ce222e58cfe2bbaa4e3329818c2b4bd74e`
- ordered stable-series commits: 394
- files touched by the official delta: 383

No Linux 6.12.25 or later commit is included. The official series was replayed
in order into `kernel_platform/common`; it was not replaced with a stock
linux-stable tree and the Android, Qualcomm, and OnePlus layers were retained.
The per-commit disposition is recorded in
`ack-6.12.23-to-6.12.24-commits.tsv`.

## Disposition summary

| Disposition | Count |
|---|---:|
| APPLY | 374 |
| ALREADY PRESENT | 17 |
| ANDROID/QUALCOMM OVERRIDE | 1 |
| CONFLICT - MANUAL REVIEW | 2 |
| NOT APPLICABLE | 0 |

All architecture- or device-dormant stable changes remain in the source
point-release. Runtime applicability is handled by the Canoe configuration;
it is not a reason to create a partial Linux point release.

## Existing OnePlus overlap

Relative to the manifest ACK snapshot, the qualified common-kernel tree had
107 changed paths. Ten overlap the 6.12.24 stable delta:

```text
arch/arm64/kvm/arm.c
arch/arm64/mm/mmu.c
drivers/bluetooth/hci_qca.c
fs/erofs/fileio.c
fs/f2fs/f2fs.h
fs/f2fs/inode.c
fs/f2fs/node.c
fs/smb/client/smb2ops.c
net/bluetooth/hci_sync.c
net/sched/sch_sfq.c
```

Nine merged without a content conflict. `net/sched/sch_sfq.c` already
contained both 6.12.24 validation fixes, so the two upstream commits were
classified `ALREADY PRESENT` rather than replayed over the newer result.

## Reviewed integration cases

### Futex selftest Android behavior

Upstream commit `206d0df7b6a52daa6a9dcb326b243bbce389344c` fixes the
`futex_waitv` would-block pass/fail condition. The qualified OnePlus import
already contains that condition and additionally treats `ENOSYS` as a skipped
test. The Android behavior was retained and the commit was classified
`ANDROID/QUALCOMM OVERRIDE`; this affects selftest reporting, not the runtime
futex implementation.

### Already-present networking, perf, F2FS, media, KVM, PCI, and arm64 fixes

Seventeen stable changes were already represented by the qualified tree.
Notable examples include the two SFQ validation fixes, perf AUX pause/resume,
the simplified perf-event free path and sigtrap hang fix, multiple F2FS
corruption/bounds fixes, Venus HFI bounds checks, and the arm64 MOPS set fix.
They were content-checked and not duplicated.

### `struct sock` lockdep owner lifetime

Upstream commit `5f7f6abd92b6c8dc8f19625ef93c3a18549ede04` adds a
lockdep-only module owner reference to keep a socket lock class owner alive.
Its insertion point conflicted with Android KABI reserves in `struct sock`.
The resolution follows the official later ACK layout exactly:

1. retain the conditional `sk_owner` field;
2. retain all eight `ANDROID_KABI_RESERVE()` entries;
3. retain `ANDROID_OEM_DATA(1)`;
4. retain the upstream set, clear, and put lifetime operations.

The field exists only when both `CONFIG_PROVE_LOCKING` and `CONFIG_MODULES`
are enabled. The generated Canoe configuration and ABI/KMI reports remain the
authoritative acceptance gates for this resolution.

### `struct cgroup_subsys` stable callback

The first strict ABI comparison exposed a second Android integration issue
that did not create a textual merge conflict. Stable commit
`cdb6e724e7c5713d13c5ad3340e9d71c3dd8c9fb` adds the `css_killed` callback
directly after `css_reset`. On the frozen Canoe KMI that expanded
`struct cgroup_subsys` from 256 to 264 bytes and propagated CRC changes across
3,748 symbols.

The qualified 6.12.23 ACK layout already reserves one pointer-sized planned
backport slot at the end of this structure. A first `ANDROID_BACKPORT_USE()`
adaptation preserved size and symbol CRCs, but libabigail correctly reported
that the visible field changed from a reserved `u64` to a union-backed
callback. That form was rejected.

The accepted adaptation keeps `ANDROID_BACKPORT_RESERVE(1)` in the structure
exactly as frozen. A typed helper stores and retrieves the callback through
that pointer-sized slot, and cgroup core performs the call through the typed
function pointer. The struct's size, offsets, field name, and ABI description
therefore remain unchanged while the complete 6.12.24 cpuset teardown behavior
is retained. No loader check, CRC, or vermagic rule is disabled. The
post-resolution ABI and module-CRC comparisons are recorded in the static
validation report.

## Resulting source identity

The applied series and reviewed Android KMI adaptation naturally change the
kernel build-input identity to
`a7f2fd6d686f38d448e8a276efe1aea7c2b9013f`. The controlled workspace-status
contract therefore supplies suffix `-ga7f2fd6d686f`; the Makefile reports
`SUBLEVEL = 24`. No UTS release or module vermagic text is patched after the
build.

Static build, configuration, ABI/KMI, module-closure, filesystem, and AVB
results are recorded separately. This audit does not authorize a physical
flash.
