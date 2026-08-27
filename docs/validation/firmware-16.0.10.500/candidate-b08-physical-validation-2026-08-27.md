# Candidate B08 physical validation — 2026-08-27

## Result

**PASS — B08 NETFILTER QUOTA2 COUNTER LIFETIME HARDENING PHYSICALLY
QUALIFIED ON OOS 16.0.10.500**

This result establishes physical compatibility and regression coverage for
Candidate B08. The procfs-creation-failure race was not deliberately induced,
so the accepted coverage classification is:

**NORMAL-RUNTIME COMPATIBILITY PASS — EXACT FIX TRIGGER NOT OBSERVED**

## Tested identity

| Field | Value |
|---|---|
| Device | OnePlus 15 CPH2747 / Canoe |
| Firmware | `CPH2747_16.0.10.500(EX01)` |
| Slot | `_a` |
| B07 qualified parent | `c2629b3fd92bc913921be48c60dc4b1ad7c68b94` |
| B08 runtime source head | `338c09465853ddbccde4861e14f8c8fa2f24342e` |
| Pre-physical documentation head | `1451c29ebd3ea1473d57e39bf4f7b0a1083af2a7` |
| Runtime kernel | `6.12.23-android16-5-o-g338c09465853-4k` |
| New runtime change | Netfilter quota2 counter lifetime hardening |

Candidate B08 cumulatively contains physically qualified B01 through B07 and
only the audited `q2_get_counter()` change added for B08. No ACK, DLKM,
vendor_boot, VBMeta, device-tree, slot-metadata, or other runtime change was
introduced.

## Payload and recovery isolation

Only `boot_a` was written.

| Partition | Physical SHA-256 | Result |
|---|---|---|
| boot_a | `66046855b5d44aae821336c6499b3b741866beb82b8413fbeaf7769b6b324c1a` | candidate input, write, recovery read-back, and Android read-back PASS |
| system_dlkm_a | `18f530dcb0e46dc81ede00e18ac4e9b39faf6564fb067f04af9a912d87fd6dd7` | exact stock read-back PASS |
| vendor_dlkm_a | `157db23ccfa516c7904d4e87614f62c3c99c2dcbb93cc3fc8ccd3388c2cc793f` | exact stock read-back PASS |
| vendor_boot_a | `3027d80a33fcc65f506d2d909db862657fe28e7eafc6e2af94193d67e6e617eb` | exact stock read-back PASS |

The system-DLKM load contract remained 82 `modules.load` entries with
`wwan.ko` at entry 28.

Recovery gates:

- device/slot guard: PASS, CPH2747/Canoe, `_a`;
- snapshot/update state: none;
- helper SHA-256:
  `84b035710c305be859aac72827489850b7d215ee372ab3341be849e051bfab50`;
- TWRP user-0 decryption was unavailable, so no credential-encrypted backup
  destination was used;
- independent full backup was created under `/tmp`, pulled to the host before
  the partition write, and retained at
  `out/oos1610500-custom-r53-b08-final/physical-20260827/rollback-independent/boot_a-b07.img`;
- the helper independently backed up `boot_a` under
  `/tmp/b08-helper-backup/controlled-stack-a-20260827-055843/`, and that backup
  was also pulled to the host before reboot;
- both backups: 100663296 bytes and
  `1ef69e85dce34a1aae60d531da327a6ce96ddaeabc097c56f2ee2fb3f486b5c1`;
- boot-only dry run: PASS, pre/post B07 hash unchanged;
- immediate complete post-write read-back: PASS.

## Boot and existing encrypted user0

| Test | Result |
|---|---|
| First Android boot | PASS |
| Second clean Android boot | PASS |
| Existing user0 | `RUNNING_UNLOCKED` on both boots after normal user unlock |
| `init_user0_failed` | NOT OBSERVED |
| Rescue Party / recovery redirect | NOT OBSERVED |
| Runtime boot hash | exact candidate on both boots and at final read-back |

No data format, metadata wipe, FBE policy change, vendor_boot/VBMeta change, or
slot-metadata change was used.

## Quota2 and network-policy coverage

OxygenOS loaded active IPv4 and IPv6 `bw_global_alert` quota2 rules and the
`bw_costly_rmnet_data2` quota2 rules before and after B08. The named proc
counters `globalAlert` and `rmnet_data2` were present on both boots.

| Test | Result |
|---|---|
| WLAN traffic | PASS; five parallel 50 MiB flows, 262144000 bytes requested and completed |
| WLAN quota movement | IPv4 `bw_global_alert` advanced from 9374 packets / 60468663 bytes to 44551 packets / 318985988 bytes |
| Cellular traffic | PASS; 100 MiB completed in two successful 50 MiB flows |
| Cellular quota movement | `rmnet_data2` decreased by 52752725 during the isolated retry flow, then netd refreshed the named counter normally |
| Network-policy queries | 100/100 PASS |
| Data-usage activity operations | 100/100 PASS |
| Final global quota counters | IPv4 48601 packets / 365761873 bytes; IPv6 2644 packets / 50862768 bytes |
| Netd / NetworkStack / system_server | PASS |

The first two-flow cellular attempt had one transient curl connect failure
while the route stabilized and one successful 50 MiB flow. The missing flow
was repeated after stabilization and passed; total completed cellular traffic
was 100 MiB. This bounded amount was used instead of 250 MiB to avoid assuming
the owner's mobile-data allowance while still proving real quota2 counter
movement.

The exact concurrent counter acquisition plus procfs creation failure was not
forced through memory or procfs failure injection. The exact trigger remains
**NOT OBSERVED**.

## Network transition results

| Test | Result |
|---|---|
| Wi-Fi/cellular handoff | 10/10 PASS; dual-stack RMNET and 6135 MHz WLAN return every cycle |
| Mobile data off/on | 10/10 PASS; RMNET IPv4/IPv6, IP, DNS, and multicast state |
| Airplane recovery | 10/10 PASS; cellular and WLAN restored every cycle |
| Deep idle/resume | 5/5 PASS; user0, WLAN, cellular, and network services healthy |
| Final WLAN | PASS; 6135 MHz WPA3-SAE, numeric IP and DNS 5/5 |
| Final cellular | PASS; Visible LTE HOME/IN_SERVICE, RMNET dual-stack and routes |

## Subsystem regression results

| Subsystem | Result |
|---|---|
| WLAN | PASS; 6135 MHz WPA3-SAE, Internet and recovery |
| Cellular | PASS; Visible LTE HOME/IN_SERVICE, RMNET IPv4/IPv6, routes, IP/DNS |
| Bluetooth | PASS; off/on recovery and existing HID reconnected |
| NFC | PASS; service on, Wallet launched, HCE registered, eSE1 route present |
| Camera | PASS; Oplus camera created `IMG20260827064704.heic` |
| Audio | PASS; ringtone played through the built-in speaker and user confirmed it was clear |
| Graphics/UI | PASS; Settings, Wallet, camera and launcher rendered normally |
| USB | PASS; ADB enumeration remained available |
| Framework | PASS; system_server, zygote, service managers, netd and surfaceflinger stable |
| Module load contract | PASS; 82 entries, `wwan.ko` entry 28 |

## Error and resource scan

Full recovery, boot, quota, traffic, network-transition, subsystem, partition,
dmesg and logcat evidence is retained under:

`out/oos1610500-custom-r53-b08-final/physical-20260827/`

The final targeted scans found no quota2/netfilter fault, use-after-free,
refcount failure, double free, invalid pointer, list corruption, Oops, BUG,
KASAN, UBSAN, panic, RCU stall, module/CRC/vermagic/signature failure,
`init_user0_failed`, or critical framework/network crash loop. Bounded final
resource samples showed no monotonic file-descriptor or quota-object leak.

## Boot-only phase decision

B08 does not select B09. The remaining SFQ fixes have no observed traffic on
the attached SFQ children, UCSI crosses the retained QTI GLINK/firmware
provider boundary, SKB shared-fragment work changes broad networking semantics,
and the remaining HID path lacks matching test hardware. None currently has a
favorable enough reachability/value/risk balance for another boot-only batch.

**BOOT-ONLY CUSTOMIZATION PHASE COMPLETE**

## Decision

**PASS — CANDIDATE B08 NETFILTER QUOTA2 COUNTER LIFETIME HARDENING PHYSICALLY
QUALIFIED**

No rollback was required. This qualification is a compatibility/stability
result. It does not authorize main promotion, ACK advancement, DLKM adaptation,
B09, or release publication.
