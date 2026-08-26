# Candidate B05 physical validation — 2026-08-26

## Result

**PASS — USB GADGET UDC LIFETIME HARDENING PHYSICALLY VALIDATED ON
OOS 16.0.10.500**

Candidate B05 cumulatively contains the physically qualified B01 Binder
lifetime hardening, B02 BPF per-CPU bounds hardening, B03 eventpoll lifetime
hardening, B04 AF_PACKET fanout lifetime hardening, and one new coherent
runtime change in `drivers/usb/gadget/udc/core.c`. The B05 change pins the UDC
device until the decoupled gadget device is released. No ACK advancement,
configuration change, DLKM adaptation, device-tree change, or other project
customization was included.

## Tested identity

| Field | Value |
|---|---|
| Device | OnePlus 15 CPH2747 / Canoe |
| Firmware | `CPH2747_16.0.10.500(EX01)` |
| Slot | `_a` |
| B04 qualified parent | `e5896aba2186c2f47cfc5d45d9d1f26cbff943eb` |
| B05 runtime source head | `515d73a3d5f436bb3b67d36ef1be44fafd22e0ae` |
| Pre-physical documentation head | `f29f136705c5d6f2415a99d5fda629e80c19c76d` |
| New runtime change | USB gadget UDC lifetime hardening |
| Runtime kernel | `6.12.23-android16-5-o-g515d73a3d5f4-4k` |

The source and machine-code proof for `usb_add_gadget()`,
`usb_del_gadget()`, `gadget_match_driver()`, and `usb_gadget_release()` is
recorded in `candidate-b05-binary-delta.md`.

## Payload isolation

Only `boot_a` was written.

| Partition | Physical SHA-256 | Result |
|---|---|---|
| boot_a | `eb9a17e47e680a48b6658c8c3d473a963aef7ccaa604a462508bd1cf4b0872a8` | candidate input, write, recovery read-back, and Android read-back PASS |
| system_dlkm_a | `18f530dcb0e46dc81ede00e18ac4e9b39faf6564fb067f04af9a912d87fd6dd7` | exact stock, unchanged |
| vendor_dlkm_a | `157db23ccfa516c7904d4e87614f62c3c99c2dcbb93cc3fc8ccd3388c2cc793f` | exact stock, unchanged |
| vendor_boot_a | `3027d80a33fcc65f506d2d909db862657fe28e7eafc6e2af94193d67e6e617eb` | exact stock, unchanged |

The current-firmware system-DLKM contract remained 82 `modules.load`
entries, with `wwan.ko` at entry 28. Neither DLKM, vendor_boot, VBMeta, nor
slot metadata was supplied to the flash operation.

## TWRP safety record

- device/slot guard: PASS, CPH2747/Canoe, `_a`
- hardened helper SHA-256:
  `84b035710c305be859aac72827489850b7d215ee372ab3341be849e051bfab50`
- independent full `boot_a` backup:
  `/home/travis/Android/oos16.0.10.500-kernel-compat/backups/b05-preflash-20260826/boot_a.img`
- source and backup size: 100663296 bytes
- source and backup SHA-256:
  `05785dc9ba171548790699c09aa2bbc9172062cfa3fc0f516d6472a944bd4ed3`
- helper backup and manifest were also pulled to the host under
  `backups/b05-preflash-20260826/helper/controlled-stack-a-20260826-131858/`
- exact B04 rollback artifact: present and hash-verified
- snapshot/update state: none
- boot-only dry run: PASS, no partition modified
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
| `vdc cryptfs init_user0`, boot 1 | status 0, 0.175 seconds |
| `vdc cryptfs init_user0`, boot 2 | status 0, 0.131 seconds |
| `init_user0_failed` | NOT OBSERVED |
| Rescue Party/TWRP redirect | NOT OBSERVED |
| Prolonged OnePlus-logo stall | NOT OBSERVED |

The current encrypted userdata was unlocked intact on both boots. No format,
metadata wipe, encryption-policy change, vendor_boot change, VBMeta change, or
slot-metadata change was used.

## USB gadget and UDC physical coverage

The active Android configfs gadget exposed FunctionFS/ADB and MTP on the Canoe
UDC. The accepted bounded tests were:

| Test | Result |
|---|---|
| Initial and final gadget state | connected, configured, `mtp,adb` |
| Safe `resetUsbGadget` re-enumerations | 50/50 PASS |
| MTP/PTP function transitions | 20/20 PASS; final `mtp,adb` |
| Host-to-device ADB transfer | 104857600 bytes PASS |
| Device-to-host ADB transfer | 104857600 bytes PASS |
| Round-trip data identity | SHA-256 `a9e17ee278e478e1865db1f05c4dbef22cb2501a56d34e6accd5cb5f2295996f`, PASS |
| USB tether interface | RNDIS, host `10.158.157.180/24`, gateway/DNS `10.158.157.105` |
| RNDIS host IP/DNS/HTTPS | PASS |
| RNDIS hardware offload | started, PASS |
| RNDIS sustained transfer | 104857600 bytes, HTTP 200, 15.112 seconds, PASS |
| USB-attached deep idle/wake | 5/5 PASS |
| Final USB mode | `mtp,adb`, connected and configured |

The cable remained attached during the bounded test. Fifty explicit Android
gadget resets supplied the plan's permitted equivalent re-enumerations; no
unsupported role switch was forced. Tethering was disabled through the stock
UI after testing, RNDIS disappeared, and normal MTP+ADB operation was restored.

The first MTP/PTP diagnostic treated `svc usb getFunctions` output as stdout
even though this build emits it on stderr and therefore recorded one harness
parse failure while the actual state was `mtp,adb`. The predicate was corrected
without changing the candidate, and a fresh accepted run completed 20/20.

The first selected 100 MiB tether source returned HTTP 403 before transfer.
That server-side source failure was not accepted as device evidence. A second
HTTPS source returned HTTP 200 and exactly 104857600 bytes through the real
RNDIS host interface.

Repeated normal function transitions produced idempotent configfs/init notices
such as already-existing symlinks, redundant UDC unbind returning `ENODEV`, and
endpoint requests already dequeued during disconnect. Every cycle returned to
a connected/configured state, transfers remained correct, and no UDC, DWC3,
memory-safety, or framework fault accompanied these messages.

The production runtime exposed no tracepoint or counter proving that the exact
UDC removal and decoupled gadget release interleaving occurred. High transition
counts alone are not proof of that window. The correct classification is:

**NORMAL-RUNTIME COMPATIBILITY PASS — EXACT FIX TRIGGER NOT OBSERVED**

## Subsystem regression results

| Subsystem | Result |
|---|---|
| WLAN | PASS; 6135 MHz WPA3-SAE, Internet, disable/enable recovery |
| Cellular | PASS; Visible LTE HOME/IN_SERVICE, RMNET IPv4/IPv6, IPv4/IPv6 routes, fail cause NONE/0x0, IP/DNS |
| Wi-Fi/cellular handoff | PASS; cellular-only and restored WLAN paths both stabilized and passed 5/5 IP/DNS |
| Bluetooth | PASS; off/on recovery and existing HID device reconnected |
| NFC | PASS; service on, HCE registered, eSE1 route present |
| Camera | PASS; Oplus camera saved `IMG20260826133347.heic` (1148995 bytes) |
| Audio | PASS; ringtone session PLAYING on built-in speaker; user confirmed clear |
| Graphics/UI | PASS; normal rendering and camera/system UI operation |
| Deep idle/resume | PASS, 5/5; CE remained available and user0 remained unlocked |
| Framework/network services | PASS; system_server, netd, adbd, NetworkStack, and ConnectivityService stable |

## Error scan

Full preflight, recovery, boot, dmesg, logcat, USB transition, RNDIS, transfer,
camera, audio, network, and deep-idle evidence is retained under:

`out/oos1610500-custom-r53-b05-final/physical-20260826/`

The accepted final scan found zero relevant match for:

- gadget/UDC lifetime fault, `gadget_match_driver()` failure, or
  `usb_add_gadget()` failure
- use-after-free, refcount failure, double-free, list corruption, Oops, BUG,
  KASAN, UBSAN, panic, RCU stall, or IOMMU/SMMU fault
- Unknown symbol, CRC/MODVERSION disagreement, invalid vermagic, signature, or
  protected-export failure
- system_server, netd, adbd, or framework crash loop
- `init_user0_failed`, fscrypt, metadata-encryption, or CE-unlock failure

The broad preliminary scan matched `binder_debug:` because the unbounded
substring `BUG:` appears inside `debug:`. The corrected word-boundary scan
reported zero kernel memory-safety/fatal errors.

The same OEM informational 60-second task snapshots for `adci_thread`,
`zram_comp`, `osml_monitor`, and `hfi_core_dbg_cl` recorded on the previously
qualified B01-B04 baselines were present early in the accepted boot. They are
not USB/UDC failures and caused no functional stall.

## Decision

**PASS — CANDIDATE B05 USB GADGET UDC LIFETIME HARDENING PHYSICALLY
VALIDATED**

No rollback was required. This qualification covers Candidate A plus
B01+B02+B03+B04+B05 only. It does not authorize ACK advancement, DLKM
adaptation, main promotion, B06, or release publication.
