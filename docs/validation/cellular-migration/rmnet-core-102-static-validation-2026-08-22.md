# RMNET core .102 bounded provider closure: static validation

Date: 2026-08-22

Status: `READY — BOUNDED RMNET_CORE .102 PROVIDER CLOSURE VALIDATED`

Physical status: `NOT FLASHED`

## Baseline and candidate

- Branch: `experiment/cellular-rmnet-core-102`
- Base commit: `70470ba4578834e6dc7cf861df245e9d4c92325d`
- Kernel release: `6.12.23-android16-5-o-g6744a3f6bcf4-4k`
- Batch-01 base vendor-DLKM SHA-256:
  `640c4f380d1ef8f1d23cd20d4e097f999f04f4d2f3e0c1fc13c1308d1b2ee958`
- Candidate vendor-DLKM:
  `out/cellular-rmnet-core-102-candidate/vendor_dlkm.img`
- Candidate SHA-256:
  `48f5b095204abe8f992bf1292159f9527058d4019b796cdf7a9e6bb119d3a99f`

No boot, system-DLKM, vendor_boot, VBMeta, slot, or device state was modified.

## Minimum complete closure

The source migration is one module:

- `rmnet_core.ko`: exact `.102` source replacement

Protected-export enforcement requires the six direct stock consumers to carry
the controlled-v1 signature when consuming the controlled provider:

- `rmnet_aps.ko`
- `rmnet_offload.ko`
- `rmnet_perf.ko`
- `rmnet_perf_tether.ko`
- `rmnet_shs.ko`
- `rmnet_wlan.ko`

Those six modules have no `.097 -> .102` runtime source delta. Their ELF
payload before the appended PKCS#7 signature remains byte-identical to the
qualified Batch-01 stock payload. They are classified `RE_SIGN_ONLY`, not
`.102 FEATURE UPDATE` and not `REBUILT FOR PROVIDER CONTRACT`.

`rmnet_sch.ko` remains the physically validated Batch-01 controlled module.
`rmnet_ctl.ko` and `rmnet_mem.ko` remain stock: they provide unchanged APIs to
the new core and do not consume a changed core contract.

Cellular ownership after assembly:

- controlled source modules: 2 (`rmnet_sch`, `rmnet_core`)
- retained stock-source cellular modules: 25
- retained byte-identical module files: 19
- exact-stock payloads re-signed only: 6

## Consumer and export contract

The complete scan covered 436 vendor-DLKM modules plus 525 retained external
modules (46 system-DLKM and 479 vendor_boot modules). It found exactly six
direct `rmnet_core` consumers, all in vendor-DLKM, with 56 matching import/CRC
edges. There are no system-DLKM, vendor_boot, Image, or other vendor module
consumers outside this bounded set.

The stock and `.102` core each expose 140 symbols:

- added: 0
- removed: 0
- CRC changed: 0
- unchanged: 140

## Structural ABI

All 21 RMNET headers are object-identical between `.097` and `.102`. The only
runtime source change is within a private function body. `pahole` measurements
of 13 shared or boundary-relevant types were recorded, and every size/member
layout is unchanged under the same Canoe configuration. Enum values, APS
marks, hook identifiers, QMAP metadata definitions, flags, callback layouts,
and skb-private metadata are unchanged.

The only semantic change is the internal QMAP priority decision documented in
`rmnet-core-102-source-delta.md`.

## Stock provider compatibility

The `.102` core has 156 resolved imports:

- vmlinux: 136
- exact-stock `ipam`: 6
- exact-stock `qmi_helpers`: 8
- exact-stock `rmnet_ctl`: 2
- exact-stock `rmnet_mem`: 4
- GSI direct imports: 0

Every CRC matches. The exact-stock IPA/GSI foundation is retained unchanged,
including `gsim`, `ipam`, `ipanetm`, `rmnet_ctl`, `rmnet_mem`, and
`usb_f_gsi`.

## Static gates

| Gate | Result |
|---|---|
| Frozen kernel-contract guard | PASS; Image functional differences 0; vmlinux functional differences 0 |
| `//soc-repo:canoe_perf_dist` | PASS |
| ABI | PASS |
| KMI symbol checks | PASS |
| ABI diff | PASS / empty |
| Vendor module inventory | PASS; 436 |
| Source replacements | PASS; 1 |
| Controlled signed closure | PASS; 7 |
| Unresolved imports | 0 |
| CRC mismatches | 0 |
| Protected-export failures | 0 |
| Signature failures | 0 |
| Structural contract failures | 0 |
| External consumer edges outside closure | 0 |
| system-DLKM `modules.load` | PASS; 46 entries |
| `wwan.ko` load order | PASS; entry 21 |
| WLAN .053 / BT .046 / NFC .102 hashes | unchanged |
| Stock IPA/GSI hashes | unchanged |
| ext4 `e2fsck -fn` | PASS |
| Embedded AVB hashtree | PASS |
| `.102` binary behavior proof | PASS |

The vendor-DLKM partition-local AVB footer/hashtree was regenerated. No
parent or top-level VBMeta image was changed.

## Future physical test plan

This task did not flash the candidate. Before a later test, push only the
candidate image to a decrypted/writable TWRP location and run the hardened
helper on the current controlled development slot:

```sh
twrp-flash-controlled-stack --dry-run \
  --backup-dir /sdcard/TWRP/kernel-flash-backups/rmnet-core-102 \
  --vendor-dlkm /path/to/vendor_dlkm.img
```

Proceed only after device, slot, snapshot, backup, capacity, ext4, hash, and
AVB checks pass. The eventual write remains vendor-DLKM-only:

```sh
twrp-flash-controlled-stack --flash \
  --backup-dir /sdcard/TWRP/kernel-flash-backups/rmnet-core-102 \
  --vendor-dlkm /path/to/vendor_dlkm.img
```

The physical matrix must cover registration, RMNET IPv4/IPv6, routes, IP/DNS,
data toggle, airplane recovery, WLAN/cellular handoff, deep idle, two reboots,
100 MiB or longer traffic, and hotspot/client traffic when available.

For the priority path, capture the IPA low-latency pipe state and search for
the `rmnet_map_v5_check_priority ... priority bit set` diagnostic during APS
LLB-marked latency-sensitive and mixed foreground/background traffic. If the
required condition does not occur, report:

`RMNET_CORE .102 COMPATIBILITY PASS — PRIORITY-FIX TRIGGER NOT OBSERVED`
