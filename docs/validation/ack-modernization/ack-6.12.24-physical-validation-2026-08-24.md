# ACK 6.12.24 physical validation

## Result

**PASS — ACK 6.12.24 PHYSICALLY VALIDATED**

The exact statically validated ACK 6.12.24 candidate was installed on the
OnePlus 15 CPH2747/Canoe test device and passed two clean Android boots plus
the required module, WLAN, cellular, audio, camera, Bluetooth, NFC, graphics,
USB, power, and error-scan gates. No payload was rebuilt during physical
qualification.

This result freezes ACK 6.12.24 as the next controlled development baseline.
It does not authorize an ACK 6.12.25 build, a push, a merge, or a release.

## Tested identity

- static candidate source HEAD: `89f374eac402785d3d9d95292594695109c4d8b2`
- kernel build-input source identity: `a7f2fd6d686f38d448e8a276efe1aea7c2b9013f`
- tested branch: `experiment/ack-6.12.24`
- device: OnePlus 15 CPH2747 / Canoe / SM8850
- slot: `_b`
- kernel release: `6.12.24-android16-5-o-ga7f2fd6d686f-4k`

| Payload | SHA-256 | Result |
|---|---|---|
| `boot.img` | `3ceb46491d029586af1a6dc494b5baf4ddb973ad0c065c0960e4ed307d9d40b9` | exact read-back PASS |
| `system_dlkm.img` | `1739941b5bda37b52657a06340411c93fa6fff2a32ee1cfe5d32c28902d8846a` | exact-length read-back PASS |
| retained `vendor_dlkm.img` | `dccb47a3ba4055b3d385d80d6fda3875bc544488e1ed2cbd9951cd0662f56b26` | unchanged PASS |
| retained `vendor_boot.img` | `5fe60f58ebe3f935acb3ec41585fa16977804cc0c2efd4e84c44a645a1eb7162` | unchanged PASS |
| retained `vbmeta_b` | `270792710e9c480cf31e023f0c711c9063a7263324ec2c616ec4cf36bd346ad2` | unchanged PASS |

The `system_dlkm.img` is 45,658,112 bytes. Its read-back comparison was made
over exactly that length. Hashing the entire larger logical partition is not
an image-identity comparison because unwritten partition tail space is outside
the image payload.

## Recovery safety and installation

The test used the hardened controlled-stack helper from TWRP lineage
`3f499bfd1f7152ea27b27935be22ff73581709a1`. The exact live helper SHA-256 was
`84b035710c305be859aac72827489850b7d215ee372ab3341be849e051bfab50`.

- TWRP device guard: PASS
- CPH2747/Canoe identity: PASS
- slot `_b`: PASS
- Virtual A/B snapshot/update state: none
- user 0 decrypted: PASS
- backup destination decrypted and writable: PASS
- dry-run: PASS; no partitions modified
- partition capacity and filesystem/AVB checks: PASS
- write set: `system_dlkm_b`, then `boot_b` last
- partitions not written: `vendor_dlkm_b`, `vendor_boot_b`, VBMeta, slot metadata
- post-write exact read-back: PASS

Backup directory:

`/sdcard/TWRP/kernel-flash-backups/controlled-stack-b-20260824-172352`

| Backup | Size | SHA-256 |
|---|---:|---|
| `boot_b` | 100,663,296 bytes | `84a85b9a40adc7bdb178bf86e2da74e9bac3feaf7bd502c7d29b1df27dea55ab` |
| full `system_dlkm_b` source/backup | 88,514,560 bytes | `453590959bf0c66e674bdcc2e29bdbc32303828454b90071d21cbdfc7002e640` |

The independently qualified 6.12.23 rollback images remained available. No
rollback was required.

## Boot and module contract

| Check | Result |
|---|---|
| first clean boot | PASS |
| second clean boot | PASS |
| release on both boots | exact 6.12.24 release PASS |
| `system_dlkm` `modules.load` | 46 entries |
| `wwan.ko` | entry 21 |
| expected WLAN/RMNET/IPA/GSI/BT/NFC modules | loaded |
| unresolved symbols | 0 |
| CRC/MODVERSION disagreements | 0 |
| vermagic failures | 0 |
| signature failures | 0 |
| protected-export failures | 0 |

The physically qualified WLAN .053, Bluetooth vendor .046, NFC .102,
RMNET_CORE .102, IPA .102, and GSI .102 vendor payloads remained unchanged.

## Functional validation

### WLAN and cellular

- 6 GHz association at 6135 MHz: PASS
- WPA3-SAE: PASS
- Wi-Fi connectivity: 5/5 PASS
- Wi-Fi OFF/ON reload and reconnection: PASS
- Visible LTE HOME / IN_SERVICE: PASS
- active `rmnet_data2`: PASS
- RMNET IPv4 and IPv6 addresses: PASS
- IPv4 and IPv6 default routes: PASS
- cellular data-call result: `NONE / 0x0`
- stabilized cellular IP test: 5/5 PASS
- cellular DNS test: 5/5 PASS
- Wi-Fi return to 6135 MHz WPA3: PASS

The first IP probe immediately after each tested Wi-Fi-to-cellular transition
lost its first packet, followed by stable IP and DNS connectivity. No RMNET,
data-call, IPA/GSI, OEM failure code, or kernel error accompanied it. This
matches the previously qualified handoff behavior and is classified as:

`TRANSIENT POST-HANDOFF PACKET LOSS — NON-BLOCKING`

### Bluetooth and NFC

- Bluetooth OFF/ON recovery: PASS on both boot sessions
- Bluetooth framework crash count: 0
- existing-device reconnect: NOT TESTED — no device was connected
- NFC true OFF/ON transition: PASS on both boot sessions
- Google Wallet/HCE registration after recovery: PASS
- off-host secure element routing to `eSE1`: PASS

### Audio, camera, graphics, USB, and power

- configured `OnePlus new feeling` ringtone playback: PASS; user confirmed clear
- audio service playback proof: active ringtone MediaPlayer, speaker device,
  48 kHz
- stock Camera app open: PASS
- still capture: PASS; new 1,370,733-byte HEIC created
- camera fault scan: PASS
- SurfaceFlinger/HWC/UI sanity: PASS
- active display mode: 1272x2772 at 120 Hz
- exposed refresh modes: 60, 90, 120, 144, and 165 Hz
- GPU device nodes: present
- USB enumeration and ADB after both boots: PASS
- forced deep-idle entry and wake: PASS
- post-wake WLAN, IP, Bluetooth, NFC, Wallet, and eSE state: PASS

## Error comparison

The final full runtime scan found no new relevant:

- Unknown symbol or unresolved-module failure
- CRC/MODVERSION disagreement
- vermagic or signature failure
- protected-export failure
- panic, Oops, or BUG
- KASAN or UBSAN report
- IOMMU/SMMU fault
- use-after-free, double-free, or refcount failure

The Oplus blocked-task informational traces for `adci_thread`, `zram_comp`,
`hfi_core_dbg_cl`, and `osml_monitor` occur at the same approximately
121/183/244-second intervals in physically qualified 6.12.23 captures. The
early `bcl_probe` and Oplus HBP `frame_put` traces are likewise unchanged
baseline warnings. They are not attributed to ACK 6.12.24.

The broad diagnostic keyword scan also sees established firmware search
fallbacks and NXP standby-write messages. Their corresponding WLAN, IPA/GSI,
GPU, camera, Bluetooth, and NFC functional paths all passed, and no new
critical divergence was found.

## Qualification boundary

This validation applies to the installed OxygenOS 16.0.9.400-based controlled
stack and the exact payload hashes above. Firmware downloaded after this test
is a separate compatibility target and was not installed or evaluated here.

## Final status

**PASS — ACK 6.12.24 PHYSICALLY VALIDATED AND READY TO FREEZE**

Do not begin ACK 6.12.25 until this exact 6.12.24 baseline is formally selected
as the source and payload rollback boundary.
