# Controlled-v1 physical validation — 2026-08-21

## Candidate

The `controlled-v1` generation was staged to slot `_b` with the following
payloads:

| Payload | SHA-256 |
| --- | --- |
| `boot.img` | `25efe5463938757339dcfada56ee47d77d3c0cc42b6707dda7dd1613c20fc313` |
| `system_dlkm.img` | `56bc9699222b3708c2e08a7d246f105fba300d548a2729bb19aafad61b5fcb4b` |
| `vendor_dlkm.img` | `f90d73b8e13629ba101826cdd3406573f07edb63f35c217b0f56357453ab6dd1` |

The lossless stock `vendor_boot_b` remained byte-identical at
`5fe60f58ebe3f935acb3ec41585fa16977804cc0c2efd4e84c44a645a1eb7162`.
No VBMeta partition was modified.

The recovery helper completed its device, snapshot, image-format, capacity,
and hash validation. It wrote `vendor_dlkm_b`, `system_dlkm_b`, then `boot_b`,
and the read-back prefix of every payload matched its input SHA-256.

## Boot and module contract

Android booted the new release:

```text
6.12.23-android16-5-o-g090459863b8c-4k
```

The project signing certificate was present in the running keyring. The
critical controlled modules (`cfg80211`, `mac80211`, Peach-v2, `cnss2`,
`wlan_firmware_service`, `ipam`, `rmnet_mem`, `smem-mailbox`, and `wonder`)
matched the packaged contract. No relevant unknown-symbol, vermagic, module
signature, CRC, or protected-export error was observed.

Wi-Fi connected at 6135 MHz using WPA3-SAE, passed a five-packet reachability
test with zero loss, and reconnected after an off/on cycle. Android NFC reached
the on state.

## Cellular failure

With Wi-Fi disabled and mobile data enabled, the controlled-v1 stack did not
establish a usable cellular default network. The radio log recorded repeated
APN setup failure (`OEM_DCFAILCAUSE_4`, `0x1004`); the RMNET data path did not
obtain a usable IP/default route and both IP and DNS reachability failed.

Because the candidate replaces and re-signs the IPA/RMNET dependency closure,
this was treated as a release-blocking failure rather than attributed to the
carrier without an A/B comparison.

## A/B rollback result

Slot `_b` was restored to the verified r7 boot plus exact stock
OxygenOS 16.0.9.400 `system_dlkm` and `vendor_dlkm` images:

| Payload | SHA-256 |
| --- | --- |
| r7 `boot.img` | `86eba62f4f93f02aaacda89ef903c91a3e531575aba1cf253ce70e0887b38d1f` |
| stock `system_dlkm.img` | `18f530dcb0e46dc81ede00e18ac4e9b39faf6564fb067f04af9a912d87fd6dd7` |
| stock `vendor_dlkm.img` | `40d4bd03e9d315aac562234019f6db192617bb1ab65532157b81022ebc7330e6` |

The restored kernel booted as
`6.12.23-android16-5-o-gbd70777d3d2c-4k`. Under the same test procedure,
LTE registered home on Visible, `rmnet_data3` received IPv4 and IPv6
addresses plus a default route, and five IP and five DNS pings both completed
with zero packet loss. Wi-Fi was re-enabled after the comparison.

This A/B result attributes the failure to the controlled-v1 IPA/RMNET module
closure or its delivery contract. The controlled-v1 artifacts must not be
released or reflashed unchanged.

## Recovery backup finding

The initial recovery backup was written under data-media while
`twrp.user.0.decrypt=0`. Although it verified in that recovery session, the
files became unreadable after recovery restarted because the credential-
encrypted key was unavailable. No restore write was attempted from those
files. The fallback used independently verified production images instead.

The TWRP helper must reject `/sdcard` and `/data/media` backup destinations
unless user 0 decryption is active. This is an installer safety requirement,
not a kernel failure.

## Result

```text
FAIL — controlled-v1 cellular IPA/RMNET regression; r7 stock-DLKM baseline restored
```
