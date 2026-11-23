# Compiled and deployed driver audit

The exact .119 linked `modules.builtin` supplies **646** module-shaped
built-in records; the complete path list is generated as
`out/coloros17001201-battery-evidence/linked-builtin-inventory.tsv`.
That list does not include every always-linked core object or prove that a
built-in probed successfully. The stock oracle inventory and live /proc/modules
sample are separate evidence. The read-only snapshot observed 689 loaded
module names; that is not the static active-module contract.

| Area | Effective deployed split | Decision |
| --- | --- | --- |
| Scheduler / frequency | Scheduler, PELT, CPUFreq, SCMI transports, SCMI cpufreq and CPUIdle/PSCI common code built in; `sched_walt`, Qualcomm SCMI client and vendor transport helpers are live stock modules. | Actual policies are `scmi/walt`. Do not infer schedutil is active from its built-in/default config. Preserve task flags, Android cpusets/util clamp/vendor hooks. |
| Idle / clusters | PSCI and tick/PM core built in; stock `qcom_lpm`, `qcom_stats`, `qcom_cpuss_sleep_stats_v4`, `oplus_powermodel` live. | Preserve validated s2idle. The OEM timekeeping warning is not a reason to change generic timekeeping or suppress diagnostics. |
| Clock / power domains / regulators | Common clock, SCMI performance/PM domain helpers, fixed regulator core built in; stock `clk_scmi`, `clk_rpmh`, `qcom_rpmh`, `rpmh_regulator`, `icc_rpmh` live. | No proven vote leak; vendor votes/remote firmware must be measured before any matched-driver change. |
| Interconnect / thermal | Common QCOM interconnect core and thermal infrastructure built in. Vendor power and thermal voters including `thermal_pause`, `lmh_stats` and `qti_thermal_vendor_hooks` live in modules. | Preserve pressure signaling, thermal limits and PM QoS. |
| SCM / IOMMU / DMA | Generic DMA/IOMMU/mapping code built in; device `qcom_scm`, `arm_smmu`, proxy/PMU providers live in stock modules. | Keep both historical arm_smmu CRC exceptions. No privatization, mapping-policy or trust changes. |
| Storage / UFS | Block/inline crypto and UFS core/host glue built in; `ufs_qcom`, `ufshcd_crypto_qti`, `ufs_oplus_dbg` and Canoe PHY providers stock modules. | No UFS autosuspend, clock policy, OPP, or write-pipelining change in this experiment. |
| USB | DWC3 common and generic QCOM wrapper code built in; Canoe's `dwc3_msm`, charging and eUSB2 dependencies are stock components. | IRQ/PHY/runtime PM depend on actual DT, TCPC/PD and private `struct dwc3` contracts. Preserve OTG, charging and wakeup. |
| GPU / display | DRM/DMA-BUF common support built in. `msm_kgsl`, `msm_drm`, Adreno governors and GEAS providers are live stock modules. | New CodeLinaro graphics tags are not boot-only updates. No change to GMU, GPU idle/frequencies, refresh, bandwidth or memory-map policy. |
| Messaging / remote processors | Shared common mailbox, virt/remoteproc/rpmsg infrastructure and device-specific stock glink, pdr, qrtr, remoteproc and power-statistics modules. | Retain modem/ADSP wake and subsystem restart paths; no child-domain assumption from raw IRQ counts. |
| Network / clocks / IRQ | Generic packet stack/NAPI/GRO, IRQ/GIC, CAN/bridge/core qdiscs present as shown by generated config; Qualcomm WLAN/RMNET/IPA stock providers remain the device path. | Do not shorten Wi-Fi/IPA wakelocks or change coalescing without traffic attribution and standby regression tests. |
| Memory / ZRAM | MM, MGLRU and compression libraries built in; `zram` and `oplus_bsp_zram_opt` live modules, not replacements. | First backport only updates static MM shrinker logic. Compression ratio and allocator size alone do not establish energy benefit of an algorithm change. |
| Battery / charging | Generic power-supply and regulator infrastructure built in; device PMIC glink, battery debug and OPlus charger providers are stock. | No battery limits, votes, negotiation, gauge, SUPERVOOC or thermal changes. |
| Camera, audio, DSP, fingerprint and touch | Common kernel primitives plus stock vendor modules/HAL and firmware dependencies. | Preserve source/binary contracts; stock HBP warning needs matching vendor components, not a boot-only bypass. |

Configured drivers for other platforms in the built-in list are not Canoe
devices just because the GKI image links them. The stock oracle identifies
dormant load records with preserved policy; no blanket claim is made that an
unseen device is absent. No driver conversion or cross-firmware replacement
was made. See [the update ledger](upstream-update-ledger.tsv).
Peeled CodeLinaro Kaanapali tags for the reviewed graphics, display,
camera, WLAN, multimedia and security/DSP repositories are also preserved
in `out/coloros17001201-battery-evidence/codelinaro-advertised-refs.log`.
Advertised platform tags are not a proven ColorOS .201 firmware fit.
