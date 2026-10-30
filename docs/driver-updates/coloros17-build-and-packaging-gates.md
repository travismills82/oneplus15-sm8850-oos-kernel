# ColorOS 17 SM8850: module-build and packaging feasibility gates

Date: 2026-10-30. Audit only, starting at `efa7ca4647889e6fe92b5e8ee70ffbfe9fadfa9c`
on `experiment/coloros17-sm8850-dlkm-updates`.
The initial audit changed no driver implementation. Subsequent isolated
RMNET source changes and module builds are recorded chronologically below.
The production kernel configuration, Image, boot container and device remain
unchanged; no flash, push or release is claimed.
LineageOS targets, firmware and images are outside this experiment.

The device is not currently visible to ADB. Installed firmware/slot/runtime
facts refer to the last verified 2026-10-30 ColorOS canary, not a fresh live
inspection. The main investigation subsequently completed Phase 1 full-image
backups and the six-entry manifest under
`/home/travis/Android/kernel-test-backups/coloros17-sm8850-dlkm-updates-baseline-20261030/`.
Implementation remains gated on the relevant component compatibility proof.

## Immutable kernel oracle

Preserve the exact tested 96 MiB `boot.img`, not a newly rebuilt equivalent.
Qualified evidence root:
`/home/travis/Android/kernel-test-backups/coloros1700102-6.12.114-five-cves-20261029/`.

| Input | SHA256 |
|---|---|
| boot.img | `1affe0774f9a483606d3ff44ac7e65214171f29ef439cae74ff49c6742a2fbb3` |
| Image | `8ebe51e2a565a55886245793f572661e87c1a40bf5016bb79b84855370242456` |
| Common Module.symvers | `6a6413d41ce692c0476a0ccd81e38a446fe1490400f3924c79424c597a390da1` |
| Common effective config | `e5495ab30241f869f12b3fc2ff3590886150fb464c736975c8054303a9e9981a` |
| Cached Canoe DDK config | `eade3c36fb1a5f25021349ea10fca051a1c418f926199a0be4ee2121c3cbbcd2` |

The corresponding common vmlinux GNU build ID is
`631855c02d3a5c50f57eedc2d640554570f807ee`.
Common config and the Canoe DDK config are distinct inputs: common has
MODULE_SIG_ALL=y, while Canoe's module fragment correctly overrides it to n.
Do not edit or substitute the common config to obtain a module build.
The sampled configs use Android clang 19.0.1/r536225, CFI enabled and the
qualified LTO_NONE setting. Preserve these settings; do not weaken them.

The old experimental worktree still has matching cached common outputs under
`kernel_platform/bazel-bin/common/kernel_aarch64/`, modules_prepare/DDK setup
scripts, matching public signing certificate and `scripts/sign-file`.
Its Canoe config is
`kernel_platform/bazel-bin/soc-repo/canoe_perf_config/out_dir/.config`.
These are evidence inputs, not permission to reuse arbitrary cached modules.
The new worktree initially has no local prebuilts link or Bazel output tree;
restore only manifest-matched tooling and audit its identity before building.

The unchanged stock oracle is 1070 module-location records: 1032 active
compatible,38 dormant,60701 hard imports,61777 version records, zero missing,
CRC, namespace, protected or active/provider blockers. A modified hybrid must
be re-inventoried; its import counts may legitimately change and must not be
forced to this all-stock total. Retained stock binaries stay byte-identical.

## Public-source match is not established by the manifest

`oneplus_15.xml` names OnePlus public source revisions
`fc30e54174d254ff7f33622a9278e4435f6718d2` (SoC kernel) and
`5ab2a689ff87d7d28c511f1762cf41c1b90d965a` (modules/device trees), on the
`oneplus/sm8850_b_16.0.0_oneplus_15` lineage. These are source provenance,
not proof of a byte/source match to ColorOS 17.0.0.102 shipping modules.
The Android 17 framework and Android 16 vendor build identities are not
interchangeable driver ABI declarations.

Inspected stock vendor modules are unsigned and naturally report the vendor
6.12.69 `gb79517a1cbef` build lineage; protected stock system modules use the
distinct 6.12.69 GKI build lineage. The candidate vmlinux is 6.12.114.
Never spoof vermagic or overwrite versions to bridge those identities.
Use actual loader version semantics, all ELF imports, natural CRCs and private
type/layout evidence. Inspected stock WLAN/DWC3/display binaries have no
DWARF/BTF sections from which a complete shipping private ABI can be recovered.
Source strings/build IDs are clues, not exact-source proof. Until an unchanged
module build and its interfaces have been compared, mark source match UNKNOWN.

## Minimum build units, after Phase 1 authorization

The following labels are derived from the checked-in Starlark definitions.
The CNSS control query/preparation/build results below supersede the original
read-only status for that component only. Run from
`kernel_platform/`, query each proposed label first, then build just that
bounded target. Bazel may build required dependency modules; that does not
authorize packaging/replacing those dependencies.

| Scope | Minimum candidate target |
|---|---|
| WLAN allocation control | `//vendor/qcom/opensource/wlan/platform:canoe_perf_cnss_prealloc` |
| WLAN netlink control | `//vendor/qcom/opensource/wlan/platform:canoe_perf_cnss_nl` |
| CNSS PCIe platform | `//vendor/qcom/opensource/wlan/platform:canoe_perf_cnss2` |
| Active CLD chipset | `//vendor/qcom/opensource/wlan/qcacld-3.0:canoe_perf_qca_cld_peach-v2` |
| Camera | `//vendor/qcom/opensource/camera-kernel:canoe_perf_camera` |
| Display | `//vendor/qcom/opensource/display-drivers:canoe_perf_msm_drm` |
| Video | `//vendor/qcom/opensource/video-driver:canoe_perf_msm_video` |
| IPA | `//vendor/qcom/opensource/dataipa:canoe_perf_ipam` |
| RMNET core | `//vendor/qcom/opensource/datarmnet:canoe_perf_rmnet_core` |
| Audio example | `//vendor/qcom/opensource/audio-kernel:canoe_perf_machine_dlkm` |
| DWC3 MSM | `//soc-repo:canoe_perf/drivers/usb/dwc3/dwc3-msm` |
| UFS glue | `//soc-repo:canoe_perf/drivers/ufs/host/ufs-qcom` |
| ICE | `//soc-repo:canoe_perf/drivers/soc/qcom/qcom_ice` |

Initial CNSS commands, subsequently used only for query and preparation:

```sh
tools/bazel query //vendor/qcom/opensource/wlan/platform:canoe_perf_cnss_prealloc
tools/bazel build //vendor/qcom/opensource/wlan/platform:canoe_perf_cnss_prealloc_makefiles \
  //vendor/qcom/opensource/wlan/platform:canoe_perf_cnss_prealloc_config
```

Use the appropriate DDK defconfig and generated merged config, not bare
`make M=...` against an unprepared output directory. Freeze effective module
configs, compiler/CFI settings, headers, imported Module.symvers files, actual
exports, provider paths and all private-header dependencies for each target.
Final qualification still requires the production build and ABI/oracle gates;
compiling one module does not make generic Canoe DLKM images deployable.

## WLAN closure and first feasible control

First **build control**, not an approved source update or flash:
unchanged `cnss_prealloc.ko` from the existing source, using the exact kernel
oracle and Canoe config. It has no separate module dependency in the inspected
stock modinfo and avoids a direct firmware/HAL/probe interface. It remains
vendor_dlkm, unsigned, modular and at its stock path.

Its existing retained consumers include CNSS2, ICNSS2 and all three shipping
CLD variants. Require these natural exported CRCs before considering a narrow
internal allocator/lifetime fix:

| Export | Expected CRC |
|---|---|
| cnss_initialize_prealloc_pool | `0x5b01e1dc` |
| cnss_deinitialize_prealloc_pool | `0xd272d446` |
| wcnss_prealloc_get | `0x61724803` |
| wcnss_prealloc_put | `0x132082dc` |

Inspect every other export too, plus allocation-size, ownership, reset/leak
tracking and teardown semantics; four CRC matches are not a whole-module PASS.
If this control does not reproduce a compatible interface, stop before patching
larger drivers and resolve source/config mismatch first.

Peach-v2 CLD's declared dependency closure includes cfg80211, CNSS2,
cnss_prealloc, cnss_utils, cnss_nl, QCOM IOMMU utility, IPA, WALT, QMI helpers,
QCOM VA minidump and rproc common. CNSS2 also uses MHI/PCIe, firmware-service,
CNSS IPC QMI, minidump/logging and OPlus boot/project interfaces. `fw-api` and
`qca-wifi-host-cmn` are compiled into CLD through its `cmn`/header graph rather
than installed as independent .ko providers. Changing CLD does not automatically
require replacing every dependency, but any changed dependency boundary must
be closed over actual stock consumers and matching firmware/HAL messages.
Do not replace all three CLD variants solely because the target supports them.

## Private ABI hard gate: DWC3

Fresh `pahole -C dwc3` of the frozen qualified vmlinux confirms:
sizeof=1488, xhci offset=384, recovery-context pointer/reserve1 offset=1456,
reserve2/3/4 offsets=1464/1472/1480. `pahole` printed Rust-CU parsing warnings
but produced this complete C type; this is not a warning-free-tool claim.

The previous stable fix without the local preservation grew dwc3 to1576 and
moved xhci to464. Stock `dwc3_msm` directly dereferences this private type,
so matching symbol CRCs did not detect the real early-boot failure.
The qualified core instead keeps recovery work/mutex/state in a private
allocation reached through the old KABI reserve slot. Preserve that fix.

Both SoC DWC3 MSM source files include `drivers/usb/dwc3/core.h`; rebuilds must
see the exact qualified private header and recheck all used offsets, not a
new upstream 1576-byte header. Stock vendor_boot and vendor_dlkm contain
byte-identical DWC3 module copies. Updating only one without proving recovery/
normal provider precedence is unsafe. DWC3 is not the first module experiment.

## Module signatures and provider classification

Qualified common enables MODVERSIONS, GENDWARFKSYMS, TRIM_UNUSED_KSYMS,
MODULE_SIG and MODULE_SIG_PROTECT, but not MODULE_SIG_FORCE. Preserve runtime
signature-enforcement state too; its live setting is not reverified here.
Current policy intentionally retains103 trusted signed and967 unsigned stock
records. A new vendor module is not automatically required to become signed.

`kernel/module/main.c` treats unsigned modules as vendor providers: protected
exports require a trusted signature, and unsigned imports from a signed/GKI
provider must be permitted. Blindly signing a former unsigned vendor provider
changes this classification and can break retained unsigned consumers even
when CRCs match. Validate import/export/signature closure in both directions.

The old matching cached build still has a private signing key (mode0600).
Only its derived public identity was inspected: public SPKI SHA256
`3206fe8c8bdcbf721c46269258d5dbffaf914903e6b04df258df4d5d982d6575`
matches the frozen qualified certificate, certificate DER SHA256
`3bfb05f880ff387f4e1a17314f04a8fd244ba946e705c870cfb0626edfc120aa`.
Therefore trusted signing can be investigated without a new boot trust anchor.
Do not print, publish, commit or move private material into an artifact tree.
An arbitrary newly generated build key is not equivalent. If a required key
is unavailable or untrusted, stop rather than change signature enforcement.

## Filesystem and AVB packaging: currently a deployment blocker

Downloaded exact firmware images were inspected read-only with the manifest
`prebuilts/kernel-build-tools/linux-x86/bin/avbtool` and host `file`:

| Image | Filesystem/data size | Complete downloaded image size |
|---|---|---|
| system_dlkm | EROFS /14270464 |14512128 |
| vendor_dlkm | ext4 /152317952 |154849280 |

Both local footers are Algorithm NONE, SHA256 hashtrees,4096-byte data/hash
blocks,2 FEC roots,flags0 and rollback0. However **top-level stock vbmeta.img
directly embeds both DLKM hashtree descriptors** and is signed SHA256_RSA4096
with public-key SHA1 `a568677dbd61e0aa97ca6f3183dc28f505d1b83c`.
The signed vbmeta_vendor/system chains do not own those DLKM descriptors.

Stock roots are system_dlkm
`ab2a8566a22027e27765f4390452b7892903efcde22f407682fb5c171bdc1335`
and vendor_dlkm
`1374ff0726349e3ca91b2999a786c12af12530aff39a836c58dae68839f1669c`.
Replacing even one .ko changes the root. Regenerating only an unsigned local
footer cannot preserve the signed parent's expected digest. No matching OEM
AVB signing key or authorized new parent contract has been established.
The qualified custom boot uses a different test key; that is not authority
to sign or change top-level stock vbmeta.

Stock first-stage fstab lines47-50 use `avb=vbmeta` for both DLKM partitions;
system_dlkm also lists GSI public keys. Android properties from the archived
baseline advertise locked/green/enforcing; rooted-device properties alone
are not proof of actual bootloader state or permission to modify AVB.
Fresh bootloader/fs_mgr policy evidence is required. Do not disable AVB,
change rollback indexes or write vbmeta to make an image mount.
This is a deployment gate, not a reason to stop read-only source analysis.

Use full logical-partition backups, not only the archived `*-verity.img`
files: those old files contain filesystem data only, without the original
full hashtree/FEC/footer geometry. Image length is not proof of live logical
partition capacity or dynamic-super free space. Preserve original raw image
bytes and mappings before any planned package creation.

## Tools and exact metadata requirements

Manifest prebuilts contain avbtool, depmod, e2fsck, fsck.erofs and mkfs.erofs.
Host dump.erofs/mkfs.erofs are not on PATH. EROFS extraction supports optional
xattrs, off by default; ordinary-user extraction does not preserve root
ownership by default. Never assume a plain extracted directory retains Android
SELinux labels, ownership, capabilities, hardlinks or inode modes.

For system_dlkm, preserve an explicit inventory of all files/symlinks, UID/GID,
modes, xattrs/contexts, compression, UUID, timestamps and module metadata.
Use deterministic EROFS options and reviewed file_contexts/fs_config inputs,
then independently re-extract and compare. Do not convert EROFS to ext4.

Vendor_dlkm is ext4. Existing `stage-matched-wlan-vendor-dlkm.sh` has useful
copy/metadata/signing safeguards and uses debugfs, but is an old-firmware
workflow: it regenerates only the local unsigned AVB footer and explicitly
does not reconcile parent vbmeta. Its hard-coded load/signing assumptions must
not be treated as a ColorOS17 image validator. Its SHA1 signing choice is not
automatically the policy for a new candidate.

Preserve modules.load order, modules.options, aliases, softdeps, blocklists,
modules.builtin and the stock relative and cross-partition modules.dep paths.
Depmod must use exactly the unchanged stock population plus approved
replacements, not generic rebuilt Canoe providers. Revalidate all retained
consumers, vendor_boot duplicates, actual init/libmodprobe provider precedence,
protected exports, all hard ELF imports and every versioned import.

## Candidate and device gates

1. Finish fresh rollback/partition/config/toolchain/provider-oracle freeze.
2. Establish the original unchanged-source module build control and exact
   source/config compatibility before adding a driver fix.
3. Review one authoritative self-contained fix; freeze its dependency closure,
   public and private ABI, firmware/HAL/DT effects and replacement path set.
4. Build the smallest DDK target without weakening validation. Compare actual
   exports/imports/CRCs/CFI/type layouts and allocation/callback semantics.
5. Run a production-realistic hybrid oracle with exact provider precedence;
   report changed counts explicitly and require zero unexplained failures.
6. Resolve the AVB parent-contract gate before claiming any DLKM image can be
   deployed while preserving enforcement and stock supporting partitions.
7. The owner authorizes isolated experimental packaging after static gates;
   resolve the actual source/AVB blockers first, then independently verify exact
   payloads/labels/load metadata/capacity/AVB. No generic DLKM image is a release
   candidate merely because a build succeeded.
8. Physical tests require fresh live device/firmware/slot, full hash-verified
   rollback, minimum writes, complete read-back and stop on ambiguity. Validate
   FBE/user0 first, then the updated subsystem, radios, USB, recovery where
   affected, suspend/resume and fatal/module/watchdog scans.

## Executed unchanged-source CNSS control

The target query passed. Initial analysis correctly stopped with zero compiler
actions for a missing generated OPlus variant file; the normal canoe/perf
generator restored the exact qualified variant before continuing.
Input/output action-closure analysis proved that the requested two preparation
targets require no KernelBuild Image action. They passed in34.239 s/618 actions.
The full DDK module target, full common Image and generic distribution were
not rebuilt.

The control then used an isolated private workspace, the qualified cached
modules_prepare archive, exact frozen Module.symvers and unchanged source,
plus the normal generated DDK Makefile/Kbuild/config. Two initial harness
attempts stopped before C compilation (glibc/musl search-path conflict, then
6.12's relative-M/VPATH requirement). Both logs are preserved; fixing only
the generated workspace/tool lookup yielded run03 compile/link/modpost PASS.
No source change, ignored modpost error or manual version record was used.

Control evidence is under
`/home/travis/Android/kernel-test-backups/coloros17-sm8850-dlkm-updates-build-20261030/control-cnss-prealloc/`.
The unsigned control module SHA256 is
`f69703ac6d48d25251a20785606044d217baadbdf40cf6669c7e9060e3770303`;
generated DDK config SHA256 is
`f24a4096b2e2b4d7c25d7a383524c578e7bfe2d5b1dcfe2cccd4a6c8042a7472`.
Compared with common config, only the expected unsigned-DDK MODULE_SIG_ALL=n
setting and the CNSS module's own five config lines differ. Production config
itself was not changed. CFI, MODVERSIONS and stable GENDWARFKSYMS remain active.

Measured stock/control results:

- All7 export names, natural CRCs, classes and namespaces are identical.
- All28 import-version records and27 hard ELF imports are identical.
- All27 hard imports resolve from the exact qualified vmlinux exports.
- All13 retained CNSS/ICNSS/CLD consumer edges remain compatible.
- All29 allocated runtime sections match bytes, sizes, alignment, flags and
  types; all342 normalized runtime relocations match target/type/addend.
  This includes init/exit code, compiled CFI/GNU properties and this_module.
- Explicit metadata exclusions are modinfo, Linux/build-ID notes and the
  stock-only extended-version representation. Actual version dictionaries
  were independently compared, not excluded. BSS has identical zero-fill size.

This is strong per-component compiled-runtime/interface fit, not recovery of
the shipping SCM revision, a whole hybrid-image oracle or a physical module
load. The normal module config has CNSS_PREALLOC_DEBUG_LEAK disabled.
Stack-recording code and sprint_symbol imports still exist in stock/control;
they are not compiled out. Normal initialization instead sets alloc_info=NULL
and tracking/resize calls guard that pointer. A symbol-buffer hardening patch
therefore must not be advertised as active production protection without
separate reachability evidence. Do not enable leak tracking for this project.

## RMNET preparation and cached-provider discrepancy

RMNET core's real module closure includes rmnet_ctl, rmnet_mem and IPA; IPA
requires GSIM plus SoC providers. The five requested module-config/Makefile
preparations passed in19.883 s/90 actions. Their actual action closure has no
kernel Image or ARM module compile. An initial wrong-working-directory query
was retained and corrected; it performed no compile. No RMNET fix is applied.

Ten legitimate cached SoC producer Module.symvers files were compared with
their actual cached ELF exports and stock normal/recovery providers. MDT,
IPC logging, SMEM, QMI, rproc common, USB GSI and VA minidump match. Three
providers do not match their whole stock export sets:

- qcom_scm:11 missing exports and2 differing shared-memory-bridge CRCs;
- qcom_iommu_util:2 differing export CRCs;
- qcom_ramdump:2 missing exports.

Those generic cached providers are not safe replacement modules. Initial
strict provider validation stopped the RMNET compile group. A follow-up
dependency scan found zero of those17 mismatches imported by shipping
RMNET/IPA/GSIM components, and zero source references to the17 names in the
bounded source closure. Actual consumed SCM/IOMMU/ramdump export CRCs match.
Using unchanged legitimate cached Symvers only for those consumed interfaces
is a distinct question from replacing the incompatible providers; no table
may be synthesized, filtered to force success or silently substituted at
runtime. Any new compiled import must be checked against shipping providers.

### Executed RMNET dependency controls

All controls below used unchanged source, normal generated DDK inputs and the
same frozen common outputs. Compilation, link and modpost completed without
ignoring failures. No producer CRC table was synthesized, filtered or edited.
No dependency SDK module is an authorized replacement merely because it built.

| Component | Natural public boundary | Compiled-runtime result | Use in this experiment |
|---|---|---|---|
| rmnet_mem | 10 exports,45 versions,44 ELF imports and9 retained consumer edges match stock | 31 allocated sections and750 canonical relocations identical; only two `.text` equality-CMP operand swaps, reviewed as equivalent | Legitimate generated SDK Symvers only; retain stock module |
| GSIM, bare standalone target config | 82 exports match;4 stock logging/minidump imports absent | Wrong standalone configuration context for shipping Canoe; major runtime differences | Not a shipping-compatible implementation |
| GSIM, bounded diagnostic with normal already-generated RMNET/Canoe closure config | Same82 exports,81 versions and80 imports match stock | 35 allocated sections match; `.text` grows8 bytes, so not byte-identical/source-qualified | Generated SDK Symvers only; retain stock GSIM |
| IPAM | 293 exports;7 USB-tether export CRCs differ; all98 retained import edges remain compatible | Runtime differences remain; new `system_percpu_wq` import replaces stock `system_wq` | SDK only for actual matched consumed interfaces; retain stock IPAM |
| rmnet_ctl | All6 exports,27 versions,26 imports and2 retained core-consumer edges match | All27 allocated runtime sections and257 canonical relocations identical | Unchanged compiled control fit; retain stock CTL in final proposed package |

The GSIM diagnostic did not enable a new production option: it used the normal
RMNET/Canoe DDK configuration where IPC_LOGGING and QCOM_VA_MINIDUMP already
equal m. A bare standalone driver Kconfig does not necessarily include the
parent SoC/OPlus context; that result alone is not proof of a Bazel source bug.
The two GSIM controls naturally generate identical SDK Module.symvers bytes.

The seven IPAM USB-tether differences are `ipa_usb_deinit_teth_prot`,
`ipa_usb_init_teth_prot`, `ipa_usb_is_teth_prot_connected`,
`ipa_usb_xdci_connect`, `ipa_usb_xdci_disconnect`, `ipa_usb_xdci_resume` and
`ipa_usb_xdci_suspend`. None is consumed by the bounded RMNET core/CTL/memory
source or actual control objects. They remain blockers to a future IPAM
replacement, not permission to replace IPAM for this RMNET experiment.
All actual newly compiled imports must continue to resolve against retained
stock providers, not whichever SDK object happened to generate the table.

### Unchanged RMNET core: compilation and CRC fit, actual loader-policy STOP

Unchanged core control path:
`/home/travis/Android/kernel-test-backups/coloros17-sm8850-dlkm-updates-build-20261030/control-rmnet-core/rmnet_core/run01/kernel-output/vendor/qcom/opensource/datarmnet/rmnet_core.ko`.
SHA256:
`940a68ddaeed81f7fd46ca8f4f064c8f5064e41108d94c56077294473ac5aacc`.

All140 export records and56 retained extension-consumer edges match stock.
All155 actual hard ELF imports and156 version records resolve naturally with
matching CRCs/namespaces against the frozen vmlinux plus retained stock
IPAM/CTL/memory/QMI providers. Exactly one import identity changes:
`system_wq` becomes `system_percpu_wq`; both actual kernel exports have CRC
`0x571c73d5`. They are distinct allocated workqueues, not literal aliases.

This does **not** establish loadability. Reading the actual frozen vmlinux's
compiled policy arrays finds10381 permitted imports and633 protected exports.
`system_wq` is permitted, but `system_percpu_wq` is not. The unsigned control
therefore has exactly one real `-EACCES` import-policy failure under
`kernel/module/main.c::resolve_symbol`; there are zero protected-export
collisions. No signature-class change, new kernel symbol-list entry or loader
bypass has been applied. The unchanged RMNET group stops at this gate.

Evidence: private `rmnet-core-production-provider-control.json` documents every
actual import; `rmnet-core-load-policy-run03.json` decodes the exact compiled
allowlist, rather than assuming that an exported symbol is vendor-importable.
Two initial read-only allowlist scans were interrupted because the generic
ELF string helper copied unnecessarily large buffers; the bounded-string scan
completed. No module or kernel bytes were changed by this diagnostic.

Compiled fit is strong but not literal byte identity. Core `.text` remains
107572 bytes; its15 changed bytes represent nine STADD-to-LDADD unused-result
per-CPU alternatives and two equality-CMP operand swaps. Two normalized text
relocations switch only the workqueue export above. All35 other compared
allocated runtime sections match, including initialization, allocation data,
CFI/GNU properties and exit code; the version section differs only for the
expected import identity. Source/disassembly review ties the LSE differences
to local stable commit `5ca91006cceaf` (upstream
`535fdfc5a228524552ee8810c9175e877e127c27`): scratch w12 is already an explicit
early-clobber output of the original LL/SC alternative, so no live value is
corrupted. Both CMP differences feed only equality branches. The workqueue
inline transition is local commit `f5875f3ceb24e` (upstream
`a2be943b46b4a7478ea8ddf9bb8e5251c59fceb7`). Actual workqueue-policy and loader
constraints must both be respected by any subsequently reviewed adaptation.

The independent private-boundary review finds all25 indirect-call KCFI checks
identical, the five leaf getters identical, and matching stock allocation
constraints: private504 bytes, port2816, descriptor88. The kernel-private
net_device prefix2688 gives the same real-device/QoS/APS accesses as stock.
See `coloros17-rmnet-private-abi.md` for the detailed limits/proofs; matching
public CRCs alone is never a substitute for this boundary analysis.

Disposition at the unchanged-control stage: CNSS and CTL have compiled-runtime
and public-interface fit. RMNET core has a concrete unsigned import-policy
blocker despite correct CRCs. Exact shipping SCM identity, modified-driver
validation and safe AVB deployment remain unproven. No updated driver/image
is qualified or ready to flash; future physical tests require separate
explicit authorization. No SDK dependency provider is in the proposed
production replacement set.

### Reviewed module-local workqueue compatibility option

Only one core source site uses `schedule_work`:
`core/rmnet_map_data.c::rmnet_map_flush_tx_packet_queue`, line1331 in this
source. The two ELF relocations at0xbea0/0xbeac form one ADRP/LDR address
materialization. Retained extension modules have their own scheduling sites;
they are stock binaries and are not being rebuilt or modified.

Replacing this single call with
`queue_work(system_wq, &state->agg_wq)` is a narrowly justified candidate:
it preserves the actual shipping queue selection and naturally uses the
already permitted `system_wq` export (`0x571c73d5`) and existing
`queue_work_on` helper (`0x8f4cd990`). Both queues are independently allocated
with flags0/max_active0. For such bound queues, `WORK_CPU_UNBOUND` selects the
submitting CPU; the pending bit, non-reentrancy and queue_work memory ordering
are unchanged. This is not an assertion that the two pointers are aliases.

The work object remains initialized once in aggregate init. The timer submits
the same object; the worker retains its aggregation spinlock and send callback.
Teardown first cancels the hrtimer, then synchronously cancels that work before
freeing aggregation state/port. No core/extension depends on flushing either
global queue; the separate rmnet_ps_wq flush is unrelated and unchanged.
The timer/worker interfaces are internal, not new module exports.

No implementation edit was made during the initial review. If implemented,
the new module must naturally restore the stock import identity and pass the
compiled unsigned-import policy. Require unchanged140 exports, all155 hard
imports/156 version records resolved, all private allocation/KCFI constraints,
and only the expected two queue-address relocations relative to the control.
Do not add a KMI symbol, sign around vendor import policy or alter the loader.

### Executed workqueue adaptation and first official security-patch build

The root agent subsequently applied the sole reviewed source-site adaptation
and committed it separately as
`4ee253465786329a1156de7aad9000e9452a841c`. Focused core run04 compile/link/
modpost passed against the same frozen kernel/SDK inputs. Its SHA256 is
`00dcdcac2e1f93600804dd5aec50e360a7359877b0a892a0105afb8bf96a1838`.
All140 exports,156 version records and155 actual ELF imports now match stock
exactly. All56 retained consumer edges match, all5590 normalized allocated
runtime relocations match stock, and actual compiled unsigned-import policy
has zero failures/protected-export collisions. The two queue-address
relocations now use system_wq as intended.

The independent private review reconfirmed all25 callback KCFI values and15
full type-layout dumps unchanged from the control, including the net_device
private base. Literal byte identity is still not claimed: the same nine LSE
and two equality-CMP changes remain. The only additional allocated-data
difference is one allocation codetag's source line1429 to1430, caused by the
one-line explanatory comment. All other fields of that codetag are identical.
The run01 unsigned-import failure remains preserved as a failed control.

The official CLO `ea47` bounds patch was then applied by the root agent and
committed separately as `084e43e044d572f30bec18ac3f0523d57d1c1351`.
Its upstream-relative patch ID is
`da5c34f9f14c793e7a502dd462d9584778d4bcb0` (root provenance check). Focused
core run05 compile/link/modpost passed; SHA256 is
`42a79a8f60c233a1c4ea3efeb35e19091d83f2c8d6c2a4320104b8e118e7aa7b`.
All140 exports,156 version records,155 actual ELF imports and56 retained
consumer edges continue to match stock. The compiled policy again has zero
unsigned-import failures/protected-export collisions. Natural exported
Module.symvers remains
`b51acea3b4df97134d9d086b391220d2102ec9c19ca7a014fb833a496b7938c0`.

Security code legitimately changes text/unwind/relocation addresses; run05
text grows1196 bytes. This is not an unchanged-runtime control. The official
patch alone is not yet declared security-complete: the separate review found
residual bounds issues requiring explicit handling and another focused build.
The complete1070-record production-realistic hybrid oracle remains pending
the final reviewed source state. Neither run04 nor run05 is a deployable
DLKM image or a physically tested module.

### Final focused bounds build and exact hybrid oracle

Run06 added reviewed local bounds checks and passed the complete non-flashable
hybrid oracle, but remains a preliminary artifact. A final review additionally
required legacy nonlinear paths to prove the actual first fragment, not just
the aggregate skb length, contains each directly dereferenced frame. Run07
includes those guards without requiring nr_frags==1 or rejecting a legal
multi-frame aggregate. V5's existing descriptor route remains intact.

Final focused module build run07: compile/link/modpost PASS. The unchanged
external-build convention skips BTF generation when vmlinux is not supplied;
this warning is disclosed, not presented as a BTF success. DWARF remains for
the independent private-type review. The unstripped, unsigned module is:
`/home/travis/Android/kernel-test-backups/coloros17-sm8850-dlkm-updates-build-20261030/control-rmnet-core/rmnet_core/run07/kernel-output/vendor/qcom/opensource/datarmnet/rmnet_core.ko`.
SHA256:
`7fa0bfcf5d79443156687c245ea3e39b995f39e1b21d1f28370fb1dac3d54191`.
Its natural Module.symvers remains `b51acea3b4df97134d9d086b391220d2102ec9c19ca7a014fb833a496b7938c0`.
All44 copied tracked C/header/Kconfig/Bazel package inputs were compared with
the reviewed worktree; they match. The normal module DDK config remains
`a505d61200d8b6c1fc61aa2c2cb5cf0257063218a5cc5c04d993889efa5ca46f`.

Focused result:140 exports,156 version records,155 hard ELF imports and56
retained consumer edges match stock exactly. The independent review again
finds15 full type-layout dumps unchanged, all25 callback KCFI identities exact
stock and the five leaf getters unchanged. Legitimate bounds code changes
text/branch/unwind addresses; literal stock code-byte identity is not claimed.

The final full oracle uses a private, non-flashable population containing all
1070 stock module-location records as both consumers and providers. Only
vendor_dlkm/lib/modules/rmnet_core.ko is replaced. All other module bytes and
all modules.* load/dependency metadata remain stock; no IPA/GSIM/CTL/memory or
other SDK module is substituted. Evidence directory:
`/home/travis/Android/kernel-test-backups/coloros17-sm8850-dlkm-updates-build-20261030/hybrid-oracle-rmnet-run07/`.

| Gate | Result |
|---|---|
| Module population | 1070;1032 active compatible;38 dormant;0 blockers |
| Hard ELF imports | 60701/60701 |
| Weak imports | 6, all accounted for |
| CRC records | 61777/61777 |
| Missing / CRC / namespace errors | 0 /0 /0 |
| Frozen-vmlinux-first provider check | 54785 edges;0 production-provider mismatches |
| Compiled GKI policy supplement | 54986 unsigned ELF edges;0 import failures;0 protected-export collisions |
| Existing provider ambiguities | 3220;0 new/changed ambiguous edges |
| Replacement-provider precedence | All56 edges have only vendor_dlkm:rmnet_core; no competing stock copy |
| Physical load/networking/tethering/USB tests | NOT TESTED |

The dependency TSV is byte-identical to the qualified all-stock baseline.
All ambiguous provider records, including their module hashes, are unchanged
from that baseline and load metadata/order is preserved. This proves no new
provider-choice substitution, not a fresh runtime trace of those old choices.
Signing classification retains the exact frozen public certificates and
unsigned-vendor class; it is not a fresh cryptographic audit of every stock
signature. No key or signature policy has been changed.

No new full common, enforced GKI or Canoe ABI invocation is claimed by these
focused tests. Common source/config/Image/exports remain unchanged. The
existing enforced GKI result still has38 reviewed downstream differences and
remains FAIL, not PASS. The fresh worktree has no cached common Image, so a
normal full target could trigger a new kernel and broad SDK build; none was
started to replace the immutable qualified kernel. Disk had323 GiB free at
the preflight; private run07 uses bounded module outputs, not a new kernel.

The module passes the stated focused public/private/policy and full hybrid
static gates. It is not an installable image and not physically qualified.
The signed parent-vbmeta DLKM descriptor/key gate remains unresolved. No image
has been packaged, no stock artifact/partition changed and no device flashed.

The final reviewed source was committed by the root agent as
`b7c709d8cd1381837afd97f170af6c5b3190a078`; the run07 copied source inputs
were rechecked against that commit without rebuilding the module. Source
guard/build metadata are preserved under the private evidence directory,
including `compiled-source-input-manifest.json` and the separate final-commit
manifest. The private builder now defaults to that exact reviewed HEAD;
historical controls require an explicit historical CONTROL_EXPECTED_HEAD.
It refuses an existing run directory. A reproducibility invocation, **not
executed again**, is:

```sh
cd /home/travis/Android/kernel-test-backups/coloros17-sm8850-dlkm-updates-build-20261030/control-rmnet-core
CONTROL_EXPECTED_HEAD=b7c709d8cd1381837afd97f170af6c5b3190a078 \
  bash build-control.sh rmnet_core run08 \
  /home/travis/Android/stock_kernel/out/coloros1700102-612114-five-cves/kernel_platform/bazel-bin/soc-repo/canoe_perf/drivers/soc/qcom/qmi_helpers/canoe_perf/drivers/soc/qcom/qmi_helpers_Module.symvers \
  "$PWD/ipam/run01/kernel-output/vendor/qcom/opensource/dataipa/Module.symvers" \
  "$PWD/rmnet_mem/run01/kernel-output/vendor/qcom/opensource/datarmnet-ext/mem/Module.symvers" \
  "$PWD/rmnet_ctl/run01/kernel-output/vendor/qcom/opensource/datarmnet/Module.symvers"
```

Those are unedited legitimate SDK producer tables, not deployment artifacts.
The source working tree must have no additional unreviewed implementation
changes when reproducing; the source-input manifest is the content oracle.
