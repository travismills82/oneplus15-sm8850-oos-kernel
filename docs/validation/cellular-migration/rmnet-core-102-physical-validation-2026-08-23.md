# RMNET core .102 physical validation

Date: 2026-08-23

Branch: `experiment/cellular-rmnet-core-102`

Tested HEAD: `0173b305cb9531ed0d335f07ae1d980c321c90b3`

Status: **PASS — RMNET_CORE .102 COMPATIBILITY VALIDATED; PRIORITY-FIX
TRIGGER NOT OBSERVED**

This was a vendor-DLKM-only test of the bounded provider closure established
by the static audit. It validates compatibility of the `.102` `rmnet_core.ko`
with the frozen g6744 kernel contract, exact-stock IPA/GSI foundation, and
stock-source RMNET consumers. It does not claim that the new priority branch
was exercised.

## Scope and payload identity

Only `vendor_dlkm_b` was written. Boot, system-DLKM, vendor boot, VBMeta, and
slot metadata were not written.

| Item | Identity |
| --- | --- |
| kernel release | `6.12.23-android16-5-o-g6744a3f6bcf4-4k` |
| slot | `_b` |
| behavioral replacement | `rmnet_core.ko` `.102` |
| retained controlled module | `rmnet_sch.ko` from Batch 01 |
| re-signed stock-source consumers | `rmnet_aps`, `rmnet_offload`, `rmnet_perf`, `rmnet_perf_tether`, `rmnet_shs`, `rmnet_wlan` |
| candidate vendor-DLKM SHA-256 | `48f5b095204abe8f992bf1292159f9527058d4019b796cdf7a9e6bb119d3a99f` |
| candidate bytes | 143,986,688 |
| pre-test Batch-01 vendor-DLKM SHA-256 | `640c4f380d1ef8f1d23cd20d4e097f999f04f4d2f3e0c1fc13c1308d1b2ee958` |

The pre-flash Android state matched the qualified controlled stack: CPH2747,
Canoe, slot `_b`, Android boot complete, the exact g6744 release, `wwan`
loaded, Visible LTE HOME/IN_SERVICE, a dual-stack RMNET PDN, default policy
routes, IP and DNS connectivity, and data-failure cause `NONE(0x0)`.

## Recovery safety and write verification

Recovery was TWRP 3.7.1_16 for OnePlus 15. User 0 was decrypted, `/sdcard` was
writable with sufficient free space, and the Virtual A/B snapshot state was
`none`. The exact hardened helper from TWRP commit
`3f499bfd1f7152ea27b27935be22ff73581709a1` was staged under `/tmp`; no
recovery partition was modified. Its SHA-256 was
`84b035710c305be859aac72827489850b7d215ee372ab3341be849e051bfab50`.

The source `vendor_dlkm_b` and the full-size backup both measured 143,986,688
bytes and matched the Batch-01 SHA-256. The verified backup is:

`/sdcard/TWRP/kernel-flash-backups/rmnet-core-102/controlled-stack-b-20260823-055624/vendor_dlkm_b.img`

The vendor-DLKM-only dry run passed the device, slot, snapshot, backup,
capacity, ext4, AVB structure, and input-hash gates and reported no write. The
subsequent flash changed only `vendor_dlkm_b`. Its immediate full-partition
read-back matched the candidate SHA-256 exactly, and the post-write ext4
structural check passed. A live block-device hash repeated after the stress
matrix and three reboots still matched
`48f5b095204abe8f992bf1292159f9527058d4019b796cdf7a9e6bb119d3a99f`.

## Runtime module proof

Android booted with the unchanged g6744 kernel. `rmnet_core`, `rmnet_sch`,
`rmnet_ctl`, `rmnet_mem`, `ipam`, `gsim`, `qmi_helpers`, and `wwan` loaded.
Live files under `/vendor_dlkm/lib/modules` matched the packaged candidate:

| Module | Runtime/package SHA-256 | Role |
| --- | --- | --- |
| `rmnet_core.ko` | `4770412e3d5ce493959491cc1420bb3a80488e19de34f688a19334f1a22ee6ce` | `.102` behavioral replacement |
| `rmnet_sch.ko` | `4fdb7d1122e430731594c7eea9dc2e8686cd7c2668e05d39ef66bfa18b0c75b4` | Batch-01 controlled module |
| `rmnet_aps.ko` | `a6461b89c136d68e76bf9aeb33731f2059efcc768d0c36776fc52e55ee322f88` | stock-source, re-signed only |
| `rmnet_offload.ko` | `5a4c4d97d69a9b09b6f0d880e4c103326a470bb384e6cf23b595bb3534d8bfab` | stock-source, re-signed only |
| `rmnet_perf.ko` | `19a2f7d1920c293b314ab5e714f875780f87e370f8abef3df4114bdaa0ddd036` | stock-source, re-signed only |
| `rmnet_perf_tether.ko` | `c309cf530eb0c6b57f98a0eb29d151c7bb08b0623077499f26f6bc580d94f55f` | stock-source, re-signed only |
| `rmnet_shs.ko` | `05babed8c54e7e16b103b7f52830e7e89502572d6299ba7bdd59c0a531b195c4` | stock-source, re-signed only |
| `rmnet_wlan.ko` | `aa3e05fb11f45926bd0c23a90591f94bd83a5ae07f2748e6f1c8b0d6c151aabb` | stock-source, re-signed only |
| `rmnet_ctl.ko` | `e17522fe927fa33d7f928b3853f237c11fe55b09bf8c4f902cd63de545b67408` | exact stock |
| `rmnet_mem.ko` | `15fecf07714c51b9ddb841dfe6b76d76731bee0c47919bbe4f3e9375ede4c92c` | exact stock |
| `ipam.ko` | `b73489c5e64cc5ab46a699d5b9a186670f64b357d78223c6ea2d4bff7ae0d274` | exact stock IPA |
| `gsim.ko` | `f3dfa4c055d3d19f91160b12602fcbca1f75ae831d99f22409791b6383986f97` | exact stock GSI |
| `qmi_helpers.ko` | `b2b6edc9fd1dfe46baba2a4de60f3afdb0ddcffe44b7515ad87b469d46e17334` | exact stock QMI helper |

The Android `modinfo` interface did not expose a signer field, so runtime
identity is established by the exact live-file hashes, successful protected
module loading, and the static signing report rather than by an inferred
runtime signer string.

The unchanged system-DLKM load contract also passed live: `modules.load`
contained 46 entries and `wwan.ko` remained entry 21.

## Cellular compatibility results

| Test | Result |
| --- | --- |
| first Android boot | PASS |
| registration | PASS — Visible LTE HOME/IN_SERVICE |
| data-call result | PASS — `NONE(0x0)` |
| active PDN | PASS — `rmnet_data2` or `rmnet_data3`, as assigned by the modem |
| IPv4 address and policy route | PASS |
| IPv6 address and policy route | PASS |
| IPv4 connectivity | PASS |
| DNS/name connectivity | PASS |
| mobile-data OFF/ON | PASS — 20/20 |
| airplane-mode recovery | PASS — 10/10 |
| Wi-Fi/cellular handoff | PASS — 10/10 |
| forced deep-idle/wake | PASS — 10/10 |
| clean Android reboots | PASS — 3/3 |
| hotspot start/stop | PASS — 5/5 |
| hotspot client traffic | NOT TESTED — EQUIPMENT UNAVAILABLE |

Recovery after each mobile-data cycle took approximately two to three
seconds. Cellular takeover during the handoff test took approximately four to
five seconds, while WLAN recovery took approximately two to three seconds.
The modem alternated between `rmnet_data2` and `rmnet_data3`, as it did on the
qualified Batch-01 baseline; interface-number changes were not failures.

All three reboot validations re-proved the exact g6744 release, the live
`rmnet_core` SHA-256, loaded `rmnet_sch` and `wwan`, Visible registration,
dual-stack RMNET, IPv4/IPv6 policy routes, IP/DNS connectivity, and
`NONE(0x0)`.

## Traffic and qdisc evidence

Ten successful 50 MiB HTTPS flows transferred 500 MiB total. The active RMNET
RX counter increased by 533,591,795 bytes with zero RX errors or drops. Two
flows ran concurrently for five rounds at a controlled aggregate rate. A
foreground ping during the transfer completed 240/240 packets with 0% loss,
52.58 ms mean latency, and 119.895 ms maximum latency. Every HTTPS flow
returned HTTP 200 and exactly 52,428,800 bytes.

`tc qdisc show` recorded the stock HTB, PPQ, TSD, SFQ, and `clsact` hierarchy
on the active RMNET path. This proves stable qdisc operation under load but is
not evidence that `rmnet_sch` was specifically attached.

An initial attempt against a different remote endpoint returned HTTP 403 and
was discarded before the successful 500 MiB run. It was a remote-server
response, not a device or cellular error.

## Priority-fix coverage

The static deterministic object/disassembly proof confirms that the generated
`rmnet_core.ko` contains the `.102` implementation of
`rmnet_map_v5_check_priority()`.

Mixed bulk and foreground traffic did not produce evidence that both required
runtime conditions were true: an APS LLB-marked packet and an unavailable IPA
low-latency pipe. Existing dmesg diagnostics, debugfs, counters, and module
logs exposed no safe read-only indicator proving that conjunction. IPA was not
deliberately disrupted.

- machine-code presence: **PASS**
- runtime trigger: **NOT OBSERVED**
- behavioral effect: **NOT DEMONSTRATED**

## Existing-subsystem regression checks

| Existing subsystem | Result |
| --- | --- |
| WLAN `.053` | PASS — 6135 MHz, 802.11ax, WPA3-SAE, reload, IP and DNS |
| WLAN 2.4/5 GHz during this run | NOT TESTED — no separate AP selection performed |
| Bluetooth `.046` | PASS — user toggle, adapter returned to `ON`, existing RFCOMM peer reconnect observed |
| NFC `.102` | PASS — OFF/ON before reboot; HAL and controller services recovered |
| Wallet/HCE | PASS — Google Wallet payment HCE service registered |
| eSE | PASS — off-host `eSE1` route present |

Android retained BLE-only operation briefly during a user Bluetooth disable
because background BLE scanning remained registered. Re-enabling returned the
full adapter to `ON`; no Bluetooth framework or HCI failure was observed.

## Error scan and baseline comparison

Across the three clean-boot captures there were zero new relevant:

- unknown symbols, MODVERSION disagreements, or vermagic failures;
- signature or protected-export failures;
- RMNET/QMAP errors or data-call failures;
- IPA/GSI IOMMU or SMMU faults;
- kernel oops, panic, KASAN, UBSAN, UAF, refcount, or hung-task failures
  attributable to this candidate.

The six unique boot-warning sites matched the earlier qualified controlled
baseline exactly: SPMI PMIC arbitration, Oplus duplicate proc registration,
sysfs group registration, IRQ enable state, GIC domain selection, and Oplus
touch-frame diagnostics. No new warning site was introduced. The occasional
`ipa3_write_done_common: tx_pkt is NULL` diagnostic observed during the
pre-reboot stress session also exists in the pre-candidate controlled-v1
baseline capture and is not new to `.102` `rmnet_core`.

No log, tracepoint, debugfs entry, or counter proved the priority-fix trigger.

Raw evidence is retained outside git at:

`/home/travis/Android/rmnet-core-102-live-captures/20260823T061900Z-physical/`

## Comparison with Batch 01

No meaningful compatibility divergence was found. Registration, RMNET
interface assignment, dual-stack addressing, policy-route creation,
airplane-mode recovery, WLAN/cellular handoff, suspend recovery, traffic
stability, and kernel warnings remained within the Batch-01 behavior. The
candidate therefore passes as a bounded provider migration against the exact
stock IPA/GSI foundation.

## Decision

**PASS — RMNET_CORE .102 COMPATIBILITY VALIDATED; PRIORITY-FIX TRIGGER NOT
OBSERVED**

This authorizes recording the bounded RMNET provider compatibility result. It
does not physically demonstrate the `.102` priority effect and does not
authorize IPA/GSI migration.
