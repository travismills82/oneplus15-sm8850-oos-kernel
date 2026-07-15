# OnePlus 15 SM8850 kernel source — OxygenOS 16.0.8.300

Source-only snapshot of the OnePlus OSS release for the OnePlus 15 (Canoe / SM8850).

## Provenance

- Upstream manifest: [`oneplus_15.xml`](oneplus_15.xml) from the [OnePlusOSS kernel manifest](https://github.com/OnePlusOSS/kernel_manifest/tree/oneplus/sm8850)
- Device-source revision: `c88c1b3f75f7e88cd651608362ac7b95f11eb400`
- Device-source release: CPH2745, CPH2747, and CPH2749 `16.0.8.300(EX01)`; PLK110 `16.0.8.302(CN01)`
- Build target: `//soc-repo:canoe_perf_dist`

This repository contains the kernel, module, build, tool, and external source trees needed for source review and development. It intentionally excludes generated `out/` artifacts, nested Git metadata, and OnePlus-provided binary toolchains under `kernel_platform/prebuilts/`.

To reproduce a complete build environment, initialize a fresh checkout from the upstream manifest, sync its pinned projects, and use the build target above. The excluded prebuilt toolchains are declared by the upstream manifest.

## Verified OxygenOS 16.0.8.300 configuration

The stock CPH2747 `16.0.8.300(EX01)` kernel identifies as
`6.12.23-android16-5-o-gb2a876903b49-4k`. The manifest in this repository
therefore pins `kernel_platform/common` to Android Common Kernel commit
`b2a876903b495c444a94b16f50d1463ffe953957`
(`android16-6.12-2025-06_r53`) rather than the older OnePlus common branch.

The checked-in build adjustments make that upstream GKI compatible with the
OnePlus packaging tree: `kernel_aarch64` does not require the OnePlus-only
`vmlinux_oki` output, and the generated boot image carries the stock Android
16, fingerprint, and June 2026 security-patch AVB properties.

`//soc-repo:canoe_perf_dist` completed successfully with this configuration.

## Built-in CIFS and verified module pairing

CIFS is built into the kernel (`CONFIG_CIFS=y`), with UTF-8 NLS enabled. The
NetFS dependency is also built in, so neither `cifs.ko` nor `netfs.ko` is
required in `system_dlkm`.

The source boot image and its source-built GKI modules must be flashed as a
pair. Flashing the boot image alone leaves Android mounting the stock GKI
module image; the stock module signature is then rejected by the source
kernel, which causes Wi-Fi and Bluetooth to turn off.

On an unlocked CPH2747 running OxygenOS 16.0.8.300, the verified pair is:

- `boot.img` flashed to `boot_a`;
- `system_dlkm.flatten.ext4.img` flashed in fastbootd to `system_dlkm_a`;
- the matching stock `vbmeta_a` flashed with fastboot's
  `--disable-verity --disable-verification` flags for this custom-image test.

Fastbootd expands `system_dlkm_a` to the companion image's 88,518,656-byte
layout. Keep backups of the stock `system_dlkm_a` and `vbmeta_a` before doing
this. Do not flash the source-generated `vendor_boot.img`, `dtbo.img`, or
`vendor_dlkm.img`; they do not match the stock partition layouts.

This exact pairing booted Android successfully, mounted `/system_dlkm` as
source ext4, loaded `rfkill`, `6lowpan`, Bluetooth, and `cfg80211` without
signature errors, connected Wi-Fi, and kept Bluetooth connected. Runtime
verification also showed `nodev cifs` in `/proc/filesystems`, `cifs_mount` in
the kernel symbol table, and no `cifs` module loaded.
