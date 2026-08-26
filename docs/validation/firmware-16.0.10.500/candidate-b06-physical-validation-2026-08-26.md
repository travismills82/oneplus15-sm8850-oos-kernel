# Candidate B06 physical validation — 2026-08-26

## Result

**PASS — B06 AF_UNIX GC/SCC HARDENING PHYSICALLY QUALIFIED ON OOS
16.0.10.500**

This result establishes physical compatibility and regression coverage for
Candidate B06. It does not make a cybersecurity claim. The exact targeted
GC/SCC and concurrent `MSG_PEEK` interleaving was not directly observable, so
the accepted coverage classification is:

**NORMAL-RUNTIME COMPATIBILITY PASS — EXACT FIX TRIGGER NOT OBSERVED**

## Tested identity

| Field | Value |
|---|---|
| Device | OnePlus 15 CPH2747 / Canoe |
| Firmware | `CPH2747_16.0.10.500(EX01)` |
| Slot | `_a` |
| B05 qualified parent | `e17fd9021ded0d11c48ef5761df59ce931c6c0b5` |
| B06 runtime source head | `aad7cdfe6542f3fb51d751236bbacde59e2d9b93` |
| Pre-physical documentation head | `78468afd126f4a3fa41c30c3f1545f5a9795d99e` |
| Runtime kernel | `6.12.23-android16-5-o-gaad7cdfe6542-4k` |
| New runtime change | AF_UNIX garbage-collection and SCC hardening |

Candidate B06 cumulatively contains physically qualified B01 through B05 and
only the two-commit AF_UNIX group added for B06. No ACK, DLKM, vendor_boot,
VBMeta, device-tree, slot-metadata, or other runtime change was introduced.

## Payload and recovery isolation

Only `boot_a` was written.

| Partition | Physical SHA-256 | Result |
|---|---|---|
| boot_a | `31e6fff0b4212916b64614c4ec96c4f88c8f8cd7168e720e5f77c05b1d402825` | candidate input, write, recovery read-back, and Android read-back PASS |
| system_dlkm_a | `18f530dcb0e46dc81ede00e18ac4e9b39faf6564fb067f04af9a912d87fd6dd7` | exact stock read-back PASS |
| vendor_dlkm_a | `157db23ccfa516c7904d4e87614f62c3c99c2dcbb93cc3fc8ccd3388c2cc793f` | exact stock read-back PASS |
| vendor_boot_a | `3027d80a33fcc65f506d2d909db862657fe28e7eafc6e2af94193d67e6e617eb` | exact stock read-back PASS |

The system-DLKM load contract remained 82 `modules.load` entries with
`wwan.ko` at entry 28.

Recovery gates:

- device/slot guard: PASS, CPH2747/Canoe, `_a`;
- snapshot/update state: none;
- helper SHA-256:
  `84b035710c305be859aac72827489850b7d215ee372ab3341be849e051bfab50`;
- independent full host backup:
  `/home/travis/Android/oos16.0.10.500-kernel-compat/backups/b06-preflash-20260826/boot_a.img`;
- helper backup:
  `/sdcard/TWRP/kernel-flash-backups/b06-physical-20260826/controlled-stack-a-20260826-171422/boot_a.img`;
- both backup size/hash checks: PASS, 100663296 bytes and
  `eb9a17e47e680a48b6658c8c3d473a963aef7ccaa604a462508bd1cf4b0872a8`;
- decrypted writable backup destination: PASS;
- boot-only dry run: PASS, pre/post B05 hash unchanged;
- immediate complete post-write read-back: PASS.

## Boot and existing encrypted user0

| Test | Result |
|---|---|
| First Android boot | PASS, boot complete in 25 seconds |
| Second clean post-stress boot | PASS, boot complete in 24 seconds |
| Existing user0 | `RUNNING_UNLOCKED` on both boots |
| `init_user0_failed` | NOT OBSERVED |
| Rescue Party / recovery redirect | NOT OBSERVED |
| Runtime boot hash | exact candidate on both boots |

No data format, metadata wipe, FBE policy change, vendor_boot/VBMeta change, or
slot-metadata change was used.

## AF_UNIX and framework coverage

AF_UNIX was active before stress with stream, datagram, and seqpacket sockets
present in `/proc/net/unix`. A statically linked Android arm64 harness used
four workers for 300 seconds with `RLIMIT_NOFILE=2048`.

| Metric | Result |
|---|---:|
| Harness loops | 3,006,667 |
| SCM_RIGHTS messages sent | 10,523,334 |
| Messages received | 6,014,798 |
| FDs passed | 16,536,668 |
| FDs received, including peek duplicates | 18,041,466 |
| `MSG_PEEK` operations | 3,007,399 |
| Two-node cyclic graphs | 1,503,334 |
| Three-node cyclic graphs | 1,503,333 |
| Process-exit operations | 732 |
| Socket/FD closes | 54,124,397 |
| Expected errors | 0 |
| Unexpected errors | 0 |
| Integrity errors | 0 |

The harness covered AF_UNIX stream/datagram/seqpacket socketpairs,
SCM_RIGHTS passing of UNIX sockets, pipes and eventfds, `MSG_PEEK` followed by
normal receive, cyclic descriptor graphs, duplication/close, concurrent
four-worker graph churn, and child exit during descriptor transfer. It used a
fixed worker count, duration, and FD ceiling.

Post-run global file-table samples decreased from 44,677 to 44,485 over the
accepted sampling window rather than growing monotonically. Global Android FD
counts are not expected to remain constant; together with the harness's zero
integrity/error result and second clean boot, this is accepted as no
harness-attributable FD leak.

Framework stress completed:

- activity launch/close: 200/200 PASS;
- Settings force-stop/relaunch: 50/50 PASS;
- package/activity/connectivity query rounds: 50/50 PASS;
- system_server, zygote, service managers, vold, keystore, surfaceflinger,
  netd, and vendor framework: PASS, no crash loop.

High operation counts do not prove the exact concurrent GC/SCC invalidation
window. No safe tracepoint or existing counter proved that overlap; the exact
fix trigger remains **NOT OBSERVED**.

## Subsystem regression results

| Subsystem | Result |
|---|---|
| WLAN | PASS; 6135 MHz WPA3-SAE, Internet, reconnect, and second-boot recovery |
| Cellular | PASS; Visible LTE HOME/IN_SERVICE, RMNET IPv4/IPv6, IPv4/IPv6 routes, IP/DNS |
| Wi-Fi/cellular handoff | 5/5 PASS with cellular-only and returned 6135 MHz paths |
| Bluetooth | PASS; off/on recovery and existing HID reconnected |
| NFC | PASS; service on, Wallet available, HCE registered, eSE1 route present |
| Camera | PASS; Oplus camera created `IMG20260826172940.heic`, 1228353 bytes |
| Audio | PASS; ringtone playing through built-in speaker, user confirmed clear |
| Graphics/UI | PASS; normal system, Settings, Wallet, camera, and launcher rendering |
| USB | PASS; ADB enumeration remained available |
| Deep idle/resume | 5/5 PASS; user0 remained unlocked and IP connectivity returned |
| Module load contract | PASS; 82 entries, `wwan.ko` entry 28 |

## Error scan

Full recovery, boot, AF_UNIX, framework, network, subsystem, deep-idle,
partition, dmesg, and logcat evidence is retained under:

`out/oos1610500-custom-r53-b06-final/physical-20260826/`

The accepted scans found no AF_UNIX/GC/SCC corruption, use-after-free,
refcount failure, double free, invalid pointer, list corruption, Oops, BUG,
KASAN, UBSAN, panic, RCU stall, IOMMU/SMMU fault, module/CRC/vermagic/signature
failure, `init_user0_failed`, or critical framework crash loop.

The same OEM informational 60-second task snapshots for `adci_thread`,
`zram_comp`, `osml_monitor`, and `hfi_core_dbg_cl` documented on qualified
B01-B05 appeared. They are unchanged baseline observations, not AF_UNIX
failures, and no functional stall accompanied them.

## Decision

**PASS — CANDIDATE B06 AF_UNIX GC/SCC HARDENING PHYSICALLY QUALIFIED**

No rollback was required. This qualification is a compatibility/stability
result only and does not establish a cybersecurity outcome. It does not
authorize main promotion, ACK advancement, DLKM adaptation, B07, or release
publication.
