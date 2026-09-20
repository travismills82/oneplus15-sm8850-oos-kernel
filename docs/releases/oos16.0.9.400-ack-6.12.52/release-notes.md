# OnePlus 15 OOS 16.0.9.400 — ACK 6.12.52 stable

This is the physically qualified, boot-only ACK 6.12.52 release for the
OnePlus 15 `infiniti` / `canoe` running OxygenOS 16.0.9.400(EX01).

## Qualified combination

- Physically tested device: CPH2749 / OP611FL1
- Firmware: `CPH2749_16.0.9.400(EX01)`
- Kernel: `6.12.52-android16-5-o-gfa9fe14314c7-4k`
- KMI generation: Android 16 generation 5
- `boot.img` SHA-256:
  `1aa1de79d79cfa5b184d2df4fb25fa56811f615d8420d5faf6c6dac29d49e41a`
- TWRP ZIP SHA-256:
  `8139c82880a3449860abba4a67d491a502f0591cdbc677671c99f13c229b22b0`

Do not install this release on OxygenOS 16.0.10.500. That firmware has its own
qualified kernel release and module contract.

## Validation

The exact included boot image passed:

- TWRP metadata/PIN decryption and F2FS `/data` mount with `inlinecrypt`;
- two clean Android boots;
- user 0 `RUNNING_UNLOCKED` and CE/FBE access;
- WPA3-SAE WLAN, cellular/RMNET, Bluetooth, and USB/ADB;
- strict KMI, symtypes, common ABI, Canoe ABI generation, and Canoe build;
- the 1,018-module retained `.400` stock contract with zero blockers;
- SHA-512 signatures on all 103 generated GKI modules;
- Android-v4 boot-container and outer-AVB verification;
- fatal kernel/module scans with no symbol, CRC, signing, protected-export,
  FBE, panic, oops, or BUG failures.

The ACK 6.12.41 through 6.12.52 history is preserved commit by commit on
`main`; it was not squashed.

## Installation scope

Both assets install the same exact qualified `boot.img`. The ZIP:

- positively identifies the OnePlus 15 `infiniti` / `canoe` family;
- recognizes CPH2745, CPH2747, CPH2749, and PLK110 regional identifiers;
- verifies the active A/B slot, capacity, firmware, and stock `.400`
  `system_dlkm` contract;
- backs up the complete active boot partition;
- writes only the active boot partition;
- reads the partition back and verifies its full SHA-256;
- restores the backup automatically if verification fails.

Only CPH2749 on OxygenOS 16.0.9.400 is physically qualified for this exact
release. Other family variants are installer-supported by positive
`infiniti`/`canoe` detection, but physical reports are requested.

The ZIP archive passed offline structure, checksum, deterministic-rebuild,
device-property matrix, and write-target tests. Its embedded boot image is the
exact physically tested image; the ZIP archive itself was not separately
flashed during this qualification.

The release does not include and must not flash `system_dlkm`, `vendor_dlkm`,
`vendor_boot`, DTBO, VBMeta, modem, radio, persist, userdata, metadata, or
slot metadata. Keep all supporting OxygenOS 16.0.9.400 partitions stock.

Full evidence:
[ACK 6.12.52 physical qualification](https://github.com/travismills82/oneplus15-sm8850-oos-kernel/blob/oos16.0.9.400-ack-6.12.52/docs/validation/ack-modernization/oos1609400-ack-6.12.52-physical-validation-2026-09-20.md)
