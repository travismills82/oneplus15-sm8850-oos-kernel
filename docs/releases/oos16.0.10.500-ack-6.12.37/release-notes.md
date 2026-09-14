# OnePlus 15 OOS 16.0.10.500 — ACK 6.12.37 stable

This firmware-specific boot-only release contains the physically qualified
Android 16 generation-5 ACK 6.12.37 kernel for the OnePlus 15 running
OxygenOS 16.0.10.500(EX01).

## Release identity

- Kernel: `6.12.37-android16-5-o-g57b52d3bb803-4k`
- Kernel-producing source: `57b52d3bb8032ed7b3ae92812f0bd7d96abac4db`
- Qualification evidence commit: `4de219a9c588`
- Qualification tag: `oos16.0.10.500-ack-6.12.37-qualified`
- Stable release tag: `oos16.0.10.500-ack-6.12.37`
- `boot.img` size: 100,663,296 bytes
- `boot.img` SHA-256:
  `24a714216b3200508533c739a4a04bf41f1c459b9213b04cff06d7eaad8ab48b`
- Embedded Image SHA-256:
  `e41df1d7115b2cf4d346c32bda5f5911808773c62823bb6091b8fca1bce01737`
- TWRP/OrangeFox/PBRP ZIP:
  `OnePlus15-OOS16.0.10.500-ACK6.12.37-TWRP.zip`
- ZIP SHA-256:
  `504b17da68a21c465c57e1184fc22623d4f2453ad87dd839a110cd20da0ff20e`

The ZIP contains the exact tested `boot.img`; it contains no replacement
DLKM, `vendor_boot`, DTBO, VBMeta, userdata, metadata, or dynamic-partition
payload.

## Supported device family

- Device family: OnePlus 15 / `infiniti` / `canoe`
- Recognized regional identifiers: CPH2745, CPH2747, CPH2749, PLK110
- Physically qualified hardware: CPH2747
- Other listed variants: installer family detection supported; physical
  reports requested

The installer does not trust `ro.product.model` alone. It requires positive
OnePlus 15 family and Canoe evidence, a valid A/B slot, distinct boot slot
partitions, compatible boot capacity, exact firmware version, and the exact
stock EROFS system-DLKM hash. Generic SM8850 hardware is rejected.

## Required stock payload contract

Keep these exact OxygenOS 16.0.10.500 supporting partitions installed:

| Payload | SHA-256 |
|---|---|
| `system_dlkm` | `18f530dcb0e46dc81ede00e18ac4e9b39faf6564fb067f04af9a912d87fd6dd7` |
| `vendor_dlkm` | `157db23ccfa516c7904d4e87614f62c3c99c2dcbb93cc3fc8ccd3388c2cc793f` |
| `vendor_boot` | `3027d80a33fcc65f506d2d909db862657fe28e7eafc6e2af94193d67e6e617eb` |

Do not flash a generated custom DLKM with this release.

## Provenance and validation

Remote main at the previous 6.12.36 state is an ancestor of the release.
All 298 intervening commits remain individually visible; no ACK interval was
squashed. Upstream authorship and subjects are retained, and OxygenOS/KABI
reconciliation remains in separate commits.

Static gates passed: Canoe distribution, enforced common ABI with empty diff,
strict KMI and symtypes, Canoe ABI dump, retained stock-module contract,
Rust Binder 166/166, SHA-512 signing for all 103 generated GKI modules,
configuration/FBE review, and AVB/container verification.

The exact boot image passed TWRP PIN/metadata decryption, userdata mapper
creation, F2FS `/data` read-write with `inlinecrypt`, and two clean Android
boots with user0 `RUNNING_UNLOCKED`. WLAN at 6135 MHz WPA3-SAE, LTE/RMNET,
Bluetooth, ZRAM/ZSMALLOC, APEX/loop, USB/ADB, critical module chains, and fatal
kernel/module scans passed. Camera, audio, fingerprint, NFC, display, and touch
services initialized, and the repository owner reported the kernel works
great after use.

The exact ZIP passed archive CRC, internal checksums, boot identity,
deterministic rebuild, a 12-case family property matrix, shell syntax, and
forbidden-write inspection. The new ZIP archive was not itself flashed; its
embedded boot image is byte-identical to the physically qualified image.

## Installation

Use the ZIP from TWRP, OrangeFox, or PBRP. Decrypt `/data` or mount durable
external storage so the installer can keep a full boot backup. It selects the
active A/B boot partition, backs up and verifies the complete source, writes
only that boot partition, reads back the complete image, and restores the
backup if verification fails.

For manual installation, verify the active slot and flash only its boot
partition. Never flash `system_dlkm`, `vendor_dlkm`, `vendor_boot`, DTBO,
VBMeta, userdata, metadata, radio, modem, persist, or slot metadata for this
release.
