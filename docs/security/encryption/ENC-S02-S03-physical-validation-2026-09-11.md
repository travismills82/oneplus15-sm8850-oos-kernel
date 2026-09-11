# ENC-S02 and ENC-S03 physical FBE canary

Validation date: 2026-09-11 (America/Chicago)

## Result

`PHYSICAL FBE CANARY: PASS`

This result covers the cumulative boot-only canary for two independent small
encrypted-storage corrections:

- ENC-S02 returns `BLK_STS_INVAL` for invalid blk-crypto bio alignment.
- ENC-S03 stops device-mapper wrapped-key derivation after the first
  underlying device.

It covers one candidate-kernel TWRP decryption oracle and one Android boot. It
does not freeze or publish the final Phase 2 encrypted-storage security
baseline.

| Item | Value |
|---|---|
| Device | OnePlus 15 CPH2747 / Infiniti / Canoe |
| Firmware | OxygenOS 16.0.10.500(EX01) |
| Slot | `_a` |
| Runtime source | `96fce14273ea4f73ccd453bf4ba3cb31c6e24ec9` |
| Kernel | `6.12.35-android16-5-o-g96fce14273ea-4k` |
| Candidate boot SHA-256 | `a5bd3347805c8f6bc96217a6e36eb4f8764be8654e531e938a769a5c29ad1511` |
| Previous tested boot SHA-256 | `a4099be9d60f2d81268d64ab9e1d38efdf3a8a6c15638ba5d60f3384f423cf3d` |

## Preflight, backup, and write

- Android was healthy on the exact ENC-S01 physically tested kernel.
- `sys.boot_completed=1`, user 0 was CE-unlocked, and `/data` was F2FS with
  `inlinecrypt` before the test.
- The live 100,663,296-byte `boot_a` hash matched the previous tested artifact.
- A new complete host backup was captured and matched the live partition.
- Fastboot reported product `canoe`, slot `a`,
  `snapshot-update-status: none`, and `slot-unbootable:a: no`.
- Candidate size exactly matched the reported `boot_a` capacity.
- The bootloader remained unlocked while secure mode remained enabled.
- Only `boot_a` was written.
- Complete TWRP and Android read-backs matched the candidate SHA-256.

The uncommitted device evidence and rollback directory is:

```text
/home/travis/Android/kernel-test-backups/enc-s02-s03-fbe-20260911-150654
```

## Candidate-kernel TWRP oracle

| Gate | Result |
|---|---|
| Candidate kernel identity | PASS |
| Full boot read-back | PASS |
| Metadata encryption | PASS |
| Userdata mapper | CREATED (`/dev/block/mapper/userdata`) |
| PIN/credential worker | PASS |
| TWRP user 0 decryption | PASS |
| CE and DE directories | ACCESSIBLE |
| `/data` filesystem | F2FS, read-write |
| Inline encryption | ACTIVE (`inlinecrypt`) |
| `fsck.f2fs` | PASS; no error and zero fixes |
| Fatal storage/module scan | PASS |

TWRP logged `User 0 Decrypted Successfully`, reported
`twrp.user.0.decrypt=1`, exposed the userdata mapper, and mounted the existing
filesystem without formatting, key regeneration, or metadata mutation.

## Android oracle

| Gate | Result |
|---|---|
| Android boot | PASS |
| `sys.boot_completed` | `1` |
| User 0 state | `RUNNING_UNLOCKED` |
| User 0 CE | `sys.user.0.ce_available=true` |
| FBE/CE | PASS |
| `/data` | F2FS, read-write, `inlinecrypt` |
| `system_server` | RUNNING |
| Wi-Fi | 6135 MHz, WPA3-SAE, 802.11ax |
| Wi-Fi direct IP | PASS; 3/3 replies |
| Cellular | LTE; `rmnet_data2` IPv4 and IPv6 present |
| RMNET direct IP | PASS; 3/3 replies through `rmnet_data2` |
| DNS | PASS; name resolution and 2/2 replies |
| Bluetooth | ENABLED, state ON |
| ZRAM | ACTIVE, 8 GiB device |
| APEX/loop | 76 APEX mounts and 47 loop devices |
| Critical storage modules | `ufs_qcom`, `ufshcd_crypto_qti`, `qcom_ice`, `qcom_scm` loaded |
| Persistent pstore | EMPTY |
| Full boot read-back | PASS |

The exact fatal scan found zero kernel panic, real oops/BUG, unknown symbol,
symbol-version disagreement, invalid module format, CRC/vermagic/signature
failure, protected-export failure, `init_user0_failed`, or
`bootstrap-apexd-failed` events. The Qualcomm UFS debug and `qti-testscripts`
one-shot services still encounter their pre-existing recovery-context
SIGABRT behavior; the same lines are present on the passing ENC-S01 baseline
and are unrelated to the successful PIN worker and FBE paths. The Oplus
attempt to remove an absent `.recovery_repair` path is likewise pre-existing
and non-fatal.

## Payload scope and qualification boundary

No `system_dlkm`, `vendor_dlkm`, `vendor_boot`, `dtbo`, VBMeta, metadata, or
userdata partition was flashed or reformatted. The candidate remains Linux
6.12.35 with Android KMI generation 5.

ENC-S02 and ENC-S03 are safe to retain in the cumulative Phase 2 branch. The
final encrypted-storage security baseline still requires the remaining
selected coherent series, final gap audit, second clean Android boot,
interactive peripherals, and five suspend/deep-idle cycles before it can be
frozen or published.
