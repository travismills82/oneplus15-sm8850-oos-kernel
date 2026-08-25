# Candidate B01 physical validation — 2026-08-25

## Result

**PASS — BINDER TRANSACTION TARGET LIFETIME PINNING PHYSICALLY VALIDATED ON
OOS 16.0.10.500**

Candidate B01 changes only the Binder transaction target lifetime handling in
the firmware-native r53 kernel. It retains the exact stock current-firmware
`system_dlkm`, `vendor_dlkm`, `vendor_boot`, VBMeta policy, and slot metadata.
No ACK advancement or second project customization was included.

## Tested identity

| Field | Value |
|---|---|
| Device | OnePlus 15 CPH2747 / Canoe |
| Firmware | `CPH2747_16.0.10.500(EX01)` |
| Slot | `_a` |
| Candidate-A qualified parent | `e0e55a0625a938c9aa8c0dbaef0e6abff9664184` |
| B01 runtime source head | `35f7cb764f0ade56b94df3b16a40443cc98be4c1` |
| Pre-physical documentation head | `eddc809943d54dc8ed233487abd28874630ef21b` |
| Runtime change | Binder transaction target lifetime pinning |
| Runtime kernel | `6.12.23-android16-5-o-g35f7cb764f0a-4k` |

## Payload isolation

Only `boot_a` was written.

| Partition | Physical SHA-256 | Result |
|---|---|---|
| boot_a | `2646a4d773ac6360cf981c4148fd37b128e8f0cd53abd07418a6807641e9d091` | candidate input/write/read-back PASS |
| system_dlkm_a | `18f530dcb0e46dc81ede00e18ac4e9b39faf6564fb067f04af9a912d87fd6dd7` | exact stock, unchanged |
| vendor_dlkm_a | `157db23ccfa516c7904d4e87614f62c3c99c2dcbb93cc3fc8ccd3388c2cc793f` | exact stock, unchanged |
| vendor_boot_a | `3027d80a33fcc65f506d2d909db862657fe28e7eafc6e2af94193d67e6e617eb` | exact stock, unchanged |

The firmware-native system-DLKM contract remained 82 `modules.load` entries,
with `wwan.ko` at entry 28. Neither DLKM was supplied to the flash operation.

## TWRP safety record

- device/slot guard: PASS, CPH2747/Canoe, `_a`
- snapshot/update state: `none`
- `/data/media/0` decryption guard: correctly refused backup use with
  `Required key not available`; no write had occurred
- verified recovery-tmpfs plus host backup:
  `/home/travis/Android/oos16.0.10.500-kernel-compat/backups/b01-preflash-20260825/boot_a.img`
- source and backup SHA-256:
  `30195cbfe97e3cd831b4e84ed575c68cb5d079f18bbed1e6ee84a91a32cdfb70`
- backup size: 100663296 bytes, full `boot_a` partition
- boot-only manual dry-run: PASS
- candidate capacity/header/hash checks: PASS
- complete post-write `boot_a` read-back SHA-256: PASS
- independent post-write stock DLKM and vendor_boot hashes: PASS

The hardened data-media guard was fail-closed. The boot backup was instead
held in recovery tmpfs, pulled to the host, and verified byte-for-byte before
the boot-only write.

## Boot and encrypted-user0 validation

| Test | Result |
|---|---|
| First Android boot | PASS |
| Second clean Android boot | PASS |
| Existing user 0 after credential, boot 1 | `RUNNING_UNLOCKED` |
| Existing user 0 after credential, boot 2 | `RUNNING_UNLOCKED` |
| `init_user0_failed` | NOT OBSERVED |
| Rescue Party/TWRP redirect | NOT OBSERVED |
| Prolonged OnePlus-logo stall | NOT OBSERVED |

The existing encrypted user data initialized and unlocked normally on both
boots. No formatting, metadata change, encryption-policy change, or recovery
workaround was used.

## Binder-specific stress

Binderfs diagnostics were available at `/dev/binderfs/binder_logs` and were
captured before and after stress.

- 60 successful application/activity launch, home, close, and safe force-stop
  operations across Settings, Oplus Camera, Wallet, Chrome, Contacts, and Play
  Store
- 400 additional package-manager, service-manager, and activity-manager Binder
  operations
- critical services were not force-stopped
- global Binder transactions increased substantially:
  - `BC_TRANSACTION`: 124388 to 427865, +303477
  - `BR_TRANSACTION`: 124245 to 427319, +303074
  - `BC_FREE_BUFFER`: 215633 to 695850, +480217
  - `BR_TRANSACTION_COMPLETE`: 215645 to 696052, +480407
  - `BR_FAILED_REPLY`: 139 to 139, no increase
  - `BR_DEAD_REPLY`: 1 to 116, consistent with deliberate non-critical process
    teardown
- pending Binder transactions after stress: 0
- `system_server`, zygote, servicemanager, vndservicemanager, and SurfaceFlinger
  remained alive
- Android crash log buffer after stress: empty

The kernel logged undelivered transactions and process-died replies during the
deliberate application force-stop churn. Those messages were temporally
associated with intentional target destruction; there was no increase in the
global `BR_FAILED_REPLY` count and no framework crash or memory-safety report.

Result: **Binder stress PASS; framework and system_server stability PASS.**

## Subsystem regression results

| Subsystem | Result |
|---|---|
| WLAN | PASS; 6135 MHz WPA3-SAE, Internet, off/on recovery |
| Cellular | PASS; Visible LTE HOME/IN_SERVICE, active `rmnet_data2`, IPv4/IPv6, routes, IP/DNS |
| Bluetooth | PASS; off/on recovery and existing HID connection state returned connected |
| NFC | PASS; service remained on, Google Wallet HCE and eSE1 routing present |
| NFC toggle | NOT TESTED — stock shell security policy |
| Camera | PASS; Oplus camera opened and created `IMG20260825060838.heic` |
| Audio | PASS; ringtone session PLAYING, AudioFlinger speaker route active, 7002240 frames written |
| Graphics/UI | PASS; normal SurfaceFlinger/UI operation |
| USB | PASS; ADB/USB enumeration remained available |
| Deep idle/resume | PASS, 5/5 cycles; user0, Wi-Fi, cellular, and UI recovered |

Immediately after one Wi-Fi-to-cellular transition, the first numeric ping
lost one of five packets. After the route stabilized, numeric IP and DNS tests
both passed 5/5 with no RMNET, data-call, IPA/GSI, or kernel error. This is
recorded as a non-blocking handoff transient, not a B01 regression.

## Error scan

Full `dmesg`, `logcat -b all`, Binder state/statistics, activity/process state,
and the Android crash buffer were captured after Binder stress and final
subsystem checks under:

`out/oos1610500-custom-r53-b01-final/physical-20260825/`

No new relevant match was found for:

- Unknown symbol, CRC/MODVERSION disagreement, invalid vermagic, signature, or
  protected-export failure
- Binder use-after-free, refcount failure, double-free, invalid pointer, list
  corruption, or general-protection fault
- kernel panic, Oops, BUG, KASAN, or UBSAN report
- IOMMU/SMMU fault
- system_server, zygote, servicemanager, hwservicemanager, or vendor-service
  crash loop
- `init_user0_failed`, fscrypt, or metadata-encryption failure

Oplus emitted its established blocked-task informational traces for
`adci_thread` and `zram_comp`. The same signatures and timing are documented
on earlier physically qualified controlled baselines. They are not Binder
failures and were not accompanied by a fatal or memory-safety condition.

## Decision

**PASS — CANDIDATE B01 BINDER LIFETIME HARDENING PHYSICALLY VALIDATED**

No rollback was required. This qualification covers B01 only and does not
authorize ACK advancement, DLKM adaptation, or another project customization.
