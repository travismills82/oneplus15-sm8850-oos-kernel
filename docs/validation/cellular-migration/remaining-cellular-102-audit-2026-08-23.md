# Remaining cellular `.102` value audit

Date: 2026-08-23

Canonical branch: `feature/controlled-v1-wlan053-bt046-nfc102-cellular102-core`

Physically tested source lineage: `88a12eaea66082a69dd42acd737b0ec5b99bd349`

## Frozen controlled cellular baseline

The physically tested IPA/GSI candidate is frozen without rebuilding any
payload. Its controlled-stack identity is:

| Item | Identity |
| --- | --- |
| kernel release | `6.12.23-android16-5-o-g6744a3f6bcf4-4k` |
| boot SHA-256 | `84a85b9a40adc7bdb178bf86e2da74e9bac3feaf7bd502c7d29b1df27dea55ab` |
| system-DLKM SHA-256 | `de77afb66f47f3430f1a5bcc8a79297792141652f8a256e17b4c715f49222cef` |
| vendor-DLKM SHA-256 | `538622b7d5ff73ab092619cdfab31099ffa3e0638b9051329bf950a04a5260a2` |
| WLAN | `.053` |
| Bluetooth vendor | `.046`, core-qualified |
| NFC vendor | `.102`, core-qualified |
| RMNET core | `.102` |
| IPA/GSI | `.102` |
| signing | controlled-v1 |

Controlled cellular modules are `rmnet_sch.ko`, `rmnet_core.ko`, `gsim.ko`,
`ipam.ko`, `ipanetm.ko`, and `rmnet_ctl.ko`. Of those, `ipanetm.ko` was rebuilt
for the `.102` IPA provider contract, `rmnet_ctl.ko` retains its exact stock
pre-signature payload under controlled-v1 signing, and `rmnet_sch.ko` is the
physically validated ownership/signing/delivery proof.

Normal cellular operation, USB tethering with a real host, RNDIS/hardware
offload, hotspot start/stop, WLAN coexistence, and the BT/NFC regression gates
passed. Hotspot client traffic was not tested because equipment was
unavailable. No natural SSR or firmware-already-loaded path occurred, and
`gsi_status_enabled()` was linked but not observed executing.

One uninterrupted run delayed PDN recovery at mobile-data request 20 and
airplane-mode request 14. The remaining aggregate cycles passed after clean
reboots, without an OEM data-failure cause or module, IPA/GSI, IOMMU, SMMU, or
kernel fault. This is permanently classified as **LONG-RUN PDN RECOVERY DELAY
OBSERVED — OBSERVATION ONLY**. It is not classified as an IPA/GSI `.102`
regression without an identical uninterrupted A/B comparison against the
qualified RMNET_CORE `.102` baseline.

## Exact source comparison

The remaining 21 exact-stock code modules were compared between:

- CPH2747 OOS 16.0.9.400 source: `5ab2a689ff87d7d28c511f1762cf41c1b90d965a`
- newer same-SM8850 `.102` comparison snapshot: `d447f713d6403f707a2910383495f4ada98cfa4d`

The comparison used each module's exact source path from
`current-cellular-closure.tsv`, not filename-prefix grouping.

Fourteen module source paths are byte-unchanged:

- `dwc3_msm`, `oplus_mm_kevent`, `oplus_mm_kevent_fb`
- `qcom_glink`, `qcom_glink_smem`, `qcom_ramdump`, `qcom_smd`
- `qcom_va_minidump`, `qmi_helpers`, `redriver`, `repeater`
- `rproc_qcom_common`, `usb_f_gsi`, `wcd_usbss_i2c`

Seven datarmnet-ext paths change only their `define_*.bzl` file:

- `rmnet_aps`, `rmnet_mem`, `rmnet_offload`, `rmnet_perf`
- `rmnet_perf_tether`, `rmnet_shs`, `rmnet_wlan`

Those seven diffs only rename the Kleaf configuration labels from
`//build/kernel/kleaf:socrepo_*` to
`//build/qcom_build_extensions:qtisocrepo_*`. There are zero changed runtime
`.c` or `.h` files, exports, shared types, module parameters, or enabled code
paths for those modules.

Classification totals:

| Classification | Count |
| --- | ---: |
| REAL `.102` VALUE — LOW RISK | 0 |
| REAL `.102` VALUE — MEDIUM RISK | 0 |
| REAL `.102` VALUE — HIGH RISK | 0 |
| NO RUNTIME CHANGE | 14 |
| BUILD-ONLY CHANGE | 7 |
| DEVICE-SPECIFIC / NOT APPLICABLE | 0 |

The row-level evidence is recorded in
`remaining-cellular-102-audit.tsv`.

## Shared-provider boundary

The remaining high-risk shared providers include QMI, GLINK/SMD/remoteproc,
ramdump/minidump, RMNET memory/preallocation, USB DWC3/GSI, redriver/repeater,
and the WLAN/tethering-facing RMNET extensions. Their external consumer sets
span WLAN, modem recovery, USB, audio, display, charging, and other platform
subsystems. None has a runtime source change in the audited `.102` snapshot.

Consequently, export, CRC, shared-type, and physical-provider risk would be
incurred without obtaining a bug, recovery, performance, or security fix.
They must remain exact stock.

## Top five remaining `.102` changes worth considering

There are no qualifying entries. No remaining module has a real runtime `.102`
change, so manufacturing a five-item ranking would misrepresent build-system
churn as subsystem value.

## Optional long-run A/B follow-up

The PDN recovery observation can be investigated later without changing this
canonical branch. Use identical uninterrupted windows on:

- A: qualified RMNET_CORE `.102` vendor-DLKM
  `48f5b095204abe8f992bf1292159f9527058d4019b796cdf7a9e6bb119d3a99f`
- B: frozen IPA/GSI `.102` vendor-DLKM
  `538622b7d5ff73ab092619cdfab31099ffa3e0638b9051329bf950a04a5260a2`

Compare at least 20 consecutive mobile-data toggles and 15 consecutive
airplane-mode cycles, recording time to usable PDN, RIL failure cause, RMNET
recreation, and IPA/GSI logs. Do not classify a regression unless B is
reproducibly worse than A under those identical conditions.

## Decision

**STOP — REMAINING CELLULAR `.102` CHANGES DO NOT JUSTIFY MIGRATION RISK.**

No additional module was imported, rebuilt, re-signed, packaged as a
replacement, or flashed during this audit.
