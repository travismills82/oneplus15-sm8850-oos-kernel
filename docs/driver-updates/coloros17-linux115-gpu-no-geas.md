# ColorOS 17 Linux 6.12.115 GPU canary without GEAS

## Decision and scope

**READY FOR AUTHORIZED PHYSICAL CANARY — GEAS DISABLED**

This is an offline-qualified diagnostic package, not a qualified release or
permission to flash. Fresh device/slot/firmware/snapshot/recovery checks and
explicit flashing authorization remain mandatory. No device was available over
ADB during this task. Historical partition evidence is not a live preflight.

Branch: `experiment/coloros17-linux115-gpu-no-geas`.
Implementation parent: `a3b501fee3c3c9e44579f46fd78e2282d05e29e8`.
Common source tree: `c74ab6f64f685145302559ce9fa2a2b00dc4c385`.
Built release: `6.12.115-android16-6-o-ga3b501fee3c3-4k`.

The new worktree is
`/home/travis/Android/stock_kernel/out/coloros17-linux115-gpu-no-geas`.
Private evidence and artifacts are under
`/home/travis/Android/kernel-test-backups/coloros17-linux115-gpu-no-geas-20261101`.
Paths below are relative to that evidence directory unless stated otherwise.

No kernel/GPU implementation, frequency, voltage, thermal limit, firmware,
device tree, KernelSU/SUSFS, watchdog, security configuration, ABI reference,
qualified branch or installed image was changed in this task. Existing source
fixes remain, including GPU client repair `eff946871d410`, task-flag separation
`651a7d4d1b2a0`, repaired core KABI, SYNX compatibility, RMNET and multimedia
backports. No commit, push, merge, release, reboot, slot change or flash occurred.

## GEAS dependency audit

Exact canonical names were read from the actual ELF `.modinfo`, not inferred
only from filenames. Each name has an identical stock binary in vendor_dlkm and
a dormant copy in vendor_boot. There are no system_dlkm GEAS modules.

| Canonical module | Normal load | Imports/dependencies of interest | Own exports |
|---|---|---|---|
| `oplus_bsp_geas_cpu` | vendor_dlkm `modules.load`, former line 204 | `oplus_bsp_game_opt` | 0 |
| `oplus_bsp_geas_system` | vendor_dlkm, former line 205 | bwmon, memlat, qcom_dcvs, qcom_pmu_lib, frame_boost, game_opt, msm_kgsl | 4 |
| `oplus_bsp_geas_data` | vendor_dlkm, former line 206 | spmi_pmic_arb, slc_mpam, mpam_msc, qcom_dcvs, msm_kgsl | 0 |

The system module exports `update_bwmon_params` (`0x85a42df9`),
`update_bwmon_params_for_cdgo` (`0xf77c5ae1`), `update_ddr_freq_table`
(`0x53ce7dd4`), and `update_gpu_params` (`0x99361549`). None is imported by any
other module in the actual 1,070-record population. No retained module declares
a dependency on the three GEAS module names. This is a three-module closure,
not a reason to disable game_opt, frame_boost, bwmon, memlat or other OPlus code.

Only GEAS data imports `get_gpu_bw_data`. Both physical copies expect
`0xd8bbf8a1`. The earlier diagnostic getter generated `0x77ccfbac` and was never
accepted. This package contains neither that reconstruction nor a dummy getter.
The rebuilt GPU has six exports and does **not** export `get_gpu_bw_data`.

`geas-dependency-audit.json` preserves paths, hashes, dependencies, exports and
load-list evidence for all six copies. `policy/original-geas/` preserves their
unmodified bytes. Vendor-boot normal/recovery load lists do not request them.

### ColorOS userspace and optional hooks

Exact `.102` vendor, odm, system, system_ext and product filesystem copies were
inspected offline. The gameopt HAL references `/proc/game_opt/geas_ctrl` and
`geas_assist`; UAH/URCC libraries contain GEAS resource controls. The HAL has
GEAS error/fallback strings. Its stock init service is `disabled`, `oneshot`,
and is not marked `critical`. No explicit GEAS module request was found in the
stock init scripts. A scan of 517 compressed APK/JAR DEX members found no exact
GEAS kernel-interface path/module references; that is not proof of every
possible dynamically constructed access.

`oplus_bsp_game_opt.ko` is retained. Its existing proc interfaces and other game
features are not removed. Actual stock disassembly verifies null guards before
all eight inspected GEAS callback-pointer invocations, including update/cleanup
and the assist hooks. These are pre-existing optional callback paths, not new
stub functions. With GEAS modules absent, those callback providers do not
initialize. Complete source parity with every closed ColorOS userspace path
cannot be proved; repeated HAL errors, frame pacing, power and thermal effects
are explicit physical-test gates, not assumed PASS.

Evidence: `boundary-validation.json`, `boundary-evidence/`,
`userspace-extraction.json`, and the stock init file referenced there.

## Clean disabling policy

Persistent inputs/tools added in this worktree:

- `tools/driver-updates/coloros17-no-geas-policy.json`
- `tools/driver-updates/prepare-coloros17-no-geas.py`
- `tools/driver-updates/test_coloros17_no_geas.py`

Only the three vendor_dlkm GEAS files are omitted from the experimental
filesystem. Only their three `modules.load` entries and three owning
`modules.dep` rows are removed. Specific `modules.blocklist` entries are added.
Every retained load-order/dependency row and all alias/softdep bytes remain
unchanged. Reverse dependencies cause preparation to fail closed.

The boot v4 command line is intentionally extended with:

```text
module_blacklist=oplus_bsp_geas_cpu,oplus_bsp_geas_data,oplus_bsp_geas_system
```

This uses the unmodified standard Linux `blacklisted()` / `early_mod_check()`
path in `kernel/module/main.c`. It rejects these exact canonical names with
`-EPERM` before import/version processing or module initialization. Existing
signature checks, MODVERSIONS, KCFI, protected exports and other module handling
are unchanged. The actual blacklist function is compiled in a host test with
positive and similar-name/unrelated-module negative cases. `.115` has
`CONFIG_CMDLINE_EXTEND=y`, not FORCE. Physical qualification must confirm this
parameter actually arrives in `/proc/cmdline` before accepting the exclusion.

Vendor_boot and its dormant copies are unchanged; the new kernel blacklist
prevents fallback loading of those copies. Recovery has its own kernel, so the
Android boot blacklist must not be claimed as a recovery-kernel modification.
Its unchanged recovery load list contains no GEAS requests.

No Kconfig conversion, module-loader shadow patch, fake export, CRC rule,
binary version patch or userspace change is used. GEAS remains buildable in the
source tree; the experiment disables its deployment, not unrelated features.

## GPU and Linux build evidence

The existing reviewed KGSL modernization is retained, including the selective
fixes audited against graphics tag `GRAPHICS.LA.16.0.r1-10300-kaanapali.0`
(`adf5712becd72bce1c6b426b41ed82545dd923bb`). This is **not** a claim that a full
10300 snapshot or every latest Qualcomm feature was imported. No new upstream
driver changes were added during this diagnostic packaging task.

Normal prepared worktree `out/coloros17-linux115-task-flags` has the identical
clean common tree. Its normal Kleaf build generated a fresh SDK, Image, config,
Module.symvers and ABI. The isolated GPU build used those hash-pinned normal
outputs and normal Canoe perf DDK configuration/makefiles. Real producer tables
from the previously verified SYNX/IOMMU/mtrack closure were reused; no table was
synthesized or edited. The five repaired GPU import CRCs naturally match the
retained providers. Actual GPU audit checks 531 version records including the
special module-layout record and finds no import, namespace, unsigned-import
policy or protected-export failure.

Commands/receipts: `sdk-preparation-command.json`, `gpu-build-command.json`,
`sdk/manifest.json`, `gpu-build/result.json`, `strict-kmi-command.json` and
`enforced-abi-command.json`. A full generic Canoe DLKM distribution was not
regenerated or used as the deployment population.

## Validation results

| Gate | Result |
|---|---|
| Normal common kernel build | PASS |
| Explicit strict KMI symbol-list check | PASS |
| Common ABI generation | PASS |
| Enforced common GKI comparison | **FAIL, exit 4**; unchanged 38 reviewed differences plus six documented .115 representation differences |
| Enforced small/short/flat/viz reports vs prior repaired .115 | All byte-identical |
| All 10,163 vmlinux export records | Identical to prior repaired .115 |
| GPU compilation/link/modpost | PASS |
| Actual packaged enabled-set module oracle | PASS |
| Host regression tests | 195 PASS |
| GPU shared stock KCFI prefixes | 2,377 match; zero differing shared prefixes |
| Five inspected expanded layout/UAPI types | Zero differences vs previous compiled updated GPU |
| Image filesystem check, hash readback, UID/GID/mode/SELinux | PASS; all 473 remaining inode records checked |
| AVB boot signature / parent signature / expected chain metadata / leaf hashes and trees | PASS offline |
| Independent FEC encoding vs stored parity | PASS, 1,220,608 parity bytes identical |
| Partition capacity against captured slot-B super metadata | PASS; exact existing allocation, no resize |
| Fresh device identity, slot and snapshot status | NOT VERIFIED; phone not visible over ADB |
| Physical GPU/GEAS-disabled operation | NOT TESTED |

The five inspected types are `kgsl_process_private`, `kgsl_memdesc`,
`kgsl_device`, `kgsl_gpumem_alloc_id`, and `kgsl_device_getproperty`. Complete
expanded DWARF output matches the prior updated-GPU build. The shipping GPU
has no DWARF/BTF: these checks are **not** invented OEM private-layout proof.
All remaining public consumers/imports are checked separately by actual ELF.
Proprietary userspace UAPI behavior still needs a physical canary.

The old capacity tool on this branch rejected the real `*-cow` LP names. The
already-reviewed parser support from the preflight branch was restored, with
four regression tests. Names containing paths, whitespace, non-ASCII or hidden
suffixes remain rejected. Checksums, extent overlap, snapshot and capacity
checks are unchanged. This host-tool correction changes no kernel/image bytes.
Unknown snapshot state still produces BLOCKED in the explicit negative check.

### Enabled population: exact scope, not a 1,032-module blanket PASS

```text
Original module locations:                1,070
Intentionally excluded canonical names:       3
Intentionally excluded locations:             6  (3 active + 3 dormant)
Physical packaged files:                   1,067  (3 dormant copies remain)
Enabled oracle records:                    1,064
Active compatible:                         1,029
Dormant enabled records:                       35
Hard imports:                     60,267 / 60,267
CRC records:                      61,337 / 61,337
Missing providers:                             0
CRC mismatches:                                0
Namespace failures:                            0
Active blockers:                               0
vmlinux-first edges checked:                54,407
Production-provider mismatches:                0
```

The final oracle uses module bytes extracted back from the completed image,
not just an unstripped staging build. No extra module is excluded. The four
installed-canary replacements (rmnet_core, qce50_dlkm, qcrypto-msm_dlkm,
wcd9xxx_dlkm) remain byte-identical to the passing .114 canary filesystem.
The GPU is the only additional replacement. There is no competing stock GPU
copy in the modeled runtime locations. All remaining modules stay stock or
the already passing canary versions; quarantined WLAN/camera/video/display
replacements have not been silently introduced.

The separate complete-stock negative audit retains all 1,070 records with the
new GPU. It reports 1,031 compatible, 38 dormant, one active blocker and exactly
two missing `get_gpu_bw_data` provider edges (active/dormant GEAS data).
CRC mismatches and namespace failures are zero. This expected **FAIL** is
preserved, not relabeled PASS or hidden by an alternate stock GPU provider.

Evidence: `final-image-oracle/`, `final-offline-result.json`,
`population-validation.json`, `full-stock-negative-oracle/`.

## Frozen candidate images and authentication

| Image | Intended partition | Bytes | SHA256 |
|---|---|---:|---|
| `images/boot.img` | `boot_b`, only after fresh slot confirmation | 100663296 | `931a69b0ce9ec911a144431a0f685bd2000c770900efdea8a49bb2cdef5bccd9` |
| `images/vendor_dlkm.img` | existing shared vendor_dlkm backing, resolved via live mapping | 154849280 | `0837cac0d888e862e4ad21eb7f2dbe58ae4f72bbb5534de2475adb783cf89804` |
| `images/vbmeta.img` | `vbmeta_b`, coordinated parent | 65536 | `54074cca444676d400f842411329a925340249bb3861abea132528d4eb97d11c` |

Kernel Image: `417dd7d89e159ecf6d0f8e7032ac6f44b79d2589c3c1b6ccd92dc47b170cf30a`.
Module.symvers: `6a6413d41ce692c0476a0ccd81e38a446fe1490400f3924c79424c597a390da1`.
Config: `abbb37e8fae9073bd5ca5bf539196f4b9baff60fa40714fe1506dc01592f1a27`.
Packaged GPU: `d8711d3bb64a936e89c7df13996af3d7e4c6b71c83e217ea09e23d3abc965aa8`
(3,268,904 bytes). Strip-debug was verified to preserve all allocated runtime
sections, exports, imports, version records and KCFI prefixes.

Boot remains header v4, zero-size ramdisk, with stock-compatible header values.
Only Image and the deliberate blacklist command line change. The known `.102`
AVB fingerprint, `os_version=16` and `security_patch=2026-10-01` properties are
preserved exactly; these are the actual firmware contract, not a fabricated
Android-17 patch level. No device tree, vendor_boot or init_boot is repackaged.

The same development root public key already accepted in the installed ORANGE
canary signs the new parent. Public-key blob SHA256:
`53626275df3c650b66b6594f5a0958dc6aa92ca14ba27d5aa30b3a3fb728e4b5`.
The new boot is signed with that development key, and the parent boot-chain
descriptor is updated to its actual matching public key. Only the parent
`boot` chain-key bytes and `vendor_dlkm` hashtree descriptor change; every
unrelated descriptor and rollback-index location/value is unchanged. Flags
remain zero. Candidate vendor root:
`b6ea40f9cd8abb4e1a84de5bfbd0dcecefe170321fa2027c1012ea0d6f7684b4`.

Offline signature/hash/tree consistency is not proof of device acceptance or
FBE. The unchanged Magisk init_boot and custom recovery retain the existing
unlocked baseline exceptions; this is not an OEM-trusted/relockable full chain.
No key enrollment, verification-disable flags, rollback changes or OEM signature
forgery are used. Do not relock. No `system_dlkm.img`, `vendor_boot.img` or
`init_boot.img` is required by this experiment.

The image fits the captured 154,849,280-byte logical partition exactly, with a
152,317,952-byte ext4 data area and unchanged hashtree/FEC geometry. The checked
filesystem has 229 free 4K blocks. The captured super metadata SHA256 is
`81a320c930b085e20afc61cf9b208240a667d8aff0127d57465756f50d93181d`.
Snapshot status must be refreshed; a historical PASS is not write authorization.

## Rollback and controlled physical-test plan

`rollback/` stages the currently passing experimental .114 canary, preserving
GEAS and its paired authentication rather than assuming a slot switch restores
shared vendor storage:

| Rollback image | SHA256 |
|---|---|
| boot | `1affe0774f9a483606d3ff44ac7e65214171f29ef439cae74ff49c6742a2fbb3` |
| vendor_dlkm | `b8a5385cb563188640395b4117681b782f8e3a97fcd914d8ce6a34af0e6e62db` |
| vbmeta | `32e48b5179cc891f74d9201b75bc3966cb93e74216ed5f286b5e34f09be6c8a4` |

The separate original-stock rollback remains preserved in the earlier AVB
evidence. Original GEAS binaries from both partitions are also preserved.
Never treat a vendor_dlkm and its parent vbmeta as independent rollback items.

After explicit authorization only:

1. Verify device identity, ColorOS `.102`, unlocked/ORANGE, slot B, image hashes,
   firmware properties, rollback access and exact unchanged Magisk init_boot
   hash `d0fce3c1448150276e06bfca3d617605ba4f5a421d8122d18f90105b8fc2257b`.
   Confirm user0/FBE health before disrupting the session.
2. Capture fresh super metadata and query actual snapshot/update state. Run the
   validator with that evidence; reject active/unknown snapshots, changed
   allocation, wrong slot/firmware or unavailable recovery. Prove the currently
   installed pair still matches the staged rollback; otherwise restage first.
3. Use the established independently recoverable TWRP/fastbootd route. Resolve
   actual vendor_dlkm backing and boot/vbmeta B mappings. No resize, metadata
   rewrite, userdata access/wipe, init_boot write or slot switch is allowed.
4. Dry-run the complete coordinated write. Write vendor_dlkm, verify full
   readback, write boot B and verify, then commit the matching parent vbmeta B
   last and verify. Do not boot between mismatched-pair stages. Keep the host
   rollback path available independently of Android/FBE.
5. On first Android boot verify exact kernel release, `/proc/cmdline` blacklist,
   absent three GEAS module names, actual updated GPU identity, dm-verity V/0,
   user0 RUNNING_UNLOCKED, CE/DE, F2FS inlinecrypt, KeyMint and no fatal errors.
   If the blacklist is absent or GEAS loads, the exclusion oracle does not
   describe runtime: stop qualification and collect evidence.
6. Check Vulkan and OpenGL ES enumeration/rendering, compositor/hardware fences,
   camera/photo/video, display rotation and app transitions. Run the same
   repeatable GPU-intensive game/workload as the .114 GEAS-enabled control for
   at least 20–30 minutes at comparable ambient temperature, brightness and
   charge state. Record frame-time distributions, jank, GPU frequency/utilization,
   GPU faults/recovery, thermal throttling and energy/battery behavior. Do not
   force clocks, voltages or thermal policies to improve the result.
7. Observe gameopt/UAH/URCC service restarts, missing-path/GEAS ioctl errors and
   repeated tuning retries. Successful boot alone does not qualify gaming or
   power behavior. Distinguish deliberate absent GEAS features from a new crash
   or unsupported-HAL loop.
8. Exercise FBE/lock/unlock, RMNET LTE/5G/data transitions, Wi-Fi/Bluetooth,
   audio/DSP, USB/OTG, charging and thermals. Perform a separately coordinated
   USB-disconnected idle/suspend/resume test. Check previous QCOM timekeeping,
   UFS telemetry and hung-task reports against the passing canary evidence.
9. Scan all logs/pstore for panic/Oops, KCFI, UAF/OOB, RCU stalls, module/probe
   failures, deadlocks, verity errors and GPU resets. TWRP decryption requires
   separately authorized reboot/recovery testing, not an assumed PASS here.
10. On a genuine regression, preserve logs and restore the complete staged
    vendor_dlkm/boot/vbmeta canary set with readbacks, parent last; confirm
    Android/user0/inlinecrypt and the prior GPU/GEAS behavior. No erase or key
    migration is part of rollback. Do not switch slots as a substitute.

All runtime/performance/thermal/battery/GEAS-absence outcomes are **NOT TESTED**.
The original GEAS source-contract problem remains unresolved for a GEAS-enabled
modernized GPU release. This experiment intentionally does not solve or conceal
it. Security completeness and full GKI compliance are not claimed.
