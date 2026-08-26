# Candidate B04 physical-test plan

Candidate B04 is not authorized for publication or main promotion by this
plan. It changes `boot_a` only and leaves stock current-firmware DLKMs,
vendor_boot, VBMeta, and slot metadata untouched. This preparation task does
not authorize or perform the flash.

## Exact inputs and rollback

- candidate boot:
  `/home/travis/Android/oneplus15-sm8850-oos1610500-ack-r53-base/out/oos1610500-custom-r53-b04-final/boot.img`
- candidate SHA-256:
  `05785dc9ba171548790699c09aa2bbc9172062cfa3fc0f516d6472a944bd4ed3`
- rollback B03 boot SHA-256:
  `0b065aa6fc5c524107ecdf10ffefb41b464be86bfb9b8673b8fb649f5541dfc0`
- exact stock system_dlkm:
  `18f530dcb0e46dc81ede00e18ac4e9b39faf6564fb067f04af9a912d87fd6dd7`
- exact stock vendor_dlkm:
  `157db23ccfa516c7904d4e87614f62c3c99c2dcbb93cc3fc8ccd3388c2cc793f`
- exact stock vendor_boot:
  `3027d80a33fcc65f506d2d909db862657fe28e7eafc6e2af94193d67e6e617eb`

## 1. Android preflight

Require healthy B03 Android on slot `_a`, boot complete, existing user0
`RUNNING_UNLOCKED`, expected B03 kernel, 6 GHz WLAN, registered dual-stack
cellular, IP/DNS, and exact stock DLKM block hashes. Capture `dmesg`, all
logcat buffers, user state, routes, loaded modules, and boot block SHA before
recovery. Stop if any starting state is degraded or unknown.

## 2. Recovery safety and boot-only write

Use the hardened TWRP workflow with only the boot candidate supplied:

1. confirm Canoe/CPH2747, slot `_a`, and snapshot state none;
2. create a complete verified `boot_a` backup in decrypted writable storage;
3. require live source and backup SHA equality;
4. run the boot-only helper dry run;
5. require device, slot, capacity, candidate hash, and backup gates to pass;
6. flash only `boot_a`;
7. read `boot_a` back and require exact B04 SHA before reboot.

Do not supply or write system_dlkm, vendor_dlkm, vendor_boot, VBMeta, or slot
metadata.

## 3. Boot, user0, and core gates

Perform two clean Android boots. Both must reach ADB and existing encrypted
user0 `RUNNING_UNLOCKED` without Rescue Party, a logo stall, or
`init_user0_failed`. Verify the runtime kernel is
`6.12.23-android16-5-o-g2f2631b951ce-4k`.

Then require:

- WLAN: 6135 MHz WPA3-SAE, Internet, OFF/ON, and reload;
- cellular: LTE HOME/IN_SERVICE, RMNET IPv4/IPv6, routes, numeric IP, DNS,
  and Wi-Fi/cellular handoff;
- Bluetooth: OFF/ON and existing HID reconnect where available;
- NFC: service, Wallet/HCE, and eSE1;
- camera: open and capture;
- audio: ringtone and media clarity;
- framework/graphics: normal UI, task switching, and system_server stability;
- USB: ADB enumeration;
- power: deep idle/wake 5/5 with WLAN and cellular recovery.

## 4. AF_PACKET fanout workload

Use a small bounded userspace harness under root/CAP_NET_RAW; do not add kernel
instrumentation. Run for at least five minutes with at least four workers:

- create pairs or small groups of `AF_PACKET` sockets;
- bind them to the current Wi-Fi interface and apply `PACKET_FANOUT` with
  unique bounded group identifiers;
- repeatedly register, receive briefly, close, and recreate every group;
- concurrently cycle Wi-Fi OFF/ON enough times to generate at least 20
  interface-down/up transitions while the close workload is active;
- record socket creations, fanout joins, closes, expected transient network
  errors, and unexpected syscall or pthread failures;
- avoid unbounded file-descriptor use, packet flooding, or changes to kernel
  tracing and networking policy.

After each Wi-Fi recovery, verify WPA3 association and IP/DNS. Keep cellular
registered and verify handoff remains usable. A root packet socket or fanout
feature rejected by SELinux/userspace policy is a coverage limitation, not by
itself a kernel failure.

The workload stresses the close and `NETDEV_UP` paths, but it does not prove
the exact scheduler interleaving. Unless existing tracepoints or logs prove a
closing fanout socket encountered concurrent re-registration, report:

```text
NORMAL-RUNTIME COMPATIBILITY PASS
EXACT FIX TRIGGER NOT OBSERVED
```

## 5. Application and framework churn

Complete at least 100 safe application/activity open-close operations while
the fanout workload and interface cycles run. Include Settings, Wi-Fi,
Bluetooth, browser/system apps, camera, Wallet, and launcher. Verify no
system_server, zygote, servicemanager, hwservicemanager, netd, or vendor-service
crash loop.

## 6. Error scan

Capture full `dmesg` and `logcat -b all` before and after stress. Scan for:

```text
AF_PACKET packet fanout packet_release packet_notifier NETDEV_UP
use-after-free UAF refcount double free dangling pointer list corruption
Oops BUG: KASAN UBSAN panic Call trace hung task
system_server zygote servicemanager hwservicemanager netd
Unknown symbol CRC MODVERSION vermagic signature protected symbol
IOMMU SMMU
```

Require no new relevant kernel, module, networking, or framework failure.

## 7. Acceptance and rollback

PASS requires boot 2/2, user0 unlocked 2/2, no `init_user0_failed`, successful
bounded AF_PACKET/fanout normal-runtime stress or an explicitly recorded
userspace coverage limitation, stable Wi-Fi interface recovery, all core
subsystem gates, deep idle 5/5, framework stability, and a clean error scan.

At the first critical failure, restore the exact B03 boot, verify the
`0b065aa6...` read-back SHA-256, and do not begin B05. B03 remains the
qualified baseline.
