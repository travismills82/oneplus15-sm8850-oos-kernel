# IPA/GSI .102 physical validation

Date: 2026-08-23

Branch: `experiment/cellular-ipa-gsi-102`

Tested HEAD: `88a12eaea66082a69dd42acd737b0ec5b99bd349`

Status: **PARTIAL — NORMAL PASS, TETHER/SSR EQUIPMENT COVERAGE REMAINS**

This was a vendor-DLKM-only physical test of the bounded IPA/GSI provider
closure established by the static audit. It validates normal-runtime
compatibility of `.102` `gsim.ko` and `ipam.ko` with the frozen g6744 kernel,
the qualified RMNET_CORE `.102` stack, exact-stock QMI/remoteproc/GLINK
providers, and the existing WLAN/BT/NFC stack. It does not claim that a modem
SSR recovery or firmware-already-loaded fix path was exercised.

## Scope and payload identity

Only `vendor_dlkm_b` was written. Boot, system-DLKM, vendor boot, VBMeta, and
slot metadata were not written.

| Item | Identity |
| --- | --- |
| kernel release | `6.12.23-android16-5-o-g6744a3f6bcf4-4k` |
| slot | `_b` |
| `.102` source providers | `gsim.ko`, `ipam.ko` |
| rebuilt provider consumer | `ipanetm.ko` |
| re-signed exact-stock payload | `rmnet_ctl.ko` |
| retained controlled modules | `rmnet_core.ko` `.102`, `rmnet_sch.ko` |
| candidate vendor-DLKM SHA-256 | `538622b7d5ff73ab092619cdfab31099ffa3e0638b9051329bf950a04a5260a2` |
| candidate bytes | 143,986,688 |
| rollback RMNET_CORE vendor-DLKM SHA-256 | `48f5b095204abe8f992bf1292159f9527058d4019b796cdf7a9e6bb119d3a99f` |

The pre-flash device matched the qualified RMNET_CORE baseline: CPH2747,
Canoe, slot `_b`, Android boot complete, exact g6744 release, Visible LTE
HOME/IN_SERVICE, dual-stack RMNET, IPv4/IPv6 policy routes, IP and DNS
connectivity, and data-call cause `NONE(0x0)`. The live pre-flash
`vendor_dlkm_b` hash matched the rollback artifact exactly.

## Recovery safety and write verification

TWRP reported CPH2747/Canoe, slot `_b`, user 0 decrypted, writable `/sdcard`,
and no active Virtual A/B snapshot. The exact hardened helper corresponding to
TWRP commit `3f499bfd1f7152ea27b27935be22ff73581709a1` was staged under `/tmp`;
no recovery partition was modified.

The manual full-size source backup and the helper backup both measured
143,986,688 bytes and matched the rollback SHA-256. Verified backup locations:

- `/sdcard/TWRP/kernel-flash-backups/ipa-gsi-102-manual-20260823T1415Z`
- `/sdcard/TWRP/kernel-flash-backups/ipa-gsi-102-helper/controlled-stack-b-20260823-091501`

The vendor-DLKM-only dry run passed the device, slot, snapshot, backup,
capacity, ext4, AVB structure, and candidate-hash gates without writing. The
subsequent flash changed only `vendor_dlkm_b`. The helper and an independent
full-partition read-back both matched the candidate SHA-256 exactly, and the
post-write ext4 check passed. The same candidate hash was re-proved after the
stress matrix and five clean boots.

## Runtime provider proof

Android booted with the unchanged g6744 kernel. `gsim`, `ipam`, `ipanetm`,
`rmnet_ctl`, `rmnet_core`, `rmnet_sch`, `wwan`, and `qmi_helpers` loaded. Live
files under `/vendor_dlkm/lib/modules` matched the candidate:

| Module | Runtime/package SHA-256 | Role |
| --- | --- | --- |
| `gsim.ko` | `f0d14471ce333548de21c178e61360b3f513b09617701d334250cc07583ca373` | `.102` GSI provider |
| `ipam.ko` | `018aa909b9caef763a0b64360dca73ec172f0e03010f62462123a1c6b960aac7` | `.102` IPA provider |
| `ipanetm.ko` | `7a6d237d8886010a36a352e5c66953b13de78f2b9130700f3eab027185030086` | rebuilt for changed `ipa3_ctx` contract |
| `rmnet_ctl.ko` | `b6931018dc0974856d7204a9f63142320d7d944272057c72a0af6e4376969757` | exact stock pre-signature payload, controlled-v1 signed |
| `rmnet_core.ko` | `4770412e3d5ce493959491cc1420bb3a80488e19de34f688a19334f1a22ee6ce` | previously qualified `.102` provider |
| `rmnet_sch.ko` | `4fdb7d1122e430731594c7eea9dc2e8686cd7c2668e05d39ef66bfa18b0c75b4` | previously qualified Batch-01 module |

The system-DLKM load contract remained unchanged: 46 `modules.load` entries
with `wwan.ko` at entry 21.

## Normal-runtime cellular results

| Test | Result |
| --- | --- |
| first Android boot | PASS |
| registration | PASS — Visible LTE HOME/IN_SERVICE |
| data-call result | PASS — `NONE(0x0)` |
| active default PDN | PASS — `rmnet_data2` or `rmnet_data3`, as assigned by the modem |
| IPv4 address and policy route | PASS |
| IPv6 address and policy route | PASS |
| IPv4 connectivity | PASS |
| DNS/name connectivity | PASS |
| mobile-data OFF/ON | 25/25 aggregate across clean-boot segments; see caveat below |
| airplane-mode recovery | 15/15 aggregate across clean-boot segments; see caveat below |
| Wi-Fi/cellular handoff | PASS — 10/10 |
| forced deep-idle/wake | PASS — 15/15 |
| clean Android reboots | PASS — 5/5 |
| hotspot start/stop | PASS — 10/10 |
| Wi-Fi hotspot client traffic | NOT TESTED — EQUIPMENT UNAVAILABLE |
| USB tethering with real host | PASS |

Two independent mobile-data sessions recovered 19 consecutive OFF/ON cycles
in one to two seconds each. In both sessions the 20th consecutive request did
not restore the default PDN inside the observation window (45 seconds in the
first run and 120 seconds in the re-test). No provider, loader, kernel, or OEM
data-failure error accompanied the condition. Per the test-direction override,
the candidate was retained, Android was cleanly rebooted, and aggregate cycles
20 through 25 then passed 6/6. This establishes 25 successful cycles across
clean-boot segments, not 25 consecutive cycles in a single boot. The cause of
the repeatable per-boot sequence ceiling was not isolated and remains a stress
qualification caveat.

The airplane test behaved similarly: cycles 1 through 13 passed with cellular,
Wi-Fi, Bluetooth, and NFC recovery; the 14th consecutive cycle did not restore
the default PDN within 180 seconds. After a clean reboot, aggregate cycles 14
and 15 passed. This is recorded as 15 successful cycles across clean-boot
segments, not 15 consecutive cycles in one boot.

All ten WLAN/cellular handoffs passed. Cellular takeover took one to two
seconds and WLAN returned in two to three seconds after association. All 15
forced deep-idle cycles recovered dual-stack cellular data in zero to one
second. Every one of the five clean boots re-proved the exact candidate hash,
required provider modules, Visible registration, dual-stack RMNET, routes, and
IP/DNS connectivity.

## Traffic, hotspot, and tethering

A single HTTPS flow transferred exactly 1,073,741,824 bytes in 98.102 seconds
at 10,945,145 bytes/second. The concurrent latency flow completed 98/98 pings
with 0% loss, 44.562 ms mean latency, and 80.580 ms maximum latency. The
`rmnet_data2` counters recorded zero RX/TX errors and zero RX/TX drops after the
transfer. The stock HTB, PPQ, TSD, and SFQ qdisc hierarchy remained present.

Personal hotspot start/stop passed 10/10 by verifying the SoftAP state in both
directions. No Wi-Fi client was available, so Wi-Fi hotspot forwarding remains
open.

USB tethering was exercised with the connected host as a real client. RNDIS
enumerated, the host received `10.148.157.142/24`, five interface-bound pings to
`1.1.1.1` passed with 0% loss, and an interface-bound HTTPS request returned
HTTP 204. Android reported `rmnet_data2` as the cellular upstream, hardware
offload started, and BPF offload maps were healthy. Disabling USB tethering
removed RNDIS and restored `mtp,adb` without intervention.

The tethering service history contained transient BPF detach diagnostics for
an already-removed `wlan2` after repeated hotspot stop operations. Hardware
offload restarted normally, USB tethering subsequently passed, and there was
no route, data, IOMMU, or provider failure; this is retained as teardown-log
evidence rather than hidden.

## WLAN, Bluetooth, and NFC coexistence

| Existing subsystem | Result |
| --- | --- |
| WLAN `.053` | PASS — 6135 MHz, 802.11ax, WPA3-SAE |
| WLAN bulk/coexistence | PASS — 100 MiB HTTP 206 transfer and 120/120 concurrent pings |
| cellular registration during WLAN test | PASS — Visible LTE and dual-stack RMNET remained provisioned |
| Bluetooth `.046` | PASS — adapter toggle and bonded HFP reconnect event observed |
| sustained Bluetooth peripheral session | NOT TESTED — EQUIPMENT UNAVAILABLE |
| NFC `.102` | PASS — root OFF/ON toggle and controller recovery |
| Wallet/HCE | PASS — Google Wallet payment HCE service registered and preferred |
| eSE | PASS — `eSE1` and secure-element services present |

The Bluetooth stack retained BLE-only internal operation briefly after the
user-facing disable, then returned to full `ON`. A bonded HFP device generated
a real reconnect event but did not remain connected; no Bluetooth framework or
HCI error was observed. That is adequate for the regression gate but not a
full peripheral qualification.

## Recovery-path coverage

Boot logs showed normal MPSS `BEFORE_POWERUP` and `AFTER_POWERUP` notifier
handling. They did not show a naturally occurring modem/IPA SSR recovery or
GSI deregistration/re-registration failure sequence. The modem was not
deliberately crashed.

- SSR/re-registration fix path: **NOT OBSERVED**
- firmware-already-loaded path: **NOT OBSERVED**
- `gsi_status_enabled()` runtime: **LINKED ONLY** — both `.102` providers
  loaded without missing-symbol, probe, or status failures, but no safe
  read-only trace proved an actual call

## Error scan and baseline comparison

The post-test dmesg/logcat scan found zero new relevant:

- unknown symbols, MODVERSION/CRC disagreements, or vermagic failures;
- module-signature or protected-export failures;
- fatal/stuck/timeout IPA or GSI failures;
- IOMMU or SMMU faults;
- kernel oops, panic, KASAN, UBSAN, UAF, refcount, double-free, or invalid-map
  failures attributable to this candidate;
- OEM data-failure causes. All captured default-PDN `DataCallResponse` entries
  reported `NONE(0x0)`.

The direct `ipa_fws.b02`/`b03` lookup failures followed by firmware fallback,
QMP mailbox `-19`, HOLB monitoring `-14`, GSI `busy, try again`, Oplus touch
warning, and Oplus blocked-task traces all have direct matches in the
pre-candidate RMNET_CORE capture. Repeated WLAN control operations also logged
the known `nl_srv_bcast` app-id diagnostic without connectivity loss. None was
introduced as a new `.102` IPA/GSI contract failure.

Raw evidence is retained outside git at:

`/home/travis/Android/controlled-ipa-gsi-102-live-captures/20260823T141011Z-preflight/`

## Decision

**PARTIAL — NORMAL PASS, TETHER/SSR EQUIPMENT COVERAGE REMAINS**

The bounded `.102` IPA/GSI provider set passes normal cellular operation,
large-transfer, mixed-traffic, handoff, deep-idle, reboot, 6 GHz WLAN
coexistence, and real USB-tether tests on Canoe. No rollback was required, and
the candidate remains installed on slot `_b`.

The result remains partial because a Wi-Fi hotspot client was unavailable, no
natural SSR or firmware-already-loaded path occurred, sustained Bluetooth
peripheral coverage was unavailable, and the requested long toggle totals had
to be completed across clean-boot segments rather than consecutively in one
boot. This does not authorize any additional cellular-provider migration.
