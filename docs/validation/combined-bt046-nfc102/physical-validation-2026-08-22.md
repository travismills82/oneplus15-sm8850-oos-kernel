# Combined Bluetooth .046 and NXP NFC .102 core qualification — 2026-08-22

## Result

**PASS — COMBINED BT046 + NFC102 CORE BASELINE VALIDATED**

The combined vendor-DLKM-only candidate booted Android three times and
preserved the frozen g6744 kernel, WLAN `.053`, and exact-stock cellular data
contracts. Bluetooth and NFC controller stress, radio coexistence, cellular
takeover, airplane-mode recovery, forced deep idle, and the module/security
error gates passed.

This is a core-qualified development baseline, not a final release. Tests that
need a dedicated Bluetooth peripheral, NFC tag, payment terminal, hotspot
client, or controllable multi-band AP remain explicitly open.

## Exact tested stack

| Item | Physical identity |
| --- | --- |
| Slot | `_b` |
| Kernel release | `6.12.23-android16-5-o-g6744a3f6bcf4-4k` |
| Boot SHA-256 | `84a85b9a40adc7bdb178bf86e2da74e9bac3feaf7bd502c7d29b1df27dea55ab` |
| System-DLKM SHA-256 | `de77afb66f47f3430f1a5bcc8a79297792141652f8a256e17b4c715f49222cef` |
| Combined vendor-DLKM SHA-256 | `3ed964f345e6f5040c70ef7c0c083c1fc4bab536b6a522ca83c61b20be032ed4` |
| Vendor boot | stock, unchanged |
| VBMeta | stock, unchanged |
| WLAN | `.053` |
| Bluetooth vendor | `.046` |
| NFC vendor | `.102` |
| Cellular data modules | exact stock OOS 16.0.9.400(EX01) 27-module closure |
| Signing generation | `controlled-v1` |

Only four vendor module payloads differ from the qualified WLAN053 baseline:

| Module | Live file SHA-256 | Result |
| --- | --- | --- |
| `btpower.ko` | `f21baebef606e2d076827cbd87a1bcde0adfac9e785dffc9ac86a0d194c0e09f` | loaded |
| `bt_fm_swr.ko` | `9408e38f2d61fc97e4610a4b97ce1d9814097a385187bd205983062c37d48f21` | loaded |
| `btfm_slim_codec.ko` | `dde56a787da9e7925bb1ca08ffadaf837654675e3d9fef9d4b560bfae00131fc` | loaded |
| `nxp-nci.ko` | `51ef28ae123a7b2c0fd851491e1a13abfbd19b3b4a9a66acf3e4b997096ca9c2` | loaded |

The live hashes of `cfg80211.ko`, `mac80211.ko`, and
`qca_cld3_peach_v2.ko` remained the qualified WLAN053 values. The complete
27-module IPA/GSI/RMNET/data closure remained byte-identical to the accepted
stock-cellular contract.

## Recovery, dry run, and write verification

- TWRP 3.7.1_16 booted independently on slot `_b`.
- User-0 decryption and writable backup-storage guards passed.
- Virtual A/B snapshot state was `none`.
- The hardened helper from TWRP commit
  `3f499bfd1f7152ea27b27935be22ff73581709a1` passed its vendor-DLKM-only
  dry run, including device, slot, ext4, capacity, payload-hash, and backup
  checks.
- The pre-flash NFC102 vendor-DLKM was backed up at
  `/sdcard/TWRP/kernel-flash-backups/controlled-stack-b-20260822-133542/`.
  Its full-size hash was verified by the backup manifest.
- The exact qualified WLAN053 rollback artifact with SHA-256
  `8f2ae729e404c96d2ffdada4a698bd70821a9f8d82bca379e8a0924a9bd0655e`
  remained available separately.
- Only `vendor_dlkm_b` was written. Boot, system-DLKM, vendor boot, and VBMeta
  were not rewritten.
- Helper and independent block-device read-back both reproduced the candidate
  SHA-256 before reboot.
- Independent `e2fsck -fn` completed cleanly after the write.

Slot `_a` was not treated as a fallback. TWRP, the verified slot `_b` backup,
and the qualified rollback artifact remained the recovery path.

## Combined core test matrix

| Test | Result | Evidence / limitation |
| --- | --- | --- |
| Android boots | PASS | three clean boots; exact g6744 release on every boot |
| Replacement modules | PASS | all four loaded with exact candidate file hashes |
| Bluetooth OFF/ON x25 | PASS | 25/25 state transitions; adapter remained usable |
| Existing Bluetooth-device reconnect | PASS | bonded Redmi Watch established an encrypted BR/EDR L2CAP connection |
| Fresh pair / forget / re-pair | NOT TESTED — EQUIPMENT INTERACTION UNAVAILABLE | bonded user equipment was not destructively forgotten |
| Controlled BLE peripheral | NOT TESTED — EQUIPMENT UNAVAILABLE | background BLE activity is not counted |
| A2DP / AVRCP | NOT TESTED — EQUIPMENT UNAVAILABLE | no active audio peripheral |
| HFP call audio | NOT TESTED — EQUIPMENT UNAVAILABLE | no call-capable headset test |
| HID | NOT TESTED — EQUIPMENT UNAVAILABLE | no active keyboard, mouse, or controller |
| NFC OFF/ON x25 | PASS | 25/25 authorized root framework transitions returned to `on` |
| NFC HAL/framework | PASS | controller loaded; NFC service and NXP/Oplus HALs healthy |
| Wallet / HCE availability | PASS | Google payment component and payment AIDs registered |
| eSE framework routing | PASS | `eSE1` connected; failure counter remained zero |
| NFC tag read/write | NOT TESTED — EQUIPMENT UNAVAILABLE | no physical tag was presented |
| Contactless payment | NOT TESTED — ENVIRONMENT UNAVAILABLE | no terminal transaction was attempted |
| Wi-Fi OFF/ON x10 | PASS | every cycle returned to 6135 MHz |
| 6 GHz / WPA3 | PASS | 6135 MHz WPA3-SAE association retained |
| 2.4/5 GHz re-association | NOT RETESTED — BAND STEERING | WLAN module hashes are exact; saved same-SSID network selected 6 GHz |
| 6 GHz sustained traffic | PASS | 100 MiB HTTPS transfer completed with Bluetooth/NFC enabled and LTE registered |
| Hotspot basic start/stop | PASS | 5 GHz SoftAP started on `wlan2` and stopped cleanly |
| Hotspot client traffic | NOT TESTED — SECOND DEVICE UNAVAILABLE | no client association/DHCP/sharing test |
| Cellular registration | PASS | Visible LTE HOME / IN_SERVICE |
| RMNET | PASS | `rmnet_data2` carried IPv4 and IPv6 addresses and default routes |
| Cellular IPv4 | PASS | `1.1.1.1` passed with 0% loss after Wi-Fi takeover |
| Cellular IPv6 | PASS | `2606:4700:4700::1111` passed with 0% loss |
| Cellular DNS | PASS | `google.com` resolved and replied with 0% loss |
| Wi-Fi/cellular handoff | PASS | Wi-Fi OFF moved traffic to RMNET; Wi-Fi ON restored WLAN |
| Airplane mode x5 | PASS | WLAN, Bluetooth, NFC, Visible LTE, RMNET, and DNS recovered each cycle |
| Forced deep idle / wake | PASS | after a 30-second forced idle, all radios and cellular reachability recovered |
| Bluetooth-disabled boot | PASS | Android booted with Bluetooth disabled; adapter was restored afterward |
| NFC-disabled boot policy | NOT RETAINED BY OXYGENOS | OxygenOS restored NFC ON at boot despite the persistent disable request; no module failure occurred |
| Cross-subsystem stress | PASS | sustained 6 GHz traffic with BT/NFC enabled and cellular registered |

The initial non-root NFC toggle command was rejected by OxygenOS with a
`SecurityException`; those shell-process crash-buffer entries were induced by
the invalid command path and are not counted as an NFC framework or HAL crash.
The counted toggle run used the authorized root command path.

## Runtime delivery contracts

- vendor modules: 436;
- intended replacements: 4;
- unexpected replacements: 0;
- exact-stock cellular modules: 27/27;
- system-DLKM `modules.load`: 46 entries;
- `wwan.ko`: entry 21;
- stale system load entries: 0; and
- missing system load entries: 0.

`bt_fm_swr` initialized successfully against the retained `swr_dlkm`
provider. `nxp_nci` loaded against its validated retained providers and the
SN220 controller/HAL path remained healthy.

## Error scan

Across the three boots and stress operations there were no new relevant:

- unknown symbols or unresolved imports;
- MODVERSION/CRC disagreements;
- vermagic, module-signature, or protected-export failures;
- duplicate registration involving a replacement module;
- Bluetooth, NFC, CNSS, IPA/RMNET firmware or subsystem crash;
- Oops, BUG, KASAN, UBSAN, panic, use-after-free, refcount failure, or
  IOMMU/SMMU fault attributable to the candidate.

The broad `Call trace` scan also found existing Oplus/vendor diagnostics:
duplicate `/proc/oplus_mem`, `/proc/task_info`, `/proc/bcl_stat`, and satellite
sysfs registration warnings; a touch HBP warning; IRQ diagnostic warnings;
and Oplus hung-task enhancement stack dumps. None originated in the four
replacement modules, and they are not classified as combined-candidate
regressions.

NXP standby-transition diagnostics and Oplus NFC property/SELinux messages
were not accompanied by controller loss, HAL restart, framework failure, or a
failed recovery operation. They remain recorded as existing vendor behavior,
not as proof of a new `.102` defect.

## Promotion decision

No rollback was required. The phone remains on the exact combined candidate
with Wi-Fi, Bluetooth, NFC, and cellular active. This result supports freezing
`feature/controlled-v1-wlan053-bt046-nfc102-core` as a **core-qualified**
development baseline. The open equipment-dependent items above remain open
and must not be reported as passing.
