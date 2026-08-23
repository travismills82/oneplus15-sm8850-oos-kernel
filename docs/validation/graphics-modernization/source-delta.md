# Graphics `.038` to `.057` source/value audit

## Provenance

- Current CPH2747 synchronization: `5ab2a689ff87d7d28c511f1762cf41c1b90d965a`
- Current tag: `AU_TECHPACK_GRAPHICS.LA.16.0.R1.00.00.00.000.038`
- New comparison synchronization: `d447f713d6403f707a2910383495f4ada98cfa4d`
- New tag: `AU_TECHPACK_GRAPHICS.LA.16.0.R1.00.00.00.000.057`
- New synchronization product: OnePlus PLZ110, not CPH2747

The comparison changes 77 Graphics kernel/device-tree files with 1,855
insertions and 606 deletions.  The active device is `Adreno840v2`, bound to
`qcom,adreno-gpu-gen8-2-0`, and uses the monolithic `msm_kgsl.ko` DDK target.

## Contract findings

- Public `include/linux/msm_kgsl.h` and `include/uapi/linux/msm_kgsl.h` objects
  are identical between `.038` and `.057`.
- Firmware request names are identical.  The installed CPH2747 Gen8.2
  firmware remains authoritative; no PLZ110 firmware is imported.
- The full `.057` driver is not a bounded update.  It changes internal KGSL
  structures, Gen8.2 GMU feature flags, BCL/mitigation data, non-context
  register programming, power-stat locking, reclaim allocation behavior, and
  active Canoe device tree.
- Active Canoe DT changes include an added QDSS register range, a larger GMU
  frequency/bandwidth table, and changed GPU bus/power levels.  They are not
  part of the selected candidate.
- The target enables `ADRENO_GMU_AB` for Gen8.2.  That is an HFI/firmware
  negotiation change and is rejected without matched CPH2747 firmware proof.
- Hybrid allocation/reclaim and devcoredump additions are disabled by the
  retained Canoe configuration and provide no current runtime value.

## Selected runtime change

`kgsl_free_secure_page()` is used to release the secure IOMMU guard page during
KGSL/IOMMU teardown.  `.038` ignores the result of `kgsl_unlock_sgt()` and
unconditionally returns the page to the allocator.  `.057` frees it only after
secure-world ownership is released successfully; otherwise it logs the error
and deliberately leaks the unrecoverable page.

This is bounded memory-ownership hardening.  It prevents a page still owned by
the secure side from being reused by Linux.  It is Canoe-reachable because the
active KGSL driver uses the secure pagetable/guard-page path.  Normal Android
runtime does not unload KGSL, so a physical compatibility test will not prove
the failure branch unless a natural teardown/unlock failure occurs.  No unsafe
fault injection is proposed.

Only the reviewed function hunk is imported.  The `.057` power, GMU, reclaim,
snapshot, DT, build-config, firmware, and new-hardware changes are excluded.

## Userspace and firmware boundary

The current proprietary graphics contract is retained byte-for-byte:

- GPU userspace package: `com.qualcomm.qti.gpudrivers.canoe.api36`
- EGL/Vulkan implementation: Adreno
- active firmware names: `gen80200_gmu.bin` and `gen80200_zap.mbn`
- Canoe panel/GPU DT and all GPU power levels: unchanged
- SurfaceFlinger/HWC/display stack: unchanged

The selected function body does not alter ioctl/UAPI, sysfs, firmware/HFI,
device tree, exported symbols, structure layout, or module parameters.
