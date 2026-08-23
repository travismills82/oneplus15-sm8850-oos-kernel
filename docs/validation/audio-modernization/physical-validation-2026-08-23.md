# Audio .059 selective GPR physical validation

Date: 2026-08-23

Tested branch: `experiment/audio-059-gpr-teardown`

Tested source HEAD: `a310804ef75d48723e37a8ae515e4ea557b7dd59`

Canonical branch:
`feature/controlled-v1-wlan053-bt046-nfc102-cellular102-audio059-gpr`

Status: **PASS — AUDIO .059 GPR NORMAL-RUNTIME COMPATIBILITY VALIDATED;
TEARDOWN PATH NOT OBSERVED**

This was a vendor-DLKM-only physical test of the selective Audio `.059` GPR
teardown hardening. The only newer runtime source is `gpr_dlkm.ko`; the other
22 audio modules in the delivery closure retain their qualified stock source
and are controlled-v1 re-signs required by protected-export enforcement. The
test validates normal boot, audio routing, playback, capture, suspend, and
coexistence. It does not claim that `gpr_remove()` or the pending-work race was
physically exercised.

## Payload identity and write boundary

| Item | Identity |
| --- | --- |
| kernel release | `6.12.23-android16-5-o-g6744a3f6bcf4-4k` |
| slot | `_b` |
| boot SHA-256 | `84a85b9a40adc7bdb178bf86e2da74e9bac3feaf7bd502c7d29b1df27dea55ab` |
| system-DLKM SHA-256 | `de77afb66f47f3430f1a5bcc8a79297792141652f8a256e17b4c715f49222cef` |
| rollback vendor-DLKM SHA-256 | `538622b7d5ff73ab092619cdfab31099ffa3e0638b9051329bf950a04a5260a2` |
| tested vendor-DLKM SHA-256 | `bb005e764ccfc3af7eec9a73f291a85a44d966478b73ee480617003ae44b079b` |
| runtime/package `gpr_dlkm.ko` SHA-256 | `e15643897eaae82a6bc01d9661182a7f2aa3e89d30145219116e2613e49756c0` |

Only `vendor_dlkm_b` was written. Boot, system-DLKM, vendor boot, VBMeta, and
slot metadata were not written. The installed kernel release remained the
qualified g6744 contract on every boot.

## Recovery safety and write verification

The pre-flash Android state matched the qualified cellular102 baseline:
CPH2747/Canoe, slot `_b`, boot complete, the exact g6744 kernel, Visible LTE
HOME/IN_SERVICE, dual-stack RMNET, routes, and working IP/DNS.

TWRP reported the expected device and slot, user 0 decrypted, a writable
backup destination, and no active Virtual A/B snapshot. The exact hardened
controlled-stack helper from commit
`3f499bfd1f7152ea27b27935be22ff73581709a1` was staged under `/tmp`; recovery
itself was not modified.

The helper made a full verified backup at:

`/sdcard/TWRP/kernel-flash-backups/controlled-stack-b-20260823-124619/vendor_dlkm_b.img`

The backup matched the rollback SHA-256 exactly. The vendor-DLKM-only dry run
passed the device, slot, snapshot, backup, capacity, ext4, AVB structure, and
input-hash gates without writing. The subsequent flash changed only
`vendor_dlkm_b`. The helper read-back and an independent full-partition
read-back both matched the tested candidate SHA-256 exactly before Android was
booted.

## Runtime module and service proof

Android loaded `gpr_dlkm` and the expected audio transport, codec, macro,
SoundWire, machine, and DSP-interface modules. The live `gpr_dlkm.ko` file
matched the packaged `.059` binary. A complete runtime/package comparison of
all 23 intended audio replacements reported 23 PASS and zero hash mismatches.

The Audio HAL, `audioserver`, AudioFlinger, and SoundTrigger services remained
alive. GPR port registrations and AGM/PAL graph creation completed during
playback, capture, and sound-trigger recovery without a GPR registration or
probe failure.

## Audio functional results

| Test | Result |
| --- | --- |
| first boot | PASS |
| clean Android boots | PASS — 3/3 total |
| ringtone / built-in speakers | PASS — user reported crystal-clear output |
| Recorder capture | PASS — active `AUDIO_DEVICE_IN_BUILTIN_MIC` route and user-confirmed complete mic pickup |
| recording playback | PASS — active 48 kHz `AUDIO_DEVICE_OUT_SPEAKER` route and user-confirmed clear voice |
| route transition, capture to speaker playback | PASS |
| forced deep-idle / wake | PASS — 5/5 |
| sound-trigger service after audio activity | PASS |
| earpiece / real call | NOT TESTED — NO CALL PLACED |
| speakerphone / HFP call route | NOT TESTED — NO CALL PLACED |
| Bluetooth A2DP / AVRCP | NOT TESTED — A2DP EQUIPMENT NOT CONNECTED |
| USB-C audio | NOT TESTED — AUDIO ADAPTER UNAVAILABLE |
| camera-recording audio | NOT TESTED — OUTSIDE THE SELECTIVE GPR SANITY SCOPE |

The Recorder created `Standard recording 1.mp3` under the normal user
recordings directory. Its SHA-256 is
`ae9fcd6cc76d4851575c1bffc2b8fdfad25e858c2c037c240703c0ddb49ed5e5`.
The user recording remains on the device; it was not copied into git or the
candidate package.

The qualitative speaker and microphone results were confirmed by the user,
not inferred from framework state alone. AudioFlinger independently showed
the expected active input/output routes, a started Recorder AudioTrack, and
advancing frame counters.

## Existing-subsystem regression results

| Subsystem | Result |
| --- | --- |
| WLAN `.053` | PASS — reload 3/3 and reconnection to 6135 MHz WPA3-SAE |
| cellular102 | PASS — Visible HOME/IN_SERVICE, dual-stack RMNET, routes, 1.1.1.1 5/5, DNS 5/5 |
| Bluetooth `.046` | PASS — toggle 3/3 and existing REDMI Watch 6 ACL/HID connection |
| Bluetooth audio | NOT TESTED — no active A2DP/HFP audio device |
| NFC `.102` | PASS — service healthy, Wallet/HCE registration and eSE route present |
| NFC toggle | NOT COMPLETED — root `svc nfc disable` was killed without changing controller state |

The proven system-DLKM load contract remained unchanged: 46 entries with
`wwan.ko` at entry 21.

## GPR teardown coverage

The `.059` source change calls `cancel_work_sync()` before GPR notifier and
device teardown in `gpr_remove()`. Normal audio playback, capture, SoundTrigger
activity, suspend, and reboot do not unload the active GPR provider. No safe
normal-runtime event invoked the module remove path during this session. The
module was not force-unloaded and ADSP SSR was not induced merely to exercise
the change.

- `.059` machine-code/source presence: **PASS**
- normal-runtime compatibility: **PASS**
- `gpr_remove()` teardown path: **NOT OBSERVED**
- pending-work race condition: **NOT OBSERVED**

The change remains classified conservatively as teardown
work-race/lifetime hardening, not a complete UAF or security fix.

## Error scan and baseline comparison

The final dmesg/logcat scan found zero new relevant:

- unknown symbols, MODVERSION/CRC disagreements, or vermagic failures;
- module-signature or protected-export failures;
- GPR probe, registration, graph, or transport failures;
- audio service crashes, kernel oops, panic, KASAN, UBSAN, UAF, or refcount
  failures attributable to the candidate.

SoundWire `SWR CMD Ignored` diagnostics and Oplus blocked-task traces for
`zram_comp`, `hfi_core_dbg_cl`, and `osml_monitor` have direct matches in the
pre-candidate RMNET_CORE/IPA-GSI captures. They were not introduced by the GPR
candidate. The audio services and tested routes continued to work after those
messages.

Raw physical evidence is retained outside git at:

`/home/travis/Android/audio-gpr-059-live-captures/20260823T174240Z-preflash/`

## Decision

**PASS — AUDIO .059 GPR NORMAL-RUNTIME COMPATIBILITY VALIDATED; TEARDOWN PATH
NOT OBSERVED**

No rollback was required. The exact tested vendor-DLKM remains installed on
slot `_b`. This pass advances the canonical development baseline for the next
ordered subsystem audit, but does not turn untested call, A2DP, HFP, or USB-C
audio routes into PASS results.
