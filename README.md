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
The staged `boot.img` (SHA-256
`3a85c055984df69d800f0d76b4423fd04b95fe58821c54e7c174744733bcc16f`) was
flashed alone to `boot_a` on an unlocked CPH2747, booted Android normally, and
allowed TWRP to map encrypted userdata and detect the device PIN.

Do not flash the source-generated `vendor_boot.img`, `dtbo.img`, or
`vendor_dlkm.img` for this test configuration. They do not match the stock
partition layouts; retain the matching stock images while testing this boot
image. The generated boot image uses the source tree's test AVB key, so it is
only suitable for an unlocked device.
