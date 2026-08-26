# Candidate B04 physical validation — 2026-08-25

## Result

**PASS — AF_PACKET FANOUT LIFETIME HARDENING PHYSICALLY VALIDATED ON
OOS 16.0.10.500**

Candidate B04 cumulatively contains the physically qualified B01 Binder
lifetime hardening, B02 BPF per-CPU map bounds hardening, B03 eventpoll file
and RCU lifetime hardening, and one new coherent runtime change in
`net/packet/af_packet.c`. The B04 change prevents a NETDEV_UP/socket-close race
from re-registering a closing fanout socket and leaving a dangling fanout
pointer. No ACK advancement, DLKM adaptation, or other customization was
included.

## Tested identity

| Field | Value |
|---|---|
| Device | OnePlus 15 CPH2747 / Canoe |
| Firmware | `CPH2747_16.0.10.500(EX01)` |
| Slot | `_a` |
| B03 qualified parent | `8f3ee08db58afff44881694676edd678d0baffe1` |
| B04 runtime source head | `2f2631b951ced2ef05a4a9643610954b26736bcd` |
| Pre-physical documentation head | `a08326537994cf35d1486080e6e35836caf85a84` |
| New runtime change | AF_PACKET fanout lifetime hardening |
| Runtime kernel | `6.12.23-android16-5-o-g2f2631b951ce-4k` |

The source and machine-code proof for `packet_notifier()` and
`packet_release()` is recorded in `candidate-b04-binary-delta.md`.

## Payload isolation

Only `boot_a` was written.

| Partition | Physical SHA-256 | Result |
|---|---|---|
| boot_a | `05785dc9ba171548790699c09aa2bbc9172062cfa3fc0f516d6472a944bd4ed3` | candidate input, write, recovery read-back, and Android read-back PASS |
| system_dlkm_a | `18f530dcb0e46dc81ede00e18ac4e9b39faf6564fb067f04af9a912d87fd6dd7` | exact stock, unchanged |
| vendor_dlkm_a | `157db23ccfa516c7904d4e87614f62c3c99c2dcbb93cc3fc8ccd3388c2cc793f` | exact stock, unchanged |
| vendor_boot_a | `3027d80a33fcc65f506d2d909db862657fe28e7eafc6e2af94193d67e6e617eb` | exact stock, unchanged |

The firmware-native system-DLKM contract remained 82 `modules.load` entries,
with `wwan.ko` at entry 28. Neither DLKM, vendor_boot, VBMeta, nor slot metadata
was supplied to the flash operation.

## TWRP safety record

- device/slot guard: PASS, CPH2747/Canoe, `_a`
- hardened helper SHA-256:
  `84b035710c305be859aac72827489850b7d215ee372ab3341be849e051bfab50`
- independent full `boot_a` backup:
  `/home/travis/Android/oos16.0.10.500-kernel-compat/backups/b04-preflash-20260825/boot_a.img`
- source and backup size: 100663296 bytes
- source and backup SHA-256:
  `0b065aa6fc5c524107ecdf10ffefb41b464be86bfb9b8673b8fb649f5541dfc0`
- helper backup and manifest were also pulled to the host
- exact B03 rollback artifact: present and hash-verified
- boot-only dry-run: PASS, no partition modified
- helper flash target: `boot_a` only
- complete immediate `boot_a` read-back: PASS
- untouched stock DLKM and `vendor_boot_a` read-backs: PASS

## Boot and encrypted-user0 validation

| Test | Result |
|---|---|
| First Android boot | PASS |
| Second clean Android boot | PASS |
| Existing user 0, boot 1 | `RUNNING_UNLOCKED` |
| Existing user 0, boot 2 | `RUNNING_UNLOCKED` |
| `vdc cryptfs init_user0` | status 0 on both boots |
| `init_user0_failed` | NOT OBSERVED |
| Rescue Party/TWRP redirect | NOT OBSERVED |
| Prolonged OnePlus-logo stall | NOT OBSERVED |

The current encrypted userdata was unlocked intact on both boots. No format,
metadata wipe, encryption-policy change, vendor_boot change, VBMeta change, or
slot-metadata change was used.

## AF_PACKET-specific physical coverage

A dynamically linked Android arm64 harness used fixed worker counts to create,
bind, join, receive on, close, and recreate both SOCK_RAW and SOCK_DGRAM
AF_PACKET fanout sockets on `wlan0`. The harness ran concurrently with actual
WLAN NETDEV down/up transitions.

The accepted bounded run produced:

| Metric | Result |
|---|---:|
| Duration | 300 seconds |
| Workers | 4 |
| Teardown loops | 2,254,584 |
| AF_PACKET sockets | 4,509,168 |
| Binds | 4,509,168 |
| Fanout joins | 4,509,168 |
| Socket closes | 4,509,168 |
| Packets sampled | 894 |
| Expected nonblocking EAGAIN | 4,508,274 |
| ENODEV/ENETDOWN/EBADF/EINVAL/EEXIST | 0 |
| Unexpected errors | 0 |

The runtime exposed no production tracepoint or counter proving that a
specific close operation interleaved with the exact notifier instruction
window. High path coverage alone is not proof of the vulnerable interleaving.
The correct coverage classification is:

**NORMAL-RUNTIME COMPATIBILITY PASS — EXACT FIX TRIGGER NOT OBSERVED**

## Network stress

| Test | Result |
|---|---|
| Wi-Fi OFF/ON concurrent with fanout churn | 25/25 PASS |
| Recovered WLAN frequency | 6135 MHz on all 25 accepted cycles |
| Airplane mode | 10/10 PASS |
| Wi-Fi/cellular/Wi-Fi handoff | 5/5 PASS after route stabilization |
| WLAN transfer | 104857600 bytes, HTTP 206, PASS |
| WLAN concurrent ping | 100/100, 0% loss |
| Cellular-only transfer | 104857600 bytes, HTTP 206, PASS |
| Cellular concurrent ping | 100/100, 0% loss |
| WLAN RX/TX errors | 0/0 |
| RMNET RX/TX errors | 0/0 |

The first one-shot handoff diagnostic sampled the path before Android had
settled its default network and saw transient first-attempt ping loss on three
of five cycles. Registration, dual-stack RMNET, and routes remained present.
The accepted bounded stabilization run completed 5/5: four cycles passed on
the second attempt and one passed on the first. Final stabilized WLAN and
cellular IP/DNS tests were each 5/5. This is classified as transient route
stabilization, not a B04 regression.

An initial airplane script diagnostic incorrectly required RMNET's operational
state string to be `UP`; Android reports the active RMNET data interface as
`UNKNOWN` while the link flags are UP/LOWER_UP. That diagnostic was stopped,
the test predicate was corrected without changing the candidate, and a clean
10/10 airplane run was performed from zero.

## Subsystem regression results

| Subsystem | Result |
|---|---|
| WLAN | PASS; 6135 MHz WPA3-SAE, Internet, 25/25 reloads, 100 MiB traffic |
| Cellular | PASS; Visible LTE HOME/IN_SERVICE, RMNET IPv4/IPv6, routes, fail cause NONE/0x0, IP/DNS, 100 MiB traffic |
| Bluetooth | PASS; off/on recovery and existing HID device reconnected |
| NFC | PASS; service on, Google Wallet/Pay HCE registered, eSE1 route present |
| Camera | PASS; active Oplus camera saved `IMG20260825224651.heic` (2026512 bytes) |
| Audio | PASS; ringtone media session PLAYING on built-in speaker, user confirmed clear |
| Graphics/UI | PASS; normal rendering and camera/system UI operation |
| USB | PASS; ADB/USB enumeration survived both clean boots |
| Deep idle/resume | PASS, 5/5 |
| Framework/network services | PASS; system_server, netd, NetworkStack and ConnectivityService stable |

## Error scan

Full preflight, recovery, dmesg, logcat, AF_PACKET harness, network transition,
traffic, handoff, camera, audio, and deep-idle evidence is retained under:

`out/oos1610500-custom-r53-b04-final/physical-20260825/`

The accepted final scan found no new relevant match for:

- AF_PACKET/fanout fault, dangling fanout state, or netdevice failure
- use-after-free, refcount failure, double-free, list corruption, Oops, BUG,
  KASAN, UBSAN, or panic
- Unknown symbol, CRC/MODVERSION disagreement, invalid vermagic, signature,
  or protected-export failure
- IOMMU/SMMU fault
- system_server, zygote, servicemanager, hwservicemanager, netd, NetworkStack,
  ConnectivityService, or framework crash loop
- `init_user0_failed`, fscrypt, metadata-encryption, or CE-unlock failure

An earlier snapshot recorded the same OEM informational hung-task trace heads
for `adci_thread` and `zram_comp` seen on the already qualified B01/B02
baselines. They are unrelated to AF_PACKET, caused no functional stall, and
were absent from the final accepted scan after all subsystem tests.

## Decision

**PASS — CANDIDATE B04 AF_PACKET FANOUT LIFETIME HARDENING PHYSICALLY
VALIDATED**

No rollback was required. This qualification covers Candidate A plus
B01+B02+B03+B04 only. It does not authorize ACK advancement, DLKM adaptation,
main promotion, B05, or release publication.
