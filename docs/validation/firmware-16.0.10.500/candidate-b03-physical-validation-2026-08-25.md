# Candidate B03 physical validation — 2026-08-25

## Result

**PASS — EVENTPOLL FILE AND RCU LIFETIME HARDENING PHYSICALLY VALIDATED ON
OOS 16.0.10.500**

Candidate B03 cumulatively contains the physically qualified B01 Binder
lifetime hardening, B02 BPF per-CPU map bounds hardening, and one new coherent
runtime series in `fs/eventpoll.c`. The new series pins watched files during
epoll removal and defers final `struct eventpoll` release until RCU readers have
completed. No ACK advancement, DLKM adaptation, or other customization was
included.

## Tested identity

| Field | Value |
|---|---|
| Device | OnePlus 15 CPH2747 / Canoe |
| Firmware | `CPH2747_16.0.10.500(EX01)` |
| Slot | `_a` |
| B02 qualified parent | `635709f3b14eaef8778abfbe92b8fbec3ed7e02e` |
| B03 runtime source head | `c3b68584dbb4638abe27a69b7f421826625d4a53` |
| Pre-physical documentation head | `b6358d7ad599648ac0c44659b3ae350090f1ed56` |
| New runtime change | Eventpoll file and RCU lifetime hardening series |
| Runtime kernel | `6.12.23-android16-5-o-gc3b68584dbb4-4k` |

The source and machine-code proof for `epi_fget()`, `ep_remove_file()`,
`ep_remove_epi()`, `ep_remove()`, `ep_clear_and_put()`,
`eventpoll_release_file()`, `ep_insert()`, and `do_epoll_ctl()` is recorded in
`candidate-b03-binary-delta.md`.

## Payload isolation

Only `boot_a` was written.

| Partition | Physical SHA-256 | Result |
|---|---|---|
| boot_a | `0b065aa6fc5c524107ecdf10ffefb41b464be86bfb9b8673b8fb649f5541dfc0` | candidate input, write, recovery read-back, and Android read-back PASS |
| system_dlkm_a | `18f530dcb0e46dc81ede00e18ac4e9b39faf6564fb067f04af9a912d87fd6dd7` | exact stock, unchanged |
| vendor_dlkm_a | `157db23ccfa516c7904d4e87614f62c3c99c2dcbb93cc3fc8ccd3388c2cc793f` | exact stock, unchanged |
| vendor_boot_a | `3027d80a33fcc65f506d2d909db862657fe28e7eafc6e2af94193d67e6e617eb` | exact stock, unchanged |

The firmware-native system-DLKM contract remained 82 `modules.load` entries,
with `wwan.ko` at entry 28. Neither DLKM, vendor_boot, VBMeta, nor slot metadata
was supplied to the flash operation.

## TWRP safety record

- device/slot guard: PASS, CPH2747/Canoe, `_a`
- hardened helper SHA-256:
  `84b035710c305be859aac72827489850b7d215ee372ab3341be849e051bfab50`
- independent full `boot_a` backup:
  `/home/travis/Android/oos16.0.10.500-kernel-compat/backups/b03-preflash-20260825/boot_a.img`
- source and backup size: 100663296 bytes
- source and backup SHA-256:
  `dd63f38c658bf81b259f41f5ade970a12e8742bf1e427ed866c532e5f308cb07`
- helper backup and manifest were also pulled to the host
- exact B02 rollback artifact: present and hash-verified
- boot-only dry-run: PASS, no partition modified
- helper flash target: `boot_a` only
- complete immediate `boot_a` read-back: PASS
- untouched stock DLKM and `vendor_boot_a` read-backs: PASS

## Boot and encrypted-user0 validation

| Test | Result |
|---|---|
| First Android boot | PASS |
| Second clean Android boot | PASS |
| Existing user 0, boot 1 | `RUNNING_UNLOCKED` |
| Existing user 0, boot 2 | `RUNNING_UNLOCKED` |
| `init_user0_failed` | NOT OBSERVED |
| Rescue Party/TWRP redirect | NOT OBSERVED |
| Prolonged OnePlus-logo stall | NOT OBSERVED |

The current encrypted userdata was unlocked intact on both boots. No format,
metadata wipe, encryption-policy change, vendor_boot change, VBMeta change, or
slot-metadata change was used.

## Eventpoll-specific physical coverage

A statically linked Android arm64 userspace harness used epoll, eventfd, pipe,
socketpair, timerfd, nested epoll registration, descriptor duplication,
watched-file close, child-process exit, concurrent wait, and epoll-instance
teardown paths.

The accepted bounded run produced:

| Metric | Result |
|---|---:|
| Duration | 300 seconds |
| Workers | 4 |
| Teardown loops | 2,972,084 |
| Epoll adds | 11,934,778 |
| Epoll deletes | 1,486,043 |
| Descriptor closes | 18,645,189 |
| Concurrent waits | 5,293,450 |
| Expected errors | 0 |
| Unexpected errors | 0 |
| Thread-creation errors | 0 |

The first diagnostic harness configuration was intentionally rejected as test
evidence after its unthrottled thread creation rate produced userspace resource
errors. The kernel and framework remained stable during that run. The harness
was then rate-bounded and instrumented for exact errno accounting; only the
clean zero-error five-minute run above is used for acceptance. The kernel
candidate was not changed or rebuilt.

Concurrent normal Android coverage included:

- 220/220 Settings and system-activity launch/close operations
- 50/50 Camera and Settings process force-stop/relaunch operations
- 60/60 package, activity, service, service-manager, and network-policy query
  rounds
- stable system_server, zygote64, SurfaceFlinger, and servicemanager processes
- 5/5 forced deep-idle/wake cycles with user0, WLAN, cellular, and connectivity
  retained

No available production tracepoint proved the exact vulnerable file-removal
and RCU-reader interleaving. The correct coverage classification is:

**NORMAL-RUNTIME COMPATIBILITY PASS — EXACT FIX TRIGGER NOT OBSERVED**

## Subsystem regression results

| Subsystem | Result |
|---|---|
| WLAN | PASS; 6135 MHz WPA3-SAE, Internet, off/on, and handoff recovery |
| Cellular | PASS; Visible LTE HOME/IN_SERVICE, RMNET IPv4/IPv6, routes, fail cause NONE/0x0, numeric IP, DNS |
| Bluetooth | PASS; off/on recovery and the existing HID device reconnected after the second boot |
| NFC | PASS; service on, Google Wallet/Pay HCE registered, eSE1 route present |
| Camera | PASS; Oplus camera opened and saved `IMG20260825184907.heic` (1294087 bytes) |
| Audio | PASS; ringtone playback active and user confirmed audio was perfect |
| Graphics/UI | PASS; normal rendering and task switching through the activity churn |
| USB | PASS; ADB/USB enumeration survived both clean boots |
| Deep idle/resume | PASS, 5/5 |
| Clean boots | PASS, 2/2 |

The first cellular sample immediately after Wi-Fi was disabled lost one of
five IPv4 packets and one of five IPv6 packets. Registration, RMNET, routes,
and data fail cause remained healthy. After the handoff stabilized, IPv4,
IPv6, and DNS each passed 5/5. This is recorded as **TRANSIENT POST-HANDOFF
PACKET LOSS — NON-BLOCKING**, not an eventpoll regression.

## Error scan

Full preflight, recovery, dmesg, logcat, eventpoll harness, activity/process
churn, handoff, camera, audio, deep-idle, and second-boot evidence is retained
under:

`out/oos1610500-custom-r53-b03-final/physical-20260825/`

No new relevant match was found for:

- eventpoll, epitem, epi-file, or epoll-removal fault
- use-after-free, refcount failure, wrong-slab free, double-free, list
  corruption, Oops, BUG, KASAN, UBSAN, or panic
- Unknown symbol, CRC/MODVERSION disagreement, invalid vermagic, signature,
  or protected-export failure
- IOMMU/SMMU fault or hung task
- system_server, zygote, servicemanager, hwservicemanager, or framework crash
  loop
- `init_user0_failed`, fscrypt, metadata-encryption, or CE-unlock failure

The first B03 boot emitted a stock
`oplus_network_702_satellite` duplicate-class warning for
`/class/satellite-702-dev`. It was unrelated to eventpoll, caused no subsystem
failure, did not recur on the required second clean boot, and is absent from
the final accepted error scan. Other boot-time OEM warning trace heads match
the previously qualified B02 pattern.

## Decision

**PASS — CANDIDATE B03 EVENTPOLL FILE AND RCU LIFETIME HARDENING PHYSICALLY
VALIDATED**

No rollback was required. This qualification covers Candidate A plus
B01+B02+B03 only. It does not authorize ACK advancement, DLKM adaptation, main
promotion, B04, or release publication.
