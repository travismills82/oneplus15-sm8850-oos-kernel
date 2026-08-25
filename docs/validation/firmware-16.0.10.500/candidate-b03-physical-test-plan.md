# Candidate B03 physical-test plan

Candidate B03 is not authorized for publication or main promotion by this
plan. It changes `boot_a` only and leaves the current-firmware stock DLKMs,
vendor_boot, VBMeta, and slot metadata untouched.

## Exact inputs and rollback

- candidate boot:
  `/home/travis/Android/oneplus15-sm8850-oos1610500-ack-r53-base/out/oos1610500-custom-r53-b03-final/boot.img`
- candidate SHA-256:
  `0b065aa6fc5c524107ecdf10ffefb41b464be86bfb9b8673b8fb649f5541dfc0`
- rollback B02 boot SHA-256:
  `dd63f38c658bf81b259f41f5ade970a12e8742bf1e427ed866c532e5f308cb07`
- exact stock system_dlkm:
  `18f530dcb0e46dc81ede00e18ac4e9b39faf6564fb067f04af9a912d87fd6dd7`
- exact stock vendor_dlkm:
  `157db23ccfa516c7904d4e87614f62c3c99c2dcbb93cc3fc8ccd3388c2cc793f`
- exact stock vendor_boot:
  `3027d80a33fcc65f506d2d909db862657fe28e7eafc6e2af94193d67e6e617eb`

## 1. Android preflight

Require healthy B02 Android on slot `_a`, boot complete, existing user0
`RUNNING_UNLOCKED`, expected B02 kernel, 6 GHz WLAN, registered dual-stack
cellular, IP/DNS, and exact stock DLKM block hashes. Capture `dmesg`, all logcat
buffers, user state, routes, and loaded-module state before recovery.

Stop if the starting state is degraded or any partition identity is unknown.

## 2. Recovery safety and write

Use the hardened TWRP workflow with only the boot candidate supplied:

1. confirm Canoe/CPH2747, slot `_a`, and snapshot state none;
2. create a complete verified `boot_a` backup in decrypted writable storage;
3. require source and backup SHA equality;
4. run the boot-only helper dry run;
5. require device, slot, capacity, candidate hash, and backup gates to pass;
6. flash only `boot_a`;
7. read `boot_a` back and require exact candidate SHA-256 before reboot.

Do not supply or write system_dlkm, vendor_dlkm, vendor_boot, VBMeta, or slot
metadata.

## 3. Boot and user0 gates

Perform two clean Android boots. Both must reach ADB and existing user0
`RUNNING_UNLOCKED` without Rescue Party, prolonged logo stall, or
`init_user0_failed`. Verify the kernel release is
`6.12.23-android16-5-o-gc3b68584dbb4-4k`.

## 4. Eventpoll-focused workload

Exercise the changed built-in path with normal userspace only:

- run a bounded userspace epoll churn harness for at least five minutes with
  multiple workers creating epoll, eventfd, pipe, socketpair, and timerfd
  descriptors;
- repeatedly add, modify, and delete watched descriptors;
- race safe close, dup, process exit, and epoll-instance teardown operations;
- include nested epoll registration where the kernel permits it;
- complete at least 100 application/activity open-close cycles across
  Settings, launcher, browser/system apps, camera, Wallet, Wi-Fi settings, and
  Bluetooth settings;
- run safe package-manager, activity-manager, service-manager, and network
  policy queries during the churn;
- verify framework, system_server, zygote, SurfaceFlinger, radio, and vendor
  services remain stable.

Do not add kernel instrumentation or deliberately exhaust file descriptors.
This workload stresses the fixed removal and teardown paths but does not prove
the precise vulnerable interleaving occurred. Unless existing trace/log data
proves both the file-removal and RCU-reader condition, report:

```text
NORMAL-RUNTIME COMPATIBILITY PASS
EXACT FIX TRIGGER NOT OBSERVED
```

## 5. Core regression gates

- WLAN: 6135 MHz WPA3, Internet, OFF/ON, and reload;
- cellular: LTE HOME/IN_SERVICE, RMNET IPv4/IPv6, routes, numeric IP, DNS, and
  Wi-Fi/cellular handoff;
- Bluetooth: OFF/ON and existing HID reconnect where available;
- NFC: service, Wallet/HCE, and eSE1;
- camera: open and capture;
- audio: ringtone and media clarity;
- graphics/framework: normal UI and task switching;
- USB: ADB enumeration;
- power: deep-idle and wake 5/5 with WLAN and cellular recovery.

## 6. Error scan

Capture full `dmesg` and `logcat -b all` before and after stress. Scan for:

```text
eventpoll epoll epitem epi_file ep_remove
use-after-free UAF refcount double free wrong slab list corruption
Oops BUG: KASAN UBSAN panic Call trace hung task
system_server zygote servicemanager hwservicemanager
Unknown symbol CRC MODVERSION vermagic signature protected symbol
IOMMU SMMU
```

No new fatal Binder/BPF/eventpoll, framework, module, or kernel error is
acceptable.

## 7. Acceptance and rollback

PASS requires boot 2/2, user0 unlocked 2/2, no `init_user0_failed`, eventpoll
stress completion, framework stability, all core subsystem gates, deep idle
5/5, and a clean error scan.

At the first critical failure, restore the exact B02 boot, verify the
`dd63f38c...` read-back SHA-256, and do not begin B04. B02 remains the qualified
baseline.
