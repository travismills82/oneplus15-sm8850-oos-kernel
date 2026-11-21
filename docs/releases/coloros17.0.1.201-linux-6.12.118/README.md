# OnePlus 15 ColorOS 17.0.1.201 — Linux 6.12.118

**Boot-only pre-release.** Tested on one unlocked OnePlus 15 **CPH2747 (Canoe)** running **CPH2747_17.0.1.201(EX01)** with **slot A active** and stock ColorOS modules. Do not use this image on another firmware, model, locked device, or other slot based on this release.

| Item | Value |
|---|---|
| Kernel release | `6.12.118-android16-6-o-g76b73bd515be-4k` |
| Exact source | `76b73bd515be1ed698c7df08ae29666eb3848b08` |
| Boot image size | 100,663,296 bytes |
| `boot.img` SHA256 | `8aecec4a35214a1ecb27e2170c28dcdd444542ac764bb3d84d3ab97447bfe16b` |

Only `boot.img` is a flashable asset. The device test preserved the installed stock DLKM stack and the existing `vbmeta`, `vendor_boot`, `dtbo`, `init_boot`, and recovery. Do not replace them, change slots, disable verification, or erase data for this release. The boot footer is development-signed, not OEM-signed; it was accepted on the tested unlocked firmware setup. No recovery ZIP is included.

## Install and roll back

1. Confirm the exact model and firmware in Android. Before any write, back up your exact known-good `boot_a` image and hash, and verify that you can reach bootloader fastboot independently if Android fails. Do not proceed during an OTA or snapshot operation.
2. Verify the downloaded image with `sha256sum -c boot.img.sha256`. In bootloader fastboot, verify `product: canoe`, `unlocked: yes`, `is-userspace: no`, `current-slot: a`, `partition-size:boot_a: 0x6000000`, and `snapshot-update-status: none`. Stop if a prerequisite cannot be verified.
3. Only on that matching setup, write `fastboot flash boot_a boot.img`. Reboot only after fastboot reports success. Verify Android starts, user0 unlocks, and the expected kernel is running. Where read-back is available, compare the complete partition against the published SHA256.
4. If boot or user unlock fails, return to bootloader fastboot and restore your verified backup to the same `boot_a` partition. Check the partition hash and user unlock afterward. Do not switch slots or touch userdata.

## Validation and known limitations

The exact published image passed physical boot and full `boot_a` read-back, Android user0/FBE with F2FS inline encryption, and stock DLKM verity checks. Offline common build, strict KMI, independent ColorOS ABI, linked layout/KCFI checks, and ARM64 guest regression passed. The stock oracle found **1,031 active modules compatible, 38 dormant, zero failures**. Enforced Google GKI **failed (exit 4; 76 documented report blocks)**; the required ColorOS `arm_smmu` CRCs were retained. This is not full hardware or extended suspend qualification.

- Stock `oplus_powermodel` issues a timekeeping warning on both cluster entry and early return in suspend-to-idle. The OEM kernel has the same diagnostic and call ordering. No driver fix or warning suppression is included.
- The stock tethering/netd userspace compatibility dependency leaves the BPF/BHB diagnostic unresolved. The stock OPlus HBP FORTIFY warning is also unresolved pending matching vendor sources or fixed modules.

See the [conditional qualification](../../validation/firmware-17.0.1.201/coloros17001201-warning-conditional-qualification.md) and [suspend investigation](../../validation/firmware-17.0.1.201/coloros17001201-powermodel-suspend-investigation.md). Full binary/log evidence referred to in those reports is retained offline, not redistributed here.
