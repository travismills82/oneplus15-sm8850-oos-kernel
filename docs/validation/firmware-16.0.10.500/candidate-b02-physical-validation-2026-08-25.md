# Candidate B02 physical validation — 2026-08-25

## Result

**PASS — BPF PER-CPU MAP COPY BOUNDS HARDENING PHYSICALLY VALIDATED ON
OOS 16.0.10.500**

Candidate B02 cumulatively contains the physically qualified B01 Binder
lifetime hardening plus one new runtime change: the BPF per-CPU map copy
bounds fix in `pcpu_init_value()`. It retains the exact stock current-firmware
`system_dlkm`, `vendor_dlkm`, `vendor_boot`, VBMeta policy, and slot metadata.
No ACK advancement or B03 customization was included.

## Tested identity

| Field | Value |
|---|---|
| Device | OnePlus 15 CPH2747 / Canoe |
| Firmware | `CPH2747_16.0.10.500(EX01)` |
| Slot | `_a` |
| B01 qualified parent | `b9b15f8a22a906786729cf830c47d0c6cd237e9a` |
| B02 runtime source head | `ab336ec00b4bf6a86fde5ba682852fefa06de0c8` |
| Pre-physical documentation head | `97180be65bc495bbb12f555c159d971b34f1c4ab` |
| Runtime change | BPF per-CPU map copy bounds hardening |
| Runtime kernel | `6.12.23-android16-5-o-gab336ec00b4b-4k` |

The machine-code/source audit proving that the built Image contains the
one-line `copy_map_value_long()` to `copy_map_value()` correction is in
`candidate-b02-binary-delta.md`. Physical testing proves compatibility of that
binary on Canoe. The exact vulnerable four-byte `CGROUP_STORAGE` per-CPU map
initialization condition was not observable with the unmodified production
candidate, so exploit-trigger coverage is recorded separately as **NOT
OBSERVED**.

## Payload isolation

Only `boot_a` was written.

| Partition | Physical SHA-256 | Result |
|---|---|---|
| boot_a | `dd63f38c658bf81b259f41f5ade970a12e8742bf1e427ed866c532e5f308cb07` | candidate input/write/read-back and Android read-back PASS |
| system_dlkm_a | `18f530dcb0e46dc81ede00e18ac4e9b39faf6564fb067f04af9a912d87fd6dd7` | exact stock, unchanged |
| vendor_dlkm_a | `157db23ccfa516c7904d4e87614f62c3c99c2dcbb93cc3fc8ccd3388c2cc793f` | exact stock, unchanged |
| vendor_boot_a | `3027d80a33fcc65f506d2d909db862657fe28e7eafc6e2af94193d67e6e617eb` | exact stock, unchanged |

The current-firmware system-DLKM contract remained 82 `modules.load` entries,
with `wwan.ko` at entry 28. Neither DLKM was supplied to the flash operation.

## TWRP safety record

- device/slot guard: PASS, CPH2747/Canoe, `_a`
- snapshot/update state: `none`
- hardened helper SHA-256:
  `84b035710c305be859aac72827489850b7d215ee372ab3341be849e051bfab50`
- undecrypted `/data/media/0` was excluded; backups used recovery tmpfs and
  were pulled to the host
- independent full `boot_a` backup:
  `/home/travis/Android/oos16.0.10.500-kernel-compat/backups/b02-preflash-20260825/boot_a.img`
- source and backup size: 100663296 bytes
- source and backup SHA-256:
  `2646a4d773ac6360cf981c4148fd37b128e8f0cd53abd07418a6807641e9d091`
- independent B01 rollback artifact: present and hash-verified
- boot-only helper dry-run: PASS, no partition modified
- helper flash targets: `boot_a` only
- complete post-write `boot_a` read-back: PASS
- untouched stock DLKM and `vendor_boot_a` hashes: PASS

## Boot and encrypted-user0 validation

| Test | Result |
|---|---|
| First Android boot | PASS |
| Second clean Android boot | PASS |
| Existing user 0, boot 1 | `RUNNING_UNLOCKED` |
| Existing user 0, boot 2 | `RUNNING_UNLOCKED` |
| `vdc cryptfs init_user0` | status 0 on both boots |
| `init_user0_failed` | NOT OBSERVED |
| Rescue Party/TWRP redirect | NOT OBSERVED |
| Prolonged OnePlus-logo stall | NOT OBSERVED |

The second-boot kernel log records successful metadata encryption, fscrypt
systemwide key initialization, native blk-crypto, `fscrypt_init_user0`, and CE
storage unlock. Existing user data remained intact; no format, metadata,
encryption-policy, vendor_boot, VBMeta, or slot-metadata change was used.

## BPF-specific physical coverage

- bpffs mounted successfully
- first-boot inventory: 252 pinned BPF objects (144 maps and 108 programs)
- `netd` running after both boots; the `bpfloader` one-shot completed
- network-data-saver policy toggled and restored 5/5
- 70/70 application/activity operations passed across Settings, Wi-Fi,
  Bluetooth, Chrome, Wallet, Camera, and launcher paths
- 50 MiB WLAN HTTPS transfer: PASS, HTTP 200, zero interface errors/drops
- 50 MiB cellular HTTPS transfer: PASS, HTTP 200, zero RMNET errors/drops
- Wi-Fi/cellular handoff: 5/5, with 6135 MHz WPA3 restored after each cycle
- 5/5 forced deep-idle/wake cycles retained user0, IP, and cellular service
- no BPF out-of-bounds, verifier memory-safety, use-after-free, KASAN, UBSAN,
  Oops, or panic report

One Oplus `oplus-netd.o` program (`skfilter_egress_sysdrop`) reports an invalid
map FD and is skipped during boot. This exact warning is present on the
physically qualified B01 baseline; `NetBpfLoad` subsequently reports success,
`bpfloader` exits status 0, and `netd` starts. It is therefore a retained
firmware observation, not a B02 regression.

Result: **BPF runtime compatibility PASS; exact vulnerable trigger NOT
OBSERVED.**

## Subsystem regression results

| Subsystem | Result |
|---|---|
| WLAN | PASS; 6135 MHz WPA3-SAE, Internet, 50 MiB transfer, off/on/handoff recovery |
| Cellular | PASS; Visible LTE HOME/IN_SERVICE, active `rmnet_data2`, IPv4/IPv6, routes, fail cause NONE/0x0, IP/DNS, 50 MiB transfer |
| Bluetooth | PASS; off/on recovery and existing HID connection returned connected; second-boot HID connected |
| NFC | PASS; service on, Google Wallet/Pay HCE enabled, preferred payment service present, eSE1 route present |
| Camera | PASS; Oplus camera opened and captured `IMG20260825164305.heic` (1147584 bytes) |
| Audio | PASS; ringtone media session played through `AUDIO_DEVICE_OUT_SPEAKER`; user confirmed clear audio |
| Graphics/UI | PASS; normal SurfaceFlinger/UI operation through app churn |
| USB | PASS; ADB/USB enumeration and one normal post-unlock re-enumeration recovered |
| Deep idle/resume | PASS, 5/5 |
| Clean boots | PASS, 2/2 |

The first data-saver transition coincided with one USB gadget re-enumeration
while post-unlock USB configuration settled. Kernel uptime continued,
framework/netd remained alive, and the following 5/5 policy cycles passed.
This is recorded as a non-blocking USB observation.

## Error scan

Full `dmesg`, `logcat -b all`, BPF state, traffic, network-policy, handoff,
camera, audio, deep-idle, and subsystem evidence is retained under:

`out/oos1610500-custom-r53-b02-final/physical-20260825/`

No new relevant match was found for:

- Unknown symbol, CRC/MODVERSION disagreement, invalid vermagic, signature, or
  protected-export failure
- BPF out-of-bounds access, use-after-free, refcount failure, double-free,
  invalid pointer, list corruption, or general-protection fault
- kernel panic, Oops, BUG, KASAN, or UBSAN report
- IOMMU/SMMU fault
- system_server, zygote, servicemanager, hwservicemanager, or vendor-service
  crash loop
- `init_user0_failed`, fscrypt, metadata-encryption, or CE-unlock failure

The Android crash buffer contains the same short-lived `init` SIGABRT event at
boot completion that was present on B01; the system continued unlocking and no
service crash loop followed. Oplus also emitted its established blocked-task
informational traces for intentionally waiting `adci_thread`, `zram_comp`,
`osml_monitor`, and `hfi_core_dbg_cl` threads. These were not accompanied by a
functional stall or fatal/memory-safety condition and are unrelated to the
single BPF source change.

## Decision

**PASS — CANDIDATE B02 BPF PER-CPU MAP COPY BOUNDS HARDENING PHYSICALLY
VALIDATED**

No rollback was required. This qualification covers B01+B02 only. It does not
authorize ACK advancement, DLKM adaptation, main promotion, or B03.
