# NXP NFC .102 physical validation — 2026-08-22

## Result

**PARTIAL — CORE PASS, TAG AND PAYMENT EQUIPMENT TESTS REMAIN**

The vendor-DLKM-only `.102` candidate booted three times and passed the
controller, HAL, framework, power-state, WLAN, Bluetooth, and cellular
regression gates exercised below. No NFC tag or payment terminal was presented
during this session, so tag I/O and a real payment transaction remain explicitly
unqualified.

## Exact payload contract

| Payload | Physical identity |
| --- | --- |
| Kernel release | `6.12.23-android16-5-o-g6744a3f6bcf4-4k` |
| Boot | qualified WLAN053 image, SHA-256 `84a85b9a40adc7bdb178bf86e2da74e9bac3feaf7bd502c7d29b1df27dea55ab` |
| System-DLKM | qualified WLAN053 image, SHA-256 `de77afb66f47f3430f1a5bcc8a79297792141652f8a256e17b4c715f49222cef` |
| Vendor-DLKM candidate | SHA-256 `6b6eb5eef18f3a30df5a063154a812baac68eded5dd2e3850f7d72133f559904` |
| `nxp-nci.ko` | SHA-256 `51ef28ae123a7b2c0fd851491e1a13abfbd19b3b4a9a66acf3e4b997096ca9c2` |
| Vendor boot | stock, unchanged |
| VBMeta | stock, unchanged |
| Slot | `_b` |

Only `nxp-nci.ko` differs from the qualified WLAN053 vendor-DLKM. The retained
Bluetooth vendor generation is `.031`; WLAN remains `.053`; the exact stock
27-module IPA/GSI/RMNET cellular closure is unchanged.

## Flash and recovery proof

The hardened controlled-stack helper from TWRP commit
`3f499bfd1f7152ea27b27935be22ff73581709a1` was used after successful user-0
decryption. A vendor-DLKM-only dry run passed before the write. Boot,
system-DLKM, vendor boot, and VBMeta were not written.

The helper backed up the exact qualified WLAN053 vendor-DLKM at:

```text
/sdcard/TWRP/kernel-flash-backups/controlled-stack-b-20260822-104722/
```

The backup is 143,986,688 bytes, hashes to
`8f2ae729e404c96d2ffdada4a698bd70821a9f8d82bca379e8a0924a9bd0655e`,
and passed `sha256sum -c SHA256SUMS`. Candidate read-back matched
`6b6eb5eef18f3a30df5a063154a812baac68eded5dd2e3850f7d72133f559904`
before the first Android boot.

Slot `_a` remains unavailable as a fallback. The verified TWRP backup and the
qualified rollback image remain the recovery path.

## Controller and runtime contract

Physical runtime evidence confirms:

- `nxp_nci` loaded and bound to the active `qcom,sn-nci` SN220 path;
- the loaded vendor-DLKM module has the expected `.102` SHA-256;
- `nfc_hal_service`, `oplusnfc_aidl_hal_service`, and `nfcExtnsService`
  remained running;
- Android NFC returned to `mState=on` after every enabled-state operation;
- Google Pay HCE service and its payment AIDs were registered; and
- the Android Digital Car Key off-host service remained routed to `eSE1`.

There were no observed module loader, CRC, vermagic, signature, protected
export, HAL crash, or framework crash failures.

## Physical test matrix

| Test | Result | Evidence / limitation |
| --- | --- | --- |
| Initial Android boot, NFC enabled | PASS | Android completed boot; controller, HALs, and framework healthy |
| Second clean boot, NFC enabled | PASS | `nxp_nci` loaded; HALs running; framework returned to `mState=on` |
| Clean boot, NFC disabled | PASS | persisted as `mState=off`; controller remained stable |
| Re-enable after disabled boot | PASS | immediate recovery to `mState=on` |
| NFC OFF/ON x25 | PASS | every cycle reached `off`, then `on` |
| Airplane mode x5 | PASS | NFC remained healthy; WLAN and cellular recovered each cycle |
| Screen-off/wake x10 | PASS | NFC remained on and controller stayed loaded |
| Tag read | NOT TESTED — EQUIPMENT UNAVAILABLE | no physical tag event was presented during two observation windows |
| Tag write/read-back | NOT TESTED — EQUIPMENT UNAVAILABLE | no writable tag was available |
| Wallet availability | PASS | Google Pay HCE payment service and AIDs registered in `dumpsys nfc` |
| Contactless payment | NOT TESTED — ENVIRONMENT UNAVAILABLE | no payment terminal transaction was attempted |
| eSE framework routing | PASS | off-host `eSE1` digital-car-key route remained registered |
| eSE transaction | NOT TESTED — ENVIRONMENT UNAVAILABLE | no secure-element transaction was initiated |
| Wi-Fi 6 GHz / WPA3 | PASS | reconnected at 6135 MHz after NFC stress and after reboot |
| Wi-Fi 2.4/5 GHz re-association | NOT TESTED — ENVIRONMENT UNAVAILABLE | available AP selection remained on 6 GHz; qualified WLAN module hashes are unchanged |
| Bluetooth basic toggle/recovery | PASS | controller ON; zero Bluetooth framework crashes |
| Visible LTE registration | PASS | LTE HOME / IN_SERVICE |
| RMNET IPv4 and IPv6 | PASS | `rmnet_data2` carried both address families and both default routes |
| Cellular IP | PASS | `1.1.1.1` 5/5, 0% loss with Wi-Fi disabled |
| Cellular DNS/IP | PASS | `google.com` resolved and replied 5/5, 0% loss |

## Standby-transition messages

Toggle, airplane-mode, and suspend stress produced some NXP messages of the
form:

```text
NxpDrv: i2c_write: write failed ret(-107), maybe in standby
NxpDrv: i2c_read: spurious interrupt detected
```

They were not accompanied by an NFC HAL restart, framework failure, controller
loss, or failed recovery. `-107` is `ENOTCONN`. The exact diagnostic strings
are present in both the qualified `.089` binary and the `.102` binary, and the
only imported `.102` source delta is in the interruptible I2C read wait; it does
not modify `i2c_write()`. These messages are therefore recorded conservatively
as transition/standby observations, not attributed to the `.102` change and not
claimed harmless beyond the operations physically exercised here.

## Error scan

The available full Android logs and pre-reboot kernel scans showed no new
relevant:

- unresolved symbols, CRC/version disagreement, vermagic failure, module
  signature failure, or protected-export failure;
- NFC/HAL fatal exception or restart;
- Oops, BUG, KASAN, UBSAN, panic, use-after-free, refcount failure, or IRQ
  storm; or
- Wi-Fi, Bluetooth, or cellular regression attributable to this candidate.

Raw live captures are retained outside git at:

```text
/home/travis/Android/oneplus15-nfc102-live-captures/
```

## Promotion decision

The experiment remains on `experiment/nxp-nfc-102`. Do not create the
canonical `feature/controlled-v1-wlan053-nfc102` branch until at least a real
SN220 tag read succeeds. A real payment remains a separate equipment-dependent
qualification item and must not be inferred from Wallet/HCE registration.
