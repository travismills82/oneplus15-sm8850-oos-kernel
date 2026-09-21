# Linux 6.12.80 KMI5 OxygenOS 16.0.9.400 boot canary

## Classification

`BOOT_QUALIFIED_CANARY`

This is a controlled boot-only result for OxygenOS 16.0.9.400.  It is not a
full physical qualification and is not an official Android KMI5 6.12.80
release.  The kernel is a custom Linux-stable 6.12.80 advancement retaining
the Android 16 generation-5 KMI contract.

## Tested identity

| Field | Value |
|---|---|
| Device | OnePlus 15 CPH2749 / OP611FL1 |
| Device tree identity | `infiniti` / `qcom,canoe-mtp` |
| Firmware | `CPH2749_16.0.9.400(EX01)` |
| Slot | `_a` |
| Tested source | `0c510667a3f5e0da2e814d257e073bb9448f4b93` |
| Kernel | `6.12.80-android16-5-o-g0c510667a3f5-4k` |
| KMI generation | 5 |
| `boot.img` size | 100,663,296 bytes |
| `boot.img` SHA-256 | `3f0110f97f0c35fb6739f19440f62d1a2f83d5c117550532edba834d5d93d436` |
| Embedded production Image SHA-256 | `29cdba704045c7738d1e9590c7e5fe94ef9f4e5d7faa361f6c9e7d79eddb431c` |

The source history from the qualified 6.12.52 parent through this candidate is
linear.  Every applicable non-merge Linux stable commit through v6.12.80 is
retained individually; Android KMI repairs and OOS compatibility work remain
separate downstream commits.

## Preflight and rollback

The phone began healthy on
`6.12.52-android16-5-o-gfa9fe14314c7-4k`, with Android boot complete and user
0 running.  The active boot partition had a capacity of 100,663,296 bytes.

The complete pre-test `boot_a` partition was copied and verified before the
write:

```text
boot_a pre-test SHA-256:
1aa1de79d79cfa5b184d2df4fb25fa56811f615d8420d5faf6c6dac29d49e41a

host rollback file:
/home/travis/Android/kernel-test-backups/linux-6.12.80-kmi5-crossfw-400-20260921-064311/boot_a.pre-61280.img
```

The source partition, device-side backup, and host backup hashes matched.
Only `boot_a` was written.  The complete 100,663,296-byte partition was read
back after the write and matched the candidate SHA-256 exactly.

## Immutable supporting payloads

No supporting partition was written.  The test retained the installed `.400`
payloads, including:

| Partition | Pre-test SHA-256 |
|---|---|
| `system_dlkm_a` | `18f530dcb0e46dc81ede00e18ac4e9b39faf6564fb067f04af9a912d87fd6dd7` |
| `vendor_dlkm_a` | `40d4bd03e9d315aac562234019f6db192617bb1ab65532157b81022ebc7330e6` |
| `vendor_boot_a` | `5fe60f58ebe3f935acb3ec41585fa16977804cc0c2efd4e84c44a645a1eb7162` |

DTBO, VBMeta, metadata, userdata, radio/modem, persist, and slot metadata were
also untouched.

## TWRP FBE oracle

Recovery used the candidate kernel and reported TWRP
`3.7.1_16-OnePlus_15`.

| Gate | Result |
|---|---|
| Candidate kernel identity | PASS |
| Metadata encryption | PASS |
| Userdata mapper | PASS, `/dev/block/mapper/userdata` -> `/dev/block/dm-36` |
| F2FS `/data` mount | PASS |
| `inlinecrypt` mount option | PASS |
| PIN worker | PASS |
| User 0 decryption | PASS |
| CE media filenames/data | Accessible |

The recovery kernel and recovery log scans found no unknown symbols, version
disagreements, invalid module formats, CRC failures, signature failures,
protected-export failures, panic, oops, or storage-encryption failure.

## Android canary

Two controlled Android boots completed successfully.

| Gate | Boot 1 | Boot 2 |
|---|---:|---:|
| ADB returned | PASS | PASS |
| `sys.boot_completed=1` | PASS | PASS |
| User 0 | `RUNNING_UNLOCKED` | `RUNNING_UNLOCKED` |
| CE/system unlocked users | `[0]` | `[0]` |
| FBE/CE | PASS | PASS |
| APEX bootstrap failure | Not observed | Not observed |
| `init_user0_failed` | Not observed | Not observed |
| Fatal module/KMI scan | Empty | Empty |
| Targeted DRM lifetime scan | Empty | Empty |

Runtime canary observations after unlock:

- WPA3-SAE WLAN connected at 5240 MHz using 802.11be;
- cellular/RMNET registered with live IPv4/IPv6 data interfaces;
- Bluetooth initialized and remained `ON`;
- NFC initialized and remained `on`;
- USB/ADB remained functional;
- ZRAM was active with an 8 GiB backing device;
- CPUFreq policies used the expected `walt` governor;
- 664 modules were loaded without fatal module-contract errors;
- no persistent pstore record was produced.

Post-unlock scans were used to exclude expected direct-boot errors emitted
while CE storage was still lockscreen-locked.  The clean post-unlock samples
contained no `Required key not available`, unknown-symbol, CRC, vermagic,
signature, protected-export, FBE, APEX, panic, or targeted DRM lifetime
failure.

## Remaining full-qualification work

The following require hands-on functional confirmation before this state can
be called fully physically qualified or promoted as a stable release:

- camera capture;
- audio playback and call audio;
- fingerprint authentication;
- charging behavior;
- extended suspend/deep-idle cycles;
- longer-duration graphics/display use.

The exact tested `boot.img` remains installed on `boot_a`.  No rebuild was
performed after physical testing.
