# Linux 6.12.89 KMI5 OxygenOS 16.0.9.400 boot canary

## Classification

`BOOT_QUALIFIED_CANARY`

The Linux 6.12.89 KMI5 candidate passed the boot-only encrypted-storage and
Android canary on the same OxygenOS 16.0.9.400 device that qualified 6.12.81.
This is not full hands-on release qualification.

## Tested identity

| Field | Value |
|---|---|
| Device | OnePlus 15 CPH2749 / OP611FL1 |
| Firmware | `CPH2749_16.0.9.400(EX01)` |
| Slot | `_a` |
| Branch | `experiment/oos1609400-kmi5-linux-6.12.89-from-6.12.81` |
| Production-build source | `4f3252651aa31f789accfd681f004f76ea651230` |
| Kernel | `6.12.89-android16-5-o-g4f3252651aa3-4k` |
| KMI generation | 5 |
| Production Image SHA-256 | `e6986b589082c4467b4e342fef322303f5526ba0644474528a2ed2fefe2987e2` |
| Tested `boot.img` SHA-256 | `babb6073b3df0ec2536232d2e9eb6b7039458db716a8aa20e14b6d37da5a62c3` |

The boot image is 100,663,296 bytes and passed header, embedded Image,
retained GKI-signature-tail, and outer SHA256_RSA4096 AVB verification before
the physical test.

## Backup and write verification

The device began healthy on the exact 6.12.81 canary:

```text
6.12.81-android16-5-o-g3e933dfd0176-4k
```

Android reported `sys.boot_completed=1`, encrypted file-based storage, and
user 0 `RUNNING_UNLOCKED` before the test.  TWRP resolved active slot `_a` to
`/dev/block/sde12`, measured a 100,663,296-byte `boot_a`, and created a full
recovery-side and host-side backup.  Both copies matched the qualified 6.12.81
SHA-256:

```text
aebbf681a12af58d3e72a0983cea207ed9fcf90bebf00f1d146963e840d891a7
```

Evidence and rollback artifact:

```text
/home/travis/Android/kernel-test-backups/linux-6.12.89-kmi5-20260922-060304
```

Only `boot_a` was written.  The complete immediate recovery read-back and
host-pulled read-back both matched the candidate SHA-256 exactly.  Rollback
was not required.

## Immutable supporting payloads

No supporting partition was written.  `system_dlkm`, `vendor_dlkm`,
`vendor_boot`, DTBO, VBMeta, dynamic-partition metadata, userdata, metadata,
radio/modem, and persist remained untouched.

## TWRP encrypted-storage oracle

TWRP `3.7.1_16-OnePlus_15` rebooted using the candidate kernel and recorded:

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

The recovery log contains `User 0 Decrypted Successfully` and
`Data successfully decrypted`.  No unknown symbol, version/CRC disagreement,
invalid module, signature, protected-export, panic, oops, fscrypt,
blk-crypto, dm-default-key, ICE, or UFS-crypto failure was found.  Recovery
QMI/deferred-probe messages occurred in its reduced userspace but did not
block storage initialization, decryption, or the subsequent Android boots.

## Android canary

Two clean Android boots passed:

| Gate | Boot 1 | Boot 2 |
|---|---:|---:|
| Candidate kernel identity | PASS | PASS |
| `sys.boot_completed=1` | PASS | PASS |
| User 0 after credential entry | `RUNNING_UNLOCKED` | `RUNNING_UNLOCKED` |
| F2FS `/data` with `inlinecrypt` | PASS | PASS |
| Runtime modules | 664 | 664 |
| WLAN | WPA3-SAE, 802.11be, connected at 5240 MHz | WPA3-SAE, 802.11be, connected at 5240 MHz |
| Cellular | LTE voice/data registered; RMNET data call observed | LTE voice/data registered |
| Bluetooth | ON | ON |
| NFC | ON | ON |
| USB/ADB | PASS | PASS |
| APEX mounts | 76; no bootstrap failure | healthy; no bootstrap failure |
| Fatal kernel/module/FBE scan after unlock | Empty | Empty |
| Targeted DRM lifetime scan | Empty | Empty |
| Persistent pstore record | None | None |

The only FBE-pattern match after user unlock was the existing Oplus
`.recovery_repair` cleanup warning.  It did not affect decryption, user0,
F2FS, APEX, or either Android boot.  No
`drm_gem_object_handle_put_unlocked`, `msm_ioctl_rmfb2`, handle-count, or DRM
refcount lifetime warning was observed.

## Remaining qualification

Camera, audio/call audio, fingerprint authentication, charging, longer
graphics use, Wi-Fi 6 GHz, and extended suspend/deep-idle remain hands-on
tests before this can be called fully physically qualified or promoted as a
stable release.

The exact tested 6.12.89 image remains installed on `boot_a`.  Main, remote
refs, tags, and releases remain unchanged.
