# Controlled-v1 slot-A safety audit — 2026-08-21

## Outcome

The controlled-v1 TEST3 baseline remains intact on slot `_b`. Slot `_a`
cannot be reconstructed as a complete r7 fallback under the current
no-super-metadata-change constraint, because the live dynamic-partition
metadata exposes only `_b` system-DLKM and vendor-DLKM partitions.

No slot-A payload or slot metadata was written. Slot A was not activated or
booted. TWRP plus verified backups remains the recovery path.

## Bootloader state before the audit

Fastboot reported:

| Property | Value |
| --- | --- |
| current slot | `b` |
| slot count | `2` |
| slot A successful | `yes` |
| slot B successful | `yes` |
| slot A unbootable | `yes` |
| slot B unbootable | `no` |
| slot A retry count | `7` |
| slot B retry count | `6` |
| bootloader unlocked | `yes` |
| secure | `yes` |

The `successful=yes` and `unbootable=yes` flags for slot A are not sufficient
to establish a usable fallback. The partition inventory below is the
controlling evidence.

## Verified slot-A backup

Before considering any repair, all directly addressable slot-A boot-chain
partitions were copied at full block-device size and hashed. The persistent
recovery backup is:

```text
/sdcard/TWRP/kernel-flash-backups/slot-a-audit-20260821-175814/
```

A pulled, independently verified copy is retained outside the repository at:

```text
/home/travis/Android/controlled-v1-slot-a-repair-20260821/slot-a-backup/
```

| Partition | Size | SHA-256 |
| --- | ---: | --- |
| `boot_a` | 100,663,296 | `1335867cacdbcf60d083748c3ee6c7a9695408776ff6c34839f7ec0d722602f9` |
| `vendor_boot_a` | 100,663,296 | `bbe2051d7f5709555e12ae1586ec6465b806eec486c515049fe146e6e8fc6166` |
| `vbmeta_a` | 65,536 | `960db222507b21a843650a43a7d4abe52f6bd9d0afe24eb4c80e0daa6785c226` |
| `recovery_a` | 104,857,600 | `da91172cba6a0a3502d0d22c9bfe87387c8e361ff20e6c1e4653b3f0065d5bec` |

Source-device and backup hashes matched, and the pulled `SHA256SUMS` check
passed.

## Slot-A content findings

The directly addressable slot-A partitions are not a coherent copy of the
current r7 or controlled-v1 stack:

- `boot_a` contains release
  `6.12.23-android16-5-o-g7b1ff1b969ac-4k`, not the r7 release and not the
  controlled-v1 release. Its extracted Image SHA-256 is
  `2d843da3828decc5d4c90d701493246c27cfb40cb023042f186f09ec48846f59`.
- `vendor_boot_a` carries the older AVB payload digest
  `8423504b9b44f9abeedf4c39083d37bcd3978008dba00877317e9f0c2b5c0b3e`
  and does not match the verified current stock vendor-boot image.
- `vbmeta_a` has AVB flags `3` and describes older system-DLKM and
  vendor-DLKM roots. Its vendor-DLKM root is
  `0237a6ae537ca22823e417c4d3c9d9f3a0a21b1960629d499e17f3d0fcee6381`.

The exact historical reason the bootloader marked slot A unbootable cannot be
proven from these artifacts alone. What is proven is that its accessible
boot-chain payloads are from a different generation and are not a verified
r7 fallback.

## Dynamic-partition stop condition

TWRP `lpdump --all` showed:

- update state `none`;
- metadata slot 0 and metadata slot 1 both define `system_dlkm_b` and
  `vendor_dlkm_b`;
- neither metadata copy defines `system_dlkm_a` or `vendor_dlkm_a`; and
- the live mapper exposes only `_b` DLKM devices.

Consequently, writing r7 `boot_a`, `vendor_boot_a`, and `vbmeta_a` would still
leave slot A without corresponding `_a` system/vendor DLKM partitions. Making
those logical targets would require dynamic-partition or super-metadata
changes, which this task explicitly prohibits without a separately reviewed
repair design.

Slot-A repair therefore stopped before all writes. The active slot was not
changed.

## TWRP controlled-stack dry run

The hardened helper from TWRP commit
`3f499bfd1f7152ea27b27935be22ff73581709a1` was exercised against the exact
canonical TEST3 package while user data was decrypted and writable.

The dry run verified:

- device identity `CPH2747` / `canoe` / project `24863`;
- current slot `_b`;
- update/snapshot state `none`;
- `vendor_dlkm_b`, `system_dlkm_b`, and `boot_b` target resolution;
- ext4 vendor-DLKM and EROFS system-DLKM formats;
- partition capacity and the three exact TEST3 SHA-256 values;
- `vendor_boot` is optional and was not requested; and
- dependency-first write order with boot last.

It ended with:

```text
DRY RUN PASSED — NO PARTITIONS MODIFIED
```

Live evidence is retained outside git at:

```text
/home/travis/Android/controlled-v1-slot-a-repair-20260821/
```

## Slot-B preservation and post-audit checks

After leaving recovery, Android booted slot `_b` with the exact canonical
TEST3 contract:

| Payload | Observed SHA-256 |
| --- | --- |
| `boot_b` | `25efe5463938757339dcfada56ee47d77d3c0cc42b6707dda7dd1613c20fc313` |
| `vendor_boot_b` | `5fe60f58ebe3f935acb3ec41585fa16977804cc0c2efd4e84c44a645a1eb7162` |
| system-DLKM payload | `edebc94818e6fa4e214d58fd82fe46f6c513fc9850b3e7b77caf076a12270f05` |
| `vendor_dlkm_b` | `24e66015a3e4ea3583f895d529008d8c7c3706d7bc506ef550df936935127b80` |

Runtime checks passed:

- controlled-v1 release `6.12.23-android16-5-o-g090459863b8c-4k`;
- Wi-Fi WPA3 at 6135 MHz and 11ax;
- Visible LTE HOME;
- `rmnet_data2` IPv4 and IPv6 addresses and default routes;
- IP and DNS reachability, both 5/5 with zero packet loss; and
- no relevant unknown-symbol, CRC, vermagic, signature, or protected-export
  failures.

## Safety status

```text
PASS — BASELINE FROZEN, SLOT A REPAIR DEFERRED
```

Slot A must continue to be treated as unbootable. A future repair requires a
separately reviewed super-metadata plan and a complete, firmware-consistent
r7 partition set; direct flashing of only the currently addressable slot-A
partitions is unsafe.
