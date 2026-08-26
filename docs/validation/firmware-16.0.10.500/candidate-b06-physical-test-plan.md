# Candidate B06 physical-test plan

Candidate B06 has not been flashed. A later authorized test changes `boot_a`
only; exact stock current-firmware system_dlkm, vendor_dlkm, vendor_boot,
VBMeta, and slot metadata remain untouched.

## Exact inputs and rollback

- candidate boot:
  `/home/travis/Android/oneplus15-sm8850-oos1610500-ack-r53-base/out/oos1610500-custom-r53-b06-final/boot.img`
- candidate SHA-256:
  `31e6fff0b4212916b64614c4ec96c4f88c8f8cd7168e720e5f77c05b1d402825`
- rollback B05 boot SHA-256:
  `eb9a17e47e680a48b6658c8c3d473a963aef7ccaa604a462508bd1cf4b0872a8`
- stock system_dlkm:
  `18f530dcb0e46dc81ede00e18ac4e9b39faf6564fb067f04af9a912d87fd6dd7`
- stock vendor_dlkm:
  `157db23ccfa516c7904d4e87614f62c3c99c2dcbb93cc3fc8ccd3388c2cc793f`
- stock vendor_boot:
  `3027d80a33fcc65f506d2d909db862657fe28e7eafc6e2af94193d67e6e617eb`

## 1. Preflight and boot-only write gate

Require qualified B05 Android on slot `_a`, boot complete, existing user0
`RUNNING_UNLOCKED`, expected B05 kernel, and healthy WLAN/cellular/BT/NFC.
Capture current user, network, module, dmesg, logcat, and `boot_a` identity.

In hardened TWRP, create and hash-verify a full `boot_a` backup and run the
boot-only dry run. Require device, slot, snapshot, capacity, candidate, and
backup gates to pass. Flash only `boot_a`, then require exact B06 read-back
before reboot. Do not supply a DLKM or other partition image.

## 2. Boot, user0, and standard gates

Perform two clean Android boots. Both must reach ADB and existing encrypted
user0 `RUNNING_UNLOCKED`, with no Rescue Party, logo stall, or
`init_user0_failed`. Verify runtime kernel
`6.12.23-android16-5-o-gaad7cdfe6542-4k`.

Require:

- WLAN: 6135 MHz WPA3-SAE, Internet, and OFF/ON;
- cellular: LTE HOME/IN_SERVICE, RMNET IPv4/IPv6, routes, IP and DNS;
- Bluetooth: OFF/ON and existing HID reconnect;
- NFC: service, Wallet/HCE, and eSE1;
- camera: open and capture;
- audio: ringtone and media clarity;
- framework/UI: system_server stability and normal rendering;
- USB: ADB enumeration;
- deep idle/wake: 5/5 with WLAN/cellular recovery.

## 3. Bounded AF_UNIX/SCM_RIGHTS stress

Use a reviewed fixed-size userspace harness under `/data/local/tmp` or an
equivalent temporary location. Do not add kernel instrumentation. Recommended
bounds are four workers for 300 seconds, `RLIMIT_NOFILE` no higher than 2,048
per worker, and a fixed memory ceiling low enough to avoid device pressure.

Each worker should repeatedly and account for:

1. AF_UNIX `SOCK_STREAM` socketpairs;
2. AF_UNIX `SOCK_DGRAM` and `SOCK_SEQPACKET` pairs where supported;
3. SCM_RIGHTS passing of duplicated eventfd, pipe, regular-file, and socket FDs;
4. `recvmsg(MSG_PEEK)` followed by an ordinary `recvmsg` of the same control
   data;
5. bounded self-cycles and two-/three-node descriptor graphs;
6. concurrent `sendmsg`, `recvmsg`, `dup`, close, and socket teardown;
7. bounded child-process creation/exit while descriptors are in flight;
8. normal GC pressure from repeated cycle teardown, without exhausting file
   tables or memory.

Record workers, duration, socketpairs, sendmsg/recvmsg calls, MSG_PEEK calls,
SCM_RIGHTS FDs sent/received, cyclic graphs, closes, child exits, expected
errnos, unexpected errnos, and data/FD-integrity failures. Expected errors due
to deliberate peer teardown may include `EPIPE`, `ECONNRESET`, `ENOTCONN`,
`EBADF`, or `EAGAIN` when the harness classifies them by operation and timing.
Unexpected errno classes, leaked workers, FD identity mismatch, framework
crash, or kernel fault require investigation.

## 4. Android framework and service churn

While or after the harness runs, perform at least 100 safe activity/app
launch-close operations and 50 non-destructive service/package queries. Do not
force-stop system_server, zygote, vold, keystore, servicemanager,
surfaceflinger, netd, or radio services. Verify no crash storm in system_server,
zygote, servicemanager/hwservicemanager, init, logd, adbd, vold, netd,
surfaceflinger, or vendor HAL services.

## 5. Coverage classification and error scan

Capture full dmesg and all logcat buffers. Scan for:

```text
AF_UNIX unix_gc unix_peek_fpl unix_scc_dead SCM_RIGHTS MSG_PEEK
use-after-free UAF refcount double free invalid pointer list corruption slab
Oops BUG: KASAN UBSAN panic Call trace hung task RCU stall
system_server zygote servicemanager hwservicemanager tombstoned crash loop
Unknown symbol CRC MODVERSION vermagic signature protected symbol IOMMU SMMU
```

Only report `EXACT FIX TRIGGER OBSERVED` if deterministic harness evidence or
an existing safe trace proves the SCC-reuse or concurrent GC/MSG_PEEK/close
interleaving. Otherwise a successful run must say:

```text
NORMAL-RUNTIME COMPATIBILITY PASS
EXACT FIX TRIGGER NOT OBSERVED
```

## 6. Acceptance and rollback

PASS requires 2/2 boots, unlocked user0 twice, no `init_user0_failed`, bounded
AF_UNIX/SCM_RIGHTS stress with zero unexpected failures, framework stability,
all standard subsystem gates, deep idle 5/5, and a clean kernel/module scan.

At the first critical regression, restore exact B05 boot and require read-back
SHA `eb9a17e4...`. Do not begin B07. B05 remains the qualified baseline.
