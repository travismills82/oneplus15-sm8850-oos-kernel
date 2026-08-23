# Graphics `.057` selective secure-guard physical validation

## Result

**PASS — NORMAL-RUNTIME COMPATIBILITY VALIDATED**

The bounded `kgsl_free_secure_page()` ownership change is physically
compatible with Canoe.  The secure-world unlock-failure branch did not occur
and was not fault-injected, so this result does not claim that the hardening
branch was physically exercised.

## Tested contract

- tested source HEAD: `a2d512ad779b082749ca10879b7b6d71aca145b6`
- kernel: `6.12.23-android16-5-o-g6744a3f6bcf4-4k`
- boot SHA-256: `84a85b9a40adc7bdb178bf86e2da74e9bac3feaf7bd502c7d29b1df27dea55ab`
- system_dlkm SHA-256: `de77afb66f47f3430f1a5bcc8a79297792141652f8a256e17b4c715f49222cef`
- candidate vendor_dlkm SHA-256: `a41a96de52a6f18fe956ad928b6c3b3f8fa58ff4f3b90c2b65df0d49b538dce0`
- pre-test Audio-qualified vendor_dlkm SHA-256:
  `bb005e764ccfc3af7eec9a73f291a85a44d966478b73ee480617003ae44b079b`
- vendor_boot: unchanged
- VBMeta: unchanged
- slot: `_b`; slot metadata unchanged

Only `vendor_dlkm_b` was written.  The runtime module hashes were:

| Module | SHA-256 | Runtime result |
|---|---|---|
| `msm_kgsl.ko` | `eedc18337b03eeefa83ec00fdcfccdc813359eebbb44fc90345319fcb6cc4472` | loaded; GEAS consumer attached |
| `oplus_bsp_geas_system.ko` | `38f376099e46ccaf060434d43df71689d44645738cd426a249e67be8a172da84` | loaded; controlled trust closure active |

## Recovery transaction

The exact helper from
`feature/controlled-kernel-installer@3f499bfd1f7152ea27b27935be22ff73581709a1`
was pushed into TWRP and used directly.

- device guard: PASS (`CPH2747`, canoe, project 24863)
- current slot: `_b`
- Virtual A/B update state: none
- user 0 decryption guard: PASS
- vendor_dlkm-only dry run: PASS
- target capacity/ext4 validation: PASS
- full pre-write backup: PASS
- backup/read source SHA-256: `bb005e764ccfc3af7eec9a73f291a85a44d966478b73ee480617003ae44b079b`
- backup directory:
  `/sdcard/TWRP/kernel-flash-backups/controlled-stack-b-20260823-135012/`
- post-write read-back SHA-256:
  `a41a96de52a6f18fe956ad928b6c3b3f8fa58ff4f3b90c2b65df0d49b538dce0`
- post-write filesystem validation: PASS

The independently packaged Audio-qualified rollback image remained available
in addition to the new verified backup.

## Graphics and display

- `msm_kgsl`, GMU, IOMMU contexts and the existing Gen8.2 firmware bound: PASS
- Vulkan device enumeration: Adreno 840, PASS
- Vulkan Android baseline 2021/2022 and Android 15/16 profiles: supported
- OpenGL ES: 3.2, Adreno 840, PASS
- SurfaceFlinger/HWC composition: PASS
- supported display modes reported: 60/90/120/144/165 Hz
- physically active during qualification: 60, 90 and 120 Hz
- 144/165 Hz: advertised and the original 165-Hz peak preference was restored,
  but the launcher policy did not select those modes; not claimed as exercised
- unlocked composition stress: 150 paired launcher transitions plus a valid
  45-second, 7.6-MiB screen recording, PASS
- sampled KGSL busy interval during stress: approximately 48 percent
- real Camera app/HAL preview: PASS; valid 1272x2772 preview capture
- 100-MiB HTTPS transfer while on WLAN: PASS
- three clean Android reboots: 3/3 PASS
- forced deep-idle/wake: 5/5 PASS

No standalone game or benchmark was installed.  The bounded candidate is
qualified for normal runtime, composition and camera-preview workloads; a
per-game 144/165-Hz workload remains optional coverage.

## Retained-subsystem regression

- WLAN `.053`: 6135 MHz, WPA3, reload/handoff and 100-MiB traffic PASS
- cellular `.102` core: Visible HOME/IN_SERVICE, dual-stack RMNET, default
  routes, IP and DNS PASS
- Bluetooth `.046`: OFF/ON recovery and an existing HID connection PASS
- NFC `.102`: service ON, host routing, HCE and `eSE1` routing PASS
- Audio `.059` selective GPR payload: byte-identical and retained
- system `modules.load`: unchanged; `wwan` loaded after every reboot

## Error scan and limitations

Final full-dmesg scan found zero:

- unresolved symbols or MODVERSION/vermagic failures
- signature or protected-export failures
- KGSL/GMU hangs, timeouts or GPU page faults
- IOMMU/SMMU faults
- Oops, panic, KASAN, UBSAN, UAF, refcount or hung-task failures

`kgsl_unlock_sgt failed` occurrences: **0**.  Therefore the selected failure
branch was not observed.

Two warnings during boot were compared with retained historical stock-DLKM
captures and proven pre-existing:

- duplicate `/proc/bcl_stat` registration in `bcl_pmic5`
- field-spanning write warning in Oplus touch-HBP `frame_put()`

Neither trace enters KGSL or the changed function.

## Decision

Freeze this exact candidate as the Graphics-qualified development baseline.
Do not import the remaining `.057` GMU/HFI, firmware, power, reclaim, build or
device-tree changes.
