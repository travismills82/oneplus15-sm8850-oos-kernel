# ACK 6.12.23 rollback baseline

This file freezes the exact physically qualified state from which the isolated
ACK 6.12.24 experiment starts. The payloads listed here are immutable rollback
artifacts and are not rebuilt by the point-release migration.

## Source identity

- physically tested runtime source HEAD: `36f40a44d700422969dc7debb6519c0f9ab977d0`
- canonical lineage HEAD at experiment creation: `39bfc1db04fe323c1a6c75ee089d8eb4817002d4`
- experiment base branch: `feature/controlled-v1-audio059-graphics057-camera073-platform086-coresight`
- OnePlus manifest ACK revision: `b2a876903b495c444a94b16f50d1463ffe953957`
- OnePlus manifest ACK tag: `android16-6.12-2025-06_r53`
- kernel release: `6.12.23-android16-5-o-g6744a3f6bcf4-4k`

The commits between the physically tested runtime source HEAD and the
canonical lineage HEAD contain release packaging, tooling, and validation
documentation only. Their `kernel_platform/common` tree is identical.

## Exact payloads

| Artifact | SHA-256 |
|---|---|
| `boot.img` | `84a85b9a40adc7bdb178bf86e2da74e9bac3feaf7bd502c7d29b1df27dea55ab` |
| `system_dlkm.img` | `de77afb66f47f3430f1a5bcc8a79297792141652f8a256e17b4c715f49222cef` |
| `vendor_dlkm.img` | `dccb47a3ba4055b3d385d80d6fda3875bc544488e1ed2cbd9951cd0662f56b26` |
| `Image` | `9295e2a8bd06fdb002750b8adb99e37c0e1e2f6c440cbccfa90a2095fee6356b` |
| `vmlinux` | `b544a89858647d3257a0b50a7c03e5feefcf85399fc2bfc5ff7904544dedcb14` |

## Kernel contract

| Contract item | Frozen value |
|---|---|
| generated `.config` SHA-256 | `b53d48b303059adb49a8dbe457145a4b7523a77fae621ea8d9e7b0b727e1615b` |
| `Module.symvers` SHA-256 | `de57709f3de38afb3e266481da09433687979ffb88ee607bda93ac4732dd7e0b` |
| `System.map` SHA-256 | `4f8ef4feb7c71abc18def8db341701e05b1ff7c0e8c8b50de7da48ef100f011a` |
| controlled signing certificate SHA-256 | `04093d5e0b816927dc1e4fcdbd4ec754df6e69960918aa2eaa69bea6878f6faf` |
| ABI | PASS; report empty |
| KMI | PASS |
| system `modules.load` | 46 entries |
| `wwan.ko` | entry 21 |

## Physical qualification

The permanent physical record is
`docs/validation/platform-modernization/physical-validation-2026-08-23.md`.
It covers the final controlled WLAN, cellular, audio, graphics, camera,
Bluetooth, NFC, USB, and normal-runtime CoreSight stack. The final single-ZIP
TWRP installation and partition read-back validation are recorded separately
under `docs/validation/release-packaging/`.

This baseline is the rollback target for the ACK 6.12.24 experiment. No
physical write is authorized by the static modernization task.
