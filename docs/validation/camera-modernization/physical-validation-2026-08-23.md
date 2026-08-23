# Camera `.073` selective RER physical validation

## Result

**PASS — NORMAL-RUNTIME COMPATIBILITY VALIDATED**

The bounded `CAMERA_SENSOR_FLASH_CMD_TYPE_RER` private-command snapshot is
physically compatible with CPH2747. The ordinary Camera workloads used during
qualification did not prove that the RER command path ran, and the hostile
userspace mutation race was intentionally not injected. This result therefore
does not claim that the hardening branch itself was physically exercised.

The user independently confirmed that the Camera was working properly after
the automated and guided functional matrix.

## Tested contract

- tested source branch: `experiment/camera-073-audit`
- tested source HEAD: `e1d4b2b61c6c8f4fb8e3afe9bb9d187c83bc7fbf`
- selected source commit: `ca405a58e9d5fd5967db4b2509f2a37b6ba9f2a9`
- kernel: `6.12.23-android16-5-o-g6744a3f6bcf4-4k`
- boot SHA-256: `84a85b9a40adc7bdb178bf86e2da74e9bac3feaf7bd502c7d29b1df27dea55ab`
- system_dlkm SHA-256: `de77afb66f47f3430f1a5bcc8a79297792141652f8a256e17b4c715f49222cef`
- candidate vendor_dlkm SHA-256: `8623653732b1f23a8250badc23bb0d99b25b812b5e8afc4ddcb9875154662531`
- pre-test Graphics-qualified vendor_dlkm SHA-256:
  `a41a96de52a6f18fe956ad928b6c3b3f8fa58ff4f3b90c2b65df0d49b538dce0`
- vendor_boot: unchanged
- VBMeta: unchanged
- slot: `_b`; slot metadata unchanged

Only `vendor_dlkm_b` was written. Runtime identity matched the packaged
candidate exactly:

| Module | SHA-256 | Runtime result |
|---|---|---|
| `camera.ko` | `604210dc629630cc322fbd7ecd57e793d9391e9cb55f2ff61382ce3671d04e2c` | loaded; `.073` RER snapshot selected |
| `camera_extension.ko` | `0919eabcfe15346b96186bd42abb23f7213353e82667d7e9970d4cee8930ac6e` | loaded; stock allocated-section contract, controlled re-sign |
| `msm_kgsl.ko` | `eedc18337b03eeefa83ec00fdcfccdc813359eebbb44fc90345319fcb6cc4472` | unchanged Graphics baseline |
| `gpr_dlkm.ko` | `e15643897eaae82a6bc01d9661182a7f2aa3e89d30145219116e2613e49756c0` | unchanged Audio baseline |

## Recovery transaction

The hardened helper from
`feature/controlled-kernel-installer@3f499bfd1f7152ea27b27935be22ff73581709a1`
was staged in TWRP and used directly.

- device guard: PASS (`CPH2747`, canoe, project 24863)
- current slot: `_b`
- Virtual A/B update state: none
- user 0 decryption and writable-backup guard: PASS
- vendor_dlkm-only dry run: PASS; no partitions modified
- target capacity/ext4 validation: PASS
- full pre-write backup: PASS
- backup/source SHA-256:
  `a41a96de52a6f18fe956ad928b6c3b3f8fa58ff4f3b90c2b65df0d49b538dce0`
- backup directory:
  `/sdcard/TWRP/kernel-flash-backups/controlled-stack-b-20260823-145820/`
- post-write read-back SHA-256:
  `8623653732b1f23a8250badc23bb0d99b25b812b5e8afc4ddcb9875154662531`
- post-write filesystem validation: PASS

The exact packaged Graphics-qualified rollback image remained available in
addition to the verified TWRP backup.

## Camera functional matrix

- stock OxygenOS Camera launch and live preview: PASS
- rear primary still capture: PASS
- rear ultrawide (`0.6x`) still capture: PASS
- rear telephoto (`3.5x`) still capture: PASS
- front-camera still capture: PASS
- autofocus/tap-to-focus: PASS
- flash-on capture: PASS; EXIF reports `On, Fired`
- always-on flash preview: PASS
- 1080p30 HEVC video: PASS; 17.626 seconds
- video audio: PASS; stereo AAC at 48 kHz
- Camera open/close stress: 25/25 PASS
- forced deep-idle/wake: 10/10 PASS; Camera reopened normally
- clean Android reboots: 3/3 PASS; live Camera preview reopened after each

The validated video is
`VID20260823150914.mp4`, SHA-256
`9e4ee0b48427032bd527962c2158edf806915751a303b4d67e8c68401f9387ed`.

## Retained-subsystem regression

- WLAN `.053`: 6135 MHz, WPA3-SAE, IP/DNS and OFF/ON reload PASS
- cellular `.102` core: Visible LTE HOME/IN_SERVICE, dual-stack
  `rmnet_data2`, IPv4/IPv6 default routes, IP and DNS PASS
- Bluetooth `.046`: OFF/ON recovery and existing HID reconnect PASS
- NFC `.102`: OFF/ON recovery, host routing, Google Pay HCE and `eSE1`
  routing PASS
- Audio `.059` selective GPR module: byte-identical and retained; recorded
  video contained valid stereo audio
- Graphics `.057` selective KGSL module: byte-identical and retained
- system `modules.load`: 46 entries; `wwan.ko` remained loaded

## Error scan and baseline comparison

Final module-loader scan found zero unresolved symbol, MODVERSION, vermagic,
signature, or protected-export failures. No Camera-attributable Oops, BUG,
panic, KASAN, UBSAN, UAF, refcount, IOMMU/SMMU fault, or memory-safety failure
was found.

Transient CSID `DRV config failed` messages appeared during aggressive
open/close and mode-transition stress. The same error class is present in the
pre-Camera Graphics-qualified baseline, user-visible Camera operation remained
healthy, and a clean post-reboot capture produced no Camera error, fault, or
timeout in the dmesg delta.

Oplus touch `frame_put()` and OEM hung-task-enhance informational traces were
also compared with the prior baseline and were not introduced by Camera.

## Coverage boundary

- RER command-path observation: NOT OBSERVED
- hostile command-count mutation race: NOT INJECTED
- selected machine-code hardening presence: PASS from static validation
- normal Camera/HAL/firmware compatibility: PASS

## Decision

Freeze this exact vendor_dlkm as the Camera-qualified development baseline.
Retain the authoritative `.061` Canoe camera stack plus only the reviewed
`.073` RER snapshot hardening. Do not import the remaining `.073` camera,
firmware, HAL, UAPI, synchronization, SMMU, sensor, or foreign-DT generation.
