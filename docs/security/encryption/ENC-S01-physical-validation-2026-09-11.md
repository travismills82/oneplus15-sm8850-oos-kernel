# ENC-S01 fscrypt physical FBE canary

Validation date: 2026-09-11 (America/Chicago)

## Result

`PHYSICAL FBE CANARY: PASS`

This result covers the first high-risk Phase 2 encryption series: the
multi-data-unit-size fscrypt prepared-key cache correction plus the real
generation-5 master-key layout adaptation.  It covers one candidate-kernel
TWRP decryption oracle and one Android boot; it is not the final two-boot,
interactive-peripheral qualification for the cumulative Phase 2 baseline.

| Item | Value |
|---|---|
| Device | OnePlus 15 CPH2747 / Infiniti / Canoe |
| Firmware | OxygenOS 16.0.10.500(EX01) |
| Slot | `_a` |
| Source | `ad4ab6465d9925abe369e20d001bd2c765417da1` |
| Kernel | `6.12.35-android16-5-o-gad4ab6465d99-4k` |
| Candidate boot SHA-256 | `a4099be9d60f2d81268d64ab9e1d38efdf3a8a6c15638ba5d60f3384f423cf3d` |
| Rollback boot SHA-256 | `816eafa394c5aaab26d2bff9ad37b612045b8fd3a5702f81d3f6fc81e78db35f` |

## Preflight, backup, and write

- Android was healthy on the exact Phase 1 physically tested kernel.
- `sys.boot_completed=1`, user 0 was CE-unlocked, and `/data` was F2FS with
  `inlinecrypt` before the test.
- The live 100,663,296-byte `boot_a` hash matched the rollback artifact.
- A new complete host backup was captured and matched the live partition.
- Fastboot reported slot `a`, `snapshot-update-status: none`, and
  `slot-unbootable:a: no`.
- Candidate size exactly matched the reported `boot_a` capacity.
- The bootloader remained unlocked while secure boot reporting remained
  enabled.
- Only `boot_a` was written.
- The complete TWRP and Android partition read-backs matched the candidate.

The verified local evidence directory is:

```text
/home/travis/Android/kernel-test-backups/enc-s01-fbe-20260911-053023
```

It contains the pre-write boot backup, post-write boot read-back, recovery
logs, Android dmesg/logcat, and loaded-module capture.  These device artifacts
are intentionally not committed.

## Candidate-kernel TWRP oracle

| Gate | Result |
|---|---|
| Candidate kernel identity | PASS |
| Full boot read-back | PASS |
| Metadata encryption | PASS |
| Userdata mapper | CREATED (`/dev/block/mapper/userdata`) |
| PIN worker | PASS |
| User 0 decryption | PASS |
| CE and DE directories | ACCESSIBLE |
| `/data` filesystem | F2FS, read-write |
| Inline encryption | ACTIVE (`inlinecrypt`) |
| Fatal storage/module scan | PASS |

Before the PIN oracle, TWRP reported `twrp.user.0.decrypt=0` and displayed
encrypted data-media names.  After the PIN worker succeeded it reported
`twrp.user.0.decrypt=1`, human-readable data-media names and accessible user-0
CE paths.  This proves the user credential gate rather than only the metadata
mapper stage.

## Android oracle

| Gate | Result |
|---|---|
| Android boot | PASS |
| `sys.boot_completed` | `1` |
| User 0 CE | `sys.user.0.ce_available=true` |
| FBE/CE | PASS |
| `/data` | F2FS, read-write, `inlinecrypt` |
| `system_server` | RUNNING |
| Wi-Fi | 6135 MHz, WPA3-SAE, 802.11ax |
| Wi-Fi Internet | PASS; 3/3 direct-IP replies |
| Cellular | LTE registered; `rmnet_data2` IPv4 present |
| RMNET direct IP | PASS; 3/3 replies through `rmnet_data2` |
| RMNET DNS | PASS; name resolution plus 2/2 replies |
| Bluetooth | ENABLED, state ON |
| ZRAM | ACTIVE, 8 GiB device |
| APEX/loop | 86 APEX mounts and 47 loop devices |
| Critical storage modules | `ufs_qcom`, `ufshcd_crypto_qti`, `qcom_ice`, `qcom_scm` loaded |
| Persistent pstore | EMPTY |
| Full boot read-back | PASS |

The current dmesg/logcat scan found no panic, oops, BUG, unknown symbol,
version disagreement, invalid module format, CRC/vermagic/signature failure,
protected-export failure, `init_user0_failed`, `bootstrap-apexd-failed`, or
F2FS/fscrypt/blk-crypto/ICE fatal error.  The two `fsck.f2fs` lines containing
the word `corrupted` explicitly reported `[Ok..]` and zero fixes.  The Oplus
attempt to remove an absent `.recovery_repair` path was non-fatal and followed
successful metadata-encrypted F2FS mounting and policy verification.

## Payload scope and qualification boundary

No `system_dlkm`, `vendor_dlkm`, `vendor_boot`, `dtbo`, VBMeta, metadata, or
userdata partition was flashed or reformatted.  The candidate remains Linux
6.12.35 with Android KMI generation 5.

ENC-S01 is safe to retain in the cumulative Phase 2 branch.  The final
encryption-security baseline still requires the remaining selected coherent
series, final gap audit, second clean Android boot, interactive peripherals,
and five suspend/deep-idle cycles before it can be frozen or published.
