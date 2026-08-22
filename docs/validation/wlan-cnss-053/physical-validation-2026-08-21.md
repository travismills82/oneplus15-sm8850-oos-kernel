# Canoe WLAN/CNSS .053 physical validation

## Result

`PASS — WLAN/CNSS .053 VALIDATED ON CONTROLLED-V1`

The result applies to the minimal controlled-v1 delivery tested on slot B. It
does not validate the rejected full-CNSS replacement image and does not change
the exact-stock 27-module IPA/GSI/RMNET/data closure.

## Candidate and rollback identities

| Payload | Candidate SHA-256 | TEST3 rollback SHA-256 |
| --- | --- | --- |
| `boot.img` | `84a85b9a40adc7bdb178bf86e2da74e9bac3feaf7bd502c7d29b1df27dea55ab` | `25efe5463938757339dcfada56ee47d77d3c0cc42b6707dda7dd1613c20fc313` |
| `system_dlkm.img` | `de77afb66f47f3430f1a5bcc8a79297792141652f8a256e17b4c715f49222cef` | `edebc94818e6fa4e214d58fd82fe46f6c513fc9850b3e7b77caf076a12270f05` |
| `vendor_dlkm-wlan053.img` | `8f2ae729e404c96d2ffdada4a698bd70821a9f8d82bca379e8a0924a9bd0655e` | `24e66015a3e4ea3583f895d529008d8c7c3706d7bc506ef550df936935127b80` |

`vendor_boot` and VBMeta remained stock and were not written. Slot A was
already unbootable and was not treated as a fallback.

Before the write, full slot-B backups were pulled and verified under:

```text
/home/travis/Android/oneplus15-wlan053-live-captures/
  20260822T012738Z-preflash-test3/
```

The hardened TWRP helper at commit
`3f499bfd1f7152ea27b27935be22ff73581709a1` also created and verified the
on-device backup:

```text
/sdcard/TWRP/kernel-flash-backups/controlled-stack-b-20260821-203025/
```

TWRP reported user 0 decrypted and a writable durable backup destination. Its
dry run validated device, firmware, slot, snapshot state, image formats,
capacities, and payload hashes. The write order was `vendor_dlkm`,
`system_dlkm`, then `boot` last. Helper read-back and a second independent
recovery read-back matched every candidate hash.

## Running contract

Both boots reported:

```text
6.12.23-android16-5-o-g6744a3f6bcf4-4k
```

The live files matched the packaged module payloads:

| Module | Delivery | Live SHA-256 |
| --- | --- | --- |
| `cfg80211.ko` | controlled `.053` | `a258ad91a47ae2be87681c4301d5196b643fdd3428945f38caa0e06fc9e1b017` |
| `mac80211.ko` | controlled `.053` | `f1f863407cc1a11569b99a685a839a60947abf6fce9f2e81b95c84c20b9e148e` |
| `qca_cld3_peach_v2.ko` | controlled `.053` | `79c342233e9e616b66e43b28f676248c8bdb164b36e3ded56dea898a900bacbb` |
| `cnss2.ko` | exact stock | `422b6c1fc183aebb54b76c6bef801118cf489306702f5cd8f9390d9d6d5e4ea1` |
| `wlan_firmware_service.ko` | exact stock | `215fde4a776d7c2707c4e7d969f06b7d6768ab12c188788ccd4a5e6468e2689d` |
| `ipam.ko` | exact stock | `b73489c5e64cc5ab46a699d5b9a186670f64b357d78223c6ea2d4bff7ae0d274` |
| `rmnet_mem.ko` | exact stock | `15fecf07714c51b9ddb841dfe6b76d76731bee0c47919bbe4f3e9375ede4c92c` |
| `wwan.ko` | controlled system DLKM | `6ec17f24bc746222add2f82c6a9208b333ed6d411cfe202c40dcc95e3a4b182e` |

The system-DLKM load contract remained at 46 entries with `wwan.ko` present.
All 27 cellular data-plane module hashes remained exact stock.

## Physical matrix

| Test | Result | Evidence |
| --- | --- | --- |
| Android boot 1 | PASS | boot completed on slot B with the `.053` release |
| Android boot 2 | PASS | clean reboot completed with the same release |
| Wi-Fi initial association | PASS | `Mills`, WPA3-SAE, 6135 MHz, 11ax |
| Wi-Fi reload | PASS | 10/10 off/on cycles reconnected at 6135 MHz |
| Wi-Fi reachability | PASS | gateway and public IPv4 probes had 0% loss |
| Sustained transfer | PASS | 50,000,000-byte HTTPS download completed |
| Cellular registration | PASS | Visible LTE, voice and data `HOME`/`IN_SERVICE` |
| RMNET | PASS | `rmnet_data2` obtained IPv4 and IPv6 on both boots |
| Cellular IPv4 | PASS | `1.1.1.1` 5/5 with Wi-Fi disabled on both boots |
| Cellular DNS | PASS | `google.com` 5/5 with Wi-Fi disabled on both boots |
| Cellular IPv6 | PASS | Cloudflare IPv6 5/5 through `rmnet_data2` on both boots |
| WLAN/cellular coexistence | PASS | both remained configured and Android-validated |
| Airplane mode recovery | PASS | Wi-Fi and Visible LTE recovered; post-cycle cellular probe 5/5 |
| Bluetooth coexistence | PASS (basic) | Bluetooth stayed ON; Wi-Fi traffic had 0% loss |
| Screen-off/doze retention | PASS | WPA3 association and validated RMNET survived doze/wake |
| AP interface | PASS (capability) | Android reported a new AP interface could be created |
| P2P interface | PASS (capability) | Android reported a new DIRECT interface could be created |

The effective final scan found zero unknown symbols, MODVERSION/CRC or
vermagic disagreement, module-signature or protected-export failure, Oops,
BUG, KASAN, UBSAN, panic, WLAN crash/SSR, or `OEM_DCFAILCAUSE` event.

The TME patch request, missing optional `cnss_softsku_peach.pfm`, CNSS cooling
node, IPA HOLB, regulatory-reference, and direct-firmware-fallback messages
also occur in the previously captured known-good TEST3/r7 logs. They are not a
new `.053` divergence; firmware subsequently loads through the shipping
userspace path and Wi-Fi operates normally.

## Coverage still requiring external control

The available saved network selected 6 GHz on every association. The test did
not pin association to the visible 2.4 or 5 GHz BSSIDs because doing so would
require AP control or rewriting the saved credential/BSSID policy. WPA2, MLO,
Wi-Fi 7, roaming between APs, a real hotspot client, an active P2P peer, and
active A2DP traffic were therefore not claimed as physically exercised.

These are extended release-qualification items, not failures in the tested
minimal `.053` contract. No release or main-branch merge is authorized by this
validation record.

## Raw captures

```text
/home/travis/Android/oneplus15-wlan053-live-captures/
  20260822T013119Z-wlan053-boot1/
  20260822T013811Z-wlan053-boot2/
```
