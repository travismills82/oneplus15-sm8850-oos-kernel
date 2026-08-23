# Cellular Batch 01 physical validation

Date: 2026-08-22

Branch: `experiment/cellular-batch-01`

Tested HEAD: `1c89a93a7c350872ccb9e9da18674dbbbbe8ffaf`

Status: **PASS — CELLULAR SOURCE-MIGRATION MECHANISM VALIDATED**

This test does not validate a newer cellular source generation. The reviewed
`rmnet_sch` runtime source has no meaningful `.097` to `.102` behavior delta.
The one-variable experiment validates source ownership, controlled-v1 module
signing, delivery through vendor-DLKM, and operation against the frozen g6744
kernel contract without a cellular regression.

## Scope and payload identity

Only `vendor_dlkm_b` was changed. Boot, system-DLKM, vendor boot, VBMeta, and
slot metadata were not written.

| Item | Identity |
| --- | --- |
| kernel release | `6.12.23-android16-5-o-g6744a3f6bcf4-4k` |
| slot | `_b` |
| changed module | `rmnet_sch.ko` only |
| candidate vendor-DLKM SHA-256 | `640c4f380d1ef8f1d23cd20d4e097f999f04f4d2f3e0c1fc13c1308d1b2ee958` |
| candidate vendor-DLKM bytes | 143,986,688 |
| packaged `rmnet_sch.ko` SHA-256 | `4fdb7d1122e430731594c7eea9dc2e8686cd7c2668e05d39ef66bfa18b0c75b4` |
| qualified rollback vendor-DLKM SHA-256 | `3ed964f345e6f5040c70ef7c0c083c1fc4bab536b6a522ca83c61b20be032ed4` |

The pre-flash Android state matched the frozen controlled stack: CPH2747,
Canoe, slot `_b`, Android boot complete, the exact g6744 kernel release,
Visible LTE registered HOME/IN_SERVICE, a working dual-stack RMNET PDN, and
`wwan` loaded. Cellular IPv4, IPv6, and DNS each passed 5/5 probes before the
device entered recovery.

## Recovery safety and write verification

USB, ADB authorization, device identity, and live Android state were rechecked
after the interrupted connection. Recovery reported CPH2747/Canoe, slot `_b`,
and Virtual A/B update state `none`. User 0 storage was decrypted and the
backup destination was writable with sufficient free space.

The packed recovery did not expose the helper in its runtime filesystem. The
exact hardened helper from local TWRP commit
`3f499bfd1f7152ea27b27935be22ff73581709a1` was therefore staged temporarily
under `/tmp`; its SHA-256 was
`84b035710c305be859aac72827489850b7d215ee372ab3341be849e051bfab50`.
No recovery partition was modified.

Two independently verified rollback copies were available:

- `/sdcard/TWRP/kernel-flash-backups/cellular-batch-01-preflash-b-20260822-162854/`
- `/sdcard/TWRP/kernel-flash-backups/controlled-stack-b-20260822-162931/`

Both full-size backups were 143,986,688 bytes and matched the source partition
SHA-256 `3ed964f345e6f5040c70ef7c0c083c1fc4bab536b6a522ca83c61b20be032ed4`.

The vendor-DLKM-only dry run passed the device, slot, snapshot, capacity, ext4,
AVB structure, input hash, and backup-destination checks and reported no
partition modification. The flash then changed only `vendor_dlkm_b`. Its
immediate full-partition read-back matched the candidate SHA-256 exactly, and
the ext4 structural check passed.

Reference hashes before and after the write prove the other selected-slot
partitions did not change:

| Partition | SHA-256 before and after |
| --- | --- |
| `boot_b` | `84a85b9a40adc7bdb178bf86e2da74e9bac3feaf7bd502c7d29b1df27dea55ab` |
| `system_dlkm_b` block image | `453590959bf0c66e674bdcc2e29bdbc32303828454b90071d21cbdfc7002e640` |
| `vendor_boot_b` | `5fe60f58ebe3f935acb3ec41585fa16977804cc0c2efd4e84c44a645a1eb7162` |
| `vbmeta_b` | `270792710e9c480cf31e023f0c711c9063a7263324ec2c616ec4cf36bd346ad2` |

## Runtime replacement proof

Android booted with the unchanged g6744 kernel. `rmnet_sch` and `wwan` were
both present in `/proc/modules`. The mounted runtime module path was
`/vendor_dlkm/lib/modules/rmnet_sch.ko`, and its live file SHA-256 was exactly:

`4fdb7d1122e430731594c7eea9dc2e8686cd7c2668e05d39ef66bfa18b0c75b4`

That matches the packaged controlled-v1 module. The static contract records
the signer as `OnePlus 15 Controlled OOS Module Signing v1`, exact g6744
vermagic, 9/9 imports resolved, zero CRC mismatches, and zero protected-export
failures. The other 26 cellular modules remained the qualified binaries in the
candidate image.

## Cellular results

| Test | Result |
| --- | --- |
| first Android boot | PASS |
| registration | PASS — Visible LTE HOME/IN_SERVICE |
| data-call result | PASS — `NONE(0x0)` |
| active PDN | PASS — `rmnet_data2` or `rmnet_data3`, as assigned by the modem |
| IPv4 address and route | PASS |
| IPv6 address and route | PASS |
| IPv4 connectivity | PASS — 5/5 to `1.1.1.1` |
| IPv6 connectivity | PASS — 5/5 to Cloudflare IPv6 |
| DNS/name connectivity | PASS — 5/5 to `google.com` |
| mobile-data OFF/ON | PASS — 10/10 |
| airplane-mode cycles | PASS — 10/10 |
| Wi-Fi/cellular handoff | PASS — 5/5 |
| forced deep-idle/wake | PASS — 10/10 |
| clean reboots | PASS — 2/2 |
| cellular traffic stress | PASS — 104,857,600-byte HTTPS transfer |

The 100 MiB transfer completed with HTTP 200 in 108.87 seconds at an average
963,156 bytes/s. The active RMNET interface recorded no RX/TX errors or drops,
and dual-stack IP plus DNS still passed after the transfer.

The mobile-data test harness initially marked cycle 1 as failed because of a
remote shell parsing error. The live interface, routes, and IPv4/IPv6/DNS
probes were immediately checked independently and all passed; this is recorded
as a harness false negative, not a device failure.

## Qdisc evidence

`tc` was available. The active `rmnet_data2` path showed the stock HTB, PPQ,
TSD, SFQ, and `clsact` qdiscs during traffic, with no qdisc packet drops. No
qdisc with the `rmnet_sch` identifier was attached. Therefore this run proves
that the controlled module loads and coexists with the cellular data plane; it
does **not** claim that `rmnet_sch` enqueue/dequeue behavior was exercised.

## Hotspot and regression checks

Hotspot start and stop passed. Android created `wlan2` in `TetheredState` with
`lastError=0`, selected `rmnet_data2` as the upstream, and configured IPv4 and
IPv6 tethering. The hotspot stopped cleanly and cellular connectivity remained
healthy. Client association, DHCP, DNS, and client traffic were **NOT TESTED —
EQUIPMENT UNAVAILABLE**.

| Existing subsystem | Result |
| --- | --- |
| WLAN `.053` | PASS — 6135 MHz WPA3 association, reload, and Internet |
| Bluetooth `.046` | PASS — user OFF/ON transition and return to full `ON`; zero Bluetooth framework crashes |
| Bluetooth device reconnect | NOT TESTED — no connected peripheral available |
| NFC `.102` | PASS — verified root-authorized service OFF/ON transition |
| Wallet/HCE availability | PASS — Wallet installed, payment HCE service registered, NFC services healthy |

Android retained `BLE_ON` while user Bluetooth was disabled because background
BLE scanning was registered. The user setting was off, and re-enabling returned
the full adapter state to `ON`; this is normal framework behavior rather than a
toggle failure.

## Error scan

Full `dmesg` and `logcat -b all` were captured after the complete test. There
were no new relevant:

- unknown symbols or MODVERSION disagreements;
- vermagic, signature, or protected-export failures;
- `rmnet_sch` errors;
- kernel oops, panic, KASAN, or UBSAN reports;
- cellular data-call failures.

OxygenOS emitted its existing boot warnings for duplicate proc/IRQ/touch
registration and periodic vendor hung-task diagnostic traces for `adci_thread`,
`zram_comp`, `osml_monitor`, and `hfi_core_dbg_cl`. The same warning signatures
are present in the frozen controlled baseline captures; none implicates
`rmnet_sch` or the cellular data plane.

Raw evidence is retained outside git at:

`/home/travis/Android/cellular-batch-01-live-captures/20260822T212407Z-preflight/`

## Decision

**PASS — CELLULAR SOURCE-MIGRATION MECHANISM VALIDATED**

This result authorizes no additional cellular replacement by itself. Batch 01
must not be described as cellular `.102` validation, and no subsequent batch
should be combined with it until separately built, reviewed, and physically
tested as its own one-variable experiment.
