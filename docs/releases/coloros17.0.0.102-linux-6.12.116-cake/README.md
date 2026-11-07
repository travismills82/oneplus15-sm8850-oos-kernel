# ColorOS 17.0.0.102 — Linux 6.12.116 with CAKE

Firmware-specific **boot.img-only** release for the OnePlus 15 running
**CPH2747_17.0.0.102(EX01)** with its original stock DLKM and AVB companion
partitions. Do not cross-flash to OxygenOS 16, LineageOS, another firmware or
another regional module contract.

Published in GitHub's **normal/latest (non-prerelease) channel at the owner's
request**. That channel selection does not turn incomplete tests or the
enforced GKI comparison into PASS. Technical qualification remains scoped to
the results below; this is not a claim of universal compatibility, complete
security coverage or full power/extended-hardware qualification.

## Included

- Linux 6.12.116, retaining the repaired ColorOS KABI and prior security work.
- Built-in CAKE, alongside the existing built-in FQ and FQ-CoDel. No default
  qdisc, shaping rate, production interface or Android networking policy is
  changed. Availability is not a measured latency/bandwidth improvement.
- KernelSU 32716 / API 6, SUSFS v2.3.0 and Droidspaces kernel prerequisites
  retained. Droidspaces userspace/rootfs must be installed separately.
- Stock GPU/GEAS, Bluetooth, PPP, RFKILL, SLHC and other stock module providers
  retained. This is **not** the experimental updated-GPU/no-GEAS DLKM stack.
- No new NTFS3 or scheduler conversion. PELT was already present; the stock
  WALT/governor arrangement is unchanged.

Only `boot.img` is supplied as a partition payload. Do **not** flash
system_dlkm, vendor_dlkm, vendor_boot, init_boot, DTBO or vbmeta for this release.
Keep the existing KernelSU-patched init_boot. No new recovery ZIP is supplied:
an exact CAKE installer has not been qualified, and older .115 ZIPs contain a
different kernel.

## Exact tested artifact

```text
Tag:             coloros17.0.0.102-linux-6.12.116-cake
Kernel:          6.12.116-android16-6-o-gc2e2f95d3c3c-4k
Boot bytes:      100663296
boot.img SHA256: f7cb7d973ce43de176a27fe8b9ad5e694b35e9d042d6e5875cf50c339f43ec16
Image SHA256:    456436bea4d1d1f126b4ebfce9001253871f4b0edf1d2af07c8116f836428038
Config SHA256:   da9bf33efeac03dd29e0cefb9fb2f6aaad272b8b2458886fb0e30c06840656c5
Symvers SHA256:  e01ecec95a9603acdb9333f6c2672d573f39e02f0f98c93e50996358ce1b4aa8
Source commit:   737f8bbabba274fec2d38cb22df9f854daebfa9b
Common tree:     481a38e99f82a476cdfa1ec1023cd9d4232dcaae
```

The exact flashed/read-back bytes are reused without rebuilding or
repackaging. The build suffix identifies the build parent plus the then-local
CAKE config line; the source commit above records that identical common tree.
The release tag adds documentation/verification only, not a new kernel build.

## Verified and outstanding gates

| Gate | Result on these bytes |
|---|---|
| Common build, strict symbol list, common ABI generation | PASS |
| Stock modules | 1,070 checked; 1,032 active compatible; 38 dormant |
| Imports / CRC records | 60,701 / 60,701; 61,777 / 61,777 |
| Missing providers / CRC / namespace / active blockers / production-provider mismatches | 0 / 0 / 0 / 0 / 0 |
| Compiled exports, 17 private layouts, 2,535 shared KCFI IDs | Unchanged from qualified source baseline |
| Enforced GKI comparison | **FAIL, exit 4: 45 unchanged downstream difference owners** |
| AVB footer/hash integrity | PASS offline; no OEM-key acceptance claim |
| Exact boot_a full write/read-back | PASS, 96 MiB |
| Android, user0 RUNNING_UNLOCKED, F2FS inlinecrypt | PASS |
| KernelSU/SUSFS, live Droidspaces requirements checker | PASS |
| Actual CAKE/FQ/FQ-CoDel traffic/counters/lifecycle | PASS in isolated phone network namespace; production qdiscs unchanged |
| Wi-Fi traffic / Bluetooth watch / hardware GLES | Ping PASS / connection observed / Adreno 840 GLES 3.2 observed |
| Startup diagnostics | Observed beyond 275 seconds; no fatal/module/CFI/RCU/hung-task/verity error observed |
| New full hardware, calls, OTG, recovery decryption, distro sessions, sustained gaming | NOT TESTED on CAKE image |
| Controlled USB-disconnected idle/deep suspend, repeated boots, extended stability | NOT QUALIFIED |

The owner reported full hardware success on the preceding plain .116 image;
those results are not silently transferred to the changed CAKE Image. The
known eBPF/BHB policy and stock HBP field-spanning-memcpy WARNs recur. Existing
QCOM suspend/timekeeping and power limitations remain disclosed, not suppressed
or fixed. The hung-task detector remains enabled. No ABI reference, CRC,
MODVERSIONS, signature, SELinux or AVB enforcement was weakened for this release.
Generic replacement-DLKM/Canoe distribution qualification is not claimed.

## Install: boot partition only

Use an unlocked bootloader and a working recovery/fastboot route. Back up the
**complete current active boot partition** before installation; verify that
backup on the host and keep it available offline. Do not unlock/relock the
bootloader or perform an OTA as part of installation. If firmware, partitions,
snapshot/update state, stock companion hashes or recovery access are uncertain,
stop instead of flashing.

1. Confirm the physical firmware really is `CPH2747_17.0.0.102(EX01)` and uses
   the exact stock companions in `manifest.json`; root property spoofing must
   not be used as compatibility evidence. Devices still on experimental DLKM
   must restore their matched stock set separately first.
2. Download all five assets into one directory and verify:

   ```sh
   sha256sum -c SHA256SUMS
   sha256sum -c boot.img.sha256
   ```

3. With your full active-boot backup verified and no OTA/snapshot operation
   pending, enter bootloader fastboot and inspect the real current slot:

   ```sh
   adb reboot bootloader
   fastboot getvar current-slot
   ```

4. Write **only that active boot slot**, never both and never choose a slot
   from this example. For current slot `a`:

   ```sh
   fastboot flash boot_a boot.img
   fastboot reboot
   ```

   For current slot `b`, use `boot_b` instead. No partition resize, slot switch,
   additional image or verification-disable command is required.
5. Confirm Android startup, PIN unlock/user0, `/data` F2FS inlinecrypt and
   normal hardware operation. Where recovery/root permits, read back all
   100,663,296 bytes of the target boot partition to the host and require its
   SHA256 to equal the published boot hash. Fastboot's write success alone is
   not an independent read-back verification.

This file contains manual commands, not an installer that validates a device
for you. Never write userdata, metadata, init_boot, shared vendor storage or AVB
parents to troubleshoot this boot-only update.

## Rollback

If Android or encryption initialization fails, stop testing and return through
the preserved recovery/fastboot route. Flash your verified **pre-install boot
backup to the same slot** and verify the complete read-back. Reboot and verify
Android, user0 and F2FS inlinecrypt. Do not wipe userdata/metadata or switch
slots as a substitute for restoring the correct boot image.

The development device's preserved plain-.116 rollback hash is
`0c39caaa3c2095b834f2687a10ddce8e445b32d3dfc9cca363edb708c5be288a`;
this does not replace each user's own verified backup.

## Source and evidence

- [Release/source record](https://github.com/travismills82/oneplus15-sm8850-oos-kernel/tree/coloros17.0.0.102-linux-6.12.116-cake)
- [CAKE feature and static/VM evidence](https://github.com/travismills82/oneplus15-sm8850-oos-kernel/blob/coloros17.0.0.102-linux-6.12.116-cake/docs/features/traffic-control-coloros17.md)
- [Exact physical canary and limitations](https://github.com/travismills82/oneplus15-sm8850-oos-kernel/blob/coloros17.0.0.102-linux-6.12.116-cake/docs/validation/firmware-17.0.0.102/coloros1700102-linux-6.12.116-cake-physical-canary-20261107.md)

The boot AVB properties deliberately retain the verified ColorOS vendor
contract: Android 16, security patch `2026-10-01`, fingerprint
`qti/canoe/canoe:16/BP2A.250605.015/1792254911768:user/release-keys`.
They are not advanced to claim a newer security level. This release makes no
claim of hardware-backed integrity/Wallet certification or security completeness.
