# ColorOS 17.0.0.102 / Linux 6.12.115: flag separation and new GEAS evidence

## Scope and preservation

Worktree: `out/coloros17-linux115-task-flags`.
Branch: `experiment/coloros17-linux115-task-flag-separation`.
Starting commit: `3d2f9755f4fd76c1a5c2167a7cff3a7361728598`.
Flag-repair commit: `651a7d4d1b2a08c529ebaf715bdcd20bd77b9ad8`.
Repaired common tree: `c74ab6f64f685145302559ce9fa2a2b00dc4c385`.
The five-import repair `eff946871d41042c51a7ecdcf04e7cb5b11f7fce`, all core
KABI repairs and the 231 individual stable imports remain ancestors.

No device commands, installed-image changes, reboots, slot changes, key
enrollment, pushes, merges or publication are performed. The known-working
slot B .114 canary and the existing quarantined GPU artifacts are untouched.
The earlier GEAS diagnostic declaration is not copied into this worktree or
promoted into a repair. No CRC or ELF version record is edited.

Evidence directory:
`/home/travis/Android/kernel-test-backups/coloros17-612115-geas-new-evidence-20261101`.

## Independent task-flag repair

### Root cause

Upstream `22b7f491be8f93ff72de0a3fe02b8ee9e64d3f21`, stable
`4a52a339587ed193e1d956c9d609283ae3f7a377`, local `c77c6cf021c24`, allocates
`task_struct::flags` bit `0x00800000` to suppress synthetic
`TIF_NOTIFY_SIGNAL` interruptions in a guarded section. The later PID
namespace reaper fix uses that guard; its behavior must remain intact.

The retained OPlus/QCOM modules already use the same numeric bit as
`PF_MEMALLOC_BOOST`. The stock `qcom_dma_heaps` allocation implementation
sets and clears it at task offset `0x44` around `alloc_pages`. As a result,
the old .115 core can mistake allocation boosting for notification suppression,
and the two save/restore lifetimes can overwrite each other.

### Mechanism

Restore the existing `PF__HOLE__00800000` source alias for shipping OPlus
modules and leave its numeric value/runtime meaning unchanged. Allocate
`PFA_NO_NOTIFY_SIGNAL = 8` in the **separate existing** `atomic_flags` word.
The audited core PFA registry previously occupies bits 0–7. No driver-side
PFA or task atomic-flags use is found in the available QCOM/OPlus sources.

`no_notify_signal_save()` atomically sets bit 8 and returns its previous
boolean state. `no_notify_signal_restore()` restores only that bit. The
return is an opaque save/restore token; all actual callers use it that way.
The sole actual guard caller, `zap_pid_ns_processes()`, remains unchanged.
Both the current common and Canoe perf configurations have `CONFIG_PID_NS=n`;
the reaper function is not in the production binary. That does not eliminate
the collision: `signal_pending()` itself is compiled throughout the core and
previously misinterpreted the OEM boost bit even without a core guard user.
The config is not changed to enable namespaces for testing.

`signal_pending()` tests the new atomic flag only when `TIF_NOTIFY_SIGNAL`
is set. Ordinary pending signals still reach `task_sigpending()` regardless
of the guard. This does not disable real signals or task-work notification;
it preserves the stable fix's scoped suppression and nested restore behavior.

No task member, size, offset, alignment, callback or exported prototype is
changed. No new allocation, side table, reference lifetime or lock is added.
The existing atomic flag machinery preserves simultaneous updates to other
PFA bits. The save operation is fully ordered; set/clear restore operations
have the existing bitops semantics and do not require a new external ordering
contract. The source does not allocate an unreviewed bit in `task->flags`.

The direct ARM64 constant-offset audit covers 1,070 module records / 704
distinct binaries. It saves 199 candidate functions accessing offset 1736;
many are unrelated structures at the same offset. These are candidate
disassemblies, **not a claim of proving all arbitrary computed-pointer
accesses**. The registry/source audit, unchanged layout, genuine compiled
module compatibility and behavioral tests are separate evidence.

### Behavioral tests

Seven new extracted-real-helper tests cover:

- nested guards and both pre-existing saved states;
- OPlus allocation boost independent of notification suppression;
- real pending signals remaining observable;
- preservation of other task and atomic flags;
- uniqueness of the allocated core PFA bit and retention of the PIDNS guard;
- a negative control reproducing the old bit collision;
- concurrent unrelated atomic-flag updates with no lost update.

All 182 host tests pass, including the previous 175. The C controls use
ASan/UBSan; they are not physical ARM64 or weak-memory qualification.

### Build/ABI validation

| Gate | Actual result |
|---|---|
| Normal common kernel plus ABI generation | PASS, build 04, 1,453 seconds |
| Explicit `//common:kernel_aarch64_abi_kmi_symbol_checks` | PASS |
| Effective common config vs prior repaired .115 | Byte-identical |
| `Module.symvers` and `vmlinux.symvers` vs prior repaired .115 | Byte-identical, all 10,163 vmlinux exports preserved |
| Filtered and full generated STG vs prior repaired .115 | Byte-identical |
| Complete `task_struct` pahole output | Identical; size 5,184, `flags` offset 68, `atomic_flags` offset 1,736 |
| Shared core function KCFI prefixes | 58,265 match, no changes/additions/removals |
| Host behavioral/source tests | 182 PASS |
| Real ARM64 helper/cleanup-guard compile probes | PASS under unchanged final common config |
| Existing-provider 1,070-module oracle | PASS: 1,032 active compatible, 38 dormant |
| Hard imports / CRC records | 60,701 / 60,701; 61,777 / 61,777 |
| Missing / CRC / namespace / active blockers | 0 / 0 / 0 / 0 |
| Independent vmlinux-first provider validation | PASS, 54,785 edges, 0 production-provider mismatches |
| Enforced common ABI diff | FAIL/exit4, unchanged prior 38 plus six documented representation differences |
| Small/short/flat/visual enforced reports vs prior repaired .115 | All byte-identical |
| Full new Canoe distribution/device ABI, unified GPU package, physical tests | NOT RUN; GEAS gate remains blocked |

The native probes compile the actual `no_notify_signal_save/restore`,
`signal_pending()` and cleanup guard using the normal recorded `signal.o`
command, the same project Clang and the same generated headers/config. Their
LSE and LL/SC instruction paths use offset `0x6c8`, mask `0x100`; none accesses
the OEM flag word at `0x44` or mask `0x800000`. These are compile-only probes,
not exported or linked into the kernel and not runtime execution of PIDNS.

The existing eight DWARF duplicate-type warning roots are unchanged. There
are no new compiler warning lines; repository-manifest/deprecated-build-rule
warnings remain disclosed. The fresh worktree needed the prepared workspace's
manifest-created links and identical generated Canoe/perf selector before
analysis could succeed; no configuration or validation was disabled to fix it.

One read-only analysis attempt was stopped because the generic ELF reader
copies an entire string-table suffix for each name in a large vmlinux. The
local KCFI analysis uses an equivalent linear string decoder, checked against
the original on empty, terminated, unterminated, non-ASCII and out-of-range
inputs. Frozen kernel files are reverified before resume. **The module oracle
and its acceptance rules are not changed.** The passing second analysis and
the original stopped log are both preserved.

This common build was produced at parent `3d2f9755f4fd...` with the exact
two-header patch subsequently committed as `651a7d4d1b2a0`; the recorded patch
hash is verified against that commit. Its truthful compiled release is
`6.12.115-android16-6-o-g3d2f9755f4fd-4k`, not a claimed rebuild at the later
commit. No kernel rebuild or repackage is performed for commit cleanliness.

| Frozen common-build artifact | SHA256 |
|---|---|
| `kernel/Image`, 42,646,016 bytes | `2bd4a9359d38edce480ace698067cb9d305e5079304fad3710257e073b1008a6` |
| `kernel/Module.symvers` | `6a6413d41ce692c0476a0ccd81e38a446fe1490400f3924c79424c597a390da1` |
| `kernel/kernel_config` | `abbb37e8fae9073bd5ca5bf539196f4b9baff60fa40714fe1506dc01592f1a27` |
| `kernel/abi.stg` | `0ce8ff105a7b5a0375b31bc182cf56624ab60d59a4ff824eda89a4c2400b19ed` |

Paths in this table are relative to the evidence directory. This is an
unpackaged common kernel, not a qualified ColorOS boot image or unified GPU
candidate. The existing-provider oracle uses the preserved running-canary
population, including its stock GPU/GEAS provider boundary; it is **not** a
substitution for the failing updated-GPU replacement model.

## New GEAS sources only

Previously searched branch heads and blob hashes are loaded from the earlier
inventories. Unchanged source is skipped, not downloaded/searched again.
No member-name guessing, alternative ABI reconstruction, compiler substitution
or CRC-generation experiment is repeated.

The exact official SM8850 check finds the same seven OnePlus/Oppo heads and
no published release or tag containing a new matching ColorOS 17 interface. The
OnePlus manifest at `ecdd4809cbd01fa5f509a4708347026f63498303`, XML blob
`0768736f87b61f782c2de954b250262aaa228df0`, still selects
`oneplus/sm8850_b_16.0.0_oneplus_15`; it does not provide a .102 Android 17
source contract. Existing public source-request issues are not source
availability or compatibility evidence.

Four previously uninspected official related-source heads are evaluated:

| Repository under `realme-kernel-opensource` | Exact commit | Result |
|---|---|---|
| [realme_GT8pro-AndroidB-vendor-source](https://github.com/realme-kernel-opensource/realme_GT8pro-AndroidB-vendor-source/tree/f96fa2fb6ca340ece7841d66eed31ba963056661) | `f96fa2fb6ca340ece7841d66eed31ba963056661` | No exact GEAS provider/type/data consumer |
| [realme_GT8pro-AndroidB-kernel-source](https://github.com/realme-kernel-opensource/realme_GT8pro-AndroidB-kernel-source/tree/64724e30399db54cf4e50e31c5819916e337b1bd) | `64724e30399db54cf4e50e31c5819916e337b1bd` | Two header symlinks into known GEAS system/game_opt interfaces, no matching data contract |
| [realme_GT8-AndroidB-vendor-source](https://github.com/realme-kernel-opensource/realme_GT8-AndroidB-vendor-source/tree/77efca144210ef41a26800bd3a48a9a6f399c76b) | `77efca144210ef41a26800bd3a48a9a6f399c76b` | Relevant selected blobs already in previous inventory; skipped |
| [realme_GT7pro-AndroidB-vendor-source](https://github.com/realme-kernel-opensource/realme_GT7pro-AndroidB-vendor-source/tree/75e40ad4b9c5f6d4378ed416ba4a88e5c2ba1362) | `75e40ad4b9c5f6d4378ed416ba4a88e5c2ba1362` | No exact GEAS provider/type/data consumer |

The selected relevant paths total 562; only 33 previously unseen unique blobs
are retrieved in that scan. Git blob identities and SHA256 are verified. There is no hit
for `get_gpu_bw_data`, `struct gpu_bw_data`, `geas_fill_system_snapshot` or
the actual GEAS data source names. These trees share relevant OPlus/Qualcomm
source organization but **none establishes the required `0xd8bbf8a1` contract**.
They are not accepted as a matching provider/consumer or deployed update.
The two kernel header links are followed into the paired vendor tree. Both
targets and `geas_assist.c` are already examined blob identities and are not
searched again. One additional new `game_opt/geas_ctrl.c` blob is inspected
without an interface hit, bringing the additional unique content to 34 blobs.

Four additional local archives are inspected by member inventory and hash:
the post-canary source checkpoint, preserved source/tools archive, the common
unstripped-module archive and DDK header archive. None contains KGSL/GEAS
source, objects or private headers. The unstripped archive is common/GKI
modules, not a missing unstripped OEM graphics provider.

The additional local intermediate/separate-debug search discovers 280 paths,
including 16 distinct objects actually implementing the export. Their DWARF
is inspected, not merely their filenames or version sidecars. All are previous
project diagnostic builds (scalar, raw-unsigned or `u32` variants), not a newly
recovered OEM object. No matching OEM declaration/debug graph is found. The
existing sidecars show `0x23455559`, `0x58cc1e32`, `0x77ccfbac` or `0xf24b42bd`,
not the required CRC; sidecars are not treated as independent build attestation.
No previously rejected variant is rebuilt or tried again. Ephemeral Bazel
sandboxes, including their deliberately inaccessible helper directory, are
excluded; persistent build outputs and backup/workspace artifacts are checked.

## GEAS remains an external blocker

The previous exact stock requirement `0xd8bbf8a1` versus diagnostic output
`0x77ccfbac` is unchanged. No new source graph is recovered, so no new GEAS
implementation, adapter or matched consumer rebuild is justified. The old
GPU build remains quarantined, with its previous 1,031-compatible / 38-dormant /
one-active-blocker result and two GEAS version-record mismatches. This is not
relabelled as a passing GPU-inclusive oracle.

Unblocking requires one of:

1. Matching ColorOS 17.0.0.102 KGSL declaration/header/include conditions or
   genuine generated symtypes establishing the OEM export graph;
2. matching unstripped DWARF/BTF for stock KGSL build ID
   `b83d0f4a58f9e10c0d5e3a8ffdd8b149bc2a2f15`;
3. complete buildable `vendor/oplus/kernel/cpu/geas/data` sources, UAPI and
   build inputs for the actual stock GEAS consumer, not `geas/system` alone;
4. exact OEM KGSL source permitting selective backports around the truthful
   original export.

A restored stock-facing provider must naturally produce `0xd8bbf8a1` and pass
real type, layout, units, lifetime, KCFI and consumer compatibility review.
A complete matched GEAS rebuild can instead use its truthful new natural CRC
only after the entire affected consumer closure is rebuilt and the ColorOS
GEAS userspace/shared-memory contract is preserved and validated; it must not
leave a retained consumer requiring the old CRC. Neither a CRC alone nor the
known two-word machine prefix is sufficient. Any future GPU rebuild must also
use the new flag-separated kernel SDK, rather than silently reusing the old
inline `signal_pending()` implementation.

**Unified release status remains BLOCKED.** The mandatory GPU/GEAS gate is
unresolved; no matched boot/DLKM/AVB physical-test or release package is
generated. No physical qualification or overall GKI PASS is claimed.
