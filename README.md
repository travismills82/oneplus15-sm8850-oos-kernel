# OnePlus 15 SM8850 kernel source — OxygenOS 16.0.9.400

Customized source for the OnePlus 15 (Canoe / SM8850), migrated as a
three-way update from the OxygenOS 16.0.8.300 OnePlus OSS release to the
OxygenOS 16.0.9.400 release. This is not a replacement of the customized
tree with a new OnePlus snapshot.

## Source provenance

| Source | Previous 16.0.8.300 baseline | Current 16.0.9.400 baseline |
| --- | --- | --- |
| `android_kernel_oneplus_sm8850` | `c88c1b3f75f7e88cd651608362ac7b95f11eb400` | `fc30e54174d254ff7f33622a9278e4435f6718d2` |
| `android_kernel_modules_and_devicetree_oneplus_sm8850` | `f45a4a587a0ba9107a5421a2dd978404f545164b` | `5ab2a689ff87d7d28c511f1762cf41c1b90d965a` |

The current OnePlus source release covers CPH2745, CPH2747, and CPH2749
`16.0.9.400(EX01)`, plus PLK110 `16.0.9.400(CN01)`. Both manifest projects
are pinned to the exact source commits above; their upstream branch remains
`oneplus/sm8850_b_16.0.0_oneplus_15` for provenance only.

The Android Common Kernel is intentionally retained, not replaced with the
OnePlus common tree:

- ACK commit: `b2a876903b495c444a94b16f50d1463ffe953957`
- ACK tag: `android16-6.12-2025-06_r53`
- Build target: `//soc-repo:canoe_perf_dist`

OnePlus still identifies both source releases with Qualcomm platform base
`AU_LINUX_KERNEL.PLATFORM.5.0.R1.00.00.00.099.064`. The release metadata's
recorded Qualcomm techpack revisions are unchanged; the source delta itself
contains six kernel-platform changes and 126 module/device-tree changes,
including charging, memory/ZRAM, display, graphics, camera, touch, and audio
areas.

## Local customizations retained

The migration preserves the repository's downstream work:

- stock-compatible `kernel_aarch64` output handling without OnePlus-only
  `vmlinux_oki`;
- `CONFIG_CIFS=y`, `CONFIG_NLS_UTF8=y`, and built-in NetFS behavior, with
  `netfs.ko` removed from the GKI module-output list;
- OxygenOS-compatible `system_dlkm` packaging and device-controlled empty
  `modules.load` policy;
- Kleaf/DDK dependency fixes and GKI ZRAM/ZSMALLOC ABI extraction handling;
- the narrow Bluetooth protected-module compatibility chain needed for stock
  `system_dlkm`, without bypassing signatures, MODVERSIONS, ABI, or KMI;
- an OxygenOS 16.0.9.400-specific trusted public module-signing certificate
  for the stock GKI `system_dlkm` modules, so their existing signatures can
  pass normal GKI protected-export enforcement;
- ADIOS as an available, non-default I/O scheduler;
- `CONFIG_WQ_POWER_EFFICIENT_DEFAULT=y`;
- current module-list, documentation, and release/installer support.

The official Oplus display subtree had been omitted by the upstream
`display-drivers/.gitignore` during the original local import. It is restored
from the official 16.0.8.300 source before applying the two corresponding
16.0.9.400 display fixes, so this tree now carries the complete official
target version instead of silently skipping those changes.

## Firmware-specific artifact policy

The historical, device-validated baseline is OxygenOS 16.0.8.300. Its boot
and `system_dlkm` artifacts remain historical only and must not be flashed on
16.0.9.400.

### 16.0.9.400 validation status

The current source baseline was built successfully with
`//soc-repo:canoe_perf_dist`. The strict
`//common:kernel_aarch64_abi`, `//common:kernel_aarch64_abi_kmi_symbol_checks`,
and `//common:kernel_aarch64_abi_diff` checks also passed. The resulting
kernel release is `6.12.23-android16-5-o-g7b1ff1b969ac-4k`.

On a rooted CPH2747 running `16.0.9.400(EX01)`, the following matched pair was
flashed and validated on slot `_b`:

- `boot.img` — 100,663,296 bytes, SHA-256
  `f78fd57d5041d0c8f875d7a182c759160f7d61b499e376210199bed2993fecc6`;
- `system_dlkm.flatten.ext4.img` — 88,514,560 bytes, SHA-256
  `6fa68a8fd4516438792fc571786b6aa9a8872235862cc0980092e6eb107d4002`.

The test retained the stock `vendor_boot`, `vendor_dlkm`, and `dtbo` images.
Android booted normally, retained Magisk root, and completed a controlled
reboot in 23 seconds. Wi-Fi connected on 6 GHz and passed a direct ping;
Bluetooth reached `OnState` with an active HCI controller and zero framework
crashes; and cellular data acquired an IPv4/IPv6 address on `rmnet_data2`.
With Wi-Fi disabled, direct-IP and DNS pings through `rmnet_data2` both passed
three of three packets. CIFS is available, ZRAM/ZSMALLOC load successfully,
and ADIOS is available without replacing the `mq-deadline` default scheduler.

This is a firmware-specific test configuration, not a production AVB claim.
The custom images were tested only after flashing the current slot's stock
`vbmeta` with fastboot's `--disable-verity --disable-verification` flags. Do
not reuse a 16.0.8.300 fingerprint, security-patch value, VBMeta image,
partition size, or TWRP package on 16.0.9.400.

Before flashing, back up the active slot and inspect its live dynamic-partition
layout in fastbootd or TWRP. The stock `system_dlkm` layout is a smaller EROFS
partition, while this matched image is ext4 and needs an 88,514,560-byte
logical partition. Confirm there is no active snapshot/merge state and enough
physical super-partition space before resizing. Leave `system_dlkm_oki`,
`vendor_boot`, `vendor_dlkm`, and `dtbo` untouched unless a separately
validated procedure specifically requires them.

The validated boot log had no panic, oops, MODVERSIONS mismatch, signature
failure, or unknown-symbol failure. A clean boot of the matched custom pair
was compared directly with a clean boot of the stock 16.0.9.400 `boot.img` and
`system_dlkm.img` pair on the same CPH2747 and slot. Both logs have the exact
same 14 `WARNING: CPU` entries and 15 call traces: two PMIC-arbiter warnings,
eight duplicate proc registrations, one duplicate sysfs group, one GIC warning,
one unbalanced IRQ enable, and one touch HBP warning. The UFS query-attribute
retry is also present once in both logs. These are therefore stock firmware
baseline warnings rather than a regression introduced by this custom kernel.

### Validated stock-`system_dlkm` mode

The following **separate**, device-validated configuration eliminates the
matching custom `system_dlkm` requirement on CPH2747
`16.0.9.400(EX01)`:

```text
custom boot.img
+ stock OOS 16.0.9.400 system_dlkm.img (EROFS)
+ stock vendor_boot
+ stock vendor_dlkm
+ stock dtbo
```

It was built from commit `5834c2d99dc74bc50bc83ad2b1e7ba8085eafdc4` and
reports kernel release `6.12.23-android16-5-o-g5834c2d99dc7-4k`. The
validated boot artifact is 100,663,296 bytes with SHA-256
`7fd8eaa32248a9dec04452b4c875f063117ae5d39779aadbe9b2d1df6802ff77`.
The stock EROFS `system_dlkm.img` is 14,131,200 bytes with SHA-256
`18f530dcb0e46dc81ede00e18ac4e9b39faf6564fb067f04af9a912d87fd6dd7`.

This mode adds only the public certificate that signed those stock GKI
modules to `CONFIG_SYSTEM_TRUSTED_KEYS`. The certificate serial is
`6E2DD27B61C8671D2C2AFB38E907FE98DC8F0230` and its SHA-256 fingerprint is
`F2:8D:BC:C6:00:85:B2:1A:3C:FF:13:42:48:28:97:FA:64:0B:46:88:47:47:31:47:83:4F:26:C4:FE:B2:DF:43`.
It does **not** disable module signatures, MODVERSIONS, CRC validation,
ABI/KMI checks, or protected-symbol enforcement. It allows legitimately
signed stock modules such as `6lowpan.ko`, `tipc.ko`, and `wwan.ko` to load
under that enforcement instead of rejecting their protected exports solely
because the prior custom kernel did not trust their signer.

Two Android boots were validated with the stock EROFS image mounted on the
active slot. The embedded eSIM was loaded and active on `subId=1`; LTE
registered HOME; `SETUP_DATA_CALL` returned `cause=NONE`; and `rmnet_data2`
received IPv4, IPv6, DNS, and gateway configuration. With Wi-Fi disabled,
bound direct-IP and DNS pings over `rmnet_data2` passed on both boots.
Wi-Fi reconnected and passed a bound ping after re-enabling it. Bluetooth
reached `OnState` with its Qualcomm `btpower`, `hci_uart`, `btqca`, and
`bluetooth` driver chain loaded; the framework reported zero Bluetooth
crashes. No protected-export, module-signature, unknown-symbol, or
MODVERSIONS/CRC mismatch was observed.

This compatibility path is deliberately scoped to the exact `.400` stock
signer above. Do not assume that a different OxygenOS build uses the same
module-signing certificate; extract and verify its stock signer before adding
another firmware-specific trusted key.

## Build environment

This source repository intentionally excludes generated `out/` artifacts,
nested Git metadata, and OnePlus-provided binary toolchains under
`kernel_platform/prebuilts/`. To reproduce a full build environment, initialize
a fresh checkout from [`oneplus_15.xml`](oneplus_15.xml), sync the pinned
projects, and build:

```bash
cd kernel_platform
tools/bazel build //soc-repo:canoe_perf_dist
```
