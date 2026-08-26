# Candidate B05 physical-test plan

Candidate B05 has not been flashed. A later authorized test changes `boot_a`
only; exact stock current-firmware system_dlkm, vendor_dlkm, vendor_boot,
VBMeta, and slot metadata remain untouched.

## Exact inputs and rollback

- candidate boot:
  `/home/travis/Android/oneplus15-sm8850-oos1610500-ack-r53-base/out/oos1610500-custom-r53-b05-final/boot.img`
- candidate SHA-256:
  `eb9a17e47e680a48b6658c8c3d473a963aef7ccaa604a462508bd1cf4b0872a8`
- rollback B04 boot SHA-256:
  `05785dc9ba171548790699c09aa2bbc9172062cfa3fc0f516d6472a944bd4ed3`
- stock system_dlkm:
  `18f530dcb0e46dc81ede00e18ac4e9b39faf6564fb067f04af9a912d87fd6dd7`
- stock vendor_dlkm:
  `157db23ccfa516c7904d4e87614f62c3c99c2dcbb93cc3fc8ccd3388c2cc793f`
- stock vendor_boot:
  `3027d80a33fcc65f506d2d909db862657fe28e7eafc6e2af94193d67e6e617eb`

## 1. Preflight, recovery, and write gate

Require qualified B04 Android on slot `_a`, boot complete, existing user0
`RUNNING_UNLOCKED`, expected B04 kernel, healthy WLAN/cellular/BT/NFC, and
known stock DLKM hashes. Capture user state, routes, modules, dmesg, all logcat
buffers, and live `boot_a` SHA.

In hardened TWRP, create and hash-verify a full `boot_a` backup, run the
boot-only dry run, and require device, slot, snapshot, capacity, candidate,
and backup gates to pass. Flash only `boot_a`, then require exact candidate
read-back before reboot. Do not supply any DLKM or other partition image.

## 2. Boot, user0, and standard regression gates

Perform two clean Android boots. Both must reach ADB and existing encrypted
user0 `RUNNING_UNLOCKED`, with no Rescue Party, logo stall, or
`init_user0_failed`. Verify runtime kernel
`6.12.23-android16-5-o-g515d73a3d5f4-4k`.

Require:

- WLAN: 6135 MHz WPA3-SAE, Internet, OFF/ON, and reload;
- cellular: LTE HOME/IN_SERVICE, RMNET IPv4/IPv6, routes, IP and DNS;
- Bluetooth: OFF/ON and existing HID reconnect;
- NFC: service, Wallet/HCE, and eSE1;
- camera: open and capture;
- audio: ringtone and media clarity;
- framework/UI: system_server stability and normal rendering;
- power: deep idle/wake 5/5 with WLAN/cellular recovery;
- USB: stable ADB enumeration.

## 3. USB gadget and FunctionFS coverage

Capture the initial UDC, gadget, and Android USB state from available configfs,
sysfs, properties, dumpsys, dmesg, and logcat paths. Do not fail merely because
a debug-only path is absent.

Exercise the active gadget lifecycle with bounded normal userspace operations:

1. keep ADB connected and transfer at least 100 MiB in both practical host-to-
   device and device-to-host directions;
2. perform at least 50 physical cable disconnect/reconnect or equivalent safe
   Android USB-function re-enumerations, confirming ADB returns every time;
3. switch between supported Android USB modes such as charging/ADB and MTP or
   PTP at least 20 times without forcing an unsupported function;
4. verify FunctionFS/ADB endpoints close and reopen without a stuck UDC;
5. perform USB tethering/RNDIS with a real host, including addressing, Internet,
   DNS, and at least 100 MiB sustained traffic, if equipment permits;
6. suspend/wake five times while USB is attached, then repeat with the cable
   detached and reattached;
7. exercise safe host/device role switching only if the connected accessory
   and stock UI already support it.

Do not unload vendor USB modules, remove platform devices, inject malformed
firmware events, or modify gadget/kernel instrumentation solely to trigger the
race.

## 4. Lifetime-specific observation

The ordinary transitions above cover gadget bind, unbind, endpoint teardown,
device release, and recreation. Record UDC names, function combinations,
enumeration counts, transfer results, expected transient disconnect errors,
and unexpected failures.

Only classify **EXACT FIX TRIGGER OBSERVED** if existing logs or safe existing
tracepoints prove that the decoupled gadget release overlapped the UDC removal
window relevant to `gadget_match_driver`. Otherwise use:

```text
NORMAL-RUNTIME COMPATIBILITY PASS
EXACT FIX TRIGGER NOT OBSERVED
```

High transition counts alone are not proof of the exact interleaving.

## 5. Error and service scan

Capture full dmesg and `logcat -b all` before and after stress. Scan for:

```text
usb gadget udc configfs FunctionFS ffs adb dwc3 rndis tether typec
gadget_match_driver usb_add_gadget usb_gadget_release
use-after-free UAF refcount double free invalid pointer list corruption
Oops BUG: KASAN UBSAN panic Call trace hung task RCU stall
adbd system_server netd NetworkStack UsbDeviceManager
Unknown symbol CRC MODVERSION vermagic signature protected symbol
IOMMU SMMU
```

Require no new kernel, module, USB-framework, network, or memory-safety failure.

## 6. Acceptance and rollback

PASS requires 2/2 boots, unlocked user0 twice, no `init_user0_failed`, stable
ADB/configfs gadget operation, successful bounded USB transition and transfer
testing, all standard subsystem gates, deep idle 5/5, framework stability, and
a clean error scan. RNDIS/USB tethering may be recorded as equipment-limited,
but it remains an open coverage item.

At the first critical failure, restore exact B04 boot, require read-back SHA
`05785dc9...`, and do not begin B06. B04 remains the qualified baseline.
