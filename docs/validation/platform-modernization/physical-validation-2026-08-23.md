# Platform `.099.086` selective CoreSight ETR physical validation

## Result

**PASS — NORMAL-RUNTIME COMPATIBILITY VALIDATED**

The bounded active-SYSFS-ETR buffer lifetime guard is physically compatible
with CPH2747. Normal operation did not enable an ETR sink or reach the guarded
buffer-resize condition, and no tracing fault was injected. This result does
not claim that the hardening branch itself was physically exercised.

## Tested contract

- tested source branch: `experiment/platform-099086-audit`
- tested source HEAD: `871e53d405b156df2584300fc3b80be5b0fc32bb`
- selected official source commit:
  `db1b06b53dcf37388f95105123ba36a854724d34`
- kernel: `6.12.23-android16-5-o-g6744a3f6bcf4-4k`
- boot SHA-256:
  `84a85b9a40adc7bdb178bf86e2da74e9bac3feaf7bd502c7d29b1df27dea55ab`
- system_dlkm SHA-256:
  `de77afb66f47f3430f1a5bcc8a79297792141652f8a256e17b4c715f49222cef`
- candidate vendor_dlkm SHA-256:
  `dccb47a3ba4055b3d385d80d6fda3875bc544488e1ed2cbd9951cd0662f56b26`
- pre-test Camera-qualified vendor_dlkm SHA-256:
  `8623653732b1f23a8250badc23bb0d99b25b812b5e8afc4ddcb9875154662531`
- vendor_boot: unchanged
- VBMeta: unchanged
- slot: `_b`; slot metadata unchanged

Only `vendor_dlkm_b` was written. The live partition read-back matched the
candidate exactly. The loaded module file matched the packaged candidate:

| Module | SHA-256 | Runtime result |
|---|---|---|
| `coresight-tmc.ko` | `27129ab30432761f65dff183b60268e63fbd7dab6390ce54c530adc737d5a0d3` | loaded; controlled-v1 signer; exact frozen `g6744` vermagic |

## Recovery transaction

The hardened helper from
`feature/controlled-kernel-installer@3f499bfd1f7152ea27b27935be22ff73581709a1`
was staged in TWRP and used directly.

- device guard: PASS (`CPH2747`, canoe, project 24863)
- current slot: `_b`
- Virtual A/B update state: none
- user 0 decryption and writable-backup guard: PASS
- vendor_dlkm-only dry run: PASS; no partitions modified
- target capacity/ext4/AVB validation: PASS
- full pre-write backup: PASS
- backup/source SHA-256:
  `8623653732b1f23a8250badc23bb0d99b25b812b5e8afc4ddcb9875154662531`
- backup image:
  `/sdcard/TWRP/kernel-flash-backups/platform-coresight-etr/controlled-stack-b-20260823-165440/vendor_dlkm_b.img`
- helper post-write read-back SHA-256:
  `dccb47a3ba4055b3d385d80d6fda3875bc544488e1ed2cbd9951cd0662f56b26`
- independent post-write read-back SHA-256: exact match
- post-write filesystem validation: PASS

The exact packaged Camera-qualified rollback image remained available in
addition to the helper-verified TWRP backup. Rollback was not required.

## CoreSight runtime contract

- `coresight_tmc`: loaded on both clean boots
- candidate module runtime hash: PASS
- signer: `OnePlus 15 Controlled OOS Module Signing v1`
- vermagic: exact frozen kernel release
- symbol/CRC/protected-export/signature failures: 0
- ETR sinks `tmc_etr0` and `tmc_etr1`: present, disabled, memory output mode
- secure modem ETR: present, disabled
- ETF sink: present, disabled
- active SYSFS ETR trace: not enabled
- guarded buffer-resize trigger: NOT OBSERVED
- fault injection: NOT PERFORMED

Read-only topology inspection was used. No sink was force-enabled, no buffer
was resized, and no modem or tracing failure was induced merely to exercise
the new branch.

## Physical regression matrix

- clean Android boots: 2/2 PASS
- forced deep-idle/wake: 5/5 PASS
- Graphics/UI: PASS; SurfaceFlinger active at 1272x2772 and 120 Hz
- Camera: PASS; rear preview opened and a new HEIC still was saved
- Audio: PASS; ringtone/media playback obtained audio focus and the user
  confirmed that the ringtone was clear and perfect
- WLAN `.053`: PASS; 6135 MHz, WPA3-SAE, IP/DNS, and OFF/ON reload 3/3
- cellular `.102` core: PASS; Visible LTE HOME/IN_SERVICE, dual-stack
  `rmnet_data2`, IPv4/IPv6 routes, IP/DNS, and failure cause `NONE (0x0)`
- Bluetooth `.046`: PASS; disable/enable recovery returned to `ON`
- NFC `.102`: PASS; disable/enable recovery, Google Wallet HCE, host routing,
  and connected `eSE1`
- USB: PASS; MTP + ADB remained available
- system load contract: `wwan` loaded; frozen 46-entry metadata unchanged

## Error scan and baseline comparison

The final live scan and both full boot captures contain zero unresolved
symbol, MODVERSION, vermagic, signature, or protected-export failures. No
CoreSight/TMC/ETR-attributable Oops, BUG, panic, KASAN, UBSAN, UAF, refcount,
lock, IOMMU/SMMU, DMA, or memory-safety failure was found.

The extra-looking boot-2 call trace was the Oplus satellite driver's duplicate
class warning. The same trace occurs in the pre-candidate Camera-qualified
baseline. Existing Oplus touch, OEM hung-task-enhance, CoreSight UETM `-22`,
and disabled policy/reset-source messages were likewise not introduced by the
candidate.

## Coverage boundary

- selected machine-code hardening presence: PASS from static validation
- normal CoreSight topology and module compatibility: PASS
- active ETR SYSFS buffer-resize condition: NOT OBSERVED
- hardening behavior under the vulnerable condition: NOT DEMONSTRATED

## Decision

Freeze this exact vendor_dlkm as the selective Platform-qualified development
baseline. Retain the authoritative `.099.064` Canoe platform generation plus
only the reviewed `.099.086` CoreSight ETR lifetime guard. Do not import the
remaining `.099.086` platform, scheduler, storage, VM, IPC, power, or
foreign-device generation.
