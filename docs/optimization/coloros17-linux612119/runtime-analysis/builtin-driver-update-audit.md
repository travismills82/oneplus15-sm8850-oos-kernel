# Driver and upstream update audit

The inherited local inventories are
`out/coloros17001201-battery-evidence/linked-builtin-inventory.tsv`
(646 linked rows) and `stock-provider-inventory.tsv` (1,069 stock
module records). Live module names, driver symlinks and the exact
.201 boot contract take priority over source presence.

| Area | Observed ownership / behavior | Decision |
| --- | --- | --- |
| Scheduler and power entry | Core scheduler, freezer, workqueues are built in. Qualcomm WALT, task and power-model policy are stock modules. | Optimize built-in freeze-path repeated work. No vendor hook or OPlus timekeeping work-around. Latest ACK proxy fixes are not active with the live switch off. |
| CPUIdle, RPMh, AOSS, SCM and power domains | PSCI and generic power framework are resident; `qcom_rpmh`, `qcom_aoss`, `qcom_scm`, qcom_lpm and vendor RPMh statistics operate as stock modules. | Sleep telemetry shows low-power operation; no verified failed domain or unnecessary vote to correct in the common kernel. |
| Clock, regulator, interconnect, mailbox, remoteproc, IOMMU | Generic frameworks are resident with `clk_rpmh`, `icc_rpmh`, RPMh regulator, SMEM/remoteproc and Qualcomm SMMU-side providers in the stock stack. | No demonstrated vote leak. Preserve arm_smmu ABI, device-tree contract and platform module order. |
| UFS, block, DMA, inline encryption | Built-in block/crypto core and live `ufs_qcom`, stock ICE and PHYs; F2FS inlinecrypt is in use. | Keyslot request completion is already present as `73169d81d5b6f`. No new UFS pipelining or persistence change for this firmware. |
| USB, storage, charging | DWC3 platform runtime-suspended at sample with usage zero; Type-C/PD and charging firmware policy unchanged. | Newly fetched ACK tSenderResponse fwnode is opt-in hardware policy; no demonstrated use or need. No charger/PD current limit or DT changes. |
| GPU, display, multimedia and sensors | Stock KGSL/GEAS, display/HFI, `camera_extension`, touch and audio, with matched ColorOS userspace. | No built-in conversion. TMF sensor logs originate in the stock camera provider. No proven substitute matching .201. |
| Modem, DSP, WLAN, Bluetooth, IPA and RMNET | Stock Qualcomm networking, transport and DSP providers. Generic IRQ/network work in vmlinux. | Do not block existing wakeups. Lifetime IRQ and byte counts do not identify a driver poll defect. CAKE/FQ/FQ-CoDel settings unchanged. |

## Upstream provenance and selection

- Verified Google ACK references: LTS `58f2449a557a99270b15eb9841152be2ead76472`,
  regular `e6cb0df8b4bdcadd286cc824f900e849fd02b5d3`. Audit the
  newly fetched change range against local code, not raw missing SHAs.
  The suspend-related freezer and workqueue changes are independent
  of the upstream haptics options and proxy-exec series. HID haptic
  touchpad config is unrelated to the handset's capacitive touch.
- Linux stable advertises the existing v6.12.119 tree
  `e2e4b92c2b2a3ae78423cb343951cb5d2241b405`. No sublevel upgrade.
  The ACK F2FS device_alias restriction already exists as
  `1df210d46c8c0`; keep existing encrypted-storage and verity semantics.
  The ashmem shrinker accounting fix `5228562af4fa` is a separate
  correctness/security review item for the built-in Rust driver, not
  a justified standby tuning change. No claim of full CVE closure or
  evidence of an ashmem fault was derived from this runtime window.
- [Official OnePlus modules/DT](https://github.com/OnePlusOSS/android_kernel_modules_and_devicetree_oneplus_sm8850)
  still advertises `4c03af1d6d462b9573a7a9f85b983c6bae49497a` on
  the public SM8850 OxygenOS 16 branch. Official device/common tips
  are `d56830f69fa7` / `4f5102fdbc811`. They do not establish .201
  proprietary source and layout compatibility.
- CodeLinaro graphics now advertises
  `GRAPHICS.LA.16.1.r10-00300-kaanapali.0`, peeled
  `7f4e7b2763c099d927ae41f8209fa054a7f6891c`. Keep stock GPU/GEAS:
  the newer tag is not evidence of a compatible private ABI, firmware
  command stream or platform calibration, and cannot replace a loaded
  module from a boot image. No claim that all proprietary driver sources
  were available.
- KernelSU head `2a60b2893b8d` and SUSFS branch tip
  `9b0b13f06967` are unchanged from the earlier battery audit.
  No traced hook bottleneck that justifies moving the validated
  paired pins; preserve both without changing protection or options.
  Droidspaces-OSS v7.2.0 remains the verified userspace pin, separate
  from the built-in namespace/mount/cgroup features. The offline Alpine
  test exercises actual container prerequisites; no app was installed.
