# OnePlus 15 CPH2747 OxygenOS 16.0.9.400 r8

Controlled-v1 kernel and module-stack release for **OnePlus 15 CPH2747 / Canoe
only**, running **OxygenOS 16.0.9.400(EX01)**.

## Installation

Flash `OnePlus15-CPH2747-OOS16.0.9.400-r8-TWRP.zip` from TWRP. The ZIP is
self-contained: it validates the device and active slot, requires a decrypted
writable backup destination, backs up every selected partition, flashes in
dependency order, and read-back verifies every write.

Changed partitions:

- `vendor_dlkm`
- `system_dlkm`
- `boot` last

Not changed:

- `vendor_boot`
- VBMeta
- `system_dlkm_oki`
- active-slot metadata

## Qualified contract

- Kernel: `6.12.23-android16-5-o-g6744a3f6bcf4-4k`
- WLAN `.053`: 6 GHz / 6135 MHz WPA3 and reload
- Cellular: RMNET IPv4/IPv6, routes, IP/DNS, handoff, IPA/GSI `.102`
- Audio `.059` selective GPR hardening
- Camera `.073` selective RER hardening
- Bluetooth vendor `.046`: OFF/ON recovery with zero framework crashes
- NFC `.102`: OFF/ON recovery, Wallet/HCE, and `eSE1`
- Graphics `.057` selective secure-guard handling and normal UI operation
- USB and cellular tethering
- CoreSight `.099.086` selective guard with normal-runtime compatibility
- 46-entry system-DLKM load contract with `wwan.ko` at entry 21
- Zero symbol, CRC, MODVERSION, vermagic, signature, or protected-export
  failures in the release scan

The exact release ZIP passed a full TWRP dry run, normal single-ZIP install,
partition backups, read-back verification, Android boot, and post-install
subsystem validation. One packet was lost during the first Wi-Fi-to-cellular
handoff ping; the stabilized IP and DNS tests passed 5/5. No data-call,
RMNET, IPA/GSI, OEM failure code, or kernel fault accompanied it, so it is
classified as a non-blocking transient.

## Coverage boundaries

- The CoreSight active ETR SYSFS buffer-resize condition was not observed and
  the hardening behavior under that condition is not claimed.
- RMNET priority-fix trigger, IPA/GSI SSR paths, IPA firmware-already-loaded
  path, and a runtime call to `gsi_status_enabled()` were not observed.
- Bluetooth fresh-pair/BLE/A2DP/AVRCP/HFP/HID equipment coverage remains
  separate from the core-qualified OFF/ON path.
- NFC tag read/write and a real payment transaction were not performed.
- Hotspot client, MLO, P2P, and natural roaming remain environment-dependent
  coverage items.

## Integrity

- `boot.img`: `84a85b9a40adc7bdb178bf86e2da74e9bac3feaf7bd502c7d29b1df27dea55ab`
- `system_dlkm.img`: `de77afb66f47f3430f1a5bcc8a79297792141652f8a256e17b4c715f49222cef`
- `vendor_dlkm.img`: `dccb47a3ba4055b3d385d80d6fda3875bc544488e1ed2cbd9951cd0662f56b26`
- TWRP ZIP: `a603e6af54325c86e20182ad0d24e91c7d5743697a1c51a6ec4bdcf7424bd8ed`

Physically tested runtime source HEAD: `36f40a44d7004`.
