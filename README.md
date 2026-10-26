# OnePlus 15 SM8850 kernel

Authorized Android/Linux kernel development for the OnePlus 15
(`infiniti` / Canoe / Qualcomm SM8850), Android 16, and firmware-specific
OxygenOS/ColorOS 16 and 17 releases.

Kernel releases are firmware-specific. Never cross-flash a boot image merely
because it targets the same phone family; each OxygenOS build has its own
kernel, module, FBE/user0, and boot-container contract.

## Current releases

| Firmware | Device physically tested | Kernel | Status | Release tag |
|---|---|---|---|---|
| ColorOS/OxygenOS 17.0.0.102(EX01) | CPH2745IN / OP611FL1 | `6.12.112-android16-6-o-g3b4aea7fd691-4k` | Stable boot-only release | `coloros17.0.0.102-linux-6.12.112-kmi6` |
| OxygenOS 16.0.10.601(EX01) | CPH2749 / OP611FL1 | `6.12.112-android16-6-o-g8dd9cb4790b7-4k` | Watchdog-corrected boot/stability release | `oos16.0.10.601-linux-6.12.112-kmi6-watchdog` |
| OxygenOS 16.0.10.601(EX01) | CPH2749 / OP611FL1 | `6.12.95-android16-6-o-g02714090683b-4k` | Boot/stability release | `oos16.0.10.601-linux-6.12.95-kmi6-canary` |
| OxygenOS 16.0.10.601(EX01) | CPH2749 / OP611FL1 | `6.12.89-android16-6-o-g5d9b99c7d104-4k` | Fully qualified rollback baseline | `oos16.0.10.601-linux-6.12.89-kmi6` |
| OxygenOS 16.0.9.400(EX01) | CPH2749 / OP611FL1 | `6.12.52-android16-5-o-gfa9fe14314c7-4k` | Fully qualified | `oos16.0.9.400-ack-6.12.52` |
| OxygenOS 16.0.10.500(EX01) | CPH2747 | `6.12.37-android16-5-o-g57b52d3bb803-4k` | Fully qualified | `oos16.0.10.500-ack-6.12.37` |

Use only the release matching the installed firmware.

## Linux 6.12.112 stable for ColorOS/OxygenOS 17.0.0.102

| Field | Tested value |
|---|---|
| Device family | OnePlus 15 / `infiniti` / `canoe` |
| Physical device | CPH2745IN / OP611FL1 |
| Firmware | `CPH2745_17.0.0.102(EX01)` |
| Kernel | `6.12.112-android16-6-o-g3b4aea7fd691-4k` |
| KMI generation | Android 16 generation 6 |
| Source commit containing the tested tree | `041fc63003508b49fd9f101e1e130c9ec96854ab` |
| `boot.img` size | 100,663,296 bytes |
| `boot.img` SHA-256 | `723c6d5339f3ce312af5add567487ecde79d0c082aae19ea575bbf6a528897ff` |
| Embedded Image SHA-256 | `7b4bdb5c42b45dab73ecf9fd671e768dd50a03252c5ee135e7e790762b7b50a9` |
| TWRP ZIP | `Kernel-ColorOS-17-17.0.0.102.zip` |
| TWRP ZIP SHA-256 | `d931af2fda503d49c47dd448081ca0a3212f3466dbb04e4ab5fbca9aff1be3ca` |

This is a boot-only release. It retains the exact stock 17.0.0.102
`system_dlkm`, `vendor_dlkm`, `vendor_boot`, `init_boot`, DTBO, VBMeta,
metadata, and userdata. The complete stock-module oracle passed with 1,032
compatible modules, 38 dormant modules, 60,701 hard imports resolved, and
61,777 MODVERSIONS records matched. Missing providers, CRC mismatches,
namespace failures, and active blockers were all zero.

The exact published `boot.img` passed a full active-slot write/read-back,
Android boot, encrypted user0 `RUNNING_UNLOCKED`, F2FS `/data` with
`inlinecrypt`, 691 stock loaded modules, Wi-Fi, Bluetooth, LTE data, NFC,
USB/charging, camera photo and video capture, microphone recording, media
playback, fingerprint unlock, ZRAM, CPUFreq, and fatal kernel/module scans.

Three warnings rooted in unchanged stock vendor modules are documented: the
Oplus HBP one-element-array fortify warning, schedinfo constructor plus
zero-allocation warning, and repeated `clk_qcom` suspend-trace registration.
No functional failure, panic, Oops, lockup, RCU stall, hung task, or module
loader failure was observed. An external NFC tag, Bluetooth peripheral
session, USB-disconnected deep suspend, TWRP decryption, and repeated
cold/warm boots were not exercised in this release session.

See:

- [17.0.0.102 compatibility report](docs/validation/firmware-17.0.0.102/oos1700102-6.12.112-kmi6-compatibility.md)
- [17.0.0.102 physical qualification](docs/validation/firmware-17.0.0.102/oos1700102-6.12.112-module-signing-canary-20261026.md)
- [17.0.0.102 release notes](docs/releases/coloros17.0.0.102-linux-6.12.112-kmi6/release-notes.md)

## Linux 6.12.112 watchdog release for OxygenOS 16.0.10.601

| Field | Tested value |
|---|---|
| Device family | OnePlus 15 / `infiniti` / `canoe` |
| Physical device | CPH2749 / OP611FL1 |
| Firmware | `CPH2749_16.0.10.601(EX01)` |
| Kernel | `6.12.112-android16-6-o-g8dd9cb4790b7-4k` |
| KMI generation | Android 16 generation 6 |
| Source commit containing the tested tree | `1cd6164a81e631e33a3d8181c357630607f5fd65` |
| `boot.img` size | 100,663,296 bytes |
| `boot.img` SHA-256 | `a44ee825f3eb58ce84e7c2d24ebaeccce4669695f9c57bc0904184ffe10d4f6f` |
| Embedded Image SHA-256 | `6afffa074630cdfa52b50ed7881e0ef85c82731186ee034637671ba080ee7531` |
| TWRP ZIP SHA-256 | `9e946551c2cf196e2a9cf685ff58a64bf87e87baf2af9d73d1c612a537312b5e` |

This release preserves the boot-only architecture with the untouched
OxygenOS 16.0.10.601 `system_dlkm`, `vendor_dlkm`, `vendor_boot`, `init_boot`,
DTBO, VBMeta, metadata, and userdata. The full `.601` oracle passed all 1,021
stock modules: 985 active compatible, 36 dormant, 56,594 hard imports and
57,621 MODVERSIONS records matched, with zero missing providers, CRC
mismatches, namespace failures, or active blockers.

The exact boot image passed full active-boot write/read-back, Android boot,
user0 `RUNNING_UNLOCKED`, F2FS `/data` with `inlinecrypt`, Wi-Fi, Bluetooth,
NFC, LTE/RMNET, USB/ADB, ZRAM, CPUFreq, and fatal kernel/module scans. A
692-second monitored canary crossed every prior watchdog report window with
zero false-positive ADCI, ZRAM, HFI, or hardware-fence idle reports. Exact
task-name and wait-channel matching leaves active subsystem stalls visible.

USB-disconnected deep suspend and direct camera/audio/fingerprint operation
were not exercised during this specific watchdog canary. See:

- [6.12.112 watchdog validation](docs/validation/firmware-16.0.10.601/oos1610601-linux-6.12.112-watchdog-validation-2026-10-08.md)
- [6.12.112 release notes](docs/releases/oos16.0.10.601-linux-6.12.112-kmi6-watchdog/release-notes.md)

## Linux 6.12.95 boot/stability release for OxygenOS 16.0.10.601

| Field | Tested value |
|---|---|
| Device family | OnePlus 15 / `infiniti` / `canoe` |
| Physical device | CPH2749 / OP611FL1 |
| Firmware | `CPH2749_16.0.10.601(EX01)` |
| Kernel | `6.12.95-android16-6-o-g02714090683b-4k` |
| KMI generation | Android 16 generation 6 |
| Tested runtime source | `02714090683b0c04a1cc5de357c579b18dc814dd` |
| `boot.img` size | 100,663,296 bytes |
| `boot.img` SHA-256 | `f4c64ffb73506b740aee22fbc1f2f1d573a4972784bf598f29a4c012f09c81f2` |
| Embedded Image SHA-256 | `c0fc3d7b443731bcc2bc48826269f438d9f3bb5e3f608038e4cfdf13c0e9ae2d` |
| TWRP ZIP SHA-256 | `1a91b0bc409fd7aa68f3ac3bd524dfcd4b5bd5ab194f051a252500a3e9148ca8` |

This boot-only canary preserves the exact OxygenOS 16.0.10.601 stock DLKM,
`vendor_boot`, DTBO, VBMeta, metadata, and userdata payloads. All 1,021 stock
modules passed the full ELF and MODVERSIONS oracle: 985 active compatible, 36
dormant, 56,594 hard imports resolved, 57,621 version records matched, and
zero missing providers, CRC mismatches, namespace failures, or active
blockers.

The exact release image passed a full `boot_a` write/read-back, TWRP
metadata/PIN decryption, userdata mapper creation, F2FS `/data` with
`inlinecrypt`, CE/DE access, two clean Android boots, user0
`RUNNING_UNLOCKED`, Rust Binder, Wi-Fi 6 GHz/WPA3-SAE, cellular data,
Bluetooth, NFC, camera launch, USB/ADB, charging, ZRAM, and targeted fatal
kernel/module scans.

This is published as a normal release. It is not yet promoted as the fully
qualified rollback baseline because USB-disconnected deep suspend and direct
owner confirmation of audio playback and fingerprint authentication were not
completed during the 6.12.95 session. The Oplus watchdog reports the same
`adci_thread`, `zram_comp`, and `hfi_core_dbg_cl` waits already observed on the
known-good 6.12.91 baseline; no 6.12.95-specific panic, Oops, BUG, lockup, RCU
stall, module-loader failure, or DRM/GEM warning was observed.

See:

- [6.12.95 boot/stability validation](docs/validation/firmware-16.0.10.601/oos1610601-linux-6.12.95-boot-canary-2026-10-04.md)
- [6.12.95 release notes](docs/releases/oos16.0.10.601-linux-6.12.95-kmi6-canary/release-notes.md)

## Linux 6.12.89 qualified rollback baseline for OxygenOS 16.0.10.601

| Field | Qualified value |
|---|---|
| Device family | OnePlus 15 / `infiniti` / `canoe` |
| Physical device | CPH2749 / OP611FL1 |
| Firmware | `CPH2749_16.0.10.601(EX01)` |
| Kernel | `6.12.89-android16-6-o-g5d9b99c7d104-4k` |
| KMI generation | Android 16 generation 6 |
| Tested runtime source | `5d9b99c7d1047fe6d392dc7ab5e6129d3981b5c8` |
| `boot.img` size | 100,663,296 bytes |
| `boot.img` SHA-256 | `c704c189e5075e6104e53dfbbb8b5d64d289f9a9a04464f4c14d5a604ec6d895` |
| TWRP ZIP SHA-256 | `3cc6509178b713eb397d0400b8c9d366050c9195019e244abf5bfe8452161de7` |

This release is boot-only and retains the exact OxygenOS 16.0.10.601 stock
module and vendor partitions. The repaired stock-module oracle checks every
ELF undefined symbol as well as MODVERSIONS records. It passed 1,021 stock
modules, 56,600 ELF imports, and 57,621 version records with zero missing
providers, CRC mismatches, namespace failures, or active blockers. The stock
`rust_binder.ko` loaded successfully with all 225 raw undefined ELF records
accounted for.

The exact boot image passed full-partition write/read-back, Android boot,
existing encrypted user0 `RUNNING_UNLOCKED`, F2FS `/data` with `inlinecrypt`,
WPA3 WLAN, LTE/RMNET initialization, Bluetooth, USB/ADB, and fatal
kernel/module scans. TWRP, audio, camera, fingerprint, and NFC operation were
confirmed on the physical device. Supporting DLKM, `vendor_boot`, DTBO,
VBMeta, metadata, and userdata partitions were not written.

See:

- [6.12.89 physical qualification](docs/validation/firmware-16.0.10.601/oos1610601-linux-6.12.89-physical-validation-2026-09-28.md)
- [Rust Binder KMI6 compatibility audit](docs/validation/stock-modules/oos1610601-rust-binder-kmi6-compatibility.md)
- [6.12.89 release notes](docs/releases/oos16.0.10.601-linux-6.12.89-kmi6/release-notes.md)

## ACK 6.12.52 stable for OxygenOS 16.0.9.400

| Field | Qualified value |
|---|---|
| Device family | OnePlus 15 / `infiniti` / `canoe` |
| Physical device | CPH2749 / OP611FL1 |
| Firmware | `CPH2749_16.0.9.400(EX01)` |
| Kernel | `6.12.52-android16-5-o-gfa9fe14314c7-4k` |
| KMI generation | Android 16 generation 5 |
| Tested source | `fa9fe14314c7b043f7d16f4b34397ff7d6eef68b` |
| Linear source equivalent | `b3d0dbb64c77c39ae497b9a0b3a8362821cc9259` |
| `boot.img` size | 100,663,296 bytes |
| `boot.img` SHA-256 | `1aa1de79d79cfa5b184d2df4fb25fa56811f615d8420d5faf6c6dac29d49e41a` |
| TWRP ZIP SHA-256 | `8139c82880a3449860abba4a67d491a502f0591cdbc677671c99f13c229b22b0` |

The GitHub release contains the exact physically tested `boot.img` and a
boot-only TWRP/OrangeFox/PBRP installer containing that same image. It does
not contain replacement DLKM, vendor_boot, DTBO, VBMeta, or dynamic-partition
images.

The kernel passed TWRP metadata/PIN decryption, two clean Android boots,
existing encrypted user0 `RUNNING_UNLOCKED`, F2FS `/data` with `inlinecrypt`,
WPA3-SAE WLAN, cellular/RMNET, Bluetooth, USB/ADB, the retained stock-module
contract, strict KMI, common ABI, Canoe ABI generation, SHA-512 module signing,
and boot-container/AVB checks.

The exact OxygenOS 16.0.9.400 supporting partitions remain stock. The
installer checks the stock EROFS `system_dlkm` payload with SHA-256:

```text
18f530dcb0e46dc81ede00e18ac4e9b39faf6564fb067f04af9a912d87fd6dd7
```

See:

- [6.12.52 physical qualification](docs/validation/ack-modernization/oos1609400-ack-6.12.52-physical-validation-2026-09-20.md)
- [6.12.40–6.12.52 linear ACK history](docs/validation/ack-modernization/ack-6.12.40-to-6.12.52-linear-history-reconciliation.md)
- [6.12.52 release notes](docs/releases/oos16.0.9.400-ack-6.12.52/release-notes.md)

## ACK ancestry and compatibility

The 6.12.52 release advances from the official
`android16-6.12.40_r00` state to `android16-6.12.52_r00`. All 2,298 official
ACK commits are visible individually in the first-parent repository history;
upstream authorship, subjects, dates, dependency order, and provenance are
preserved. OnePlus/OxygenOS conflict reconciliation, compatibility, KABI, ABI,
validation, and release work remain separate downstream commits.

Module signatures, MODVERSIONS, GENDWARFKSYMS, CRC checks, protected exports,
trusted-key handling, and ABI/KMI enforcement remain enabled. The retained
OxygenOS 16.0.9.400 stock-module audit passed 1,018 modules: 982 active
compatible, 36 dormant, and zero blockers.

## Installation

Use only on an unlocked OnePlus 15 `infiniti` / `canoe` running the exact
firmware named by the selected release. Back up the active boot partition and
retain a known-good rollback image.

Confirm the active slot:

```bash
adb shell getprop ro.boot.slot_suffix
fastboot getvar current-slot
```

Flash only the matching active boot partition. For example, for slot `_a`:

```bash
sha256sum boot.img
fastboot flash boot_a boot.img
fastboot reboot
```

For the current 17.0.0.102 release the expected `boot.img` SHA-256 is:

```text
723c6d5339f3ce312af5add567487ecde79d0c082aae19ea575bbf6a528897ff
```

Alternatively, use the firmware-specific TWRP ZIP. It verifies the
`infiniti`/`canoe` family, regional identifiers, active slot, partition
capacity, exact firmware/module contract, backup, write, and full read-back.
It writes only the active boot partition and fails closed on unrelated devices
or an incompatible payload contract.

Do not flash `system_dlkm`, `vendor_dlkm`, `vendor_boot`, DTBO, VBMeta,
userdata, metadata, or slot metadata for a boot-only release.

## Build and validation

Primary Canoe distribution build:

```bash
cd kernel_platform
tools/bazel build //soc-repo:canoe_perf_dist
```

Enforced common ABI comparison:

```bash
cd kernel_platform
tools/bazel run //common:kernel_aarch64_abi_dist -- --destdir=out/dist
```

Strict KMI and device-inclusive ABI dump:

```bash
cd kernel_platform
tools/bazel build //common:kernel_aarch64_abi_kmi_symbol_checks
tools/bazel build --kbuild_symtypes //common:kernel_aarch64
tools/bazel build //soc-repo:canoe_perf_abi
```

The Canoe ABI target generates a device-module-inclusive dump; it is not an
enforced device ABI diff. ABI-sensitive work also requires the retained stock
module, imports, CRC, signing, protected-export, and structural-provider
audits documented in `AGENTS.md`.

## Repository layout

| Path | Purpose |
|---|---|
| `kernel_platform/common/` | ACK/GKI kernel and ABI/KMI definitions |
| `kernel_platform/soc-repo/` | Canoe build targets, configuration, and module policy |
| `kernel_platform/qcom/` | Qualcomm integration and device-tree inputs |
| `kernel_platform/oplus/` | Oplus build/configuration integration |
| `vendor/` | Qualcomm, OnePlus/Oplus, NXP, and ST source inputs |
| `tools/` | Repository validation and release tooling |
| `docs/` | Qualification, compatibility, and release evidence |

No generated output, private key, device backup, or proprietary firmware
payload belongs in source control.
