# OnePlus 15 SM8850 kernel

Authorized Android/Linux kernel development for the OnePlus 15
(`infiniti` / Canoe / Qualcomm SM8850), Android 16, and firmware-specific
OxygenOS/ColorOS 16 and 17 releases.

Kernel releases are firmware-specific. Never cross-flash a boot image merely
because it targets the same phone family; each OxygenOS build has its own
kernel, module, FBE/user0, and boot-container contract.

## Latest release: Linux 6.12.117 (boot.img only)

Main includes the Linux stable .116 to .117 interval: 1,525 individual imports,
ten already-present equivalents and one coherent infrared prerequisite, with
seven separate ColorOS KABI preservation commits. Upstream authorship and
commit order remain reviewable; no squash, CRC forcing or security-check bypass.

The exact tested `6.12.117-android16-6-o-g9218fe3f104b-4k` boot image passed
full read-back, Android/PIN/user0 FBE, F2FS inlinecrypt, stock dm-verity and the
1,070-module stock compatibility oracle on **CPH2747_17.0.0.102(EX01)**.
All 691 actually loaded module build IDs match stock. The owner reports
radios, camera/video, calls, fingerprint and USB sharing working. KernelSU/SUSFS,
Droidspaces prerequisites and built-in CAKE/FQ/FQ-CoDel remain operational;
production network policy is unchanged. Stock GPU/GEAS, DLKM and KernelSU
init_boot are untouched.

This is a **scoped normal release**, not full stable or GKI qualification.
Enforced GKI remains **FAIL: 45 unchanged prior + eight reviewed new difference
owners**. Known HFI/HBP/eBPF warnings, QCOM suspend/timekeeping and UFS telemetry
limitations remain disclosed. OTG storage, separate audio recording, TWRP
decrypt, repeated boots, unplugged idle/deep suspend and extended power/stability
are not qualified by these results. One phone's test does not prove universal
firmware or regional compatibility.

Only the exact tested `boot.img`, checksums, manifest and instructions are
published. No DLKM, vbmeta, init_boot or unqualified recovery ZIP is supplied;
old .115 ZIPs contain different kernels. The previous .116 CAKE release remains
available as a firmware-matched rollback. No image was rebuilt for publication.

[Download Linux 6.12.117](https://github.com/travismills82/oneplus15-sm8850-oos-kernel/releases/tag/coloros17.0.0.102-linux-6.12.117) ·
[Installation, rollback and limitations](docs/releases/coloros17.0.0.102-linux-6.12.117/README.md) ·
[Integration/ABI record](docs/validation/firmware-17.0.0.102/coloros1700102-linux-6.12.117-integration-20261111.md) ·
[Physical canary and owner results](docs/validation/firmware-17.0.0.102/coloros1700102-linux-6.12.117-physical-canary-20261111.md).

## Repository layout

| Path | Purpose |
|---|---|
| `kernel_platform/common/` | ACK/GKI kernel and ABI/KMI definitions |
| `kernel_platform/soc-repo/` | Canoe build targets, configuration, and module policy |
| `kernel_platform/qcom/` | Qualcomm integration and device-tree inputs |
| `kernel_platform/oplus/` | Oplus build/configuration integration |
| `vendor/` | Qualcomm, OnePlus/Oplus, NXP, and ST source inputs |
| `tools/` | Repository validation and release tooling |
| `docs/` | Qualification, compatibility, and release evidence |

No generated output, private key, device backup, or proprietary firmware
payload belongs in source control.
