# ColorOS 17.0.0.102 — Linux 6.12.115 KernelSU + SUSFS canary

OnePlus 15 SM8850, physically tested on **CPH2745IN / OP611FL1**, firmware
`CPH2745_17.0.0.102(EX01)`, unlocked bootloader, ORANGE state, slot B.
This is a **canary prerelease**, not a stable promotion or universal OnePlus 15
image. Do not install on OxygenOS 16, LineageOS, another firmware or an
unqualified regional model. The previous .114 stable release is not replaced.

## Assets and exact identity

- `boot.img`: exact physically tested bytes; no release rebuild/repackage.
- `kernel-coloros-17.0.0.102-linux-6.12.115-kernelsu-susfs-twrp.zip`: boot-only
  custom-recovery installer containing that identical boot image.
- `KernelSU_v3.3.1-36-g28db74f6_32716-release.apk`: unmodified, officially signed
  matching KernelSU Manager; KernelSU UAPI 6.
- `SHA256SUMS`, `manifest.json` and this `README.md`.

| Artifact | SHA256 |
|---|---|
| boot.img | `3136cd36cfbb82be4423b46e0849527ea3bc26045ca925ddbd78611d591148f5` |
| Recovery ZIP | `b684e6051c19650da87a61ddbac4228c3a443d0cdfda9b3406fb76f8f1399532` |
| Official Manager APK | `aeb9a8eac801509d406ca8613a9bac17b389805aa8b9e976fb7421ada786b868` |

Kernel release: `6.12.115-android16-6-o-gf116d2166542-4k` (Android16 KMI6;
ColorOS framework is Android17). The suffix identifies the original build
parent. Committed kernel content is
`bb4992ba6b0a7814b4eec02d4201a175478d6e42`, common-source tree
`be76448748b0d005af30e20235c5447b1e878dd0`; it is byte-for-byte the tested
source snapshot. Source was committed afterward without rebuilding the payload.

Official KernelSU pin: `28db74f6f4692e97006ce9ebd8b19b9cc614b5b7`, version
32716, paired with SUSFS v2.3.0 Android16/6.12 pin
`68f62fae75ada3959f816841a3c64299383521ed`.
KernelSU/SUSFS are built into vmlinux. Uname spoofing support is enabled, but
no false release/version strings or hide rules are applied automatically.
Bootconfig spoofing, symbol hiding and the priority UID filter remain off.

No DLKM, vbmeta, vendor_boot, init_boot, DTBO, recovery or firmware image is
included or written. Keep the **original stock ColorOS .102** module stack,
including stock GPU/GEAS, Bluetooth and PPP. NTFS3 is not enabled.

## Prerequisites

1. Match the exact supported firmware/device above. The bootloader must already
   be unlocked. Do not unlock/relock, enroll keys or disable AVB as an install step.
2. Original .102 system_dlkm, vendor_dlkm, vendor_boot, DTBO and stock parent
   vbmeta must be installed. If coming from a modified-DLKM canary, restore the
   matched original set separately first; this installer does not do so.
3. Finish any OTA/snapshot merge, preserve a known-working boot backup and
   recovery access, and back up important personal files independently.
4. Do not alter existing init_boot, encryption policy, userdata or metadata.
   Magisk coexistence/migration has not been fully qualified by this release.

## Installation — choose one method

First download the assets and verify them on the host:

```sh
sha256sum -c SHA256SUMS
```

### TWRP / compatible custom recovery

Boot the already-installed trusted recovery. Decrypt /data, or mount durable
external storage, so the installer can save a complete boot rollback image.
Choose **Install**, select the recovery ZIP, and confirm. After success choose
**Reboot System**, without changing slots. Install the Manager APK from Android
separately; it is not installed by the recovery ZIP.

The installer requires exact stock partition hashes, a read-only match of the
installed firmware manifest, raw unlocked/ORANGE bootconfig, consistent active
slot/mappings, exact boot capacity, and idle snapshot metadata. It aborts if
these are unavailable or inconsistent. It backs up boot to durable storage,
verifies the backup, writes only active boot, and verifies the complete readback.
Write/readback failures trigger restoration of the saved boot. Backups remain
under `TWRP/kernel-flash-backups/` on the selected durable storage.

The exact payload was physically flashed/read back; **this newly packaged ZIP
has host-fixture validation only**, not a separate physical installer run.
Slot A has host simulation coverage, not physical qualification of these bytes.
If a guard rejects the device, do not remove it: check firmware, stock metadata,
snapshot state, recovery tools and module contract first.

### Manual boot-only fastboot

After independently verifying all prerequisites, save the active boot in
recovery and keep that backup on the host. Then:

```sh
adb shell getprop ro.build.display.id
adb shell getprop ro.boot.slot_suffix
adb reboot bootloader
fastboot devices
fastboot getvar current-slot
```

Only if the confirmed **current slot is B**, write:

```sh
fastboot flash boot_b boot.img
fastboot reboot
```

For an active A slot use boot_a instead; that path is not physically repeated
for this release. Never write both slots, switch slots or flash other partitions
as part of this boot-only installation. The manual command does not reproduce
the ZIP's complete preflight and backup checks: perform them independently.

### Matching KernelSU Manager

Install the provided official APK as an ordinary **in-place Android app update**.
Do not uninstall the existing Manager first. It has the same official signing
certificate and package name (`me.weishu.kernelsu`) as the previous version.
It resolves the Manager UAPI-5 versus kernel UAPI-6 mismatch; UAPI is not the
Android API level or KMI generation.

KernelSU is already built in (`CONFIG_KSU=y`). Do not use Direct install,
generic-kernel installation or LKM/init_boot patching just to update the app.
Manager/root-policy lifecycle remains a separate runtime qualification item.

APK provenance: [exact official workflow](https://github.com/tiann/KernelSU/actions/runs/41311436762),
final `manager` artifact. Certificate SHA256:
`c371061b19d8c7d7d6133c6a9bafe198fa944e50c1b31c9d8daa8d7f1fc2d2d6`.
[Complete upstream source](https://github.com/tiann/KernelSU/tree/28db74f6f4692e97006ce9ebd8b19b9cc614b5b7)
is available; this APK was not modified or re-signed locally.

## Rollback and recovery

If Android/FBE fails, stop and preserve failure evidence. From working recovery
or bootloader, restore your saved complete boot image to the same active boot
partition. For the tested B slot, after placing its saved image on the host:

```sh
fastboot flash boot_b saved-boot_b.img
fastboot reboot
```

Verify the saved image's SHA256 before restoring and read back the complete
partition afterward where recovery supports it. No DLKM/vbmeta restoration is
needed if this boot-only procedure was followed. Do not format userdata or
metadata, disable encryption, repeatedly boot a failing candidate or switch
slots as a substitute for rollback.

## Qualification and known limits

- Common build, strict KMI symbol-list check, ABI generation: PASS.
- Complete export CRC records and filtered ABI: unchanged from repaired .115.
- Twelve named BTF layouts: unchanged. Original 1,070 module locations checked:
  1,032 active compatible, 38 dormant; 60,701/60,701 hard imports and
  61,777/61,777 CRC records; zero missing providers, CRC/namespace failures,
  active blockers or production-provider mismatches.
- 202 existing plus 12 focused integration tests: PASS on final runs. Installer
  syntax and 19 regular-file host scenarios (including rollback): PASS.
- Physical full boot read-back, Android startup, user0 RUNNING_UNLOCKED,
  F2FS inlinecrypt, enforcing SELinux and live SUSFS feature queries: PASS.
  KernelSU reports version32716/UAPI6; SUSFS reports v2.3.0/GKI with uname support.
- **Enforced GKI comparison remains FAIL**, with the same 38 prior reviewed
  downstream differences plus six .115 representation differences. No ABI
  reference was updated; this is not full GKI compliance/security completeness.
- The same stock Oplus hbp frame_put recovery warning remains. Full hardware,
  gaming, calls, USB/OTG, repeated boot, root-manager lifecycle and unplugged
  suspend qualification have not been repeated for these bytes. Uname spoof
  values have not been applied/tested; only support availability was confirmed.

AVB packaging intentionally preserves the validated OS16/QTI fingerprint and
2026-10-01 security-patch contract despite the Android17 framework. Same qualified
development/test boot key, flags/index/location zero; not OEM locked-device
support. Do not repackage or "correct" these properties without a new FBE test.

See [source/integration evidence](https://github.com/travismills82/oneplus15-sm8850-oos-kernel/blob/coloros17.0.0.102-linux-6.12.115-kernelsu-susfs-canary/docs/features/kernelsu-susfs-coloros17.md)
and [public physical validation](https://github.com/travismills82/oneplus15-sm8850-oos-kernel/blob/coloros17.0.0.102-linux-6.12.115-kernelsu-susfs-canary/docs/validation/firmware-17.0.0.102/coloros1700102-linux115-kernelsu-susfs-20261101.md).
