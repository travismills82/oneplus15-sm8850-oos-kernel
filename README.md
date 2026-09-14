# OnePlus 15 SM8850 kernel — OxygenOS 16.0.10.500

Authorized Android/Linux kernel development for the OnePlus 15
(infiniti / Canoe / Qualcomm SM8850). The current stable release is a
physically qualified, boot-only Android 16 ACK 6.12.37 kernel for:

~~~text
OxygenOS 16.0.10.500(EX01)
kernel 6.12.37-android16-5-o-g57b52d3bb803-4k
~~~

This release is firmware-specific. Do not use it on another OxygenOS build
without repeating the firmware, module, FBE/user0, boot-container, and physical
qualification.

## Stable release

| Field | Qualified value |
|---|---|
| Device | OnePlus 15 CPH2747 / infiniti / Canoe |
| Firmware | OxygenOS 16.0.10.500(EX01) |
| Kernel | `6.12.37-android16-5-o-g57b52d3bb803-4k` |
| KMI generation | Android 16 generation 5 |
| Runtime source | `57b52d3bb8032ed7b3ae92812f0bd7d96abac4db` |
| Qualification commit | `4de219a9c588` |
| Qualification tag | `oos16.0.10.500-ack-6.12.37-qualified` |
| Stable release tag | `oos16.0.10.500-ack-6.12.37` |
| `boot.img` size | 100,663,296 bytes |
| `boot.img` SHA-256 | `24a714216b3200508533c739a4a04bf41f1c459b9213b04cff06d7eaad8ab48b` |

The GitHub release contains the exact physically tested `boot.img` and a
boot-only TWRP installer containing that same image. It does not contain or
require replacement DLKM, vendor_boot, DTBO, VBMeta, or dynamic-partition
images.

## Firmware payload contract

The supported configuration is:

~~~text
qualified custom boot.img
+ exact stock OOS 16.0.10.500 system_dlkm
+ exact stock OOS 16.0.10.500 vendor_dlkm
+ exact stock OOS 16.0.10.500 vendor_boot
+ stock DTBO, VBMeta, userdata, metadata, and slot state
~~~

Immutable stock payloads:

| Payload | SHA-256 |
|---|---|
| `system_dlkm` | `18f530dcb0e46dc81ede00e18ac4e9b39faf6564fb067f04af9a912d87fd6dd7` |
| `vendor_dlkm` | `157db23ccfa516c7904d4e87614f62c3c99c2dcbb93cc3fc8ccd3388c2cc793f` |
| `vendor_boot` | `3027d80a33fcc65f506d2d909db862657fe28e7eafc6e2af94193d67e6e617eb` |

The stock system-DLKM contract contains 103 modules and 82
`modules.load` entries, with `wwan.ko` at entry 28. The complete
current-firmware compatibility audit checks 1,020 modules across system,
vendor, and vendor_boot load domains.

Do not flash a historical controlled 46-entry system-DLKM with this release.

## Source provenance

The firmware-native common-kernel baseline is:

- ACK tag: `android16-6.12-2025-06_r53`
- ACK commit: `b2a876903b495c444a94b16f50d1463ffe953957`
- stock kernel build identity:
  `6.12.23-android16-5-gb2a876903b49-ab14541642-4k`

The repository retains the OnePlus/Oplus and Qualcomm source pins declared by
[`oneplus_15.xml`](oneplus_15.xml). Those available OnePlus source pins
predate the 16.0.10.500 firmware binaries; compatibility with the newer
firmware is established by the exact stock-module audit and physical
qualification, not inferred from the source-release label.

The stable Image cumulatively includes the physically qualified boot-only
hardening batches:

- Binder transaction target lifetime pinning
- BPF per-CPU map copy bounds hardening
- eventpoll file/RCU lifetime hardening
- AF_PACKET fanout lifetime hardening
- USB gadget UDC lifetime hardening
- AF_UNIX garbage-collection/SCC hardening
- IPv6 MLD query skb lifetime hardening
- Netfilter quota2 counter lifetime hardening

It then applies reviewed Linux/ACK 6.12.24 through 6.12.37 point-release
changes with Android KMI-preserving integrations where required. The 6.12.26
compatibility work retains the qualified OEM-visible request layout and uses a
pinned hermetic SHA-512 module signer without reverting the stable changes.
The 6.12.37 source is 298 individual commits ahead of the prior published
6.12.36 main state, including the complete upstream interval and separate
Android/OxygenOS compatibility and ABI repairs. No ACK interval was squashed.
Module signatures, MODVERSIONS, GENDWARFKSYMS, CRC validation, protected
exports, trusted-key handling, and ABI/KMI enforcement remain enabled.

## Qualification

Static validation passed:

- common Image build and truthful 6.12.37 release identity
- requested power-efficient workqueue, ADIOS, CIFS/NetFS, signing, and KMI
  configuration retained
- FBE/fscrypt/storage contract
- GKI ABI report: empty
- KMI symbol checks
- all pre-existing provider records preserved, with reviewed additions only,
  against the 1,020-module current-firmware contract
- zero unresolved imports, CRC mismatches, protected-export failures,
  signature failures, or structural-provider failures
- boot header, embedded kernel identity, GKI signature tail, and outer AVB
  container checks

The exact release image physically passed:

- two clean Android boots on slot `_a`
- existing encrypted user0 `RUNNING_UNLOCKED`
- no `init_user0_failed` or Rescue Party redirect
- 6135 MHz WPA3-SAE WLAN
- LTE/RMNET, IPv4/IPv6 addressing and routes, direct IP, and DNS traffic
- Bluetooth initialization/state and an existing wearable connection
- camera, audio, fingerprint, graphics/UI, USB/ADB, and NFC framework/HAL
  service initialization
- stable framework/system_server and clean kernel/module failure scan
- direct boot-only flashing with a complete backup and exact read-back;
- repository-owner daily-use confirmation after the automated checks

See:

- [6.12.37 physical validation](docs/validation/ack-modernization/oos1610500-ack-6.12.37-physical-validation-2026-09-14.md)
- [6.12.35 base static validation](docs/validation/firmware-16.0.10.500/oos1610500-ack-6.12.35-static-validation.md)
- [qualified firmware manifest](docs/validation/firmware-16.0.10.500/oos1610500-ack-6.12.35-manifest.json)

## Installation

Use only on an unlocked OnePlus 15 `infiniti` / `canoe` running OxygenOS
16.0.10.500(EX01) with the exact stock module stack above. The installer
recognizes CPH2745, CPH2747, CPH2749, and PLK110-derived family identities;
only CPH2747 has been physically qualified so far. Back up the active boot
partition first and retain the qualified rollback image.

Confirm the active slot:

~~~bash
adb shell getprop ro.boot.slot_suffix
fastboot getvar current-slot
~~~

Flash only the matching active boot partition. For example, if the confirmed
slot is `_a`:

~~~bash
fastboot flash boot_a boot.img
fastboot reboot
~~~

Use `boot_b` instead only when the device explicitly reports slot `_b`.
Verify the local image before flashing:

~~~bash
sha256sum boot.img
~~~

Expected:

~~~text
24a714216b3200508533c739a4a04bf41f1c459b9213b04cff06d7eaad8ab48b  boot.img
~~~

Alternatively, install the firmware-specific TWRP/OrangeFox/PBRP ZIP. It
positively verifies the `infiniti`/`canoe` family, active slot, exact `.500`
firmware manifest, and stock EROFS `system_dlkm`;
creates a durable boot backup; writes only the active boot partition; and
verifies the complete boot read-back. It refuses installation during an OTA
snapshot/merge or when the firmware/module contract cannot be proven.

The 6.12.37 ZIP passed offline structure, checksum, deterministic-rebuild,
property-matrix, and write-target tests. Its embedded boot image is the exact
physically qualified image; this exact ZIP archive was not separately flashed.

Do not flash `system_dlkm`, `vendor_dlkm`, `vendor_boot`, DTBO, VBMeta,
userdata, metadata, or slot metadata for this release.

## Repository layout and build

| Path | Purpose |
|---|---|
| `kernel_platform/common/` | ACK/GKI kernel and ABI/KMI definitions |
| `kernel_platform/soc-repo/` | Canoe build targets, configuration, and module policy |
| `kernel_platform/qcom/` | Qualcomm integration and device-tree inputs |
| `kernel_platform/oplus/` | Oplus build/configuration integration |
| `vendor/` | Qualcomm, OnePlus/Oplus, NXP, and ST source inputs |
| `tools/` | Repository validation and release tooling |
| `docs/` | Qualification, compatibility, and release evidence |

Primary Canoe build:

~~~bash
cd kernel_platform
tools/bazel build //soc-repo:canoe_perf_dist
~~~

Enforced common ABI comparison:

~~~bash
cd kernel_platform
tools/bazel run //common:kernel_aarch64_abi_dist -- --destdir=out/dist
~~~

No generated output, private key, device backup, or proprietary firmware
payload belongs in source control.

## Newer generation-5 ACK work

Future ACK point releases continue one subversion at a time from this exact
qualified baseline. Every applicable official ACK commit must remain an
individual commit in original dependency order. Each point release must repeat
the full stock-module, ABI/KMI, existing-user0, radio, and physical validation
gates; CRC patching, forged vermagic, and disabled validation are not
acceptable.

ACK generation 6 is a separate, broader KMI migration.

Historical OxygenOS 16.0.9.400 releases remain available on the GitHub
Releases page but are not the current stable firmware target.
