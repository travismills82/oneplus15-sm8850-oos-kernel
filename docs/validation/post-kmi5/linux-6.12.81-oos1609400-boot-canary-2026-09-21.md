# Linux 6.12.81 KMI5 OxygenOS 16.0.9.400 boot canary

## Classification

`BOOT_QUALIFIED_CANARY`

The repaired first post-6.12.80 KMI5 boundary passed the boot-only encrypted
storage and Android canary on the same OxygenOS 16.0.9.400 device that
qualified 6.12.80.  This is not full hands-on release qualification.

## Tested identity

| Field | Value |
|---|---|
| Device | OnePlus 15 CPH2749 / OP611FL1 / serial `3C15AT0018R00000` |
| Firmware | `CPH2749_16.0.9.400(EX01)` |
| Slot | `_a` |
| Branch | `experiment/oos1609400-kmi5-linux-6.12.81-from-6.12.80` |
| Runtime-source commit | `0cb55e61f8a27bb9cd4a7d72d24bf9a99a2d7d36` |
| Production-build source | `3e933dfd017614b17f230c93b19fdb84c7d1d6f2` |
| Kernel | `6.12.81-android16-5-o-g3e933dfd0176-4k` |
| KMI generation | 5 |
| Production Image SHA-256 | `413bbb96a60720088dd4c23d9ba60f73183afd990d0baf8d16dea13ee7813f0b` |
| Tested `boot.img` SHA-256 | `aebbf681a12af58d3e72a0983cea207ed9fcf90bebf00f1d146963e840d891a7` |

The boot image is 100,663,296 bytes.  It retains the exact qualified 6.12.80
4,096-byte Android-v4 header and 17,920-byte trailing region, replaces only
the 40,565,248-byte production Image, and passes outer SHA256_RSA4096 AVB
verification with the qualified `.400` salt and properties.

## Backup and write verification

The device began healthy on the exact qualified 6.12.80 canary:

```text
6.12.80-android16-5-o-g0c510667a3f5-4k
```

Recovery reported a 100,663,296-byte `boot_a`.  The live source, recovery-side
backup, and host backup all had SHA-256:

```text
3f0110f97f0c35fb6739f19440f62d1a2f83d5c117550532edba834d5d93d436
```

Evidence and rollback artifact:

```text
/home/travis/Android/kernel-test-backups/linux-6.12.81-kmi5-20260921-224834
```

Only `boot_a` was written.  The complete immediate recovery read-back matched
the candidate SHA-256 exactly.  Rollback was not required.

## Immutable supporting payloads

No supporting partition was written.  Pre-test recovery reads matched the
qualified `.400` payloads:

| Partition | SHA-256 |
|---|---|
| `system_dlkm_a` | `18f530dcb0e46dc81ede00e18ac4e9b39faf6564fb067f04af9a912d87fd6dd7` |
| `vendor_dlkm_a` | `40d4bd03e9d315aac562234019f6db192617bb1ab65532157b81022ebc7330e6` |
| `vendor_boot_a` | `5fe60f58ebe3f935acb3ec41585fa16977804cc0c2efd4e84c44a645a1eb7162` |

DTBO, VBMeta, dynamic-partition metadata, userdata, metadata, radio/modem, and
persist were not written.

## TWRP encrypted-storage oracle

TWRP `3.7.1_16-OnePlus_15` booted the candidate kernel and recorded:

| Gate | Result |
|---|---|
| Candidate kernel identity | PASS |
| Metadata encryption | PASS |
| Userdata mapper | PASS, `/dev/block/mapper/userdata` -> `/dev/block/dm-36` |
| PIN worker | PASS |
| User 0 decryption | PASS |
| F2FS `/data` mount | PASS |
| `inlinecrypt` | ACTIVE |
| CE/DE data access | PASS |

The recovery log explicitly contains `User 0 Decrypted Successfully` and
`Data successfully decrypted`.  Recovery scans found no fatal module, KMI,
FBE, blk-crypto, ICE, UFS-crypto, panic, oops, or BUG report.

## Android canary

Two clean Android boots passed:

| Gate | Boot 1 | Boot 2 |
|---|---:|---:|
| `sys.boot_completed=1` | PASS | PASS |
| User 0 | `RUNNING_UNLOCKED` | `RUNNING_UNLOCKED` |
| F2FS `/data` with `inlinecrypt` | PASS | PASS |
| Runtime modules | 664 | 664 |
| WLAN | WPA3-SAE, 802.11be, connected | WPA3-SAE, 802.11be, connected |
| Cellular/RMNET | LTE registered, active data route | LTE registered, active data route |
| Bluetooth | ON | ON |
| NFC | ON | ON |
| Fatal kernel/module/FBE scan | Empty | Empty |
| Targeted DRM lifetime scan | Empty | Empty |
| Persistent pstore record | None | None |

The observed UFS `clkscale_enable`/`clkgate_enable` init permission warnings
and Oplus `.recovery_repair` cleanup warning are also present in the qualified
6.12.80 logs and are not new to 6.12.81.

## Remaining qualification

Camera, audio/call audio, fingerprint authentication, charging, longer
graphics use, and extended suspend/deep-idle still require hands-on testing
before this can be called fully physically qualified or promoted as a stable
release.

The exact tested 6.12.81 image remains installed on `boot_a`.  Main, remote
refs, tags, and releases remain unchanged.
