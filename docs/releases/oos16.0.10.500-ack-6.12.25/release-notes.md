# OnePlus 15 OOS 16.0.10.500 — ACK 6.12.25 stable

This boot-only release is the physically qualified Android 16 ACK 6.12.25
kernel for the OnePlus 15 CPH2747 / Canoe running OxygenOS
16.0.10.500(EX01).

## Release identity

- Kernel: `6.12.25-android16-5-o-g48618bcd6275-4k`
- Runtime source: `48618bcd62756eb3dc7e497fc74fb704fbec3a66`
- Qualification commit: `e32d8f9502d7b03c0ffa49e584bf96f6d2bc9000`
- Qualified parent tag: `oos16.0.10.500-ack-6.12.25-qualified`
- Asset: `boot.img`
- Size: 100,663,296 bytes
- SHA-256: `494bf88004b13e11379ad3639238f897347d0f8a9b10aa742a553e6c8d812b99`

The release contains only the exact physically tested `boot.img`. It does not
contain replacement DLKM, vendor_boot, DTBO, VBMeta, or dynamic-partition
images.

## Required stock firmware payloads

- `system_dlkm` SHA-256:
  `18f530dcb0e46dc81ede00e18ac4e9b39faf6564fb067f04af9a912d87fd6dd7`
- `vendor_dlkm` SHA-256:
  `157db23ccfa516c7904d4e87614f62c3c99c2dcbb93cc3fc8ccd3388c2cc793f`
- `vendor_boot` SHA-256:
  `3027d80a33fcc65f506d2d909db862657fe28e7eafc6e2af94193d67e6e617eb`
- Stock system `modules.load`: 82 entries
- `wwan.ko`: entry 28

Do not use the historical controlled 46-entry system-DLKM policy with this
release.

## Qualification

Static validation passed the FBE/user0 contract, enforced ABI/KMI checks, and
the complete 1,020-module current-firmware audit with zero unresolved imports,
CRC mismatches, protected-export failures, signature failures, or structural
provider failures.

The exact image physically passed two clean boots with existing encrypted
user0 `RUNNING_UNLOCKED`; 6135 MHz WPA3-SAE WLAN; LTE/RMNET, IP, routes, and
DNS; Bluetooth/HID; NFC/Wallet/HCE/eSE; camera; clear audio; USB/ADB; five
deep-idle cycles; framework health; and the kernel/module error scan. No
rollback was required.

## Installation scope

Use only on an unlocked CPH2747 running OxygenOS 16.0.10.500(EX01). Verify the
asset hash, confirm the active slot, create a verified backup, and flash only
the corresponding `boot_a` or `boot_b` partition. Do not modify any DLKM,
vendor_boot, DTBO, VBMeta, userdata, metadata, or slot metadata.

The next development phase is a deliberate controlled `rust_binder.ko`
system-DLKM migration for newer Android 16 KMI-generation-5 ACK kernels. It is
not part of this stable release.
