# NXP NFC .102 static validation

## Result

**PASS — STATICALLY VALIDATED; PHYSICAL NFC TEST NOT PERFORMED**

The candidate is a vendor-DLKM-only delta from the exact physically qualified
WLAN053 image. Only `nxp_nci` changes. Boot, system-DLKM, vendor boot, VBMeta,
WLAN053, Bluetooth vendor `.031`, and the 27-module stock cellular closure are
unchanged.

| Gate | Result |
| --- | --- |
| Kernel contract guard | PASS |
| Kernel release | exact g6744 contract |
| Image functional delta | 0 |
| `.config` delta | 0 |
| `Module.symvers` / exported CRC delta | 0 |
| `canoe_perf_dist` | PASS |
| ABI | PASS, report empty |
| KMI | PASS |
| Vendor modules | 436 |
| Changed vendor modules | `nxp_nci` only |
| NXP imports | 81/81 matched |
| NXP exports | 0, unchanged |
| Unresolved imports | 0 |
| CRC mismatches | 0 |
| Protected-export closure | one source replacement; no retained re-signing and no external signed-provider edges |
| Signature failures | 0 |
| DT contract | PASS |
| Firmware contract | PASS |
| HAL contract | PASS |
| Exact stock cellular modules | 27/27 |
| System modules.load | 46 entries, `wwan.ko` entry 21 |
| ext4 `e2fsck -fn` | PASS |
| Partition-local AVB hashtree/FEC/footer | PASS |

The staged candidate is 143,986,688 bytes and has SHA-256:

```text
6b6eb5eef18f3a30df5a063154a812baac68eded5dd2e3850f7d72133f559904
```

It is retained locally at:

```text
out/controlled-v1-wlan053-nfc102-static/vendor_dlkm-nfc102.img
```

No device write was performed. A later qualification must start from an exact
WLAN053 vendor-DLKM read-back, use the hardened TWRP helper in dry-run mode,
back up and verify vendor-DLKM, and flash only this vendor image after explicit
authorization.
