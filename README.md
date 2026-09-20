# OnePlus 15 SM8850 kernel

Authorized Android/Linux kernel development for the OnePlus 15
(`infiniti` / Canoe / Qualcomm SM8850), Android 16, and OxygenOS 16.

Kernel releases are firmware-specific. Never cross-flash a boot image merely
because it targets the same phone family; each OxygenOS build has its own
kernel, module, FBE/user0, and boot-container contract.

## Stable releases

| Firmware | Device physically tested | Kernel | Release tag |
|---|---|---|---|
| OxygenOS 16.0.9.400(EX01) | CPH2749 / OP611FL1 | `6.12.52-android16-5-o-gfa9fe14314c7-4k` | `oos16.0.9.400-ack-6.12.52` |
| OxygenOS 16.0.10.500(EX01) | CPH2747 | `6.12.37-android16-5-o-g57b52d3bb803-4k` | `oos16.0.10.500-ack-6.12.37` |

Use only the release matching the installed firmware.

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

For the 6.12.52 `.400` release the expected `boot.img` SHA-256 is:

```text
1aa1de79d79cfa5b184d2df4fb25fa56811f615d8420d5faf6c6dac29d49e41a
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
