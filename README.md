# OnePlus 15 SM8850 kernel source — OxygenOS 16.0.9.400

Customized source for the OnePlus 15 (Canoe / SM8850), migrated as a
three-way update from the OxygenOS 16.0.8.300 OnePlus OSS release to the
OxygenOS 16.0.9.400 release. This is not a replacement of the customized
tree with a new OnePlus snapshot.

## Source provenance

| Source | Previous 16.0.8.300 baseline | Current 16.0.9.400 baseline |
| --- | --- | --- |
| android_kernel_oneplus_sm8850 | c88c1b3f75f7e88cd651608362ac7b95f11eb400 | fc30e54174d254ff7f33622a9278e4435f6718d2 |
| android_kernel_modules_and_devicetree_oneplus_sm8850 | f45a4a587a0ba9107a5421a2dd978404f545164b | 5ab2a689ff87d7d28c511f1762cf41c1b90d965a |

The current OnePlus source release covers CPH2745, CPH2747, and CPH2749
16.0.9.400(EX01), plus PLK110 16.0.9.400(CN01). Both manifest projects are
pinned to the exact source commits above; their upstream branch remains
oneplus/sm8850_b_16.0.0_oneplus_15 for provenance only.

The Android Common Kernel is intentionally retained, not replaced with the
OnePlus common tree:

- ACK commit: b2a876903b495c444a94b16f50d1463ffe953957
- ACK tag: android16-6.12-2025-06_r53
- Build target: //soc-repo:canoe_perf_dist

## Local customizations retained

- Stock-compatible kernel_aarch64 output handling without OnePlus-only
  vmlinux_oki.
- CONFIG_CIFS=y, CONFIG_NLS_UTF8=y, and built-in NetFS behavior, with netfs.ko
  removed from the GKI module-output list.
- Kleaf/DDK dependency fixes and GKI ZRAM/ZSMALLOC ABI extraction handling.
- The narrow Bluetooth protected-module compatibility chain needed for stock
  system_dlkm, without bypassing signatures, MODVERSIONS, ABI, or KMI.
- An OxygenOS 16.0.9.400-specific trusted public module-signing certificate
  for stock GKI modules.
- ADIOS as an available, non-default I/O scheduler.
- CONFIG_WQ_POWER_EFFICIENT_DEFAULT=y.

The Oplus display subtree omitted by the original upstream import is restored
from the official 16.0.8.300 source before applying the corresponding
16.0.9.400 display fixes.

## Default supported configuration

~~~text
custom boot.img
+ stock OxygenOS 16.0.9.400 EROFS system_dlkm
+ stock vendor_boot
+ stock vendor_dlkm
+ stock dtbo
~~~

The kernel is not dependent on the generated custom
system_dlkm.flatten.ext4.img. It remains dependent on compatible GKI modules
supplied by the stock OxygenOS 16.0.9.400 system_dlkm partition.

Only the boot partition is replaced by the normal release. Do not flash,
resize, remap, or overwrite system_dlkm, system_dlkm_oki, vendor_boot,
vendor_dlkm, dtbo, or VBMeta for this configuration.

### Current boot-only validation

The verified kernel image was built from source commit
ee96bf9dec626eef39f2ba17f855339366e29f0f with:

~~~bash
cd kernel_platform
tools/bazel build //soc-repo:canoe_perf_dist
tools/bazel build \
  //common:kernel_aarch64_abi \
  //common:kernel_aarch64_abi_kmi_symbol_checks \
  //common:kernel_aarch64_abi_diff
~~~

Both commands passed. Strict ABI/KMI validation remains enabled, as do
CONFIG_MODULE_SIG, CONFIG_MODVERSIONS, CONFIG_GENDWARFKSYMS, symbol CRC
validation, and protected-module enforcement.

The fresh boot-only test artifact is:

- Kernel release: 6.12.23-android16-5-o-gee96bf9dec62-4k.
- boot.img: 100,663,296 bytes, SHA-256
  45847acea8eec0d9e5d9272a7a147235e3aaea6421fb72e4b6bf61e10a38219e.

The build still generates system_dlkm.flatten.ext4.img for ABI/KMI development
and recovery work. The fresh artifact was 88,510,464 bytes with SHA-256
e259b31d7d48dab1b1cf76dc1d5402c69c2a02cc45da85893519e1d86fefec87; it is
not a normal release asset.

The tested stock system_dlkm.img is the CPH2747 OxygenOS
16.0.9.400(EX01) EROFS image:

- Image bytes: 14,131,200.
- SHA-256:
  18f530dcb0e46dc81ede00e18ac4e9b39faf6564fb067f04af9a912d87fd6dd7.
- Active runtime mount: /dev/block/dm-11 on /system_dlkm type erofs.
- Active logical-partition capacity: 88,514,560 bytes, unchanged during this
  test; the first 14,131,200 bytes matched the stock image hash exactly.

The embedded trusted public certificate is the signer reported by stock GKI
modules such as 6lowpan.ko:

- Certificate serial:
  6E2DD27B61C8671D2C2AFB38E907FE98DC8F0230.
- SHA-256 fingerprint:
  F2:8D:BC:C6:00:85:B2:1A:3C:FF:13:42:48:28:97:FA:64:0B:46:88:47:47:31:47:83:4F:26:C4:FE:B2:DF:43.

It is set as system_trusted_key in kernel_platform/common/BUILD.bazel. This
trusts the stock modules without weakening module signatures, MODVERSIONS, CRC
checking, ABI/KMI checks, or protected exports.

Two clean Android boots, followed by a recovery boot and successful return to
Android, were completed on rooted CPH2747 16.0.9.400(EX01) using the fresh
custom boot image and untouched stock EROFS system_dlkm. Each Android boot
mounted the same stock EROFS payload and loaded 661 modules; 78 loaded module
names were directly identified from stock /system_dlkm.

No required module produced an unknown symbol, MODVERSIONS/CRC mismatch,
vermagic error, invalid module format, signature/key failure, namespace
failure, or protected-export failure. The stock wwan.ko module loaded under
normal protected-module enforcement, so no broad WWAN exception is needed.

Validated runtime results:

- Wi-Fi connected to a WPA3 6 GHz network at 6135 MHz and passed direct-IP and
  DNS traffic checks, including a settled five-packet test with no loss.
- Bluetooth reached OnState after both boots with zero framework crashes. The
  stock rfkill, bluetooth, btqca, btbcm, hci_uart, and pwrseq_core chain
  loaded; two existing bonded devices remained present.
- The eSIM subscription loaded as subId=1; LTE registered HOME;
  SETUP_DATA_CALL reported cause=NONE; and rmnet_data2 received IPv4, IPv6,
  DNS, and default routes. With Wi-Fi disabled, direct-IP and DNS traffic
  through rmnet_data2 passed a settled five-packet test with no loss.
- ZRAM/ZSMALLOC loaded and swap was active with LZ4 selected. CIFS appears in
  /proc/filesystems. ADIOS is available but mq-deadline remains selected by
  default on UFS block devices.
- Encrypted userdata, charging/USB, display/touch, GPU, camera HAL, audio
  paths, fingerprint HAL, NFC, and GNSS service state were present without a
  kernel fault. Screen sleep/wake completed while wired ADB kept the device
  awake; a fully unplugged deep-suspend test remains a separate physical test.

The observed qti-testscripts SIGABRT and associated SELinux context warning
are unchanged stock OxygenOS boot behavior, verified against the prior
known-good stock-DLKM boot. There was no new kernel panic, oops, or pstore
crash record.

The r2 TWRP installer was also live-validated in
TWRP 3.7.1_16-OnePlus_15. It verified the exact firmware manifest and stock
EROFS system_dlkm before creating a durable boot_b backup, flashing only
boot_b, and recording matching backup and readback hashes. Android then
returned in 22 seconds with stock EROFS system_dlkm, Wi-Fi, Bluetooth, and
Wi-Fi-disabled cellular traffic all working.

**PASS:** system_dlkm.flatten.ext4.img is not required for normal OOS
16.0.9.400 installation.

## Installation

Use only on an unlocked OnePlus 15 / CPH2747 running
OxygenOS 16.0.9.400(EX01) with its stock EROFS system_dlkm still installed.
The installer does not alter AVB; it is for the established unlocked test
configuration only.

### TWRP (recommended)

Install OnePlus15-OOS16.0.9.400-r2-TWRP.zip in a compatible TWRP recovery.
Before it writes anything, it verifies:

- CPH2747 / Canoe identity and the active slot.
- The exact active firmware manifest.
- Stock EROFS magic and the known .400 system_dlkm SHA-256.
- A durable backup location for boot_<active-slot>.

It then flashes only boot_<active-slot>, reads back exactly the image length,
and restores the backup automatically if writing or verification fails. The ZIP
never touches dynamic-partition metadata or any non-boot image.

### Fastboot

Back up the active boot image first, determine the active slot, and replace b
below with the slot reported by fastboot getvar current-slot:

~~~bash
fastboot getvar current-slot
fastboot flash boot_b boot.img
fastboot reboot
~~~

Do not flash the generated custom ext4 system_dlkm image for this normal
release.

## Legacy development/fallback mode

Release oos16.0.9.400-r1 and the older matched custom boot.img plus
system_dlkm.flatten.ext4.img procedure remain historical development/fallback
material only. That path requires a custom ext4 logical system_dlkm image and
can require dynamic-partition resizing. It is not included in, or required by,
the normal r2 boot-only release.

Keep the generated ext4 image available locally for ABI/KMI work, future ACK
updates, and diagnostic recovery. Do not attach it to normal GitHub releases.

## Build environment

This source repository intentionally excludes generated out/ artifacts, nested
Git metadata, and OnePlus-provided binary toolchains under
kernel_platform/prebuilts/. To reproduce a full build environment, initialize
a fresh checkout from oneplus_15.xml, sync the pinned projects, and build:

~~~bash
cd kernel_platform
tools/bazel build //soc-repo:canoe_perf_dist
~~~
