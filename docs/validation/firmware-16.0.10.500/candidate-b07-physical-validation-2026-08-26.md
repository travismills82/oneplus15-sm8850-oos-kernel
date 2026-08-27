# Candidate B07 physical validation — 2026-08-26

## Result

**PASS — B07 IPV6 MLD QUERY SKB LIFETIME HARDENING PHYSICALLY QUALIFIED ON
OOS 16.0.10.500**

This result establishes physical compatibility and regression coverage for
Candidate B07. The exact MLD-query skb relocation condition was not directly
observable, so the accepted coverage classification is:

**NORMAL-RUNTIME COMPATIBILITY PASS — EXACT FIX TRIGGER NOT OBSERVED**

## Tested identity

| Field | Value |
|---|---|
| Device | OnePlus 15 CPH2747 / Canoe |
| Firmware | `CPH2747_16.0.10.500(EX01)` |
| Slot | `_a` |
| B06 qualified parent | `67ddf38649ea9b60fe30ec7be2a352620fe8ea2f` |
| B07 runtime source head | `969639e8ca81ec5048338b4366cf17de28941029` |
| Pre-physical documentation head | `e4cecaaa09aba5c8e8a62cabad6c3bfedc793d52` |
| Runtime kernel | `6.12.23-android16-5-o-g969639e8ca81-4k` |
| New runtime change | IPv6 MLD query skb lifetime hardening |

Candidate B07 cumulatively contains physically qualified B01 through B06 and
only the audited `net/ipv6/mcast.c` change added for B07. No ACK, DLKM,
vendor_boot, VBMeta, device-tree, slot-metadata, or other runtime change was
introduced.

## Payload and recovery isolation

Only `boot_a` was written.

| Partition | Physical SHA-256 | Result |
|---|---|---|
| boot_a | `1ef69e85dce34a1aae60d531da327a6ce96ddaeabc097c56f2ee2fb3f486b5c1` | candidate input, write, recovery read-back, and Android read-back PASS |
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
- independent full backup:
  `/sdcard/TWRP/kernel-flash-backups/b07-physical-20260826-independent/boot_a.img`;
- helper backup:
  `/sdcard/TWRP/kernel-flash-backups/b07-physical-20260826/controlled-stack-a-20260826-184217/boot_a.img`;
- both backup size/hash checks: PASS, 100663296 bytes and
  `31e6fff0b4212916b64614c4ec96c4f88c8f8cd7168e720e5f77c05b1d402825`;
- decrypted writable backup destination: PASS;
- boot-only dry run: PASS, pre/post B06 hash unchanged;
- immediate complete post-write read-back: PASS.

## Boot and existing encrypted user0

| Test | Result |
|---|---|
| First Android boot | PASS |
| Second clean Android boot | PASS |
| Existing user0 | `RUNNING_UNLOCKED` on both boots |
| `init_user0_failed` | NOT OBSERVED |
| Rescue Party / recovery redirect | NOT OBSERVED |
| Runtime boot hash | exact candidate on both boots and at final read-back |

No data format, metadata wipe, FBE policy change, vendor_boot/VBMeta change, or
slot-metadata change was used.

## IPv6 multicast coverage

IPv6 multicast membership was confirmed on `wlan0` and active RMNET
interfaces before and after stress. The bounded arm64 harness exercised IPv6
multicast socket creation, group join/leave, send/receive, close, and
recreation.

| Path | Duration | Workers | Joins / leaves / closes | Sends | Receives | Unexpected errors |
|---|---:|---:|---:|---:|---:|---:|
| wlan0 | 300.005 s | 4 | 2,723,911 each | 2,229,274 | 2,229,258 | 0 |
| rmnet_data2 | 60.001 s | 2 | 1,520,766 each | 1,520,766 | 1,520,732 | 0 |

The WLAN run accounted for 494,637 expected transition/routing errors:
494,594 `ENETUNREACH` and 43 `EADDRNOTAVAIL`. There were no unexpected errno
classes or worker failures. The RMNET run had zero expected or unexpected
errors.

The device's WLAN had only link-local IPv6 and no usable global IPv6 route
both before B07 and at final validation. This firmware/network baseline state
is unchanged. Cellular RMNET retained global IPv6 and passed a final 5/5 IPv6
Internet test. IPv6 multicast membership and local multicast traffic passed
on both paths.

No existing tracepoint or counter proved the precise received-query,
`pskb_may_pull()` relocation, and later group-address-use sequence. The exact
fix trigger remains **NOT OBSERVED**.

## Network transition results

| Test | Result |
|---|---|
| Mobile data off/on | 10/10 PASS; dual-stack RMNET, IP, DNS, and multicast membership |
| Airplane recovery | 9/10 in the initial run plus an isolated replacement recovery 1/1 PASS |
| Wi-Fi/cellular handoff | 10/10 PASS; every cellular leg dual-stack and every WLAN return 6135 MHz |
| Final cellular-only | PASS; Visible LTE HOME/IN_SERVICE, RMNET IPv4/IPv6, routes, IP and DNS 5/5 |
| Final WLAN return | PASS; 6135 MHz WPA3-SAE, IPv4/DNS 5/5, multicast membership restored |

The original 25-cycle Wi-Fi run completed 23 returns inside its 30-second
observation window. Cycles 11 and 15 timed out without a kernel or framework
fault; following cycles recovered and the remainder completed normally. Per
the subsequently narrowed test scope, the full 25-cycle run was not repeated.
The later 10/10 handoff series, five deep-idle recoveries, and final WLAN
return all recovered in two to three seconds on 6135 MHz. The two observation
timeouts are retained here and classified as non-blocking stabilization
observations, not hidden or counted as 25/25.

## Subsystem regression results

| Subsystem | Result |
|---|---|
| WLAN | PASS; 6135 MHz WPA3-SAE, Internet, multicast membership, and final recovery |
| Cellular | PASS; Visible LTE HOME/IN_SERVICE, RMNET IPv4/IPv6, routes, IP/DNS |
| Bluetooth | PASS; off/on recovery and existing HID reconnected |
| NFC | PASS; service on, Wallet launched, HCE registered, eSE1 route present |
| Camera | PASS; Oplus camera created `IMG20260826193223.heic` |
| Audio | PASS; ringtone played through the built-in speaker and user confirmed it was audible |
| Graphics/UI | PASS; system, Wallet, camera, and launcher rendered normally |
| USB | PASS; ADB enumeration remained available |
| Deep idle/resume | 5/5 PASS; user0, WLAN, cellular, and IPv6 membership remained healthy |
| Framework | PASS; system_server, zygote, service managers, netd, and surfaceflinger stable |
| Module load contract | PASS; 82 entries, `wwan.ko` entry 28 |

## Error scan

Full recovery, boot, multicast, network-transition, subsystem, deep-idle,
partition, dmesg, and logcat evidence is retained under:

`out/oos1610500-custom-r53-b07-final/physical-20260826/`

The accepted scans found no IPv6/MLD fault, use-after-free, refcount failure,
double free, invalid pointer, list corruption, Oops, BUG, KASAN, UBSAN, panic,
RCU stall, module/CRC/vermagic/signature failure, `init_user0_failed`, or
critical framework/network crash loop. A WLAN-driver log containing `MLD ID`
referred to Wi-Fi multi-link-device state, not IPv6 multicast-listener
discovery. A framework telemetry line listing a `crashSys` field was not a
crash event.

## Decision

**PASS — CANDIDATE B07 IPV6 MLD QUERY SKB LIFETIME HARDENING PHYSICALLY
QUALIFIED**

No rollback was required. This qualification is a compatibility/stability
result. It does not authorize main promotion, ACK advancement, DLKM
adaptation, B08, or release publication.
