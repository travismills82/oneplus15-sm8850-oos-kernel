# Platform `.099.064` to `.099.086` selective audit

## Provenance

The two halves of the official OnePlus source publication were audited.

| Source | CPH2747 current | Newer comparison |
|---|---|---|
| kernel/SOC repository | `fc30e54174d254ff7f33622a9278e4435f6718d2` | `0cfd948c0d9507d0bc04706f6f919a25787c2d89` |
| modules/devicetree repository | `5ab2a689ff87d7d28c511f1762cf41c1b90d965a` | `d447f713d6403f707a2910383495f4ada98cfa4d` |
| product sync | CPH2747 16.0.9.400(EX01) | PLZ110 16.0.8.300(CN01) |
| Qualcomm platform tag | `AU_LINUX_KERNEL.PLATFORM.5.0.R1.00.00.00.099.064` | `AU_LINUX_KERNEL.PLATFORM.5.0.R1.00.00.00.099.086` |

The kernel/SOC comparison contains 378 changed paths and 866 commits not in
the CPH2747 branch. Most of that history is new-chip support, build
infrastructure, or coherent changes to foundational providers. The
modules/devicetree comparison contains 2,207 changed paths, but most belong to
the Audio, Graphics, Display, Camera, WLAN, Bluetooth, NFC, and cellular
generations already audited separately. PLZ110 device-tree content is not a
Canoe hardware contract and is not imported.

The selected CoreSight source file in this workspace was byte-identical to the
official `.099.064` object before the backport:

- `.099.064` object: `8597dd39c86a0b190d34237ac43254aa8520c1e2`
- `.099.086` object: `c9f5070de6d7684b787c1f485860badd58479f88`
- selected upstream/vendor commit:
  `db1b06b53dcf37388f95105123ba36a854724d34`

## Selected fix

The bounded change is in
`drivers/hwtracing/coresight/coresight-tmc-etr.c`, function
`tmc_etr_get_sysfs_buffer()`.

When an ETR sink is already enabled in SYSFS mode, changing its requested
buffer size and enabling another source can make the old implementation
replace and free the live buffer. The `.099.086` fix observes the already
enabled SYSFS mode under the existing spinlock and leaves the active buffer in
place. This is a seven-line memory-lifetime guard with no new UAPI, DT,
firmware, module parameter, import, export, or shared-type contract.

`coresight_tmc.ko` is loaded on the physically qualified CPH2747 baseline.
The vulnerable control path is privileged tracing/debug functionality and was
not shown to run during ordinary Android use, so the change is classified as
memory-safety and reliability hardening, not as a demonstrated Android
application exploit fix.

## Scope rejected from the first candidate

- Gunyah vCPU lifetime fixes: real UAF/race value, but a multi-commit
  foundational VM-provider closure.
- QPACE/ZRAM allocation and completion fixes: active and useful, but touch the
  live swap provider and interact with Oplus ZRAM ownership.
- DWC3, UFS, IOMMU, SCM/TZMEM, RPMSG/GLINK, remoteproc, GDSC, scheduler, and
  DMA-heap changes: broad providers with many retained consumers.
- `memlat` missing-unlock fix: a good later one-module candidate, but ranked
  behind the explicit CoreSight UAF guard.
- foreign SoC clocks, pinctrl, RPM, interconnect, Ethernet, EOM, and LED
  additions: not applicable to CPH2747.
- `.099.086` build/Kleaf and Canoe config changes: not needed for the selected
  external module fix and would alter the frozen kernel contract.

No wholesale Platform tree or PLZ110 device tree is imported.
