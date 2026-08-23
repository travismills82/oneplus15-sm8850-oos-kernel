# Camera `.061` to `.073` audit

## Provenance

- CPH2747/OxygenOS 16.0.9.400 synchronization:
  `5ab2a689ff87d7d28c511f1762cf41c1b90d965a`
- current camera-kernel and camera-devicetree generation:
  `AU_TECHPACK_CAMERA.LA.6.0.R1.00.00.00.000.061`
- comparison synchronization:
  `d447f713d6403f707a2910383495f4ada98cfa4d`
- comparison generation:
  `AU_TECHPACK_CAMERA.LA.6.0.R1.00.00.00.000.073`
- comparison product: PLZ110/OnePlus 15T, not CPH2747

The complete `.073` delta spans 130 camera files and 14,990 insertions plus
3,615 deletions. It changes IFE/CSID/VFE, SMMU, synchronization, sensor,
actuator, flash, OPE, UAPI, and device-tree contracts. The target device tree
also replaces Infiniti/CPH2747 sensor and CSIPHY definitions with Fairlady and
other-product definitions. That generation is not accepted as a matched Canoe
camera stack.

## Selected bounded change

The only selected `.073` hunk is in:

`drivers/cam_sensor_module/cam_flash/cam_flash_core.c`

Function: `cam_flash_pmic_pkt_parser()`

Command: `CAMERA_SENSOR_FLASH_CMD_TYPE_RER`

The `.061` code validates `count` in a command buffer mapped from userspace
and then reads that same mutable value again while copying LED currents into a
fixed-size kernel array. A concurrent userspace mutation can therefore change
the loop bound after validation. The `.073` behavior snapshots the fixed-size
RER command into private kernel memory, verifies that `count` did not change
during the copy, validates the private count, and consumes only the private
snapshot.

This is a real userspace-boundary race/bounds hardening change. It does not
change a UAPI structure, exported symbol, device-tree property, firmware
request, sensor description, HAL interface, or proprietary calibration
contract. It is compiled into the active monolithic `camera.ko`; RER command
use by ordinary OxygenOS camera flows has not yet been proven.

Source object identities:

- `.061` whole-file object: `d48d9298bdb4896e9448b8bd8162b91df1acb2c3`
- `.073` whole-file object: `bd59af1eee86033ac2a18aefd920e3b533d4303b`
- selected Canoe object is recorded by the build guard after applying only the
  reviewed RER hunk

## Rejected `.073` scope

- Fairlady/PLZ110 and other-product camera DT and CSIPHY definitions
- IFE/CSID/VFE generation changes
- camera UAPI and request-manager changes
- sensor, actuator, OPE, sync, and SMMU generation-wide changes
- changes requiring matched camera firmware, HAL, tuning blobs, proprietary
  libraries, or foreign sensor descriptions

Those changes have a broad kernel/firmware/HAL/device-tree closure and are
deferred.

