# Linux 6.12.115 Droidspaces + KernelSU/SUSFS — boot-only canary

This is the **same boot.img currently running on the owner's OnePlus 15**,
published without a kernel rebuild or boot-image repackage. It adds built-in
Droidspaces container prerequisites, not the Droidspaces Android app or a Linux
rootfs. Install the userspace/runtime separately from the
[Droidspaces project](https://github.com/ravindu644/Droidspaces-OSS).

This is a **canary/prerelease**, not a fully qualified stable release. Initial
Android/FBE and the installed requirements checker passed. Full Linux distro
sessions, extended hardware tests and USB-disconnected suspend remain pending.
The new ZIP has host fixture tests only; its embedded boot bytes were physically
flashed and verified directly, not installed through this exact ZIP.

## Supported contract

- OnePlus 15 SM8850 / OP611FL1, **CPH2747_17.0.0.102(EX01)** only.
- Unlocked bootloader, raw verified-boot state ORANGE; exact original stock
  system_dlkm, vendor_dlkm, vbmeta, vendor_boot and DTBO hashes in `manifest.json`.
- Stock KGSL/GEAS, Bluetooth, PPP and other stock module providers are retained.
  This is **not** the older updated-Adreno/no-GEAS experimental stack.
- Existing **KernelSU-patched init_boot stays unchanged**. Do not install an old
  Magisk init_boot, replace DLKM, change vbmeta, disable verification or switch
  slots for this release. It supplies only boot.img and a boot-only installer.
- Other firmware/region variants are not qualified. If the installer rejects
  the device, investigate the mismatch; do not bypass its guards.

## Exact artifacts and source

| Item | Identity |
|---|---|
| Kernel | `6.12.115-android16-6-o-gd6552d3625e0-4k` |
| Kernel implementation commit | `4f960022774c379000abdb40d230ab563bc86dd7` |
| Common source tree | `6d074b67a27e4ccd1582be8178ab0bf8284870aa` |
| boot.img | 100,663,296 bytes |
| boot.img SHA256 | `0cb2cdff300f522beccb4cbdedb0549579161bb48e30f756ea020c7bf5e643cc` |
| TWRP ZIP SHA256 | `67858b4dc623ce4944fabcfc865faca2507ec08739d323814cb4b2991571c6cb` |
| Kernel Image SHA256 | `a952292f1067c8a4507cb60346e6de82d69104f6a22cbf0093ffe94bb69489a1` |

The build-parent suffix predates the separate source/validation commits. Those
commits preserve the exact tested common tree; the suffix was deliberately not
changed by rebuilding for publication. Release-only commits change packaging
and documentation, not the running kernel. All source commits remain separate.

## What is enabled

PID/IPC/user namespaces, SysV IPC, POSIX message queues, devtmpfs and xtables
ADDRTYPE support are added to the existing cgroups, seccomp, loop/ext4,
OverlayFS, FUSE, TUN, VETH, bridge and IPv4 NAT infrastructure. Task layout and
Rust Binder exports are preserved with reviewed KABI adaptations. Optional
MACVLAN, NAT66, PIDs-controller, nested Docker/Podman and hardware passthrough
are not qualified by this release. No NTFS3 change is included.

KernelSU 32716 / UAPI 6 and SUSFS v2.3.0 remain built in, including uname-spoof
support. No spoof values or hide rules are automatically configured. A matching
official API-6 Manager is available in the
[previous KernelSU release](https://github.com/travismills82/oneplus15-sm8850-oos-kernel/releases/tag/coloros17.0.0.102-linux-6.12.115-kernelsu-susfs-canary);
an ordinary APK update is separate from flashing boot/init_boot.

The audited Droidspaces guide labels SUSFS unsupported and requires disabling
**Hide sus mounts for all processes** if the user enabled it. No runtime hiding
setting, SELinux policy or security enforcement is changed by this release.
Test with the real uname first. Rootful containers are not VM-strength isolation:
use trusted ARM64 rootfs images and keep raw GPU/USB/block/Binder passthrough off
for the base test. See the [feature and ABI audit](../../features/droidspaces-coloros17.md).

## Installation — custom recovery

1. Back up the current active boot partition to durable host/external storage.
   Retain a known-working boot and recovery route. Verify `SHA256SUMS` for all
   downloaded files; the standalone sidecars also verify boot.img and the ZIP.
2. Enter a trusted compatible TWRP recovery on the **current slot**. Decrypt
   `/data` or mount durable USB storage so the installer can save a mandatory
   full boot backup. Do not wipe or format userdata/metadata.
3. Install `kernel-coloros-17.0.0.102-linux-6.12.115-droidspaces-TWRP.zip`.
   It verifies firmware, raw unlock/slot state, partition capacity, exact stock
   companion hashes, super mapping and inactive OTA/snapshot state. It backs up
   and writes **only active boot**, then verifies the full 96 MiB read-back.
   An interrupted/failed write attempts restoration and verifies the backup.
4. After an installation success message, choose **Reboot System** without
   changing slots. Preserve the backup under `TWRP/kernel-flash-backups/`.

The recovery must expose the exact `.102` logical partitions, `/metadata`,
bootconfig and required tools including `lpdump`. Missing evidence causes a
safe refusal, not permission to remove the checks. This exact ZIP is not yet
physically qualified; retain host recovery access before trying it.

## Alternative — bootloader fastboot

For experienced users who independently confirm the supported contract and no
pending OTA/snapshot: verify hashes, preserve a full active-boot backup, then
inspect `fastboot getvar current-slot`. Flash **only the confirmed active boot**:

```sh
# Example ONLY when current-slot is a:
fastboot flash boot_a boot.img
# When current-slot is b, use boot_b instead; do not run both commands.
fastboot reboot
```

Do not flash DLKM/vbmeta/init_boot/vendor_boot/recovery/DTBO, erase anything,
relock, or change slots. Manual fastboot has none of the ZIP's automated guards;
verify the written full partition from recovery/Android where available.

## Rollback

Restore the saved full `boot_<slot>.img` to the **same active boot partition**
from recovery or bootloader fastboot, verify its complete SHA256, then boot
normally. The previously tested KernelSU/SUSFS boot has SHA256
`3136cd36cfbb82be4423b46e0849527ea3bc26045ca925ddbd78611d591148f5`
and is in the prior release linked above. Keep current KernelSU-patched
init_boot and original stock companion partitions. A slot switch is not a
replacement for restoring the correct image. Never wipe encryption metadata.

## Validation and limitations

- Common build, strict symbol-list check and ABI generation: PASS.
- Stock oracle: 1,070 modules, 1,032 compatible active, 38 dormant;
  60,701/60,701 hard imports and 61,777/61,777 CRC records. Missing providers,
  CRC mismatches, namespaces, active blockers and production mismatches: zero.
- Shared KCFI IDs: 2,525 unchanged; 17 inspected layouts retain sizes/old offsets.
- **Enforced GKI comparison: FAIL**, 44 unchanged previous owners plus one
  reviewed C-private IPC-padding owner (45 total). No ABI reference or KMI list
  was updated. This is not full GKI compliance or a security-completeness claim.
- ARM64 VM tests: official pinned checker `requirements_met=1`, IPC lifecycle,
  namespace isolation, seccomp, cgroups, loop/ext4 persistence and OverlayFS PASS.
- Device: exact boot_a read-back, Android/user0 RUNNING_UNLOCKED, F2FS inlinecrypt,
  SELinux Enforcing, native KernelSU and installed v7.1.0 checker PASS on Nov 3.
  Nov 5 read-only revalidation found the identical image/config and stock
  companions, healthy unlocked user0 and approximately 42 hours uptime. Uptime
  is not a controlled 42-hour stress test.
- Known unchanged stock HBP warning remains disclosed. An eBPF/BHB policy warning
  is also recorded; its configuration is unchanged. No panic/Oops/BUG/KCFI/RCU,
  module-version or hung-task failure was observed in the startup health window.
- Full distro startup/restart, comprehensive peripherals, repeated boots and
  USB-disconnected deep suspend are **NOT TESTED** on this exact payload.

See the [physical canary record](../../validation/firmware-17.0.0.102/coloros1700102-droidspaces-canary-20261103.md),
[publication verification](../../validation/firmware-17.0.0.102/coloros1700102-droidspaces-release-20261105.md)
and [remaining functional plan](../../validation/firmware-17.0.0.102/coloros1700102-droidspaces-test-plan.md).
