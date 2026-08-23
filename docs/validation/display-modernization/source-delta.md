# Display `.071` to `.097` source/value audit

## Provenance

- Current CPH2747 synchronization: `5ab2a689ff87d7d28c511f1762cf41c1b90d965a`
- Current display, display-DT, MM-driver and MM-DT tag:
  `AU_TECHPACK_DISPLAY.LA.6.0.R1.00.00.00.000.071`
- New comparison synchronization: `d447f713d6403f707a2910383495f4ada98cfa4d`
- New display, display-DT, MM-driver and MM-DT tag:
  `AU_TECHPACK_DISPLAY.LA.6.0.R1.00.00.00.000.097`
- New synchronization product: OnePlus PLZ110/15T, not CPH2747

These identities come from the complete Qualcomm-tag manifests in the two
official OnePlus synchronization commits.  They are not inferred from path or
file names.

## Active Canoe hardware and delivery contract

The live device binds
`qcom,mdss_dsi_panel_AA601_P_7_A0020_dsc_cmd` on the CPH2747 Canoe DSI path.
The panel reports `A0020`, `P_7`; touch is the matching Tianma/Synaptics S3910
path.  The three AA601 panel files have identical Git objects in `.071` and
`.097`:

- common: `9f5839396cb5fd364bff199bd38ac76012b83994`
- command-mode panel: `99fa3b42422e596c5132d32ed1563126281776d6`
- loading-effect: `2d2b2fd13de38b8108bf1a3f135a783c2d3b69c5`

The active CPH2747 overlay is
`infiniti-24831-display-canoe-overlay`.  It is not retained by the newer
PLZ110 source: the comparison product replaces that board layer with
`fairlady-25821-display-canoe-overlay`.  That rename is not a Canoe-generic
upgrade and is explicitly rejected.

The current proprietary runtime is likewise device-specific.  It includes the
Oplus display-panel-feature service, Qualcomm HWC/post-processing interfaces,
SurfaceFlinger policies, AA601 calibration and panel command tables.  The live
panel advertises 1272x2772 and 1080x2354 modes at 60/90/120/144/165 Hz, HDR,
Oplus AOD/FOD, variable-refresh and panel-feature controls.  No matched `.097`
CPH2747 display firmware, HAL, calibration or panel overlay exists.

## Delta boundary

Across the Display-tagged repositories the comparison changes 280 files with
35,126 insertions and 4,334 deletions:

| Area | Changed files | Insertions | Deletions |
|---|---:|---:|---:|
| display drivers | 123 | 6,964 | 3,022 |
| display device tree | 99 | 24,491 | 930 |
| MM/HFI/fence drivers | 41 | 3,288 | 367 |
| MM device tree | 11 | 242 | 5 |
| proprietary display DT | 6 | 141 | 10 |

The active `msm_drm.ko` is one monolithic DDK module.  `.097` changes DSI,
SDE, DP, HDMI, DCP-HFI, writeback, color, fence, SMMU, UAPI and Oplus code
inside that module.  It also changes the separate active providers
`msm_hfi_core`, `msm_hw_fence`, `sync_fence` and their shared structures and
SSR protocol.  This is not a leaf-module update.

Material runtime changes include DCP-HFI command-buffer ownership and SSR
handling, response-timeout notification, hardware-recovery events, display
writeback/HFI objects, panel timing/capability exchange, fence power/SSR state,
SDE timing/VRR/CESTA paths, DP/HDMI handling and error cleanup.  These may
contain reliability value, but they change the firmware-facing protocol and
cross-module provider contracts as a coherent generation.

Two Oplus SM8850 source files differ.  The AP-UIR change removes the existing
DPMS-on guard before panel commands; that is not accepted as a standalone
reliability fix.  The on-screen-fingerprint delta removes one explicit
backlight-notifier call after AOD exit, plus a whitespace-only change.  The
same notifier is used by the surrounding backlight interface, but the
snapshot provides no defect provenance proving that this one-line removal is
safe in the retained `.071` notification sequence.  It is therefore not a
justified standalone candidate.

## Decision

The AA601 panel data gains no `.097` runtime update.  The remaining changes
require a matched display-DCP firmware/HAL/provider generation and a different
product's board overlay, or lack enough defect provenance to justify a
selective backport.  A full provider migration would expose touch,
fingerprint/AOD, HDCP, external display, camera/video/synx, hardware-fence and
proprietary HWC consumers to an unnecessarily broad contract change.

**DEFER — DISPLAY `.097` DOES NOT PROVIDE SAFE CANOE VALUE.**

No source, device tree, module or payload is changed by this audit.
