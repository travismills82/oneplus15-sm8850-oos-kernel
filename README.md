# OnePlus 15 SM8850 kernel source — OxygenOS 16.0.8.300

Source-only snapshot of the OnePlus OSS release for the OnePlus 15 (Canoe / SM8850).

## Provenance

- Upstream manifest: [`oneplus_15.xml`](oneplus_15.xml) from the [OnePlusOSS kernel manifest](https://github.com/OnePlusOSS/kernel_manifest/tree/oneplus/sm8850)
- Device-source revision: `c88c1b3f75f7e88cd651608362ac7b95f11eb400`
- Device-source release: CPH2745, CPH2747, and CPH2749 `16.0.8.300(EX01)`; PLK110 `16.0.8.302(CN01)`
- Build target: `//soc-repo:canoe_perf_dist`

This repository contains the kernel, module, build, tool, and external source trees needed for source review and development. It intentionally excludes generated `out/` artifacts, nested Git metadata, and OnePlus-provided binary toolchains under `kernel_platform/prebuilts/`.

To reproduce a complete build environment, initialize a fresh checkout from the upstream manifest, sync its pinned projects, and use the build target above. The excluded prebuilt toolchains are declared by the upstream manifest.
