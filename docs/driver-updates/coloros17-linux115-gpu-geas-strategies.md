# ColorOS 17 / Linux 6.12.115: autonomous GPU/GEAS strategy results

## Outcome

**The five GPU import contracts are repaired and compiled. The mandatory
GPU-inclusive release remains BLOCKED by the unavailable OEM GEAS export
declaration/type graph or complete buildable GEAS data consumer source.**

All five requested strategy families are accounted for below. No remaining
safe implementation path is established from the available inputs. This is
not a claim that an unpublished/private OEM source package cannot exist.

Experimental branch: `experiment/coloros17-linux115-gpu-geas-strategies`.
Source repair commit: `eff946871d41042c51a7ecdcf04e7cb5b11f7fce`, parent
`a5367aeb603cb65bfb10ceb95fe2b89f91e6f296`. Common source tree remains
`d291044f0cab4c86a562498ca77fc5c6aedf3338`.

No installed-image/device commands, reboots, slot changes, flashing, key
enrollment, pushes, merges or publication occurred. Slot B's known-working
.114 canary was not touched. No configuration, signing, KCFI, MODVERSIONS,
GENDWARFKSYMS, symbol list, ABI reference or common-kernel implementation was
changed. The GPU client/header changes are listed below.
The earlier 231 individual imports, 15 already-present fixes, core KABI
repairs, SYNX and other driver work remain in their original ancestry.

Evidence root:
`P=/home/travis/Android/kernel-test-backups/coloros17-612115-kabi-repair-20261031-trcyzT/gpu-autonomous-20261101`.
The final hash-sealed inventory is `P/sealed-manifest.json`; its generator
verifies the previous seal, final compiled input/output pins, normal producer
table and the exact expected oracle failure before writing a new manifest.
Previous evidence/rejected hypotheses remain under its parent and under
`/home/travis/Android/kernel-test-backups/coloros17-unified-20261031-rdEWAJ`.
The previous recovery report is preserved, not overwritten.

## 1. Exact five-import inventory and real repair

These are GPU **consumer** interfaces. The remaining GEAS problem is a
separate GPU **provider** interface; changing import SDK inputs cannot fix
the GPU's own generated `get_gpu_bw_data` export CRC.

| Import | Stock provider | Old GPU import | Repaired natural import |
|---|---|---|---|
| `qcom_iommu_set_secure_vmid` | `qcom_iommu_util` | `0x9aa7e17e` | `0x913ad69d` |
| `register_mtrack_debugger` | `oplus_bsp_mm_osvelte` | `0x561885cb` | `0x20577724` |
| `register_mtrack_procfs` | `oplus_bsp_mm_osvelte` | `0xa18d2361` | `0xba9a9d4d` |
| `unregister_mtrack_debugger` | `oplus_bsp_mm_osvelte` | `0x00c01888` | `0xf78ca29e` |
| `unregister_mtrack_procfs` | `oplus_bsp_mm_osvelte` | `0x8a1380b4` | `0x5bf9e57a` |

[Machine-readable inventory](coloros17-linux115-gpu-five-imports.tsv) records
actual provider paths, export classes, namespaces, KCFI prefixes, type roots
and source headers. All five stock providers are retained, not substituted
with analysis modules.

### VMID root

The older SDK enum lacks `VMID_GVM1 = 0x34`; its terminal `VMID_LAST` differs.
The declaration is recovered from the official [OnePlus 15T source at
0cfd948c0d9507d0bc04706f6f919a25787c2d89](https://github.com/OnePlusOSS/android_kernel_oneplus_sm8850/blob/0cfd948c0d9507d0bc04706f6f919a25787c2d89/include/soc/qcom/secure_buffer.h).
The repaired complete enum exactly equals that source enum. Existing
non-terminal VMID values are unchanged; KGSL continues to use `VMID_CP_PIXEL`
`0x0a`, not the new VMID. No new VM/firmware feature is enabled.

`qcom_iommu_set_secure_vmid(struct iommu_domain *, enum vmid)` retains its
signature, alignment, callback convention and export class. Its complete
source-built function has the same 116 instruction bytes as the shipping
provider; the source/stock callback load and KCFI path are unchanged.

### Mtrack root

The older shared header assigns GPU to registry slot 2. Shipping ColorOS uses
DMABUF=0, GPU=1, ZRAM=2, ERM=3 and UXMEM_POOL=4. Stock KGSL's register/unregister
calls independently supply 1; callbacks test GPU subtype 3/4. The source-proven
stock enum graph includes the ZRAM subtypes before ERM/UXMEM.

A firmware-bound GPU client header supplies this real contract. The unrelated
shared OPlus implementation/header is not globally changed. This fixes actual
registry selection, not just version metadata. GPU callbacks still use
subtypes 3/4; no memory-accounting behavior is replaced with a stub.

Native AArch64 compilation asserts enum widths, registry values,
`sizeof(struct mtrack_debugger)=24`, alignment 8 and pointer offsets 0/8/16.
Actual stock/candidate callback KCFI prefixes match. All four API classes
remain `EXPORT_SYMBOL_GPL`, with no namespace; IOMMU remains `EXPORT_SYMBOL`,
with no namespace.

The original MM/mmzone/vmalloc/procfs include chain is retained for complete
type visibility. An intermediate stripped-down include chain generated
`0x9ebf0e0f` for `register_mtrack_procfs`; restoring the native definitions
naturally produces `0xba9a9d4d`. No type string or CRC override is involved.

### SDK and dependency closure

Import versions come from exporter build metadata in normal modpost, not from
rehashing a consumer's C declaration. Therefore truthful client headers and
correct producer SDK inputs are both necessary.

Fresh SDK interface producers compile genuine OPlus API bodies and the complete
configured QCOM IOMMU source group through normal Kbuild/gendwarfksyms/modpost.
The initial single-file IOMMU build exposed eight unresolved internal symbols;
the actual registry's six LPAE/FAST component sources resolve that closure.
No missing dependency is stubbed or downgraded to a warning.

The OSVELTE API harness also supplies real proc-registry initialization, so
procfs operations are functional rather than optimized into permanently-empty
error paths. Final bodies of all five analyzed functions have instruction
bytes identical to stock; native geometry and KCFI are separately checked.
These are analysis harnesses, **not deployable OSVELTE/IOMMU replacements**.
The actual runtime providers remain the original stock modules.

Normal generated producer module names are `oplus_bsp_mm_osvelte` and
`qcom_iommu_util`; the GPU's dependency names are correct and contain no probe
module names. Two stale SDK tables are removed from the input list and one
fresh, unedited normal producer table is supplied. No table/CRC/ELF output is
hand-edited. A later functional-harness rebuild yields the identical table
bytes used by the final GPU build.

Full symtypes, native assertions, generated producer metadata, stock/provider
KCFI checks and disassembly controls are under `P/final-import-evidence/`,
`P/provider-function-controls-functional.json` and `P/contract-geometry.log`.

## 2. GEAS evidence and generator controls

The actual active consumer is stock `vendor_dlkm:oplus_bsp_geas_data`; its
vendor_boot copy is dormant. Both hard undefined imports require
`get_gpu_bw_data` CRC `0xd8bbf8a1`. The rebuilt GPU produces `0x77ccfbac`.
This is a real loader boundary, not merely an internal STG type difference.

At `geas_fill_system_snapshot+0x88`, the caller passes one stack pointer,
tests the integer return, and on success reads 32-bit words at offsets 0/4
into bandwidth snapshot storage. On error it logs and skips those reads.
It clears 16 bytes before the call. The stock getter writes two 32-bit words,
using current DDR-table/bus-level conversion and `bus_ab_mbytes`. The caller
owns the temporary storage; the observed path performs no transfer of that
pointer's ownership.

This proves the used prefix and data path, not the complete source type,
member names, typedef/qualifier chain, alignment annotations or unused tail.
Matching the `5e9e7585` KCFI bytes does not supply that missing information.
The diagnostic eight-byte `gpu_bw_data { u32 ib; u32 ab; }` remains an
unaccepted reconstruction. Its real-data implementation is not a dummy, but
neither the complete OEM ABI nor teardown/lifetime behavior is qualified.

The fully expanded generated graph is saved, including formal-parameter
names and `u32 -> __u32 -> unsigned int`. No KGSL private device type occurs
in that exported graph. Its CRC cannot be repaired by changing the function
body while retaining the same external graph.

| Genuine full-TU control | `get_gpu_bw_data` | Existing `geas_update_gpu_params` |
|---|---|---|
| Normal project Clang 12833971 / stable DWARF | `0x77ccfbac` | `0x7815dd83`, stock match |
| Exact stock Clang 14043575 / stable DWARF | `0x77ccfbac` | `0x7815dd83` |
| Stock Clang 14043575 / separate ThinLTO control | `0x77ccfbac` | `0x7815dd83` |
| DWARF visibility/stable-flag control | `0x77ccfbac` | `0x7815dd83` |
| Legacy C genksyms / real preprocessed TU | `0xedc59bd9` | `0x16753e99`, not stock |
| Earlier declaration-only hypothesis | `0x3787b598` | not a solution |

The exact stock compiler is obtained from [Google's immutable prebuilt
commit 77c95e8253c2c82ffd4fee92c0ad9f5ac1e8ca04](https://android.googlesource.com/platform/prebuilts/clang/host/linux-x86/+/77c95e8253c2c82ffd4fee92c0ad9f5ac1e8ca04/clang-r536225/),
Git-blob checked, and SHA256 pinned to
`93d88cfd3d346b0ba009ff4119f9c3b4e642bddedbd08e547bfbe7e27fffa1ed`.
The project compiler remains unchanged at
`69e6d00e8596eba85cbcf2cd87639fe3f8dbed397cba6d98a8c2def68a7a4044`.
Compiler controls hold source, SDK, resource headers and other flags constant;
they are not production toolchain/config changes or claims of reproducing
every original OEM build input. Production LTO/configuration is unchanged.

These tests reject a compiler-build, simple LTO, stable-mode or legacy-generator
explanation for this reconstructed graph. They do **not** recover the unknown
OEM token. No member-name search, CRC collision construction, type-string
mask, forced value, incompatible cast or consumer `__versions` patch is used.

## 3. Strategy ledger

| Strategy | Work and reproducible evidence | Disposition |
|---|---|---|
| A — restore OEM declaration | Stock ELF/disassembly, KCFI, full generated graphs, source/history/debug-artifact recovery and exact compiler controls | BLOCKED: original full declaration not recovered |
| B — typed compatibility adapter | Used prefix/units/return handling recovered; real diagnostic getter compiled; earlier opaque/declaration-only treatment rejected; generator depends on external graph, not adapter body | BLOCKED by the same missing external type; inventing it or forcing CRC would violate requirements |
| C — matched GEAS rebuild | Actual consumer identified, full provider closure recorded, additional OEM/MediaTek sources and history inspected; available registry builds CPU/system only | BLOCKED: complete `geas/data` sources/build inputs/shared-memory UAPI missing |
| D — preserve OEM provider and selectively backport | Stock object/debug availability and required internal loads compared with compiled KGSL; existing Qualcomm VMA fix retained | BLOCKED without matching OEM KGSL source; binary getter transplant is unsafe, stock binary alone does not integrate the required source fixes |
| E — wider matched dependency closure | Five source/SDK roots repaired; full configured IOMMU component closure built; native layouts/KCFI, dependency names and complete oracle checked | COMPLETE for five imports; no wider provider replacement is required for them; GEAS remains independent |

### Additional recovery beyond the earlier investigation

The previous 34 branch heads / 509 graphics blobs / 66 historical bus/header
revisions are preserved. This phase adds **70 branch heads in 10 additional
official repositories**: OnePlus/Oppo SM8550, SM8650, MT6989, MT6991 and MT6993.
Their 590 selected source paths comprise 108 distinct blobs within this new
scope. All trees are untruncated and all completed retrievals succeeded.
Ten GEAS-related source symlinks were followed to their downloaded targets;
no overlooked GEAS submodule or selected C/header LFS pointer supplies the API.

Reachable history adds 98 historical bus/header file revisions, with no API
match. Neither inspected GEAS data path has historical commits on these new
heads. Explicit fork-inclusive public code queries for the function, type and
consumer also return zero, with no incomplete-result flag.

All 26 local KGSL objects existing at the debug-artifact check were examined.
Fourteen match stock build ID `b83d0f4a58f9e10c0d5e3a8ffdd8b149bc2a2f15`;
none of those has DWARF/BTF. The configured public debug server returns HTTP
404 for that build ID. This is not a global assertion about private servers.

The available official OnePlus 15 revision returns HTTP 404 for the five
actual consumer source files identified by binary paths:
`geas_data_ioctl.c`, `geas_data_shm.c`, `cpu_data.c`, `ddr_data.c` and
`share_pmu_data.c` under `vendor/oplus/kernel/cpu/geas/data/`.
The available GEAS build registry registers CPU and system targets, not data.
The stock data module is GPL-labelled; missing source must not be confused
with proof of a proprietary license. Its complete buildable source is simply
not available among the inspected inputs.

`geas/system/geas_gpu.c` calls the different `geas_update_gpu_params` API;
rebuilding it does not replace the actual snapshot/ioctl/shared-memory data
consumer. Replacing the latter requires its own complete source and ColorOS
userspace contract, not a renamed module or disabled performance function.

### Why a binary OEM getter cannot be transplanted

| Member load | Stock instruction offset | Rebuilt device offset |
|---|---:|---:|
| AB vote | `0x2c98` | `0x2890` |
| DDR table pointer | `0x2ca0` | `0x2898` |
| Current DCVS bus level | `0x2ed8` | `0x2ad0` |

The old getter's instructions would read the wrong private fields in the
rebuilt object. Retaining the entire original stock module avoids that but
does not import the missing Qualcomm `vm_pgoff` fix into its implementation.
The source backport remains preserved at `6285ba23b5c97`, upstream
`1d15bd800fe5dad77ba5a4a4571e1478bae3a52e`. Selective modernization against
the exact OEM source remains the preferred alternative when that source is
available. Binary/CRC splicing is not used as a substitute.

## 4. Final compiled/module results

Final diagnostic GPU build source: commit `eff946871d410...` plus the
explicitly uncommitted, unaccepted `kgsl_bus.c/.h` diagnostic export patch.
The builder freezes the additional SoC include closure. SDK common source
is unchanged `d291044f...`; kernel release remains truthfully
`6.12.115-android16-6-o-ga5367aeb603c-4k`.

| Gate | Result |
|---|---|
| Normal repaired .115 common build / strict symbol list / common ABI generation | Preserved PASS; common inputs unchanged |
| Full updated GPU compilation | PASS |
| Actual GPU imports vs retained stock providers | PASS; five mismatches resolved, zero missing/CRC/namespace failures |
| Unsigned-import/protected-export policy | PASS; zero failures/collisions |
| Shared stock/candidate function KCFI | 2,378 match, zero differences |
| Five actual provider function KCFI / three GPU mtrack callbacks | MATCH |
| Native AArch64 enum/callback geometry assertions | PASS |
| Three KGSL private structures plus diagnostic output type vs prior compiled control | IDENTICAL complete pahole output |
| Complete stock private type/layout recovery | NOT PROVEN; unavailable metadata |
| Host tests | 175 PASS (171 preserved plus 4 client-contract tests) |
| Enforced common GKI | FAIL / 4; unchanged 44 owners, not relabelled PASS |
| Existing-provider full population control | PASS: 1,032 active compatible, 38 dormant |
| Final GPU replacement population | FAIL: 1,031 active compatible, 38 dormant, 1 active blocker |

The final 1,070-module replacement model has 60,707 undefined symbols
(60,701 hard, 6 weak), zero missing providers, zero namespace failures and
61,775 / 61,777 matching CRC records. **Only two CRC records fail**, both
`get_gpu_bw_data`: one active GEAS data consumer and its dormant recovery copy.
The old GPU provider is removed from the model; no oracle-only fallback is
allowed to hide the mismatch. The GPU's own 530 ELF imports resolve correctly.

No complete matched boot/DLKM/AVB package is generated while this gate fails.
Final Canoe distribution/device-ABI, new partition-capacity/snapshot checks and
physical qualification are not claimed from older artifacts.

| Artifact | SHA256 |
|---|---|
| Final diagnostic GPU module | `256afcdeff835ae906660885ae05a8178cd95106435f270a5bd5eb982fc8d9c3` |
| Earlier five-import proof GPU module | `c60b8f13029b97cdc931ad1a47ce08b08a477dd24205444445274d24aab7e3ed` |
| Unchanged normal .115 common Image | `05fcd84b513d4f465c9ec4c0ce325294b35701183c73e4e0285ee090bd6d3064` |
| Common Module.symvers | `6a6413d41ce692c0476a0ccd81e38a446fe1490400f3924c79424c597a390da1` |

The final module is at
`P/gpu-final-imports/workspace/build/kernel-output/vendor/qcom/opensource/graphics-kernel/msm_kgsl.ko`.
It is **not an installable/qualified candidate**.

## 5. Independent task-flag issue

The overlapping `PF_MEMALLOC_BOOST` and new `PF_NO_NOTIFY_SIGNAL` meanings at
`0x00800000` remain separately documented. Actual retained QCOM DMA-heap code
sets/clears this bit in task flags around page allocation; new core signal
logic interprets it differently. The logging-only include change does not
resolve or conceal that conflict. No flag alias, stable-fix revert, unused-bit
guess or watchdog suppression is introduced. It remains a separate .115
runtime-compatibility prerequisite before any unified physical candidate.

The core introduction is local `c77c6cf021c24e7b1330f616dbb87c837764b4de`,
stable `4a52a339587ed193e1d956c9d609283ae3f7a377`, upstream
`22b7f491be8f93ff72de0a3fe02b8ee9e64d3f21`. The retained
`qcom_dma_heaps.ko:qcom_sys_heap_alloc_largest_available` sets/clears that bit
at `current->flags` offset `0x44`. Its disassembly and the introducing commit
are preserved in the previous `gpu-recovery-115-interface-evidence/` seal.
This is source/binary conflict evidence, not an observed .115 device symptom;
the phone remains on .114.

## 6. Precise external dependency / next safe action

At least one of the following matching inputs is required:

1. The actual ColorOS 17.0.0.102 declaration/header/include chain and build
   conditions for `get_gpu_bw_data`, including the complete OEM
   `gpu_bw_data` type; original normal `kgsl_bus.symtypes` would also help.
2. Matching unstripped KGSL/GEAS DWARF for stock build ID
   `b83d0f4a58f9e10c0d5e3a8ffdd8b149bc2a2f15`, allowing faithful recovery.
3. Complete buildable `vendor/oplus/kernel/cpu/geas/data/**` and its
   shared-memory/ioctl headers/build configuration, to rebuild the real
   consumer against a truthful updated provider.
4. Matching OEM ColorOS KGSL source retaining that export, for selective
   upstream security/stability backports while preserving the original ABI.

The 32-bit CRC and the used machine-code prefix do not uniquely encode the
missing source graph. Changing names/definitions until the CRC happens to
match would not establish the required contract. Stock GEAS cannot be
silently dropped or have its version records patched.

**Status: BLOCKED — OEM GEAS type contract or complete matched consumer
source unavailable. Five-import source/SDK repair is complete and preserved;
mandatory GPU/GEAS compatibility and a unified physical-test package are not.**
