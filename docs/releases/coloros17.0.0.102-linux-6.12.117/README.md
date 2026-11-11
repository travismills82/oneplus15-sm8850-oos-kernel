# OnePlus 15 ColorOS 17.0.0.102 — Linux 6.12.117

Firmware-specific **boot.img-only** release for **CPH2747_17.0.0.102(EX01)**,
with the exact original stock DLKM and AVB companion partitions. Do not
cross-flash to OxygenOS 16, LineageOS, another firmware or regional module
contract. Physical evidence covers one OnePlus 15, active slot A, not universal
device compatibility.

Published as a normal GitHub release following the owner's authorization.
Publication is not full stable or GKI qualification: **enforced GKI remains
FAIL with 53 reviewed difference owners**. Controlled unplugged suspend/power,
OTG storage, repeated boots and extended stability are still outstanding.
These limitations are not converted to PASS by the release designation.

## Changes and preserved features

- Linux stable 6.12.116 to 6.12.117: 1,535 ordered upstream commits reviewed;
  1,525 individually imported, ten already-present equivalents retained,
  one separate infrared-lifetime prerequisite, and fourteen reviewed context
  adaptations. Original authorship, subjects and commit order are preserved.
  This is not a claimed new ACK release-tag merge.
- Seven isolated ColorOS KABI preservation commits retain the newer USB/IRQ,
  remoteproc, HID, DBS and Rust security/correctness behavior while restoring
  the genuine stock-module contract. No numeric CRC forcing or consumer ELF
  version edits. The .117 Rust KCFI function-folding safety fix stays enabled.
- KernelSU 32716/API 6, SUSFS v2.3.0 including uname-spoof support, Droidspaces
  kernel prerequisites, and built-in CAKE/FQ/FQ-CoDel retained. No new spoof
  values, hide rules, network shaping policy or default qdisc is installed.
- Stock GPU/GEAS, Bluetooth, PPP, RFKILL, SLHC and all other stock modules
  remain providers. This is **not** the experimental updated-GPU/no-GEAS stack.
- No NTFS3 enabling, scheduler conversion, clock/voltage/thermal tuning or
  replacement-module deployment. Droidspaces app/runtime/rootfs are separate.

Only **boot.img** is a partition payload. Keep stock system_dlkm, vendor_dlkm,
vendor_boot, DTBO and vbmeta, plus the existing KernelSU-patched init_boot,
unchanged. No DLKM, init_boot, VBMeta, recovery image, Manager APK or TWRP ZIP
is included. Old .115 recovery ZIPs contain a different kernel and must not be
mistaken for this release.

## Exact tested bytes

```text
Tag:             coloros17.0.0.102-linux-6.12.117
Kernel:          6.12.117-android16-6-o-g9218fe3f104b-4k
Source commit:   9218fe3f104bda23fb7e638f3f4a7236794b598b
Common tree:     2e69273bcb66991b801cdce148ff66a7958e08e1
Boot bytes:      100663296
boot.img SHA256: 3cef6c673b60f189d19c95113a72e49c924a2f2800d8726b3b43f0fe1e8d4c51
Image SHA256:    2e9a03d847908563c07f56338b3e9a0a9a8af7486a63d99742479552789b9091
Config SHA256:   902bfd6eff25d941cf2a19791022ea3a8cdd338fd6801b793b62c54b81a83ded
Symvers SHA256:  af4d9aab9fb20ed169d6251f58af3579fa32fa1d3f3e6f346b7d4686224e7371
```

These are the exact flashed, full-read-back verified and hardware-tested boot
bytes. They are not rebuilt or repackaged for publication. The release tag
adds evidence and release engineering only; its common source tree is identical
to the build source. The finalized asset manifest records the tag-target SHA.

## Verification and limits

| Gate | Result on the exact candidate |
| --- | --- |
| Common build / strict KMI symbol list / common ABI generation | PASS |
| Existing exported records | All 10,676 preserve CRC/provider/class/namespace; one additive __skb_vlan_pop export |
| Compiled layouts | 17 core + 13 additional driver roots PASS |
| KCFI | 2,535 shared symbol records unchanged; six Rust Binder interfaces separately audited; two Default prefixes intentionally corrected safely |
| Stock module oracle | 1,070 modules: 1,032 compatible, 38 dormant |
| Hard imports / CRC records | 60,701/60,701 and 61,777/61,777 |
| Missing providers / CRC / namespace / active blockers / production mismatches | 0 / 0 / 0 / 0 / 0 |
| Enforced GKI | **FAIL, exit 4: 45 unchanged prior + eight reviewed new owners; zero unexplained new owners** |
| Host tests | 17 focused .117 guards PASS; historical suite 201/202, obsolete .115 whole-file futex freeze assertion FAIL disclosed |
| AVB footer/hash integrity | PASS offline; existing development boot key, not OEM signing authority |
| Active boot_a full read-back | PASS, all 96 MiB; independent SHA256 and byte comparison |
| Android, PIN/user0, CE/DE, F2FS inlinecrypt | PASS |
| System/vendor dm-verity | PASS, both V 0; all thirteen companion hashes unchanged |
| Actual loaded modules | All 691 build-ID notes match stock .102 ELF files |
| KSU/SUSFS / installed Droidspaces checker | Root/SUSFS operational; checker 7.2.0 requirements_met=1 |
| CAKE/FQ/FQ-CoDel traffic/counters/lifecycle | PASS, eight cycles each in disposable phone network namespace; production qdiscs unchanged |
| Radios, camera/video, calls, fingerprint, USB sharing | PASS, owner-performed report |
| Standalone audio recording / OTG storage read-eject | NOT separately confirmed; calls/USB sharing are not those tests |
| TWRP decrypt / full distro sessions / sustained gaming | NOT TESTED for this artifact |
| Unplugged idle/deep suspend / repeated boots / extended stability | NOT QUALIFIED |

The read-only reconnect check after hardware testing reached about 16 minutes
uptime, with user0 unlocked, healthy verity, unchanged boot/companion hashes,
empty pstore and no new fatal/module/stall or WARN root in retained logs.

Known stock HFI IRQ-wake accounting, HBP field-spanning-copy and eBPF/BHB
policy warnings remain. Existing QCOM timekeeping/suspend and uptime-like UFS
command-delay telemetry remain unresolved; the latter also occurs in the
fresh .116 baseline. No diagnostics were suppressed. The inherited hung-task
filter uses task/wait-channel predicates rather than live transaction state,
so zero reports are not independent proof of all SCM/QPACE/HFI work health.
The detector stays enabled at 60 seconds.

All 6,573 effective config options and pinned compiler hashes match .116.
No MODVERSIONS, signing, CFI, SELinux, KMI list or ABI-reference check was
weakened. Generic Canoe replacement-DLKM build qualification is not claimed.
Source fixes affecting non-deployed modules do not patch the stock binaries.
This is not a security-completeness, Wallet or hardware-attestation claim.

## Installation — boot only

This is a manual image release, **not an installer that validates your device**.
Use an already-unlocked bootloader with a proven recovery/fastboot path.
Do not unlock/relock the bootloader, install an OTA, erase userdata/metadata,
disable verification or change AVB keys as part of this update.

1. Verify physical firmware `CPH2747_17.0.0.102(EX01)`, actual slot/mapping,
   no pending OTA/snapshot operation, and exact stock companions against
   manifest.json. Spoofed properties are not compatibility evidence. Devices
   with experimental DLKM/AVB must restore their matched stock set separately.
2. Back up the **complete current active boot partition**, hash-verify it on
   the host, and keep it offline with a working restoration route.
3. Download all five assets into one directory and verify:

   ```sh
   sha256sum -c SHA256SUMS
   sha256sum -c boot.img.sha256
   ```

4. Enter bootloader fastboot and inspect the actual current slot:

   ```sh
   adb reboot bootloader
   fastboot getvar current-slot
   ```

5. Flash **only that active boot slot**. Example ONLY if the result is a:

   ```sh
   fastboot flash boot_a boot.img
   fastboot reboot
   ```

   For b, use boot_b instead. Do not flash both, switch slots, resize partitions,
   or flash any companion image.
6. Verify Android startup, PIN unlock/user0, F2FS inlinecrypt and hardware.
   Where recovery/root permits, independently read back all 100,663,296 bytes
   from the written boot partition and require the published SHA256. A
   fastboot write-success message alone is not full read-back verification.

## Rollback

If boot, encryption or required hardware fails, stop testing and use the
preserved recovery/fastboot route. Restore your hash-verified pre-install
boot image to the **same boot slot**, verify the complete read-back, then
verify Android, user0 and F2FS inlinecrypt. Do not wipe data/metadata or switch
slots as a substitute.

The development rollback is the previous [.116 CAKE release](https://github.com/travismills82/oneplus15-sm8850-oos-kernel/releases/tag/coloros17.0.0.102-linux-6.12.116-cake),
SHA256 `f7cb7d973ce43de176a27fe8b9ad5e694b35e9d042d6e5875cf50c339f43ec16`.
That firmware-specific artifact is not a substitute for each user's own backup.

## Source, provenance and evidence

- [Source at release tag](https://github.com/travismills82/oneplus15-sm8850-oos-kernel/tree/coloros17.0.0.102-linux-6.12.117)
- [Integration and ABI review](https://github.com/travismills82/oneplus15-sm8850-oos-kernel/blob/coloros17.0.0.102-linux-6.12.117/docs/validation/firmware-17.0.0.102/coloros1700102-linux-6.12.117-integration-20261111.md)
- [Individual source/adaptation decisions](https://github.com/travismills82/oneplus15-sm8850-oos-kernel/blob/coloros17.0.0.102-linux-6.12.117/docs/validation/firmware-17.0.0.102/coloros1700102-linux-6.12.117-source-adaptations.md)
- [Exact physical canary and owner results](https://github.com/travismills82/oneplus15-sm8850-oos-kernel/blob/coloros17.0.0.102-linux-6.12.117/docs/validation/firmware-17.0.0.102/coloros1700102-linux-6.12.117-physical-canary-20261111.md)

Official interval: `5c40a1750f44dd6c7292d8c8d0b4ffff1734576e..d3417611a3b40b7f04a29e558cebac2bf69889d7`.
HTTPS refs and the official changelog agree; local GPG signature verification
was not established because the signer public key was absent.

Boot AVB properties intentionally preserve the verified vendor/KeyMint contract:
Android 16, patch 2026-10-01, fingerprint
`qti/canoe/canoe:16/BP2A.250605.015/1792254911768:user/release-keys`.
They are not advanced to fabricate a newer security level.
