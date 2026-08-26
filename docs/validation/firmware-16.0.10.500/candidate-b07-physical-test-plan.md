# Candidate B07 physical-test plan

Candidate B07 has not been flashed. A later test changes `boot_a` only; exact
stock current-firmware system_dlkm, vendor_dlkm, vendor_boot, VBMeta and slot
metadata remain untouched.

## Exact inputs and rollback

- candidate boot:
  `/home/travis/Android/oneplus15-sm8850-oos1610500-ack-r53-base/out/oos1610500-custom-r53-b07-final/boot.img`
- candidate SHA-256:
  `1ef69e85dce34a1aae60d531da327a6ce96ddaeabc097c56f2ee2fb3f486b5c1`
- rollback B06 boot SHA-256:
  `31e6fff0b4212916b64614c4ec96c4f88c8f8cd7168e720e5f77c05b1d402825`
- stock system_dlkm:
  `18f530dcb0e46dc81ede00e18ac4e9b39faf6564fb067f04af9a912d87fd6dd7`
- stock vendor_dlkm:
  `157db23ccfa516c7904d4e87614f62c3c99c2dcbb93cc3fc8ccd3388c2cc793f`
- stock vendor_boot:
  `3027d80a33fcc65f506d2d909db862657fe28e7eafc6e2af94193d67e6e617eb`

## 1. Preflight and boot-only write gate

Require qualified B06 Android on slot `_a`, boot complete, existing user0
`RUNNING_UNLOCKED`, expected B06 kernel and healthy WLAN/cellular/BT/NFC.
Capture current user, IPv6 multicast, network, module, dmesg, logcat and
`boot_a` identity.

In hardened TWRP, create and hash-verify a full `boot_a` backup and run the
boot-only dry run. Require device, slot, snapshot, capacity, candidate and
backup gates to pass. Flash only `boot_a`, then require exact B07 read-back
before reboot. Do not supply a DLKM or other partition image.

## 2. Boot, user0, and standard gates

Perform two clean Android boots. Both must reach ADB and existing encrypted
user0 `RUNNING_UNLOCKED`, with no Rescue Party, logo stall or
`init_user0_failed`. Verify runtime kernel
`6.12.23-android16-5-o-g969639e8ca81-4k`.

Require:

- WLAN: 6135 MHz WPA3-SAE, Internet and OFF/ON;
- cellular: LTE HOME/IN_SERVICE, RMNET IPv4/IPv6, routes, IP and DNS;
- Wi-Fi/cellular handoff: 5/5 with IPv6 recovery;
- Bluetooth: OFF/ON and existing HID reconnect;
- NFC: service, Wallet/HCE and eSE1;
- camera: open and capture;
- audio: ringtone and media clarity;
- framework/UI: system_server stability and normal rendering;
- USB: ADB enumeration;
- deep idle/wake: 5/5 with WLAN/cellular recovery.

## 3. Bounded IPv6 multicast coverage

Before stress, record `/proc/net/igmp6`, `ip -6 maddr`, IPv6 addresses and
routes for `wlan0` and active `rmnet_dataN` interfaces. Use a reviewed bounded
userspace harness or available Android tools to:

1. join and leave several non-sensitive IPv6 multicast groups on `wlan0`;
2. send and receive bounded local multicast traffic where an available peer
   permits it;
3. run at least 10 join/leave rounds with exact errno accounting;
4. repeat joins around Wi-Fi OFF/ON for 10 cycles;
5. verify membership tables, IPv6 address, route and Internet recovery after
   each transition;
6. retain cellular registration and dual-stack service throughout;
7. perform 5 Wi-Fi/cellular handoffs and verify IPv6 after stabilization;
8. perform deep idle/wake 5/5 with memberships and normal connectivity
   returning.

Do not inject malformed carrier traffic, manipulate modem-control interfaces,
exhaust memory, or force skb reallocation merely to claim exact trigger
coverage. Normal `ENETDOWN`, `ENODEV`, `EADDRNOTAVAIL` or transient send
failure during an intentional interface-down window may be classified as
expected only when the interface recovers cleanly.

## 4. Network and error scan

Capture full dmesg and all logcat buffers. Scan for:

```text
MLD igmp6 mld_query_work IPv6 multicast pskb skb
use-after-free UAF refcount double free invalid pointer list corruption slab
Oops BUG: KASAN UBSAN panic Call trace hung task RCU stall
WLAN RMNET IPA GSI netd ConnectivityService NetworkStack system_server
Unknown symbol CRC MODVERSION vermagic signature protected symbol IOMMU SMMU
```

Require no relevant kernel fault, persistent multicast failure, interface
wedge, route loss, framework crash storm, or module-contract failure.

## 5. Coverage classification and rollback

Only report `EXACT FIX TRIGGER OBSERVED` if safe runtime evidence proves a
received MLD query, a later pull that relocated the skb head, and continued
use of the preserved group address. High join/leave counts alone are not that
proof. Otherwise a successful run must state:

```text
NORMAL-RUNTIME COMPATIBILITY PASS
EXACT FIX TRIGGER NOT OBSERVED
```

At the first critical regression, restore exact B06 boot and require read-back
SHA `31e6fff0...`. Do not begin B08. B06 remains the qualified baseline.
