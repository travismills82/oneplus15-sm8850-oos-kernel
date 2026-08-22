# Canoe WLAN/CNSS .053 extended qualification

## Result

`PARTIAL — CORE PASS, OPTIONAL ENVIRONMENT TESTS REMAIN`

The controlled-v1 minimal WLAN `.053` stack passed all available band,
connectivity, hotspot, cellular-coexistence, transition-stress, reboot, and
screen-off tests. WPA2, natural roaming, an active P2P peer, active Bluetooth
audio, and MLO/802.11be could not be exercised with the available environment;
they are recorded as not tested rather than inferred from other results.

No partition was written during this extended qualification. The running
payloads were hashed before qualification and again after all stress tests.

## Qualified payload contract

| Item | Qualified value |
| --- | --- |
| Device | OnePlus 15 / CPH2747 / Canoe |
| Firmware | OxygenOS 16.0.9.400(EX01) |
| Slot | `_b` (`_a` remains unbootable and is not a fallback) |
| Kernel release | `6.12.23-android16-5-o-g6744a3f6bcf4-4k` |
| `boot_b` SHA-256 | `84a85b9a40adc7bdb178bf86e2da74e9bac3feaf7bd502c7d29b1df27dea55ab` |
| `system_dlkm_b` payload SHA-256 | `de77afb66f47f3430f1a5bcc8a79297792141652f8a256e17b4c715f49222cef` |
| `vendor_dlkm_b` SHA-256 | `8f2ae729e404c96d2ffdada4a698bd70821a9f8d82bca379e8a0924a9bd0655e` |
| `vendor_boot` | stock, unchanged |
| VBMeta | stock, unchanged |
| Source-built replacements | `cfg80211.ko`, `mac80211.ko`, `qca_cld3_peach_v2.ko` |
| CNSS platform/services | exact stock `.037` binaries |
| Cellular closure | all 27 IPA/GSI/RMNET/data modules exact stock |
| `system_dlkm` load contract | 46 entries, `wwan.ko` entry 21, zero stale or missing entries |

The `system_dlkm` hash covers the exact 45,658,112-byte EROFS payload written
at the start of the larger logical partition. The boot and vendor-DLKM hashes
cover their full partition-sized images.

## Association and security matrix

| Test | Result | Evidence |
| --- | --- | --- |
| 2.4 GHz association | PASS | `Mills`, BSSID `24:2f:d0:e2:a7:55`, 2457 MHz/channel 10, 11ax, approximately -27 to -32 dBm, 192.168.3.22/24, gateway 192.168.3.1, DNS 8.8.8.8/1.1.1.1 |
| 2.4 GHz traffic | PASS | gateway, `1.1.1.1`, and DNS 5/5; exact 50,000,000-byte HTTPS object completed; OFF/ON, screen-off, and airplane recovery passed |
| 5 GHz association | PASS | `Mills`, BSSID `24:2f:d0:e2:a7:56`, 5220 MHz/channel 44, 160 MHz, 11ax, approximately -38 to -42 dBm |
| 5 GHz traffic | PASS | gateway, `1.1.1.1`, and DNS 5/5; exact 50,000,000-byte HTTPS object completed; OFF/ON and 45-second screen-off recovery passed |
| 6 GHz association | PASS | `Mills`, BSSID `06:2f:d0:e2:a7:58`, 6135 MHz/channel 37, 160 MHz, 11ax, approximately -54 to -60 dBm |
| WPA3 | PASS | SAE shown by Android on 2.4, 5, and 6 GHz; final repeat used 6 GHz/6135 MHz |
| WPA2 | NOT TESTED — ENVIRONMENT UNAVAILABLE | the only usable known AP was a WPA2/WPA3 transition BSS; Android selected SAE even after recreating the saved profile as WPA2, so no WPA2 result is claimed |
| Natural roaming | NOT TESTED — ENVIRONMENT UNAVAILABLE | multiple same-SSID BSSIDs were visible, but the stationary cabled test environment could not produce a controlled coverage transition |
| MLO / Wi-Fi 7 | NOT TESTED — SUITABLE AP UNAVAILABLE | Android reported 11ax and no AP MLD address or affiliated links |

The temporary BSSID pins used for explicit band qualification were removed at
the end. The saved `Mills` profile was restored with `BSSID: null` and remained
connected normally.

## Hotspot / SoftAP

| Test | Result | Evidence |
| --- | --- | --- |
| Real client | PASS | the workstation's independent Wi-Fi adapter associated and received 10.242.36.158/24, gateway/DNS 10.242.36.62 |
| 2.4 GHz SoftAP | PASS | 2412 MHz, 11ax; gateway and Internet passed; a 50,000,000-byte ranged HTTPS transfer completed in 31.53 seconds |
| Hotspot disable/re-enable | PASS | the workstation disconnected, hotspot stopped, hotspot restarted with a new BSSID, and the client reassociated with DHCP and Internet |
| 5 GHz SoftAP | PASS | with STA disabled, the configured 5 GHz hotspot operated at 5745 MHz/80 MHz/11ax; gateway and Internet passed; 50,000,000 bytes completed in 38.36 seconds |
| Return to STA | PASS | the host adapter was disconnected, hotspot disabled, and the phone returned to normal 6135 MHz client operation |

The 2.4 GHz SoftAP result occurred while the phone concurrently retained its
6 GHz STA link. The 5 GHz SoftAP result used cellular upstream so the configured
5 GHz band was not constrained by concurrent STA operation.

## Coexistence and optional peer tests

| Test | Result | Evidence |
| --- | --- | --- |
| Cellular coexistence | PASS | Visible LTE remained HOME/IN_SERVICE and `rmnet_data2` remained provisioned while Wi-Fi was active |
| Cellular fallback | PASS | with Wi-Fi disabled, RMNET supplied IPv4 and IPv6 default routes; IPv4, IPv6, and DNS probes passed |
| Airplane recovery | PASS | 10/10 cycles restored the 6135 MHz Wi-Fi link and dual-stack RMNET |
| Bluetooth basic coexistence | PASS | carried forward from primary physical validation; Bluetooth remained ON while Wi-Fi traffic passed |
| Bluetooth audio coexistence | NOT TESTED — ENVIRONMENT UNAVAILABLE | five devices were bonded, but no A2DP device was connected or available; `mA2dpActiveDevice=null` |
| Wi-Fi Direct/P2P | NOT TESTED — ENVIRONMENT UNAVAILABLE | no ready peer was available; interface capability alone is not counted as a physical P2P test |

## Stress and power qualification

| Test | Result | Evidence |
| --- | --- | --- |
| Wi-Fi OFF/ON x25 | PASS | 25/25 cycles rejoined 6135 MHz and passed an immediate public-IP probe |
| Airplane mode x10 | PASS | 10/10 cycles restored Wi-Fi and dual-stack RMNET |
| Screen-off/deep-idle x10 | PASS | 10/10 forced deep-idle transitions restored Wi-Fi, RMNET, and live traffic |
| Extended screen-off | PASS | five-minute forced deep-idle hold returned to the same 6135 MHz association; cellular remained dual-stack and traffic passed immediately |
| Sustained client traffic | PASS | uninterrupted four-minute HTTPS stream received 513,572,606 bytes before the deliberate 240-second ceiling; post-stream traffic passed |
| Wi-Fi-enabled reboots | PASS | three fresh boots completed; each rejoined 6135 MHz WPA3, restored Visible dual-stack RMNET, and passed traffic |
| Wi-Fi-disabled reboot | PASS | Android booted with Wi-Fi disabled; the modem attached after `boot_completed`, then Visible HOME/IN_SERVICE, cellular IPv4/IPv6, and DNS passed; Wi-Fi was subsequently restored |

The first attempted long-run endpoint produced HTTP 429 responses after 28
successful 50 MB objects. Those were server rate limits, not network errors,
and the run was replaced by the single four-minute OVH transfer above.

## Error and regression scan

The final boot captured 18,116 dmesg lines and 22,276 all-buffer logcat lines.
The effective scan found:

| Condition | New relevant findings |
| --- | ---: |
| Unknown symbol | 0 |
| MODVERSION/CRC disagreement | 0 |
| vermagic failure | 0 |
| module-signature failure | 0 |
| protected-export failure | 0 |
| Oops / kernel BUG | 0 |
| KASAN / UBSAN | 0 |
| panic | 0 |
| WLAN `fatal_event` nonzero | 0 |
| CNSS crash/assert/SSR | 0 |
| firmware crash | 0 |
| SMMU/IOMMU fault | 0 |

Two text matches for `BUG:` were `FBEDebug:` userspace-init messages, not
kernel BUG reports. The duplicate `/proc/bcl_stat` warning and Oplus hung-task
diagnostic traces are also present in earlier controlled-v1/stock-DLKM
captures and are unrelated to WLAN `.053`.

During forced-idle stress, qcacld sometimes logged `FW not ready to WOW reason
code: 0` before retrying. The identical message exists in the preflash TEST3
and earlier controlled-v1/r7 captures. Each sequence subsequently reported
`drv wow is enabled`; all fatal-event counters stayed zero, association and
traffic survived, and no CNSS restart occurred. This is therefore recorded as
a known baseline behavior, not a new `.053` regression.

## Evidence retention

Raw device evidence remains outside git at:

`/home/travis/Android/oneplus15-wlan053-live-captures/20260822T015901Z-wlan053-extended/`

It contains the start/end payload hashes, per-band association and traffic
records, 25 Wi-Fi cycles, 10 airplane cycles, 10 forced-idle cycles, four
reboot records, hotspot client results, full final dmesg/logcat, and filtered
error/CNSS reports. It contains no release payload and must not be packaged.

## Release decision

The exact three-payload contract is the canonical controlled-v1 WLAN `.053`
baseline for further development. Core WLAN, hotspot, suspend, stress, and
cellular safety gates passed. A public release is still not authorized by this
record, and the optional WPA2, natural-roaming, P2P, active-A2DP, and MLO tests
remain explicitly open pending suitable equipment.
