# Linux 6.12.115 boot-only canary — ColorOS 17.0.0.102

Canary/prerelease for the OnePlus 15 SM8850, physically tested on
CPH2745IN / OP611FL1 running `CPH2745_17.0.0.102(EX01)`, unlocked/ORANGE.
Not qualified for other firmware, regional variants, OxygenOS 16 or LineageOS.
Linux .114 remains the latest stable release.

## What is included

- `boot.img`: the exact physically tested 100,663,296-byte image.
- `boot.img.sha256`, `SHA256SUMS`, `manifest.json` and these instructions.
- Kernel: `6.12.115-android16-6-o-ga3b501fee3c3-4k`, Android 16 KMI6.
- Build source: `a3b501fee3c3c9e44579f46fd78e2282d05e29e8`.
- Boot SHA256: `7eba8fe7265c316265a274c893ae7886ba4598942862fb642d0b34fc61bd8e6e`.

No modules, DLKM images, vbmeta, vendor_boot, init_boot, DTBO or installer ZIP
are included. Keep the **original stock .102 module providers**, including
stock KGSL and all three GEAS modules. GEAS is enabled; there is no module
blacklist, built-in Bluetooth/PPP conversion or module-shadow loader patch.
The quarantined updated GPU implementation is not deployed by this release.
NTFS3 is not enabled.

The 231 individual .115 stable imports, 15 already-present fixes, core KABI
repairs and OPlus/core task-flag separation are retained. No kernel rebuild or
repackage was performed for this publication. The release commit changes only
documentation and offline boot-only scope tooling, not kernel implementation,
configuration, ABI references or tested payload bytes.

## Prerequisites — read before flashing

1. Match the exact tested device/firmware above; do not assume same-SoC support.
2. The bootloader must already be unlocked. Do not unlock or relock as part of
   these instructions; unlocking can erase userdata. This is not an OEM-signed
   locked-bootloader image.
3. Original .102 `vendor_dlkm`, `system_dlkm`, `vendor_boot` and their matching
   original parent AVB metadata must already be installed. Their firmware-file
   hashes are recorded in `manifest.json` for comparison, not as release assets.
4. **If an earlier modified-DLKM canary/development parent is installed, stop.**
   This boot image does not restore that pair. Restore the exact matching stock
   partition set through a separately verified recovery procedure first. An A/B
   slot switch is not a substitute for restoring shared vendor storage.
5. Finish any OTA/snapshot merge before testing. Preserve the current active
   boot image, a known-working matching rollback boot and working recovery
   access. Back up important data independently.
6. Leave the existing Magisk-patched `init_boot` and recovery images unchanged.
   Do not disable verity/verification, change AVB keys or rollback indexes,
   format metadata/userdata or resize partitions.

## Manual boot-only installation

Download all five assets into an otherwise empty directory. On the host:

```sh
sha256sum -c SHA256SUMS
sha256sum -c boot.img.sha256
adb shell getprop ro.build.display.id
adb shell getprop ro.boot.slot_suffix
adb reboot bootloader
fastboot devices
fastboot getvar current-slot
```

Confirm the firmware, intended device and **current active slot** yourself.
The tested device used slot B. Only if `current-slot` is `b`, write:

```sh
fastboot flash boot_b boot.img
fastboot reboot
```

If the current slot is A, use `boot_a` instead; that installation path has not
been physically repeated for these exact bytes. Do not write both slots or
change the active slot. No other partition is part of this installation.
Retain the host-side rollback image and recovery access throughout testing.
If boot/FBE fails, stop testing and restore the saved matching boot to the same
slot through bootloader/recovery; do not wipe or repeatedly reboot the failure.

After unlocking Android, verify the release string with `adb shell uname -r`,
user0 `RUNNING_UNLOCKED`, F2FS `/data` with inlinecrypt and normal module loading.
Full boot-partition readback was performed for the maintainer's tested device;
this manual installation recipe is not an automated hash-readback installer.

## Qualification and limitations

The complete original stock oracle passed: 1,070 module locations, 1,032 active
compatible / 38 dormant, 60,701/60,701 hard imports, 61,777/61,777 CRC records,
zero missing providers, CRC mismatches, namespace failures, active blockers or
production-provider mismatches. The stock GEAS-to-stock-KGSL interface naturally
matches `get_gpu_bw_data` CRC `0xd8bbf8a1`; no CRC was forced or edited.

The exact image passed full active-slot readback, Android startup, user0
RUNNING_UNLOCKED, CE/DE access, F2FS inlinecrypt, original vendor dm-verity root,
stock loaded-module identities, basic Wi-Fi traffic and a 240-second observation
window. No fatal kernel/module error was observed. Known stock WARN families
remain; this is not a clean-WARN result.

Common build, strict KMI symbol check and ABI generation evidence are preserved.
**Enforced GKI comparison remains FAIL: 38 prior reviewed downstream differences
plus six documented .115 representation differences.** No ABI reference was
updated. Module-runtime compatibility and this reference comparison are separate
results; this release does not claim full GKI compliance or security completeness.

Camera/video, calls, microphone, fingerprint, OTG, gaming, TWRP PIN decryption,
repeated boots and USB-disconnected suspend have not been requalified for this
exact boot-only/stock-DLKM image. Previous updated-DLKM hardware tests do not
transfer to it. Use only as an informed canary with a verified rollback path.

The validated outer boot AVB contract intentionally retains OS version `16`,
the qualified QTI fingerprint and security patch `2026-10-01` despite ColorOS's
Android 17 framework. It uses the same development/test boot signing key as
the prior qualified .114 boot; it is not signed by the OEM boot key. Flags and
rollback indexes remain zero. Do not repackage or "correct" these declarations
without a new FBE/KeyMint qualification cycle.
