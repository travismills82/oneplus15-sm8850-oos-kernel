# ColorOS 17.0.0.102 Linux 6.12.115 no-GEAS extended qualification

## Decision and scope

**EXTENDED QUALIFICATION INCOMPLETE — NOT READY FOR STABLE RELEASE.**

The installed GPU-inclusive candidate has positive physical rendering, Android,
encrypted-user and dm-verity evidence. No fatal GPU/kernel failure has been
identified in the captured extended interval. One additional stock schedinfo
WARN reproduced a trace already recorded on .114; it is assessed below rather
than omitted from the warning count. All six saved
startup warnings now have source/binary-grounded assessments below, including
the previously unresolved sync-fence warning. This is not an assertion that
every warning is repaired or that all device, power and security gates pass.

The owner explicitly confirmed PASS for the hands-on connectivity, call,
camera/video, audio/microphone, fingerprint and OTG checks on this candidate.
Remaining gates include USB-disconnected suspend, repeatable gaming/frame-time and thermal/power
measurements, and review of the disclosed GKI/security limitations. Permanent
GEAS removal is not approved by this diagnostic exercise. Removing the consumer
does not repair its original `get_gpu_bw_data` ABI incompatibility.

This session made **no kernel/module/image rebuild, flash, reboot, rollback,
slot change, bootloader/key change, OTA, clock/voltage adjustment, counter reset,
security-policy change, source implementation change, commit, push or release**.
Slot B and the Magisk-patched init_boot remain unchanged. Small native diagnostic
executables were built on the host and run from `/data/local/tmp`; they are not
release kernel binaries or replacement modules.

## Frozen identities

| Identity | Verified value |
|---|---|
| Branch | `experiment/coloros17-linux115-gpu-no-geas` |
| Source HEAD | `a3b501fee3c3c9e44579f46fd78e2282d05e29e8` |
| Common source tree | `c74ab6f64f685145302559ce9fa2a2b00dc4c385` |
| Kernel | `6.12.115-android16-6-o-ga3b501fee3c3-4k` |
| Firmware | `CPH2745_17.0.0.102(EX01)` |
| Firmware fingerprint | `OnePlus/CPH2745IN/OP611FL1:17/CP2A.260605.016/B.R4T3.55357f5-2eb9868-2eda512:user/release-keys` |
| Active slot | B, `_b` |
| Boot UUID | `99f92a17-f4ec-4e34-a8ec-96419d5fbc76` |
| Loaded module count | 688; three fewer than the prior GEAS-enabled canary |
| Raw verified-boot state | ORANGE/unlocked, development-key path; no enrollment/relock |
| SELinux | Enforcing |
| Vendor dm-verity | `V 0`; candidate root matches the parent descriptor |

Mutable properties can report GREEN/locked because of the owner's Magisk
property spoofing. They are not evidence that the bootloader was relocked.
Raw bootconfig and accepted-key evidence are preserved separately.

Full installed block-device hashes were verified against the frozen payloads:

| Artifact | SHA256 |
|---|---|
| boot B, 100,663,296 bytes | `931a69b0ce9ec911a144431a0f685bd2000c770900efdea8a49bb2cdef5bccd9` |
| vendor_dlkm, 154,849,280 bytes | `0837cac0d888e862e4ad21eb7f2dbe58ae4f72bbb5534de2475adb783cf89804` |
| Parent vbmeta B, 65,536 bytes | `54074cca444676d400f842411329a925340249bb3861abea132528d4eb97d11c` |
| Magisk init_boot B, unchanged | `d0fce3c1448150276e06bfca3d617605ba4f5a421d8122d18f90105b8fc2257b` |
| Kernel Image | `417dd7d89e159ecf6d0f8e7032ac6f44b79d2589c3c1b6ccd92dc47b170cf30a` |
| Module.symvers | `6a6413d41ce692c0476a0ccd81e38a446fe1490400f3924c79424c597a390da1` |
| Effective config | `abbb37e8fae9073bd5ca5bf539196f4b9baff60fa40714fe1506dc01592f1a27` |

The parent remains SHA256_RSA4096, flags 0, rollback index/location 0, with the
preserved stock companion descriptors and October 1 firmware AVB patch contract.
Its vendor_dlkm root is
`b6ea40f9cd8abb4e1a84de5bfbd0dcecefe170321fa2027c1012ea0d6f7684b4`.
Some stock vendor/boot properties identify Android 16 while init_boot identifies
17; these are the actual preserved firmware values, not values to normalize by
guessing from the Android marketing version. No AVB property was changed here.

The exact installed replacement module hashes are:

| File in vendor_dlkm/lib/modules | SHA256 |
|---|---|
| msm_kgsl.ko | `d8711d3bb64a936e89c7df13996af3d7e4c6b71c83e217ea09e23d3abc965aa8` |
| rmnet_core.ko | `0ad99c698fcaffdf0acd2656b8e4c4a113ec41c173828f4dec40ea25e6d5e287` |
| qce50_dlkm.ko | `eea2b5252aa6838b1091af37fa901827254cf33e7707cd5113d7eaa0aead5f62` |
| qcrypto-msm_dlkm.ko | `6606b6a4bcb9818271015f66658c201020a1a9761bbb37554020ad8e65fa0bfb` |
| wcd9xxx_dlkm.ko | `5e10de199ea0eb39cde0d4c771995851f9dfc6c284eb8a0b98fc595cbfe5bec4` |

The four non-GPU replacements are byte-identical to the passing .114 canary.
System_dlkm, vendor_boot, init_boot and the other stock vendor modules have not
been substituted during this qualification.

### Rollback preservation, not execution

The matched passing .114 canary rollback files were rehashed on the host:

| Rollback artifact | SHA256 |
|---|---|
| boot.img | `1affe0774f9a483606d3ff44ac7e65214171f29ef439cae74ff49c6742a2fbb3` |
| vendor_dlkm.img | `b8a5385cb563188640395b4117681b782f8e3a97fcd914d8ce6a34af0e6e62db` |
| vbmeta.img | `32e48b5179cc891f74d9201b75bc3966cb93e74216ed5f286b5e34f09be6c8a4` |

Original stock vendor_dlkm remains
`82f90a9e0aba489a880bfa844ec13e0facb870c70752647568a290d45492dc47`;
its original stock parent is
`750918fafedd2ffbaaceee5830f2f8a77ed1b5c8d6e149f05739dadc39c93918`.
Do not mix stock and experimental parent/payload pairs. Vendor storage is shared
across the slots' mappings; switching slots is not a vendor rollback. Restoration
is neither requested nor executed in this session.

## Six startup warning assessments

The timestamps and exact stacks are immutable records from the initial boot,
not reconstructed from an overwritten live kernel ring. For all six modules,
the current device hashes match the preserved stock binaries. All ten inspected
reporter/driver source files are byte-identical between the prior .114 canary
source `b6bd4a40410d` and this HEAD (`warning-source-diff-from-114.txt` is empty).
That does not prove that every exact warning instance was captured on .114.

| # / uptime | Reporter and originating module | Classification | Root-cause assessment and baseline evidence |
|---|---|---|---|
| 1 / 1.678749 s | `proc_register`, `oplus_bsp_sigkill_diagnosis` | **NON-FATAL / UNDERSTOOD** | Multiple OEM modules attempt `proc_mkdir("oplus_mem", NULL)`. Core rejects/frees the new duplicate entry without removing the live parent. This driver explicitly falls back to `oplus_mem/sigkill_reason` when mkdir returns NULL; the readable endpoint exists. Prior .114 has the same duplicate-parent warning family with other racing registrants, not this exact instance. |
| 2 / 1.698961 s | `internal_create_group`, `oplus_sync_fence` | **NON-FATAL / UNDERSTOOD** | Stock `sync_attr.attr.mode = S_IRUGO | S_IWUGO` requests 0666. `fs/sysfs/group.c:73` diagnoses the unsupported mode and masks it to 0664 before adding the attribute. Live sysfs mode is 0664; `/dev/oplus_sync` exists. The character-device ioctl interface is separate from this attribute, whose show/store callbacks are NULL. This exact warning is absent from the earlier eleven-warning inventory, but both stock module and kernel permission handling are unchanged. No missing attribute-group ABI, heap corruption, GPU crash or newly introduced permission relaxation is indicated by this trace. |
| 3 / 1.714293 s | `proc_register`, `oplus_bsp_file_read_record` | **NON-FATAL / UNDERSTOOD** | Same duplicate parent. Exact OEM source is unavailable, so the actual ELF was inspected: `init_module` calls proc_mkdir, tests its return, selects `oplus_mem/oplus_frr` on NULL or `oplus_frr` on success, then calls proc_create. The live endpoint exists. Prior .114 has the same parent-registration family, not the exact registrant. Proc-interface access-policy security is not certified by this assessment. |
| 4 / 1.753681 s | `gic_irq_domain_select`, `msm_geni_serial_probe` | **PRE-EXISTING** | GIC translation rejects the UART's optional second/wakeup interrupt specifier. Immediately following logs identify `1a84000.qcom,qup_uart`, `IRQ index 1 not found`, `No wakeup IRQ configured`, followed by successful ttyHS3 registration. The public driver treats the RX wake IRQ as optional. The same reporter/module/probe warning occurs on .114 at 1.767684 s. UART-specific wake capability remains a separate limitation; not all UART wake paths are thereby qualified. |
| 5 / 1.758404 s | `irq_set_irq_wake`, `hfi_core_ssr_irq_init` in `msm_hfi_core` | **PRE-EXISTING** | `Unbalanced IRQ 327 wake disable`. Stock disassembly proves a successful IRQ request followed by `irq_set_irq_wake(irq, 0)` at function +0x1ac; the IRQ is the `smp2p_dcp_clock_ready_irq` acquired into offset 0x3cc. The pong IRQ is acquired afterward into 0x3d0. Core finds wake_depth already zero and warns without underflowing it. The same call/site appears on .114 at 1.788728 s. This is an OEM wake-accounting defect, not proof of a firmware transaction stall; HFI/PM recovery still requires runtime coverage. |
| 6 / 4.625418 s | `frame_put`, `oplus_hbp_core`, hbp_frame.c:88 | **PRE-EXISTING** | Fortify reports 2,048 bytes copied through `char data[1]`. Boot logs record a 2,087-byte allocation and 2,048-byte data capacity. Actual stock disassembly compares requested length to capacity, then copies at offset 0x24 (36). This copy ends at byte 2,084, inside the 2,087-byte allocation. Source allocation/capacity checks agree. This exact occurrence is a legacy one-element trailing-array declaration warning, not evidence of an allocation overflow. The same warning exists on .114 and the earlier stock-pair evidence. This is not an audit of every possible HBP operation. |

Relevant stack suffixes, before the common module-init/syscall tail:

```text
1: proc_register -> proc_mkdir -> init_module [oplus_bsp_sigkill_diagnosis]
2: internal_create_group -> sysfs_create_groups -> device_add
   -> device_create_groups_vargs -> device_create_with_groups
   -> init_module [oplus_sync_fence]
3: proc_register -> proc_mkdir -> init_module [oplus_bsp_file_read_record]
4: gic_irq_domain_select -> irq_create_fwspec_mapping
   -> irq_create_of_mapping -> of_irq_get -> platform_get_irq_optional
   -> platform_get_irq -> msm_geni_serial_probe
5: irq_set_irq_wake -> hfi_core_ssr_irq_init [msm_hfi_core]
6: frame_put -> hbp_irq_threaded_fn [oplus_hbp_core]
   -> irq_thread_fn -> irq_thread -> kthread
```

The complete, exact printed traces are `warning-1.txt` through `warning-6.txt`
in the initial evidence directory; the abbreviated paths above are not substitutes
for those files. No warning was suppressed or repaired during qualification.
For HFI, the existing audit and a fresh primary-source check identify
[Lineage corrective commit 4afc7a51c7366c5f6931a5006e5e1bb6e9f332a0](https://github.com/LineageOS/android_kernel_oneplus_sm8850-modules/commit/4afc7a51c7366c5f6931a5006e5e1bb6e9f332a0.patch),
which removes exactly the unmatched clock-ready wake-disable call. Its Fixes
reference is Qualcomm `61035ed8e760`, HFI SSR handling changes. This is a
Lineage-provenance patch, not an independently verified official CodeLinaro
corrective release. The available local public HFI generation lacks the exact
stock `hfi_core_irq.c`; no module is rebuilt or fix claimed deployed. No exact
verified upstream corrective SHA was established for the other five conditions.
Future source corrections would require separate reviewed module/DT builds:
consistent proc-parent ownership, a legal/necessary sysfs attribute, valid optional
UART wake configuration, balanced HFI wake enable/disable, and a correctly sized
flexible-array declaration/allocation. None is a reason to revert stable fixes.

### Additional runtime warning and driver diagnostics

At **3273.218526 s**, PID 3212 (`dex2oat`) was interrupted by a scheduler tick:

```text
___slab_alloc.cold -> kmem_cache_alloc_noprof
 -> jank_hotthread_update_tick [oplus_bsp_schedinfo]
 -> android_vh_irqtime_account_process_tick_handler
 -> __traceiter_android_vh_irqtime_account_process_tick
 -> account_process_tick.cold -> update_process_times
 -> tick_nohz_handler -> hrtimer_interrupt -> arch_timer_handler_virt
```

Classification: **PRE-EXISTING**. The .114 canary has the same module/function
trace at 20.482316 s. `new_slab()` warns on `s->ctor && (flags & __GFP_ZERO)`.
The OEM caller uses `kmem_cache_zalloc(..., GFP_ATOMIC)` on a cache with the
`init_hot_thread_struct` constructor. Prior stock-binary analysis proves that
constructor only zeroes the object, not a lock/list initialization invariant.
This is real allocator-API misuse, not evidence of a sleepable allocation in
hard IRQ context or a GPU UAF. It remains an unfixed OEM defect; no global
diagnostic suppression or source change was made. The full new trace is saved
as `warning-7-schedinfo.txt`. Total observed WARNs for this boot are therefore
**six initial + one additional**, not a clean-WARN PASS.

Camera exercise also generated CSID/DRV configuration `rc=-22` messages and
`Unexpected regupdate in activated Substate` diagnostics. All four exact message
families also occur in the .114 canary's camera traces. Classification:
**PRE-EXISTING**, not a newly proven GPU/GEAS regression. The implausible fields
in the formatted diagnostic also occur in that baseline; they are not treated
as trustworthy frame identifiers or proof of memory corruption. The owner
confirmed working photo/video, but a full camera-mode/recovery matrix is not
certified. These CAM_WARN/CAM_ERR logs are disclosed separately from kernel
WARN splats.

## Actual GPU rendering and memory tests

The native diagnostic binary is SHA256
`c2625a6429f95dcb10f30709e906789f25ff48135b27db17d9c4d959909f015e`.
Its source, shaders/SPIR-V, compiler paths/hashes, exact commands and raw results
are preserved in the extended evidence. It was compiled with Android NDK Clang
18.0.4, `-std=c11 -O2 -Wall -Wextra -Werror`, EGL/GLESv3/Vulkan. This is a
userspace diagnostic build, not a change to the kernel toolchain or configuration.

Both APIs select **Adreno 840 hardware**, not a software renderer. GLES identifies
OpenGL ES 3.2 V@0842.52; Vulkan reports Qualcomm vendor 0x5143. Each run draws
a procedural triangle into a 512x512 RGBA target, validates sampled pixel
contents, cycles four contexts/devices, and checks separately allocated buffers.
Vulkan uses a real graphics pipeline, render pass and `vkCmdDraw`, followed by
image transfer/readback and fences. It is not merely enumeration or compute.

Initial sustained tests:

| API | Duration | Drawn frames | Pixel checks | Context/device cycles | Buffer checks | Result |
|---|---:|---:|---:|---:|---:|---|
| GLES3 | 60.010 s | 67,497 | 1,055 | 4 | 32 | PASS |
| Vulkan graphics | 60.007 s | 50,802 | 50,802 | 4 | 1,592 | PASS |

The initial run's submit/wait medians and p95 were GLES 0.709/1.437 ms and
Vulkan 0.962/1.643 ms. These are **offscreen harness timings, not gaming FPS,
display frame pacing, input latency or a .114 performance comparison**.

The additional six-run alternating API endurance sequence also **PASSed**, with
no guard stop, device loss, pixel mismatch or test-allocation failure. Combined
totals over eight runs are:

| API | Duration | Drawn frames | Pixel checks | Context/device cycles | Buffer checks |
|---|---:|---:|---:|---:|---:|
| GLES3, four runs | 240.050 s | 269,399 | 4,211 | 16 | 128 |
| Vulkan graphics, four runs | 240.023 s | 201,346 | 201,346 | 16 | 6,304 |

That is **480.073 seconds of bounded rendering, 470,745 frames, 205,557 pixel
checks and 32 context/device cycles**. Direct sampled GPU temperature across
the runs was 29.4–31.0 C, battery 26.9–27.5 C; all sampled GPU fault vectors
remained zero. Final persistent GPU fault/page-fault vectors are also all zero;
faulting-process lists remain empty. Final direct GPU temperature was 34.5 C,
which is not a benchmark peak-temperature measurement. The raw reset_count was
1,228 at final capture; this includes normal GPU wake/power-up and is not a count
of crashes or recoveries, as documented in the earlier counter-source review.

The retained extended observation starts at uptime 2551.51 s and ends at
4423.23 s: **31 minutes 11.72 seconds**, on the same boot UUID, with final
total boot uptime about 73.7 minutes. This is not uninterrupted log coverage
from power-on: the earlier owner-benchmark gap remains disclosed.

During the initial sampled tests, direct GPU temperature ranged 29.4–31.0 C
and battery temperature 27.4–27.5 C. All sampled fault vectors were zero and
thermal_pwrlevel was zero. Samples saw 160 MHz; that low-rate sampling cannot
prove the absence of short clock changes. Separate clock accounting and post-load
reads show additional operating frequencies. No governor/clock/thermal policy was
changed. The safety guard would stop only our test on excessive temperature or
duration; it did not override the device's thermal controls.

The harness checks bounded 32 MiB allocations per cycle and destroys its own
resources. It is not exhaustive VRAM pressure, power-loss, out-of-memory, long-game
or hardware-fence qualification. EGL/Vulkan fence success and normal compositor
operation are positive synchronization evidence, not proof of every display-fence
or camera/display interoperation path.

Owner-reported heavy benchmarks are separately preserved in the
[post-benchmark review](coloros1700102-linux115-no-geas-post-benchmark-20261101.md).
App lifecycle and Android GPU statistics support real benchmark activity, including
a 3DMark Vulkan device/swapchain. Scores, exact benchmark modes and comparable
.114 runs were not supplied. They must not be fabricated or inferred from an
application name. Retained logs and persistent counters show no GPU failure, but
the earlier benchmark kernel-ring interval was partly overwritten.

## GEAS impact, thermal and performance limitations

`oplus_bsp_geas_cpu`, `oplus_bsp_geas_system` and `oplus_bsp_geas_data` remain
absent. Their three active vendor copies are omitted and three dormant vendor_boot
copies are blocked by the standard exact-name module blacklist. No unrelated
module is excluded, no stock binary version record is edited and no dummy
bandwidth provider is installed. The stock GEAS getter contract remains unresolved
for a future GEAS-enabled release.

Android boots, hardware GLES/Vulkan render, SurfaceFlinger remains alive, and the
current captures show no repeated GEAS service crash/restart loop. A retained
OPlusUsageService message says `/data/vendor/urcc/urcclog.csv` is absent; this is
disclosed rather than treated as proof of full userspace equivalence. Its causal
relationship to GEAS absence is not established. No userspace file or service
was altered to hide it. Optional OEM callback checks documented in the package
allow unregistered GEAS callbacks, but not every proprietary app path is provable
from available source.

The previous benchmark capture contained 8,312 cooling-callback informational
messages in 11.76 seconds, up to 777/s, requesting rapidly changing frequency
caps. Source queues deferred work, so message count is not a hardware clock-change
count. No additional cooling-callback requests were found in the joined logs
from the first native sustained test at uptime 2927.18 s through the final
capture. That bounded workload does not reproduce the owner's heavy benchmark
thermal oscillation or establish its logging/power cost. Cached HAL temperatures,
including 99.6 C sentinel-like entries, are not
substituted for time-aligned direct sensor measurements.

**Performance, frame pacing, CPU/GPU coordination, bandwidth efficiency, battery
drain, peak thermals and GEAS-enabled equivalence remain UNMEASURED/UNRESOLVED.**
No matched baseline was flashed. No improved/degraded performance conclusion or
permanent GEAS-removal recommendation follows from successful rendering alone.

## Whole-device and power matrix

| Area | Result on this exact installation and evidence boundary |
|---|---|
| Android / encrypted user0 | PASS: boot_completed=1, RUNNING_UNLOCKED persists |
| CE/DE | Parent storage accessible after encrypted boot; app-specific access/PIN-after-new-reboot not repeated |
| F2FS / inlinecrypt | PASS: actual mounted userdata mapper and mount options remain valid |
| KeyMint / recovery | Existing Android FBE boot passed; no new exhaustive KeyMint or TWRP session, since reboot is prohibited |
| dm-verity / AVB | PASS for installed matching pair and V/0 live mount; developer key on unlocked device, not OEM-signed universal support |
| Wi-Fi | PASS, owner-confirmed reconnect; connected, validated Wi-Fi and packet traffic also observed |
| Bluetooth | PASS, owner-confirmed reconnect; ON, zero service crash count; fresh pairing to a new accessory is not separately claimed |
| LTE / RMNET | PASS, owner-confirmed mobile-data traffic and short voice call; registration/interfaces observed; controlled LTE/5G handover/tethering not separately qualified |
| Display / GPU | Hardware compositor and bounded GLES/Vulkan graphics PASS; gaming/presentation quality needs owner measurements |
| Camera / video | PASS, owner-confirmed photo/video; service/preview evidence captured; not an exhaustive camera-mode matrix |
| Audio / microphone | PASS, owner-confirmed speaker and microphone recording plus voice call |
| Fingerprint | PASS, owner-confirmed unlock |
| USB / OTG | PASS, owner-confirmed OTG read/eject; ADB also works; OTG write endurance not separately claimed |
| NFC | Service presence is not an NFC transaction test; pending |
| Charging / thermal | USB-powered observation and bounded direct readings available; charging transitions and sustained-load comparison pending |
| UFS | Runtime link HIBERN8 / controller suspended observed; health/exception counters described below; no destructive test |
| Screen lock/wake | Framework on/off and unlock/lock events occur in logs; repeated owner-confirmed authentication cycles still pending |
| System suspend/resume | **NOT QUALIFIED**: current USB-connected boot reports success=0, fail=0, attempts=0; this is not a suspend PASS |

The owner supplied the new hands-on PASS report explicitly during this session;
these are user-observed outcomes, not claims that the agent heard audio or
visually inspected a camera file. Older .114 reports were not substituted. The
heavy benchmark report remains valid only for that exercise. The owner deferred
USB-disconnected idle until later; it remains pending without further prompting.

For valid suspend testing: finish native diagnostics, disconnect USB completely,
screen off for 10–15 minutes, manually wake/unlock, reconnect, then collect
suspend counters, wakeup reasons, dmesg, GPU counters, verity and pstore. Do not
force suspend with a sysfs write or treat USB-connected idle as the same test.
The prior .114 QCOM timekeeping warnings cannot be declared fixed merely because
this .115 boot has not attempted suspend.

UFS read-only telemetry reports critical_health=0, device exception count=0,
EOL/lifetime A/lifetime B raw values 0x01, gear5/two lanes, runtime suspended and
link HIBERN8. Reading `device_lvl_exception_id` returns EINVAL; this diagnostic
query is not successful and is not reported as healthy simply because the
counter is zero. Older .114 logs contain the same long-latency logging family
(`READ_10/WRITE_10 cost more than 100ms` with uptime-like values). Current retained
logs also contain it; those numbers are not taken as measured multi-minute I/O
latencies. No new reset/verity/F2FS fatal evidence accompanies these captures,
but the diagnostic-interface/latency interpretation is not full storage-stress
qualification. Missing debugfs nodes are recorded as unavailable, not PASS.

## ABI, security and offline gates

The complete actual enabled-population oracle was rerun against frozen compiled
outputs and the module bytes extracted back from the packaged filesystem:

```text
Enabled records:                 1,064
Active compatible:               1,029
Dormant:                            35
Hard imports:             60,267 / 60,267
CRC records:              61,337 / 61,337
Missing providers:                    0
CRC mismatches:                       0
Namespace failures:                   0
Active blockers:                      0
Oracle exit:                          0 / PASS
```

There are 1,067 physical files, including three intentionally blacklisted dormant
GEAS copies; the original inventory was 1,070. The six excluded locations represent
three canonical modules, not a blanket 1,032-module compatibility PASS. The frozen
vmlinux-first provider audit covers 54,407 edges with zero production mismatches.
There is no stock GPU provider substitution masking an incompatible replacement.

The full-stock negative audit remains FAIL with two missing getter edges for
the active/dormant GEAS-data copies. Its zero CRC mismatches does not make GEAS
compatible. That exclusion is explicit and diagnostic.

Previously executed, hash-preserved gates for these exact artifacts:

| Gate | Result |
|---|---|
| Common kernel build | PASS; not rebuilt during runtime qualification |
| Strict KMI symbol-list check | PASS |
| Common ABI generation | PASS |
| Enforced common GKI comparison | **FAIL, exit 4: unchanged 38 reviewed differences + six documented .115 representation differences** |
| Core vmlinux exports vs repaired baseline | All 10,163 match |
| GPU build/link/modpost | PASS |
| Shared stock GPU KCFI prefixes | 2,377 match, zero differing shared prefixes |
| Five expanded private/UAPI types vs previous updated-GPU build | Identical, rechecked by diff in this session; not complete stock-private proof because stock GPU lacks DWARF/BTF |
| Host regression suite | 195 PASS in the frozen build evidence; not a fresh runtime test |
| Focused real GPU client-contract host tests | 4/4 PASS rerun in this session, including the deliberately wrong mtrack-slot negative control |
| Filesystem metadata / FEC / offline AVB | PASS, preserved exact image evidence |
| Partition capacity / no resize | PASS; physical preflight/readback was already performed for this installation |
| Runtime KCFI/module-version/namespace failure | None observed in captured logs |

The OPlus/core task-flag repair and five GPU-import repairs remain intact.
No ABI reference, CRC record, symbol list, security enforcement or test acceptance
criterion was modified.

### Security scope

CVE-2026-80668 is **not an outstanding omitted conntrack fix in this candidate**.
The separately reviewed backport
`633ac4e5f56b64aa9cf80d5813c6fac82cf43d82` is an ancestor of this HEAD.
It preserves the public ABI while correcting expectation lifetime/GC/reference
handling. Provenance: upstream `b8b09dc2bf35a00d4e0556b5d6308c7b917ebda2`,
official 6.12 counterpart `02f7809a19fc88cfd9bc113a4bfe7add6305db39`.
The [Linux CNA record](https://raw.githubusercontent.com/CVEProject/cvelistV5/main/cves/2026/80xxx/CVE-2026-80668.json)
and [existing source review](../../security/coloros1700102-cve-2026-80668-source-review.md)
support that mapping. Prior isolated KASAN/NAT/lifetime tests are preserved;
this session did not rerun a race test against the live phone.

The subsequent five-CVE series and .115 fixes remain present. Their existing
[security report](../../security/coloros1700102-five-cve-backports-20261029.md)
is not an exhaustive current vulnerability clearance. This task is not a fresh
all-CVE audit. Qualcomm firmware/userspace defects and fixes in retained stock
modules are not credited as fixed by kernel ancestry or by changing five vendor
modules. Earlier OxygenOS .601 findings must not be assumed to apply unchanged
to ColorOS 17 binaries. **Security completeness is NOT ESTABLISHED.**

## Evidence and remaining release gates

Private, host-side evidence directory:
`/home/travis/Android/kernel-test-backups/coloros17-linux115-no-geas-extended-20261101-wJfjbC/`.
It includes complete captured logs/state, live streams, diagnostic source and
build provenance, per-run outputs/samples, stock disassemblies, warning module
hashes, source comparisons, current oracle outputs and preservation receipts.
Raw logs contain private device/network data and are not release assets.

The live ADB readers ended during USB reconfiguration at about 07:30:30 CST
(transport 20 became 21). Final captures after reconnection retain kernel
uptimes 2678.772472–4422.757877 s, overlapping the live stream's
1148.642926–4311.103581 s range, and logcat overlap is retained as well.
Consequently the native rendering intervals and final owner-test interval have
saved overlapping evidence; no claim is made that a disconnected ADB reader
continued streaming. ANSI colors were removed before analysis—plain text
matching without normalization can miss highlighted `WARNING:` lines.

The normalized joined scan after native testing began finds **one kernel WARN**
(the pre-existing schedinfo trace), zero panic/Oops/BUG, KCFI failure, lockup,
RCU-stall, hung-task, reported sanitizer/UAF, module-import/version or verity
failure signatures. Pstore is empty. This does not provide sanitizer coverage
or prove the absence of uninstrumented bugs. Persisted logcat still contains
the previously documented early UFS/qti diagnostic-child abort records, not
new PID1/user0 failures; they are not silently counted as a clean crash buffer.

The original frozen package, initial physical report and post-benchmark evidence
remain unchanged: 129 + 129 + 33 sealed artifact records were rechecked with
zero changes; the six sealed source-input/packaging files also match. Existing
dirty/untracked work is preserved. Earlier sealed reports are not rewritten to
retroactively imply newly completed tests.

Before stable-release approval:

1. Complete USB-disconnected idle/suspend/wake, then collect counters and any
   QCOM timekeeping/HFI/UFS events without changing the installed candidate.
2. Complete NFC, controlled LTE/5G handover and any additional peripheral modes
   required for the eventual release scope; the listed call/data/reconnect,
   camera/audio/fingerprint/OTG owner tests have passed.
3. Capture repeatable gaming scores/frame times and aligned thermal/power data;
   resolve or explicitly review the cooling-request oscillation/log-volume issue.
4. Decide whether loss of GEAS optimization is acceptable; no current measurement
   establishes performance/power equivalence with the .114 GEAS-enabled baseline.
5. Review the disclosed GKI exceptions and remaining security applicability gaps.
   Do not label either gate PASS without separate evidence.

**Recommendation: retain the unchanged canary on slot B for further authorized
testing; do not publish it as a stable unified release yet.**
