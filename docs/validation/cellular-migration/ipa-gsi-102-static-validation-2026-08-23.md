# IPA/GSI .102 bounded provider closure: static validation

Date: 2026-08-23

Status: `READY — BOUNDED IPA/GSI .102 CLOSURE VALIDATED`

Physical status: `NOT FLASHED`

## Baseline and candidate

- Branch: `experiment/cellular-ipa-gsi-102`
- Base commit: `f81d7bb1ba201066868332f8702f5ac13af58240`
- Kernel release: `6.12.23-android16-5-o-g6744a3f6bcf4-4k`
- Physically validated RMNET-core .102 base vendor-DLKM SHA-256:
  `48f5b095204abe8f992bf1292159f9527058d4019b796cdf7a9e6bb119d3a99f`
- Candidate vendor-DLKM:
  `out/cellular-ipa-gsi-102-candidate/vendor_dlkm.img`
- Candidate SHA-256:
  `538622b7d5ff73ab092619cdfab31099ffa3e0638b9051329bf950a04a5260a2`

No boot, system-DLKM, vendor_boot, VBMeta, slot metadata, or device state was
modified. The candidate has not been copied to or flashed on the device.

## Source provenance

The exact previous DataIPA source is
`5ab2a689ff87d7d28c511f1762cf41c1b90d965a` (`.078`). The selected newer
same-SM8850 DataIPA source is
`d447f713d6403f707a2910383495f4ada98cfa4d` (`.102`). Eleven runtime source
files used by the Canoe `gsim`, `ipam`, and `ipanetm` actions were imported.
Unrelated targets and the newer tree's Bazel target-selection changes were
excluded.

No reviewed change is classified as a security fix. The runtime deltas are
bug fixes and lifecycle, recovery, memory, power, data-path, export, and
structural-contract changes.

## Minimum contract-complete closure

The bounded closure contains four changed vendor-DLKM module files:

| Module | Action | Reason |
|---|---|---|
| `gsim.ko` | `.102` source upgrade | Provides the new GSI lifecycle behavior and `gsi_status_enabled()` export. |
| `ipam.ko` | `.102` source upgrade | Consumes the new GSI export and contains the selected IPA lifecycle/data-path changes. |
| `ipanetm.ko` | rebuilt for provider contract | Its source is unchanged, but it consumes `ipa3_ctx`, whose provider CRC changes. |
| `rmnet_ctl.ko` | exact stock pre-signature payload re-signed | It consumes protected `ipam` exports and therefore must carry the controlled-v1 signature. |

The remaining 432 vendor-DLKM module files are byte-identical to the
physically validated RMNET-core .102 baseline. In particular, the controlled
`rmnet_core.ko` and `rmnet_sch.ko`, WLAN .053, Bluetooth .046, NFC .102,
stock `rmnet_mem.ko`, and all stock QMI/GSI-adjacent helpers retain their
qualified bytes.

## Export and consumer contract

The complete scan covered 436 vendor-DLKM, 46 system-DLKM, and 479
vendor-boot module binaries: 961 total, including 525 retained external
binaries.

- `gsim` retains all 81 old exports and adds only
  `gsi_status_enabled()` (`0x5e505530`). Only `.102` `ipam` imports it.
- `ipam` retains its 293-symbol export set. Four CRCs change:
  `ipa3_ctx`, `ipa3_get_ctx`, `ipa_bridge_tx_dp`, and `ipa_tx_dp`.
- `ipanetm` is the only shipped external consumer of those four changed
  exports; it imports `ipa3_ctx` and is rebuilt in the closure.
- No shipped module imports `ipa3_get_ctx`, `ipa_bridge_tx_dp`, or
  `ipa_tx_dp`.
- Active Peach-v2 imports 28 unchanged IPA exports. Dormant Kiwi-v2 and
  WCN7750 packaged paths import only unchanged IPA exports.
- Controlled `.102` `rmnet_core` imports six unchanged IPA exports.
- Exact-stock-source `rmnet_ctl` imports four unchanged IPA exports and is
  re-signed without changing its pre-signature payload.
- No system-DLKM or vendor-boot module consumes an affected GSI/IPA export.

The complete import names and CRCs are generated at
`out/cellular-ipa-gsi-102-candidate/ipa-gsi-consumers.tsv`.

## Shared-structure boundary

`struct ipa_tx_meta` remains 24 bytes. `.102` consumes one byte of former
padding at offset 3 for `pkt_ex_init_valid`; all old member offsets remain
unchanged. No shipped external binary imports `ipa_tx_dp` or
`ipa_bridge_tx_dp`, so the changed semantic field does not cross into a
retained binary.

`struct ipa3_context` and its internal statistics change. `ipanetm` imports
the `ipa3_ctx` pointer and is rebuilt against the new provider contract. No
other retained external binary dereferences the changed internal layout.
All other public GSI/IPA structures, enums, flags, callback tables, and QMAP
metadata contracts used across retained module boundaries are unchanged.

Structural-contract failures: `0`.

## WLAN, USB/tethering, and SSR boundaries

- WLAN: PASS. All 28 active Peach-v2 IPA import CRCs are unchanged, and the
  qualified WLAN .053 module hashes are preserved.
- USB/tethering: static PASS. Stock `usb_f_gsi` provides
  `ipa_ready_callback` to `ipam`; it does not consume a changed IPA export.
  USB tethering and hotspot client traffic remain mandatory future physical
  gates because no client-path qualification occurred in this task.
- SSR/modem recovery: static PASS. The stock QMI, remoteproc, GLINK,
  ramdump, and minidump provider imports required by `.102` `ipam` all
  resolve with matching CRCs. Actual modem recovery remains unqualified and
  must only be tested with a safe supported trigger; the modem must not be
  deliberately crashed merely to exercise SSR.

## Kernel and module static gates

| Gate | Result |
|---|---|
| Frozen kernel-contract guard | PASS; Image functional differences 0; vmlinux functional differences 0 |
| Kernel release | PASS; exact `g6744` contract |
| `.config` | PASS; SHA-256 `b53d48b303059adb49a8dbe457145a4b7523a77fae621ea8d9e7b0b727e1615b` |
| `Module.symvers` | PASS; SHA-256 `de57709f3de38afb3e266481da09433687979ffb88ee607bda93ac4732dd7e0b` |
| `//soc-repo:canoe_perf_dist` | PASS |
| ABI | PASS / empty |
| KMI symbol checks | PASS |
| Vendor module inventory | PASS; 436 |
| Intended changed modules | PASS; 4 |
| Unexpected changed modules | 0 |
| Unresolved imports | 0 |
| CRC mismatches | 0 |
| Protected-export failures | 0 |
| Signature failures | 0 |
| Structural-contract failures | 0 |
| system-DLKM `modules.load` | PASS; 46 entries |
| `wwan.ko` load order | PASS; entry 21 |
| WLAN .053 / BT .046 / NFC .102 hashes | unchanged |
| Controlled RMNET core/scheduler hashes | unchanged |
| ext4 `e2fsck -fn` | PASS |
| Partition-local AVB hashtree/footer | PASS |

The partition-local vendor-DLKM AVB footer/hashtree was regenerated. No
parent or top-level VBMeta image was created or changed.

## Future physical test plan

This task deliberately stops before any device write. A later approved test
must first prove the running RMNET-core .102 baseline healthy and hash the
current vendor-DLKM. In TWRP, use the hardened controlled-stack helper with a
decrypted/writable backup destination, perform a full verified backup, and
run a vendor-DLKM-only dry run. Proceed only if the device/slot guard,
snapshot state, capacity, ext4, AVB structure, input hash, and backup
verification all pass. The write, if later authorized, must be
vendor-DLKM-only and the read-back SHA-256 must equal the candidate hash
before Android is booted.

The physical qualification matrix must include:

- cellular registration, RMNET IPv4/IPv6, routes, IP, and DNS;
- mobile-data toggle at least 20 times;
- airplane-mode recovery at least 10 times;
- WLAN/cellular handoff at least 10 times;
- deep-idle/wake at least 10 times and at least three clean reboots;
- at least 500 MiB or 10 minutes of sustained cellular traffic;
- hotspot client traffic and USB tethering when suitable equipment exists;
- WLAN .053 coexistence, including 6 GHz/WPA3 and reload;
- Bluetooth .046 and NFC .102 core regression checks;
- GSI/IPA firmware-load and re-registration logs;
- IOMMU/SMMU, SSR, QMI, GLINK, remoteproc, symbol, CRC, vermagic, signing,
  protected-export, crash, and warning scans.

If a safe supported modem-recovery trigger exists it may be tested. Do not
deliberately crash the modem solely to exercise SSR. If no safe trigger is
available, report SSR recovery as not tested rather than passing it by
inference.

Rollback must use the exact physically validated RMNET-core .102
vendor-DLKM SHA-256
`48f5b095204abe8f992bf1292159f9527058d4019b796cdf7a9e6bb119d3a99f`,
with read-back verification before reboot.
