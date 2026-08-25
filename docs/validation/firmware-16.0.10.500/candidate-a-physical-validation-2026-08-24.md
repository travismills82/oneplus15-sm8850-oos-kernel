# Candidate A physical validation — 2026-08-24

## Result

**PASS — FIRMWARE-NATIVE R53 SOURCE KERNEL BOOTS OOS 16.0.10.500 WITH
EXISTING ENCRYPTED USERDATA AND EXACT STOCK DLKMS**

This is a temporary firmware-compatibility baseline, not the final controlled
OOS 16.0.10.500 stack. Candidate B was not prepared or flashed.

## Tested identity

| Field | Value |
|---|---|
| Device | OnePlus 15 CPH2747 / Canoe |
| Firmware | `CPH2747_16.0.10.500(EX01)` |
| Slot | `_a` |
| Source branch | `experiment/oos1610500-ack-r53-base` |
| Runtime source head | `99077ba8e1792b46d341d60f446b79d260f5f639` |
| ACK base | `android16-6.12-2025-06_r53` / `b2a876903b495c444a94b16f50d1463ffe953957` |
| Runtime kernel | `6.12.23-android16-5-o-g99077ba8e179-4k` |

## Payload isolation

Only `boot_a` was written.

| Partition | Tested SHA-256 | Result |
|---|---|---|
| boot_a | `30195cbfe97e3cd831b4e84ed575c68cb5d079f18bbed1e6ee84a91a32cdfb70` | candidate write/read-back PASS |
| system_dlkm_a | `18f530dcb0e46dc81ede00e18ac4e9b39faf6564fb067f04af9a912d87fd6dd7` | exact stock, unchanged |
| vendor_dlkm_a | `157db23ccfa516c7904d4e87614f62c3c99c2dcbb93cc3fc8ccd3388c2cc793f` | exact stock, unchanged |
| vendor_boot_a | `3027d80a33fcc65f506d2d909db862657fe28e7eafc6e2af94193d67e6e617eb` | exact stock, unchanged |

No system_dlkm, vendor_dlkm, vendor_boot, VBMeta, or slot-metadata payload was
supplied to the flash helper.

## TWRP safety record

- recovery helper lineage:
  `3f499bfd1f7152ea27b27935be22ff73581709a1`
- live helper SHA-256:
  `84b035710c305be859aac72827489850b7d215ee372ab3341be849e051bfab50`
- device/slot guard: PASS, CPH2747/Canoe, `_a`
- TWRP user 0 decryption: PASS
- snapshot/update state: `none`
- dry run: PASS, boot only, no partitions modified
- backup directory:
  `/sdcard/TWRP/kernel-flash-backups/controlled-stack-a-20260824-225419`
- verified stock boot backup SHA-256:
  `b811796df7ce5fa3b6da07fd00b65c2f9d7bbe1c38939f91f2fc48cc12ada46d`
- candidate input SHA-256: PASS
- complete boot partition read-back SHA-256: PASS
- independent post-write stock DLKM/vendor_boot hashes: PASS

## Boot and encrypted-user0 validation

| Test | Result |
|---|---|
| First Android boot | PASS, `sys.boot_completed=1` at 18 seconds |
| Second clean Android boot | PASS, `sys.boot_completed=1` at 19 seconds |
| Recovery/Rescue Party return | none |
| Existing user 0 before credential | `RUNNING_LOCKED`, expected |
| Existing user 0 after credential | `RUNNING_UNLOCKED` |
| Credential-encrypted emulated storage | readable; existing directories present |
| Android firmware identity | `CPH2747_16.0.10.500(EX01)` |

This directly resolves Candidate A's physical question: a source-built
firmware-native r53 kernel can initialize and unlock the existing encrypted
user0 on OOS 16.0.10.500 without changing either DLKM.

## Runtime module and subsystem sanity

- `wwan`, `qca_cld3_peach_v2`, `rmnet_core`, `rmnet_sch`, `ipam`, `gsim`, and
  the stock new `oplus_bsp_file_read_record` were loaded.
- Wi-Fi: PASS, WPA3-SAE, 6135 MHz, Internet recovery after off/on.
- Cellular registration: PASS, Visible LTE HOME/IN_SERVICE.
- RMNET: PASS, active `rmnet_data2` with IPv4 and IPv6.
- Cellular policy routes: PASS, IPv4 and IPv6 defaults present.
- Cellular IP: 5/5 PASS to `1.1.1.1`.
- Cellular DNS: 5/5 PASS to `google.com`.
- Bluetooth: OFF/ON PASS; state returned ON and an existing HID device
  reconnected.
- NFC: service enabled. Shell-command toggle was not accepted by the stock NFC
  service's package/AppOps policy, so no NFC toggle claim is made for this
  boot-kernel isolation test.
- USB: ADB enumeration PASS after both boots.

## Error scan

Full first- and second-boot `logcat -b all`, properties, and `/proc/modules`
were retained outside the repository under:

`/home/travis/Android/oos16.0.10.500-kernel-compat/captures/candidate-a-physical-20260824/`

No new relevant match was found for:

- `init_user0_failed`
- Unknown symbol, CRC disagreement, invalid vermagic, module signature, or
  protected-export failure
- kernel panic, Oops, BUG, KASAN, UBSAN, or Call trace
- IOMMU/SMMU fault
- fscrypt, dm-default-key, blk-crypto, inline-crypto, vold, or cryptfs failure

The Android shell was not permitted to read the kernel log buffer directly;
the available all-buffer boot logs plus runtime module state were scanned. No
claim beyond those available logs is made.

## Interpretation and next boundary

The prior `init_user0_failed` result is not an inherent failure of stock OOS
16.0.10.500 DLKMs or of a source-built official r53 kernel. The exact first-bad
change remains somewhere between this passing firmware-native Candidate A and
the previous customized ACK 6.12.24 boot: project customizations, the ACK
6.12.24 delta, or the former boot-container/build/trust integration.

The next authorized ladder step is Candidate B: reapply required project
kernel customizations in small audited groups while continuing to use the
exact stock 16.0.10.500 DLKMs. That work must start from this tested runtime
source identity and must not skip directly to ACK 6.12.24.
