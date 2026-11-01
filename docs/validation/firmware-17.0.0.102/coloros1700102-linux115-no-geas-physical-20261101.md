# ColorOS 17 Linux 6.12.115 no-GEAS initial physical canary

## Outcome

**INITIAL PHYSICAL CANARY PASS WITH DISCLOSED WARNINGS — NOT RELEASE QUALIFIED.**

The owner explicitly authorized flashing and rebooting on 2026-11-01. The exact
frozen no-GEAS candidate was installed on slot B and left running. No rollback,
rebuild, slot change, key enrollment, verification-disable operation, OTA,
partition resize, source commit, push or publication occurred.

Source parent: `a3b501fee3c3c9e44579f46fd78e2282d05e29e8`.
Branch: `experiment/coloros17-linux115-gpu-no-geas`.
Runtime: `6.12.115-android16-6-o-ga3b501fee3c3-4k`.
Firmware: `CPH2745_17.0.0.102(EX01)`, OnePlus 15 / CPH2745IN.

Private physical evidence:
`/home/travis/Android/kernel-test-backups/coloros17-linux115-no-geas-canary-20261101/`.
Frozen package and original offline report:
[no-GEAS package](../../driver-updates/coloros17-linux115-gpu-no-geas.md).

## Fresh preflight and coordinated writes

All 129 sealed offline artifacts were hash-verified before reboot. Fresh full
readbacks of the installed .114 boot, vendor_dlkm and vbmeta matched the staged
passing-canary rollback exactly. Android was healthy, user0 RUNNING_UNLOCKED,
and /data was F2FS with inlinecrypt before the experiment.

Android properties reported GREEN/locked, but the raw `/proc/bootconfig` showed
ORANGE/unlocked and the already accepted development public-key digest. The
owner confirmed Magisk property spoofing. Mutable properties were not used as
bootloader evidence and no trust-state change was attempted.

Fresh metadata showed slot B and a linear vendor_dlkm mapping: 302440 sectors,
super start 22396928. `/metadata/ota/state` was empty and its snapshots directory
had no records. Live vendor verity was V/0. The fresh capacity check passed
against actual captured super metadata; no resize was necessary. Allocated
`*-cow` LP entries alone were not confused with active snapshot records.

`adb reboot recovery` reached the existing `3.7.1_16-OnePlus_15` TWRP on B,
using its unchanged .114 recovery kernel. Both recovery partition hashes were
verified; neither recovery was flashed. Recovery independently exposed ADB,
the relevant block devices, metadata and the host restoration workflow. All
three candidate images and all three passing-canary rollback images were
staged in `/tmp` tmpfs. Vendor_dlkm was unmounted before writing; its linear
mapping, capacity, lack of holders and absence from mounts were checked.

The guarded dry run passed. The authorized write order was vendor_dlkm, boot B,
then parent vbmeta B last. Every write was followed by a complete independent
host-side readback and SHA256 comparison. No reboot occurred between stages.

| Written image | Partition | Exact installed/readback SHA256 |
|---|---|---|
| boot.img | boot_b | `931a69b0ce9ec911a144431a0f685bd2000c770900efdea8a49bb2cdef5bccd9` |
| vendor_dlkm.img | mapped vendor_dlkm_b shared backing | `0837cac0d888e862e4ad21eb7f2dbe58ae4f72bbb5534de2475adb783cf89804` |
| vbmeta.img | vbmeta_b | `54074cca444676d400f842411329a925340249bb3861abea132528d4eb97d11c` |

All three hashes were verified again from Android after boot. Nine protected
partitions remained byte-identical: boot A, vbmeta A, init_boot A/B, recovery
A/B, vendor_boot B, dtbo B and system_dlkm B. In particular the Magisk-patched
init_boot B remains
`d0fce3c1448150276e06bfca3d617605ba4f5a421d8122d18f90105b8fc2257b`.
The before/after 8 MiB super-metadata prefixes are byte-identical, SHA256
`81a320c930b085e20afc61cf9b208240a667d8aff0127d57465756f50d93181d`.
Switching slots is not a rollback for the shared vendor backing.

## Observed runtime results

The automated observation loop verified healthy Android, user0 and vendor
verity through uptime 243.41 seconds, passing the previous ~121s/~183s warning
windows. A subsequent detailed capture began at uptime 273.27 seconds.
Continuous kernel/logcat capture and post-boot hash checks were preserved.

| Check | Actual result |
|---|---|
| Correct .115 kernel and slot B | PASS |
| `sys.boot_completed=1` | PASS |
| User0 RUNNING_UNLOCKED | PASS |
| Userdata mapper, F2FS /data + inlinecrypt | PASS |
| Vendor dm-verity | PASS, V/0 and matching candidate root |
| Standard kernel blacklist received | PASS; exact three canonical GEAS names in /proc/cmdline |
| GEAS CPU/system/data loaded | NO; absent from /proc/modules and sysfs module directories |
| Updated msm_kgsl loaded | PASS; actual installed GPU SHA256 matches `d8711d3bb64a936e89c7df13996af3d7e4c6b71c83e217ea09e23d3abc965aa8` |
| GPU model / activity counters | Adreno840v2; active clock/busy counters observed |
| OpenGL ES / compositor | PASS for observed SurfaceFlinger hardware rendering: Qualcomm Adreno 840, OpenGL ES 3.2 V@0842.52 |
| Vulkan enumeration | PASS: Adreno 840, advertised device API integer 4210983; not a Vulkan rendering/stress test |
| Loaded module count | 688, consistent with omission of three GEAS providers from the prior 691-module canary |
| Wi-Fi connectivity / packet probe | Connected; three replies, zero packet loss on the default route |
| Cellular/RMNET | Provider loaded and cellular interfaces observed; independent cellular traffic/network-transition test NOT PERFORMED |
| Bluetooth pairing/reconnection | NOT TESTED in this session |
| Camera/video/audio/microphone/OTG/NFC/fingerprint | NOT TESTED in this session |
| Gaming/frame pacing/thermal/battery qualification | NOT TESTED |
| USB-disconnected suspend/resume | NOT TESTED |
| TWRP decryption with a .115 recovery kernel | NOT TESTED; recovery kernel was unchanged .114 |
| Panic/Oops/KCFI/RCU stall/hung-task/module-symbol/version failures | Zero matches in the captured initial window |
| Pstore fatal records | None present in the capture |

FBE/user0 success demonstrates encrypted Android startup for this artifact.
It does not independently qualify every KeyMint API, recovery kernel or app
authentication operation. No userdata, key or encryption-metadata operation
was used as a workaround.

## Firmware messages and non-clean-log findings

All four GPU firmware names initially produced direct-load `-2` messages.
Each subsequently loaded successfully through ueventd's `/vendor/firmware`
fallback: `gen80200_sqe.fw`, `gen80200_aqe.fw`, `gen80200_gmu.bin` and
`gen80200_zap.mbn`. The same successful fallback paths are present in the
earlier .114 paired canary. These messages are not evidence of a failed GPU
initialization; hardware GLES rendering and Vulkan enumeration also succeeded.

Six real early-boot WARN traces were captured and retained individually:

| Uptime | Reporter / originating module | Classification and evidence boundary |
|---:|---|---|
| 1.678749 | proc_register / oplus_bsp_sigkill_diagnosis | Duplicate /proc/oplus_mem; prior canary contains the same warning family, but exact racing registrant differs. Not proof that this exact instance is harmless. |
| 1.698961 | internal_create_group / oplus_sync_fence | Attribute `oplus_sync` has invalid 0666 permissions; kernel diagnoses and masks them. Not in the prior 11-warning inventory; NEEDS INVESTIGATION. |
| 1.714293 | proc_register / oplus_bsp_file_read_record | Duplicate proc registration; same prior warning family, different exact registrant. Retained for follow-up. |
| 1.753681 | gic_irq_domain_select / msm_geni_serial | Same site/probe family as the prior .114 canary. |
| 1.758404 | irq_set_irq_wake / msm_hfi_core | Same HFI SSR wake-IRQ family as the prior .114 canary. |
| 4.625418 | frame_put / oplus_hbp_core | Previously observed stock HBP fortify warning. |

The `oplus_sync_fence` warning is not a KGSL crash or a missing attribute-group
layout: its message explicitly identifies mode 0666. The relevant
`fs/sysfs/group.c` and `include/linux/sysfs.h` bytes are unchanged from the
qualified .114 source, and this stock module was not replaced. That does not
establish why the earlier capture lacked this warning, nor justify suppressing
it. No overall clean-WARN or zero-new-regressions assertion is made.

Logcat also contains two SIGABRT reports with process name `init`. The kernel
log identifies them as forked UFS debug exec and `qti-testscripts` service
children, not PID1 or an encrypted-user failure. Both fail their SELinux
`setexeccon` operation on the earlier passing .114 canary as well. These
pre-existing diagnostic failures remain disclosed; SELinux was not weakened.

There were no captured repeated GEAS HAL error/restart records after excluding
our own ADB diagnostic-command strings. This is a limited initial observation,
not proof of correct gaming behavior or of every closed userspace code path.

## Release boundary and next tests

The candidate remains installed for diagnostics on B, with the verified .114
boot/vendor_dlkm/vbmeta rollback trio retained on the host. No automatic
rollback was needed. All currently installed image bytes remain the exact
frozen canary; there was no post-test rebuild or repack.

Next: owner hands-on testing of the UI and GPU-intensive games, frame pacing,
camera/video/display synchronization, connectivity transitions and peripherals,
followed by comparable thermal/battery observations and a coordinated unplugged
suspend test. Keep the same image hashes throughout. Investigate the disclosed
sysfs/proc warnings without changing the installed test baseline silently.

The original GEAS source-contract blocker is intentionally bypassed only by
excluding GEAS deployment, not by falsifying its CRC or interface. Permanent
GEAS removal is not authorized. Enforced GKI remains **FAIL** with the unchanged
38 reviewed differences plus six .115 representation differences. Security
completeness, full GKI compliance and stable-release qualification are not
claimed. Nothing was pushed, merged or published.

Evidence entry points: `preflight-result.json`, `fresh-capacity.json`,
`transaction-plan.txt`, `write-result.json`, the three complete readback images,
`installed-verification.json`, `boot-observation.json`, `initial-analysis.json`,
`early-*`, `final-*`, full continuous logs and `warning-1.txt` through
`warning-6.txt` in the private evidence directory.
