# OnePlus 15 ColorOS 17.0.0.102 — Linux 6.12.114 KMI6 stable security release

Owner-authorized, firmware-bound boot/stability release. **Flash boot.img
only.** Keep the exact stock 17.0.0.102 DLKM and supporting partitions.
This is not an official Google GKI certification, universal OnePlus firmware
image, exhaustive CVE clearance, or all-features-qualified claim.

## Exact artifact

| Item | Value |
|---|---|
| Physically tested device | OnePlus 15 CPH2745IN / OP611FL1, Canoe / infiniti |
| Firmware | `CPH2745_17.0.0.102(EX01)` |
| Kernel | `6.12.114-android16-6-o-gb6bd4a40410d-4k` |
| Android / KMI | Android 16, generation 6 |
| Boot asset | `boot.img`, 100663296 bytes |
| Boot SHA256 | `1affe0774f9a483606d3ff44ac7e65214171f29ef439cae74ff49c6742a2fbb3` |
| Embedded Image SHA256 | `8ebe51e2a565a55886245793f572661e87c1a40bf5016bb79b84855370242456` |
| Content-equivalent source commit | `10b349653c7b3a3a1e55d2fd357ed7220190c1b9` |
| Common source tree | `818ee1b0ff65d6d475afeeba55bf4099cfcbfc22` |
| Runtime GNU build ID | `631855c02d3a5c50f57eedc2d640554570f807ee` |

The build was made before its compatibility/security changes were committed.
Its uname suffix therefore identifies original build parent `b6bd4a40410d`,
not the later content-equivalent source commit. Committed source blobs were
compared against the frozen tested source; the common trees are identical.
The physically tested payload was **not rebuilt or repackaged** for release.

## Changes and security scope

The 841 individual upstream interval commits are retained unchanged, followed
by 14 descriptive compatibility commits, the local conntrack compatibility
port, and six individually preserved security/dependency backports.

| Fix | Purpose | Local source commit |
|---|---|---|
| CVE-2026-80668 | Conntrack expectation GC/lifetime safety and coherent helper-reference handling | `633ac4e5f56b6` |
| Workqueue prerequisite | Pool-type selection without changing watchdog accounting | `2afd847ab8a5b` |
| CVE-2026-99103 | Unlink pending workqueue ownership before releasing its pool | `708bbd8e4dccb` |
| CVE-2026-99136 | dm-crypt final ordered error-status handling | `2a993e8f8f924` |
| CVE-2026-99139 | RCU-safe TCP_CONGESTION option/name access | `1fe72679857b8` |
| CVE-2026-99138 | RCU-safe TCP_CC_INFO callback access | `569c1e097a63b` |
| CVE-2026-99167 | Serialize TC action binding/reference/IDR lifetime updates | `10b349653c7b3` |

Upstream authorship and provenance are retained. No security fix, encryption,
SELinux, AVB, signature checking, MODVERSIONS, trimming, strict KMI, watchdog
protection, KernelSU/SUSFS or stock module ownership was disabled to pass.
NTFS3 remains disabled, as in the tested configuration. No version beyond
6.12.114 is introduced. Vendor-module and firmware vulnerabilities require
separate matched updates; an interface-oracle PASS does not patch stock code.

## Validation and disclosed limitations

- Production Canoe distribution and strict KMI: PASS.
- Common and Canoe ABI generation: PASS; zero new ABI delta from the prior
  working candidate. Complete config and Module.symvers are byte-identical.
- Enforced common GKI reference comparison: **FAIL/exit4, the same 38 reviewed
  downstream owners**. This is not relabelled PASS and the reference is not
  updated. The exhaustive historical addendum accounts for every owner.
- Untouched stock oracle: 1070 modules, 1032 active compatible, 38 dormant;
  60701/60701 hard imports, 61777/61777 CRC records, zero missing providers,
  CRC/namespace/protected/active blockers or production-provider mismatches.
- Stock signing policy is preserved: 103 trusted signed and 967 retained
  unsigned modules. This is not an all-modules-signed claim.
- Two complete isolated x86_64 KASAN VM runs passed bounded workqueue, TCP,
  encrypted-I/O/error propagation, TC lifecycle and conntrack regression tests.
  These do not prove every race interleaving or ARM64/hardware behavior.
- Exact boot_b full write/read-back: PASS. Live build ID matched the new
  frozen Image. Only active boot was changed; all seven protected hashes matched.
- Ten-minute physical canary through 610 s: Android completed, user0
  RUNNING_UNLOCKED, CE/DE accessible, F2FS read/write with inlinecrypt, Rust
  Binder live, same 691 loaded module names, Wi-Fi traffic, Bluetooth/NFC
  enabled, ZRAM and CPUFreq operational. No new crash, module/FBE failure,
  lockup, RCU stall, DRM lifetime fault or hung-task report was observed.

Existing stock HBP and schedinfo warnings, the unprivileged-eBPF/Spectre BHB
startup advisory and two firmware debug-script child SIGABRTs remain disclosed.
The schedinfo warning matches exact earlier .113/.114 offsets and module build
ID; it is not a new five-CVE warning. Two earliest SPMI warnings in a preceding
canary remain unclassified. This boot's saved logs begin at 1.79 s, so their
absence/resolution is **not** established. This is not a warning-free kernel.

On these exact new bytes, hands-on camera/audio/fingerprint/OTG, actual
Bluetooth pairing, NFC transactions, LTE Internet/VPN/tethering traffic,
TWRP decryption, repeated cold/warm boots and USB-disconnected deep suspend
were **not repeated**. Earlier base-kernel owner reports and preceding VPN
tests are not silently transferred to this new artifact. USB-connected idle
does not qualify deep suspend. The existing watchdog's task/wait-channel
filtering is not independent proof of Secure World/firmware transaction health.

## Installation

Use only the exact firmware above with its stock module partitions and an
already-unlocked bootloader. Do not unlock a bootloader as an incidental
installation step: unlocking may wipe data. Keep a complete tested backup of
your current active boot and verify the downloaded checksum before writing.

```sh
sha256sum -c SHA256SUMS
adb shell getprop ro.build.display.id
adb shell getprop ro.boot.slot_suffix
adb reboot bootloader
fastboot getvar current-slot
```

Confirm both slot observations agree. On **active B**, which was physically
tested, flash only boot_b:

```sh
fastboot flash boot_b boot.img
fastboot reboot
```

Use `boot_a` instead only when A is confirmed active. Do not switch slots or
flash both. Do not flash generic Canoe system_dlkm/vendor_dlkm outputs, recovery,
vendor_boot, init_boot, DTBO or VBMeta. Do not wipe metadata/userdata, disable
verification, force module loading or change encryption policy.

A recovery that supports raw boot-image installation may install this same
boot.img to the confirmed active boot partition, not recovery. No new recovery
ZIP is included or claimed physically tested in this release. Do not use the
older .112 ZIP: it embeds a different kernel.

After boot, confirm the expected release/build identity, user0 unlock, encrypted
storage, module loading and connectivity. If it fails, restore your own exact
verified pre-install boot to that same active partition; do not use a boot from
another firmware or alter supporting partitions.

## Records

- [Boot-only provenance manifest](boot-only-manifest.json)
- [Individual source commits](source-provenance.tsv)
- [New physical canary](../../validation/firmware-17.0.0.102/coloros1700102-6.12.114-five-cves-physical-20261030.md)
- [Build, module and KASAN results](../../validation/firmware-17.0.0.102/coloros1700102-6.12.114-five-cves-validation-20261029.md)
- [Five-CVE upstream provenance](../../security/coloros1700102-five-cve-backports-20261029.md)
- [Conntrack implementation/dependencies](../../security/coloros1700102-cve-2026-80668-source-review.md)
- [Historical exhaustive GKI/addendum](../../validation/firmware-17.0.0.102/coloros1700102-6.12.114-release-readiness-addendum-20261029.md)

Original failed attempts, earlier artifact identities and subsequent corrections
are preserved as dated records. They are not new installation instructions.
