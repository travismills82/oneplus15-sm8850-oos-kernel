# OnePlus 15 OOS 16.0.10.500 — ACK 6.12.27 stable

This boot-only release is the physically qualified Android 16 ACK 6.12.27
kernel for the OnePlus 15 CPH2747 / Canoe running OxygenOS
16.0.10.500(EX01).

## Release identity

- Kernel: `6.12.27-android16-5-o-g20d91bf4ec43-4k`
- Runtime source: `20d91bf4ec43f6171bab445c4123350e64ab0883`
- Qualification commit: `169fd4e9c3cbd6178bc40f4b6769ace1dff0bbe3`
- Qualified tag: `oos16.0.10.500-ack-6.12.27-qualified`
- Stable release tag: `oos16.0.10.500-ack-6.12.27`
- `boot.img` size: 100,663,296 bytes
- `boot.img` SHA-256:
  `8b5753c49a3899c0635558584ef6814e927662b459ecb4233761d532faad15b5`
- TWRP ZIP:
  `OnePlus15-CPH2747-OOS16.0.10.500-ACK6.12.27-TWRP.zip`
- TWRP ZIP SHA-256:
  `cc27530e8d880105a15a95d1c59a6c9a2e9d440d4f08efbefee3e04cd5703033`

Both release paths install the exact physically qualified `boot.img`. The
release contains no replacement DLKM, `vendor_boot`, DTBO, VBMeta, or dynamic
partition image.

## Required stock firmware payloads

- `system_dlkm` SHA-256:
  `18f530dcb0e46dc81ede00e18ac4e9b39faf6564fb067f04af9a912d87fd6dd7`
- `vendor_dlkm` SHA-256:
  `157db23ccfa516c7904d4e87614f62c3c99c2dcbb93cc3fc8ccd3388c2cc793f`
- `vendor_boot` SHA-256:
  `3027d80a33fcc65f506d2d909db862657fe28e7eafc6e2af94193d67e6e617eb`
- Stock system `modules.load`: 82 entries
- `wwan.ko`: entry 28

Do not use the historical controlled 46-entry system-DLKM policy.

## Qualification

The ACK delta consists of two separately preserved official commits:

1. `793d3faa9246539c008a89570b0e0357e6a699f3` —
   `bpf: Fix BPF_INTERNAL namespace import`;
2. `fa3c82a7f526c9f81483599ce80034154cbed316` —
   `Linux 6.12.27`.

Static validation passed the FBE/storage contract, enforced ABI/KMI checks,
and the complete 1,020-module current-firmware audit with zero unresolved
imports, CRC mismatches, protected-export failures, signature failures, or
structural-provider failures.

The exact image passed two clean Android boots with existing encrypted user0
`RUNNING_UNLOCKED`; 6135 MHz WPA3-SAE WLAN; LTE/RMNET IP and DNS with Wi-Fi
disabled; cellular voice/audio; Bluetooth/HID; NFC/Wallet/HCE/eSE; camera;
fingerprint authentication; USB/ADB; five deep-idle cycles; framework health;
and the kernel/module error scan. No rollback was required.

The TWRP archive is deterministic across independent builds. Static archive
validation confirms its exact six-file payload, embedded boot identity,
firmware guard, stock EROFS system-DLKM guard, internal checksums, executable
installer, and absence of dynamic-partition payloads or resize/AVB-disable
operations.

The exact release ZIP was also installed through TWRP 3.7.1_16 on slot `_a`.
It created and verified a durable 100,663,296-byte `boot_a` backup, flashed
only `boot_a`, and produced the expected full-partition read-back SHA-256
`8b5753c49a3899c0635558584ef6814e927662b459ecb4233761d532faad15b5`.
Stock `system_dlkm`, `vendor_dlkm`, `vendor_boot`, DTBO, and VBMeta hashes were
unchanged. Android then reached `sys.boot_completed=1` with user0
`RUNNING_UNLOCKED`; Wi-Fi, Bluetooth, LTE/RMNET, fingerprint, ZRAM, and
Internet connectivity remained healthy, with no module ABI/signature/
protected-export failure or persistent crash record.

## Installation

Use only on an unlocked CPH2747 running OxygenOS 16.0.10.500(EX01) with the
exact stock supporting partitions above.

### TWRP

1. Copy the TWRP ZIP to the device.
2. Boot the current OnePlus 15 TWRP and decrypt data or mount durable external
   storage so the installer can retain a boot backup.
3. Install the ZIP normally.
4. Reboot System without switching slots.

The installer verifies CPH2747/Canoe identity, the active slot, clean snapshot
state, exact firmware manifest, exact stock EROFS `system_dlkm`, and embedded
boot checksum. It creates a durable backup, flashes only
`boot_<active-slot>`, reads back exactly 100,663,296 bytes, and restores the
backup automatically if verification fails.

### Fastboot

Confirm the active slot, retain a verified boot backup, then flash only the
matching active boot partition:

```text
fastboot flash boot_a boot.img
fastboot reboot
```

Use `boot_b` only when the device explicitly reports slot `_b`.

Never write `system_dlkm`, `system_dlkm_oki`, `vendor_dlkm`, `vendor_boot`,
DTBO, VBMeta, userdata, metadata, dynamic-partition metadata, or slot metadata
for this release.
