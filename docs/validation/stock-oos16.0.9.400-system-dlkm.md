# Stock OxygenOS 16.0.9.400 system_dlkm validation

Status: PASS for the normal boot-only release contract.

## Scope

- Device: OnePlus 15 CPH2747 / Canoe / SM8850.
- Firmware: OxygenOS 16.0.9.400(EX01).
- Kernel source commit: ee96bf9dec626eef39f2ba17f855339366e29f0f.
- Test configuration: custom boot.img plus untouched stock system_dlkm EROFS,
  stock vendor_boot, stock vendor_dlkm, and stock dtbo.

No custom system_dlkm image, system_dlkm resize, dynamic-partition metadata
change, vendor image, DTBO image, or VBMeta image was written for this test.

## Build and security contract

The normal Canoe distribution target and all strict ABI/KMI targets passed:

- //soc-repo:canoe_perf_dist
- //common:kernel_aarch64_abi
- //common:kernel_aarch64_abi_kmi_symbol_checks
- //common:kernel_aarch64_abi_diff

The tested kernel release was 6.12.23-android16-5-o-gee96bf9dec62-4k. The
100,663,296-byte boot image SHA-256 was
45847acea8eec0d9e5d9272a7a147235e3aaea6421fb72e4b6bf61e10a38219e.

The active stock system_dlkm payload remained EROFS and its first 14,131,200
bytes SHA-256 matched the official image:

18f530dcb0e46dc81ede00e18ac4e9b39faf6564fb067f04af9a912d87fd6dd7

The custom kernel embeds the public stock module-signing certificate with
serial 6E2DD27B61C8671D2C2AFB38E907FE98DC8F0230. Module signatures,
MODVERSIONS, symbol CRC validation, protected-module enforcement, and strict
ABI/KMI checks remained enabled.

## Runtime result

Two clean Android boots completed with /system_dlkm mounted as stock EROFS. A
subsequent recovery boot and return to Android also completed. Each Android
boot loaded 661 modules; 78 loaded module names were identified directly from
stock /system_dlkm.

The compatibility scan found no unknown symbol, symbol-version, vermagic,
invalid-module-format, signature/key, namespace, CRC, MODVERSIONS, or
protected-export failure.

- Wi-Fi connected at 6 GHz and passed direct-IP plus DNS traffic.
- Bluetooth reached OnState with zero framework crashes and the expected
  rfkill, bluetooth, btqca, btbcm, hci_uart, and pwrseq_core chain.
- eSIM subscription 1 registered on LTE. SETUP_DATA_CALL returned cause=NONE;
  rmnet_data2 received IPv4, IPv6, DNS, and routes. Direct-IP and DNS traffic
  passed with Wi-Fi disabled.
- ZRAM/ZSMALLOC were active, CIFS was available, and ADIOS was optional while
  mq-deadline remained the selected UFS scheduler.

The observed qti-testscripts SELinux-context SIGABRT sequence exactly matches
the prior known-good OxygenOS baseline. There was no new kernel panic, oops, or
pstore crash record.

## Boot-only TWRP installer validation

The r2 installer was built from the fresh boot image and its ZIP contained
only META-INF updater files, boot.img, kernel-info.txt, checksums.sha256, and
README.txt. All embedded checksums verified.

It was then run in TWRP 3.7.1_16-OnePlus_15 on slot b. The installer verified
CPH2747/Canoe identity, the exact active firmware manifest, EROFS magic, and
the stock system_dlkm hash. It created a durable 100,663,296-byte boot_b
backup, flashed only boot_b, and recorded matching readback and backup hashes.
After reboot, Android returned in 22 seconds with stock EROFS system_dlkm,
Wi-Fi, Bluetooth, and Wi-Fi-disabled cellular traffic all working.

## Release consequence

The generated system_dlkm.flatten.ext4.img remains a development and recovery
fallback artifact, but it is not required in the normal OOS 16.0.9.400
installation or release ZIP. Normal releases must preserve the stock
system_dlkm EROFS partition and flash only boot.
