# Bluetooth vendor .046 physical qualification — 2026-08-22

## Result

**PARTIAL — CORE PASS, OPTIONAL EQUIPMENT TESTS REMAIN**

The vendor-DLKM-only Bluetooth `.046` candidate boots and preserves the
qualified WLAN053 and stock cellular behavior.  Core initialization, repeated
Bluetooth power cycling, suspend/resume, airplane-mode recovery, enabled and
disabled reboot paths, 5/6 GHz Wi-Fi coexistence, hotspot initialization, and
cellular packet data passed.  Fresh pairing, deliberate BLE connection,
A2DP/AVRCP, HFP call audio, and HID require suitable peripherals and remain
unqualified.

This result does not yet freeze a canonical release branch.

## Exact tested contract

| Item | Tested value |
| --- | --- |
| Source branch | `experiment/bluetooth-vendor-046` |
| Source HEAD | `79c231fcd767d5dd73acbf04bb47152f4b6c31c6` |
| Kernel release | `6.12.23-android16-5-o-g6744a3f6bcf4-4k` |
| `boot_b` SHA-256 | `84a85b9a40adc7bdb178bf86e2da74e9bac3feaf7bd502c7d29b1df27dea55ab` |
| `system_dlkm` payload SHA-256 | `de77afb66f47f3430f1a5bcc8a79297792141652f8a256e17b4c715f49222cef` |
| `vendor_dlkm_b` SHA-256 | `50943999a1b5c006d64b7397edeb1debff343fc8d5602c930820f820968f60b2` |
| WLAN generation | `.053` unchanged |
| Bluetooth vendor generation | `.046` |
| Cellular closure | 27 exact stock OOS modules unchanged |
| Vendor boot | stock, unchanged |
| VBMeta | stock, unchanged |

The live `system_dlkm_b` mapper is larger than the EROFS payload.  Hashing the
first 45,658,112 bytes, the exact payload length, reproduced the qualified
`de77afb6...22cef` hash.  The full mapper hash includes logical-partition
padding and is not the payload identity.

## Recovery and flash gate

- The live pre-flash `vendor_dlkm_b` hash exactly matched the qualified
  WLAN053 rollback image:
  `8f2ae729e404c96d2ffdada4a698bd70821a9f8d82bca379e8a0924a9bd0655e`.
- TWRP booted independently on slot `_b`; user storage was decrypted and
  writable.
- The hardened `twrp-flash-controlled-stack` helper passed device, slot,
  snapshot (`none`), ext4, capacity, candidate-hash, and backup checks in a
  vendor-DLKM-only dry run.
- A full-size backup was created at
  `/sdcard/TWRP/kernel-flash-backups/controlled-stack-b-20260822-085753/` and
  reproduced the qualified rollback SHA-256.
- Only `vendor_dlkm_b` was written.  Its read-back hash exactly matched the
  `.046` candidate before Android was booted.
- Slot `_a` was not treated as a fallback.  TWRP plus the verified slot `_b`
  backup and qualified rollback image remained the recovery path.

## Runtime module contract

All three replacement modules loaded from the candidate vendor-DLKM:

| Module | Live file SHA-256 | Runtime result |
| --- | --- | --- |
| `btpower.ko` | `f21baebef606e2d076827cbd87a1bcde0adfac9e785dffc9ac86a0d194c0e09f` | loaded |
| `bt_fm_swr.ko` | `9408e38f2d61fc97e4610a4b97ce1d9814097a385187bd205983062c37d48f21` | loaded |
| `btfm_slim_codec.ko` | `dde56a787da9e7925bb1ca08ffadaf837654675e3d9fef9d4b560bfae00131fc` | loaded |

`btfm_swr_probe` completed and registered the expected BTFMCODEC hardware
endpoint and five DAIs.  Together with the absence of an unknown-symbol or
version error, this is runtime evidence that `bt_fm_swr` resolved `swr_read`
from the retained `swr_dlkm` provider.

The `.046` `btpower` change is described only as **task-liveness hardening**.
This physical test does not establish complete reference ownership or prove a
complete UAF fix.

## Qualification matrix

| Test | Result | Evidence / limitation |
| --- | --- | --- |
| Android first boot | PASS | exact g6744 kernel release; all three replacement modules live |
| Bluetooth OFF/ON x25 | PASS | 25/25 cycles; zero framework crashes; adapter still scanned and initialized |
| Existing-device reconnect | PASS | bonded Redmi Watch established ACL/HFP/RFCOMM connections during cycling |
| Fresh pair, forget, re-pair | NOT TESTED — EQUIPMENT INTERACTION UNAVAILABLE | no peripheral was placed into a safe fresh-pair state; bonded user devices were not destructively forgotten |
| Deliberate BLE scan/connect | NOT TESTED — EQUIPMENT UNAVAILABLE | background BLE activity is not counted as a controlled BLE test |
| A2DP | NOT TESTED — EQUIPMENT UNAVAILABLE | no active headphones/speaker |
| AVRCP | NOT TESTED — EQUIPMENT UNAVAILABLE | no active media peripheral |
| HFP call audio | NOT TESTED — EQUIPMENT UNAVAILABLE | no call-capable headset test |
| HID | NOT TESTED — EQUIPMENT UNAVAILABLE | bonded controller was not active |
| Screen suspend/resume x10 | PASS | Bluetooth, WLAN, and cellular recovered after every cycle |
| Forced deep idle | PASS | 30-second forced idle; clean exit; Bluetooth ON, WLAN and cellular healthy |
| Connected-peripheral deep-idle continuity | NOT TESTED — EQUIPMENT UNAVAILABLE | no peripheral was continuously connected for the forced-idle interval |
| Airplane mode x5 | PASS | Bluetooth policy, WLAN, Visible registration, RMNET, and DNS recovered each time |
| Clean boots with Bluetooth enabled | PASS | first candidate boot plus a second enabled boot |
| Clean boot with Bluetooth disabled | PASS | disabled through OxygenOS UI before reboot; booted in OFF/BLE-only state; subsequently restored ON |
| 2.4 GHz association + Bluetooth | NOT TESTED — SAFE BAND STEERING UNAVAILABLE | same-SSID 2.4 GHz BSSIDs were visible, but saved network configuration was not altered to force association |
| 5 GHz association + Bluetooth | PASS | 5220 MHz, WPA3-SAE, Internet healthy |
| 6 GHz association + Bluetooth | PASS | 6135 MHz, WPA3-SAE, Internet healthy |
| Bluetooth audio + WLAN | NOT TESTED — EQUIPMENT UNAVAILABLE | no active audio peripheral |
| 6 GHz sustained traffic | PASS | six 50 MB HTTPS transfers completed; 50/50 Internet pings, 0% loss; Bluetooth stayed ON with zero crashes |
| Hotspot basic start/stop | PASS | `wlan2` reached `TetheredState`, selected `rmnet_data2` upstream, and stopped cleanly |
| Hotspot client association/traffic | NOT TESTED — SECOND DEVICE UNAVAILABLE | no client was connected |
| Cellular coexistence | PASS | Visible LTE HOME/IN_SERVICE; RMNET IPv4+IPv6; IPv4/IPv6 routes; IP and DNS passed |

The first shell-only Bluetooth-disable attempt did not persist across reboot
and was not counted.  The counted disabled-state boot used OxygenOS's UI and
verified the adapter OFF before reboot.

## Error scan

Across the captured boots and final runtime logs:

- unknown symbols: 0;
- MODVERSION/CRC disagreements: 0;
- vermagic failures: 0;
- module-signature failures: 0;
- protected-export failures: 0;
- duplicate registrations: 0;
- Bluetooth framework crashes: 0;
- new Bluetooth/CNSS firmware crash, panic, or fatal fault: 0.

OxygenOS emitted existing boot-time UART optional-IRQ messages, Bluetooth
property SELinux denials, and Oplus diagnostic call traces.  The same messages
and the same hung-task subjects occur in the frozen WLAN053 captures, so they
are recorded as pre-existing rather than `.046` regressions.

## Captures

Raw local evidence is retained outside Git at:

`/home/travis/Android/oneplus15-bt046-live-captures/20260822T085451-preflash/`

It includes pre-flash state, TWRP dry-run/flash output, backup verification,
each boot's `dmesg` and `logcat`, Bluetooth toggles, airplane-mode and suspend
cycles, cellular state, hotspot state, coexistence traffic, and final hashes.

## Release decision

No rollback was required; the phone remains on the exact `.046` candidate
with Bluetooth restored ON.  Core platform behavior passed, but the missing
fresh-pair/audio/HFP/HID/controlled-BLE hardware tests prevent a full physical
qualification claim and canonical branch freeze.
