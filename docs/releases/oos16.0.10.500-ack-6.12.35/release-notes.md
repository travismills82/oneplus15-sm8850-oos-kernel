# OnePlus 15 OOS 16.0.10.500 — ACK 6.12.35 stable

This firmware-specific boot-only release contains the physically tested
Android 16 ACK 6.12.35 kernel for the OnePlus 15 CPH2747 / Canoe running
OxygenOS 16.0.10.500(EX01).

## Release identity

- Kernel: `6.12.35-android16-5-o-gb42c1af35b26-4k`
- Runtime source: `b42c1af35b26f7d2e1b0e6c8e9eaaf3628c7e32e`
- Qualification commit: `174e1cefa63e618f2baa8c479ab84e302600e5b3`
- Qualification tag: `oos16.0.10.500-ack-6.12.35-qualified`
- Stable release tag: `oos16.0.10.500-ack-6.12.35`
- `boot.img` size: 100,663,296 bytes
- `boot.img` SHA-256:
  `bbf3e9ed0fae1e55b3c7522cadff9509decc24bfd5a412ae66d4cbebea5effdc`
- TWRP ZIP:
  `OnePlus15-CPH2747-OOS16.0.10.500-ACK6.12.35-TWRP.zip`
- TWRP ZIP SHA-256:
  `f829602c2717540fa42fba8479afceaa167a096abb65d3ab2db7ac4ac8d0ab73`

The archive contains the exact tested `boot.img` and no replacement DLKM,
`vendor_boot`, DTBO, VBMeta, userdata, metadata, or dynamic-partition payload.

## Required stock firmware contract

Keep these exact OxygenOS 16.0.10.500 supporting partitions installed:

| Payload | SHA-256 |
|---|---|
| `system_dlkm` | `18f530dcb0e46dc81ede00e18ac4e9b39faf6564fb067f04af9a912d87fd6dd7` |
| `vendor_dlkm` | `157db23ccfa516c7904d4e87614f62c3c99c2dcbb93cc3fc8ccd3388c2cc793f` |
| `vendor_boot` | `3027d80a33fcc65f506d2d909db862657fe28e7eafc6e2af94193d67e6e617eb` |

Do not flash the generated development `system_dlkm.flatten.ext4.img` with
this release.

## Validation

The 6.12.33, 6.12.34, and 6.12.35 stable intervals retain all 945 official
stable commits individually, in order and with upstream authorship. Six
Android KABI repairs are also retained as separate provenance-preserving
commits. No 6.12.36-or-newer stable change is included.

The Canoe distribution, symtypes, strict KMI targets, enforced common ABI
comparison, Canoe ABI dump, configuration, SHA-512 signing, and AVB/container
checks passed. `Module.symvers` remains byte-identical to the previously
qualified stock-module provider contract.

The exact boot image passed two clean Android boots, existing encrypted user0
`RUNNING_UNLOCKED`, 6135 MHz WPA3-SAE WLAN, LTE/RMNET data and DNS with Wi-Fi
disabled, Bluetooth OFF/ON recovery, ZRAM, NFC and framework service health,
USB/charging, five deep-idle cycles, and a clean kernel/module scan.

The exact final ZIP was installed through TWRP 3.7.1_16 on slot `_a`. It
verified the firmware and stock EROFS system-DLKM, created a durable boot
backup, wrote only `boot_a`, verified the complete read-back hash, and booted
Android with user0 and all tested radios healthy. Camera, audio, and
fingerprint services were healthy; this run did not add a new hands-on photo,
audible-playback, or biometric-accept observation.

## TWRP installation

1. Verify the device is an unlocked OnePlus 15 CPH2747 on exact OxygenOS
   16.0.10.500(EX01).
2. Copy the TWRP ZIP to internal storage or durable external storage.
3. Boot the current OnePlus 15 TWRP and decrypt `/data`, or mount writable
   external storage, so the installer can retain a boot backup.
4. Install the ZIP normally.
5. Reboot System without changing slots.

The installer checks the device, active slot, clean snapshot state, exact
firmware manifest, exact stock EROFS `system_dlkm`, archive checksum, and boot
partition geometry. It backs up and writes only `boot_<active-slot>`, verifies
the complete read-back, and restores the backup automatically if verification
fails.

It never writes or resizes `system_dlkm`, `system_dlkm_oki`, `vendor_dlkm`,
`vendor_boot`, DTBO, VBMeta, userdata, metadata, or logical partitions.

## Manual fastboot installation

After confirming the active slot and retaining a verified backup, flash only
that slot's boot partition. Example for slot `_a`:

```text
fastboot flash boot_a boot.img
fastboot reboot
```

Use `boot_b` only when the bootloader explicitly reports slot `_b`.
