# OnePlus 15 ColorOS 17.0.0.102 — Linux 6.12.112 KMI6 stable

This is the physically tested, boot-only Linux 6.12.112 release for the
OnePlus 15 `infiniti` / `canoe` running firmware 17.0.0.102(EX01).

## Exact tested combination

- Physically tested device: CPH2745IN / OP611FL1
- Firmware: `CPH2745_17.0.0.102(EX01)`
- Kernel: `6.12.112-android16-6-o-g3b4aea7fd691-4k`
- KMI generation: Android 16 generation 6
- Source commit containing the tested tree: `041fc63003508b49fd9f101e1e130c9ec96854ab`
- Boot asset: `boot.img`
- Boot size: 100,663,296 bytes
- Boot SHA-256: `723c6d5339f3ce312af5add567487ecde79d0c082aae19ea575bbf6a528897ff`
- Embedded Image SHA-256: `7b4bdb5c42b45dab73ecf9fd671e768dd50a03252c5ee135e7e790762b7b50a9`
- Recovery asset: `Kernel-ColorOS-17-17.0.0.102.zip`
- Recovery ZIP SHA-256: `d931af2fda503d49c47dd448081ca0a3212f3466dbb04e4ab5fbca9aff1be3ca`

The release artifact was built from the source changes before those changes
were committed, so `uname -r` retains the build parent identifier
`g3b4aea7fd691`. Commit `041fc6300350` records the content-equivalent tested
source tree. The physically tested image was not rebuilt for publication.

Do not install this release on another firmware version.

## What changed for firmware 17.0.0.102

- restored the stock OxygenOS/ColorOS 17 Rust provider representation;
- preserved the shipping opaque Tegra memory-controller KMI type graph used
  by stock `arm_smmu.ko` without changing runtime layouts;
- adopted the exact stock boot AVB fingerprint and October 2026 security-patch
  property contract;
- trusted the exact public certificate used by the protected 17.0.0.102 GKI
  module population while retaining signature enforcement;
- preserved Linux 6.12.112, Android 16 KMI generation 6, KernelSU/SUSFS, and
  the stock modular driver architecture.

No symbol CRC was forged, `Module.symvers` was not edited, and no module,
KMI, signature, AVB, or SELinux enforcement was weakened.

## Static validation

- production Canoe distribution: PASS;
- common strict KMI actions: PASS;
- Canoe ABI generation: PASS;
- exact stock-module oracle: PASS;
- compatible modules: 1,032; dormant modules: 38;
- hard imports: 60,701 / 60,701 resolved;
- all undefined ELF symbols: 60,707 / 60,707 resolved;
- MODVERSIONS records: 61,777 / 61,777 matched;
- missing providers, CRC mismatches, namespace failures, blockers: 0;
- boot header v4 and SHA256_RSA4096 AVB container: PASS.

The enforced common ABI command is structurally unavailable in this manifest
because its declared `external/libcap` and `external/libcap-ng` repositories
are absent. No host substitute was used and enforcement was not disabled.

## Physical qualification

The exact published boot image passed:

- active `boot_b` write and complete 96 MiB read-back verification;
- Android boot and `sys.boot_completed=1`;
- encrypted user0 `RUNNING_UNLOCKED`;
- F2FS `/data` with `inlinecrypt`;
- 691 loaded stock modules, including Rust Binder, Bluetooth, WLAN, PPP, and
  RFKILL providers;
- Wi-Fi, Bluetooth service, LTE data, NFC service toggle, USB/charging;
- camera preview, 3072x4096 JPEG capture, 1080p HEVC video capture;
- stereo 48 kHz microphone recording and media/audio playback;
- real fingerprint sensor authentication and unlock;
- ZRAM, CPUFreq, display composition, and fatal kernel/module scans.

No panic, Oops, lockup, RCU stall, hung-task report, module signature failure,
CRC disagreement, unknown symbol, or relevant service crash was observed.

Three warnings originate in unchanged stock vendor modules: the Oplus HBP
one-element-array fortify warning, schedinfo constructor plus zero-allocation
warning, and repeated `clk_qcom` suspend tracepoint registration. They caused
no observed functional failure and are documented rather than suppressed.

An external NFC tag, an actual Bluetooth peripheral session,
USB-disconnected deep suspend, TWRP decryption, and repeated cold/warm boots
were not exercised during this release session.

## Installation

### Fastboot (`boot.img`)

Back up the active boot partition first. Confirm the current slot:

```sh
adb shell getprop ro.boot.slot_suffix
fastboot getvar current-slot
```

Verify the download:

```sh
sha256sum boot.img
```

For slot B, which was the physically tested slot:

```sh
fastboot flash boot_b boot.img
fastboot reboot
```

Use `boot_a` instead only when slot A is confirmed active.

### Custom recovery

Install `Kernel-ColorOS-17-17.0.0.102.zip` from TWRP, OrangeFox, or PBRP,
then reboot System without switching slots. The installer:

- verifies the `infiniti` / `canoe` family and firmware 17.0.0.102(EX01);
- verifies the exact stock active-slot `system_dlkm` EROFS payload;
- refuses an active OTA snapshot/merge or an incompatible module contract;
- makes and verifies a complete durable backup of active boot;
- writes only active `boot_a` or `boot_b`;
- verifies the complete post-write partition and restores the backup if that
  verification fails.

The ZIP contains the exact tested `boot.img`, but the ZIP installer itself was
statically validated rather than flashed during this qualification session.

## Boot-only scope

Keep stock 17.0.0.102 `system_dlkm`, `vendor_dlkm`, `vendor_boot`, `init_boot`,
DTBO, VBMeta, modem/radio, persist, metadata, and userdata. Do not flash the
generic Canoe DLKM outputs with this release.

Full evidence:

- [Compatibility report](https://github.com/travismills82/oneplus15-sm8850-oos-kernel/blob/main/docs/validation/firmware-17.0.0.102/oos1700102-6.12.112-kmi6-compatibility.md)
- [Physical qualification](https://github.com/travismills82/oneplus15-sm8850-oos-kernel/blob/main/docs/validation/firmware-17.0.0.102/oos1700102-6.12.112-module-signing-canary-20261026.md)
