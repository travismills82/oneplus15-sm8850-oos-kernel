# Linux 6.12.116 + CAKE boot-only physical canary — 2026-11-07

**Android/FBE and actual CAKE/FQ/FQ-CoDel functional checks PASS.**
This is not stable-release, congestion-performance or full power qualification.

The owner explicitly authorized flashing the frozen CAKE candidate. Only
active boot_a was written; the normal Android reboot did not change slots.
No rebuild or repackage occurred. Stock DLKM, GPU/GEAS, root image, AVB parent,
vendor_boot, DTBO and recovery were preserved.

## Identity and safety

- Physical firmware manifest: `CPH2747_17.0.0.102(EX01)`.
- Raw bootconfig: slot A, unlocked/ORANGE. Spoofable uname/getprop values are
  not used to establish kernel or bootloader identity.
- Actual `/proc/version`: `6.12.116-android16-6-o-gc2e2f95d3c3c-4k`.
- Build parent `c2e2f95d3c3c4ece99591320ebee1dc69a6f2ed4` plus the uncommitted
  single CAKE config line; common snapshot
  `481a38e99f82a476cdfa1ec1023cd9d4232dcaae`.
- Full 100,663,296-byte boot image/read-back SHA256:
  `f7cb7d973ce43de176a27fe8b9ad5e694b35e9d042d6e5875cf50c339f43ec16`.
- Live config SHA256:
  `da9bf33efeac03dd29e0cefb9fb2f6aaad272b8b2458886fb0e30c06840656c5`,
  byte-identical to the frozen config. All three scheduler options are `y`.
- Fresh full original boot_a backup:
  `0c39caaa3c2095b834f2687a10ddce8e445b32d3dfc9cca363edb708c5be288a`.

Firmware/slot/mapping/capacity, unlocked/ORANGE, stock companions, healthy
Android/FBE and empty OTA/snapshot state were verified before writing.
Offline AVB verification, guarded dry-run, fresh device and independent host
backup, guarded full write/read-back and independent host read-back passed.

Thirteen protected companion hashes compare unchanged before writing, after
writing and after reboot: boot_b, both init_boot/vbmeta/recovery/vendor_boot/
DTBO slots and active stock system_dlkm/vendor_dlkm. The actual root image is
**KernelSU-patched init_boot**, not Magisk. No key or security policy changes,
raw userdata/encryption-metadata writes, partition resize, wipe or slot change
occurred. Normal Android boot and test staging under /data are not described
as having made zero filesystem writes.

Full private evidence, guarded write procedure and rollback backup:
`/home/travis/Android/kernel-test-backups/coloros17-linux116-cake-canary-20261107-tCxC5H/`.
Its `boot_a-before.img` is the verified plain-.116 rollback. Restore only that
boot_a image through the existing recovery/fastboot route if needed, and verify
full read-back plus Android/user0/F2FS inlinecrypt. Do not switch slots or
replace init_boot/DLKM/AVB as a substitute. No rollback was required.

## Actual on-phone scheduler test

The exact VM-tested static `qdisc-smoke` binary was hash-verified on the phone:
`ffe7dbab625b945b10c6a73dab3165623e6c6ed2e177ac70cba7bd7f4ee4066c`.
It creates a private network namespace before any socket/interface operation,
uses only its loopback and disposes of the namespace on exit. The VM-only
PID1/guest-init script was not run on Android.

```text
QDISC_NEGATIVE_CONTROL=PASS
QDISC_fq=PASS (8 create/traffic/stats/delete cycles)
QDISC_fq_codel=PASS (8 create/traffic/stats/delete cycles)
QDISC_cake=PASS (8 create/traffic/stats/delete cycles)
```

Each cycle verifies 16 UDP datagrams, actual qdisc kind and nonzero packet/
byte counters, then deletes the temporary qdisc. An unavailable kind must be
rejected rather than silently substituted. Parent network namespace IDs and
all **159 production qdisc listing entries** are identical before/after.
No actual interface, default qdisc, netd policy, bandwidth, shaping rate,
offload or route is modified. This proves operational availability and
lifecycle, not a congestion/latency/bandwidth improvement.

## Other observed gates

| Gate | Result |
|---|---|
| Android boot_completed=1 | PASS |
| user0 RUNNING_UNLOCKED | PASS |
| /data F2FS + inlinecrypt | PASS |
| Exact live CAKE config | PASS |
| SELinux | Enforcing |
| KernelSU root / ksud UAPI6 | PASS |
| SUSFS v2.3.0 / existing features | PASS |
| Installed Droidspaces checker | PASS, requirements_met=1 |
| Wi-Fi traffic | PASS, 3/3 ping responses, no packet loss |
| Bluetooth watch connection | Observed |
| Hardware GLES | Adreno 840 / GLES3.2 observed |
| New full hardware, calls, OTG, TWRP, distro sessions, gaming | NOT TESTED |
| Controlled USB-disconnected idle/deep suspend | NOT TESTED |

The owner confirmed unlocked/USB-connected state; previous plain-.116 hardware
results are not automatically reassigned to these changed Image bytes.

## Diagnostics and disposition

The final log snapshot covers startup through 275.422582 kernel seconds,
passing the old 121/183-second points and the 240-second observation gate.
The state/boot/companion hash checks still pass at 276.03 seconds. The fixed
240-second live collector ends by its intended timeout, not a device failure.
The retained scan has no observed panic,
Oops, BUG, module symbol/CRC/load error, CFI failure, RCU stall, lockup, hung-task,
GPU fault, UFS I/O error or verity-error signature. Pstore is empty. The global
hung-task detector remains enabled with unchanged runtime timeout 60 seconds.

Two known warning roots recur: eBPF/BHB policy at 4.127432 seconds, and stock
HBP `frame_put` field-spanning memcpy at 4.879355 seconds. No warning is hidden
or claimed fixed. Existing QCOM suspend/timekeeping and controlled-power
limitations remain applicable; no new suspend qualification is inferred while
USB is connected.

Static [feature validation](../../features/traffic-control-coloros17.md)
remains tied to the exact flashed artifact: common build, strict symbol-list,
ABI generation, unchanged compiled exports/layouts/KCFI, full 1,070-module
stock contract and provider checks PASS. Enforced GKI remains **FAIL with 45
unchanged owners**; no ABI reference or acceptance criterion was changed.

The passing CAKE image remains installed. The plain-.116 boot backup is
preserved. No commit, push, main update, release or source-behavior change was
performed during flashing/testing. Extended physical and performance testing
remain necessary before making broader release or improvement claims.

The owner subsequently authorized committing and pushing the source to main.
The isolated CAKE config commit is
`737f8bbabba274fec2d38cb22df9f854daebfa9b`; its common tree is byte-identical
to the frozen source snapshot used for this exact image. Tests and this
validation/README record are committed separately. No image rebuild, release
publication or new physical-device action is implied by that source push.
