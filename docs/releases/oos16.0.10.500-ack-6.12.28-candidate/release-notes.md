# OnePlus 15 OOS 16.0.10.500 — ACK 6.12.28 test candidate

> **TEST CANDIDATE — NOT YET PHYSICALLY QUALIFIED**

This boot-only candidate is provided for owner-directed testing on the
OnePlus 15 CPH2747 / Canoe running exactly OxygenOS 16.0.10.500(EX01).
The current stable and physically qualified release remains ACK 6.12.27.

## Candidate identity

- Kernel: `6.12.28-android16-5-o-g07a0a465cbb9-4k`
- Runtime source: `07a0a465cbb98217328f53bf8cec9bd9d7f7bd8f`
- KMI generation: Android 16 generation 5
- `boot.img` SHA-256:
  `fd21e481cb11aec2442b3e854c30a393eb1cb51cdebe1542fc2b700c1d8a684b`
- TWRP ZIP:
  `OnePlus15-CPH2747-OOS16.0.10.500-ACK6.12.28-CANDIDATE-TWRP.zip`
- TWRP ZIP SHA-256:
  `d0f741537d4e6c3d8be746cd8c63c504330752fc738093ae72af9775a55d8735`

The archive was rebuilt independently with a byte-identical result. Static
validation confirms its exact six-file contents, internal checksums,
executable installer, embedded boot identity, and AVB container.

## Boot-only write contract

The installer:

1. verifies CPH2747/Canoe identity and the active slot;
2. refuses an active OTA snapshot/merge;
3. verifies exact OxygenOS 16.0.10.500 firmware metadata;
4. reads and hashes stock `system_dlkm` to prove the required EROFS image;
5. creates a durable full boot-partition backup;
6. flashes only `boot_<active-slot>`;
7. reads back the flashed boot bytes and verifies SHA-256;
8. restores the original boot backup if write verification fails.

It does not contain or write `system_dlkm`, `system_dlkm_oki`, `vendor_dlkm`,
`vendor_boot`, DTBO, VBMeta, userdata, metadata, dynamic-partition metadata,
or slot metadata. A negative archive containing `vendor_boot.img` was rejected
by the repository validator.

## Required stock firmware contract

The following exact stock OxygenOS 16.0.10.500 images must remain installed:

| Payload | SHA-256 |
|---|---|
| `system_dlkm` | `18f530dcb0e46dc81ede00e18ac4e9b39faf6564fb067f04af9a912d87fd6dd7` |
| `vendor_dlkm` | `157db23ccfa516c7904d4e87614f62c3c99c2dcbb93cc3fc8ccd3388c2cc793f` |
| `vendor_boot` | `3027d80a33fcc65f506d2d909db862657fe28e7eafc6e2af94193d67e6e617eb` |

Do not install this candidate on another firmware build.

## Static compatibility result

- complete Canoe distribution build: PASS;
- enforced common ABI and strict KMI: PASS;
- 1,020 stock modules checked;
- 984 active compatible and 36 dormant modules;
- 57,216 import/CRC edges;
- zero active blockers, unresolved imports, CRC mismatches,
  protected-export failures, signature failures, or structural failures;
- stock `rust_binder.ko`: 166/166 imports compatible;
- SHA-512 source-module signing: PASS.

The cpufreq external three-argument ABI is retained through a private
limit-aware implementation. The AF_XDP pool-layout change is reconciled using
the official Android KABI strategy, restoring the exact qualified
`xsk_buff_pool` layout without CRC manipulation.

## TWRP installation

1. Retain the qualified 6.12.27 rollback image.
2. Copy the candidate ZIP to the phone.
3. Boot the current OnePlus 15 TWRP.
4. Decrypt data or mount durable writable external storage so the installer
   can retain its boot backup.
5. Install the ZIP normally.
6. Reboot System without switching slots.

The first boot must reach `sys.boot_completed=1` with existing user0
`RUNNING_UNLOCKED` and no `init_user0_failed`. Full qualification requires two
clean boots plus Wi-Fi, cellular/RMNET, Bluetooth/HID, NFC, camera,
fingerprint, audio, USB, ZRAM, suspend/resume, framework, and kernel/module
error validation.

Until those tests pass, do not treat this candidate as stable and do not begin
ACK 6.12.29 qualification from it.
