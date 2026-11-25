# ColorOS .201 / Linux 6.12.119 security maintenance canary

2026-11-24. OnePlus 15 CPH2747, `CPH2747_17.0.1.201(EX01)`, slot A.
This is a boot-only experimental canary, not a stable release. The
[maintenance audit](coloros17001201-linux-6.12.119-maintenance-2026-11-24.md)
records the source review before production validation and flashing.

## Frozen build

- Base: `e536034c49674f4efdfe6003d556b22552aca36a`, plus eight modified
  kernel source files for CVE-2026-99652, CVE-2026-99664 and
  CVE-2026-99665. The source diff SHA256 is
  `768cb2f3f8276c9ef785ec7258ed8e9b97ee0742d15255d0f29e414aad7a652b`.
  The base SHA in `uname` is not a clean-tree build attestation.
- The tested kernel-source changes were subsequently recorded in
  `1bb8c57dc0c34582396db85b12c454f54854e0d0`,
  `568a669d8aff6ceca2b74c856faeaefe150eb3c9` and
  `f442aab4b5bdee3a19e04868fc99e0d2b61eb6fa`. The binary Git diff from the
  base to that source tip has the same SHA256. The original image has
  not been rebuilt; a new build may show a different source ID in `uname`.
  The original build used the matching external ColorOS build inputs;
  they and the physical evidence are not included in these source commits.
- Running release: `6.12.119-android16-6-o-ge536034c4967-4k`.
- Raw Image SHA256:
  `43263f4e62ed39479cc690d09e5bc193a3cf4de1830eff8c0ebc78480e822f11`.
- `boot.img`, 100,663,296 bytes, SHA256:
  `35a990c2d3c8eaaa7828ca5c0476deda178f637b98e6abe5e838a4be2334625a`.
- Existing KernelSU and SuSFS v2.4.0 were retained, not updated. Existing
  native Droidspaces support was retained. The candidate passed the official
  v7.2.0 requirements checker and an Alpine start/file/stop smoke test in an
  isolated ARM64 guest. Optional MACVLAN, NAT66 and PID-limit functionality
  were not qualified. The Droidspaces Android app was not updated.

## Offline validation

Production common build, strict KMI, ABI generation, corrected independent
ColorOS STG comparison, config, KCFI, private layouts, protected exports,
ARM64 smoke tests and offline AVB verification all passed. The stock-module
oracle passed for 1,031 active records with 38 dormant; no provider, CRC or
namespace failures. Official enforced Google GKI remains **FAIL (exit 4)**;
both its enforced and corrected reports are byte-identical to B4. The
checked-in reference and enforcement were unchanged. The boot-only change
does not establish the security status of stock Bluetooth and NFC binaries
or replace proprietary firmware or userspace fixes.

## Physical boot and read-back

- Verified the original B4 rollback with a fresh full read-back. Its
  `boot_a` SHA256 is
  `7ed5ecc5ab9c692752761f8aae2d92690d981aa6eaebdb115f518181912dd4d3`.
- Android OTA state file was empty and its snapshot directory had no entries.
  Real bootloader Fastboot confirmed unlocked Canoe, active slot A,
  `snapshot-update-status: none`, and a raw 96 MiB `boot_a`.
- Wrote only `boot_a`. Full partition read-back exactly matched the frozen
  candidate SHA256. The eleven separately hashed companion partitions were
  unchanged, including stock DLKM, VBMeta chain, recovery, vendor_boot,
  init_boot, DTBO, ABL and my_manifest. No slot change or rollback was run.
- Raw bootconfig confirmed unlocked/ORANGE and the calculated AVB chain
  digest for the new boot with the current recovery:
  `ad53b0de0e4d4865d0b211730d04bfc462de2bbbbba9a91ee660eeed469bca64`.
- Android reached `sys.boot_completed=1`. `/data` mounted F2FS with
  `inlinecrypt`, SELinux remained enforcing, and the authorized root shell
  worked. All 689 loaded module names matched the pre-flash inventory.
- Initial kernel scan found no panic, oops, module CRC/provider error, CFI
  failure, RCU stall, or filesystem corruption/verity error. HBP, BHB, GIC,
  duplicate proc/sysfs registration and an HFI SSR IRQ wake-disable warning
  were present. The exact HFI handler and module build ID also appear in the
  earlier B2 log. No warning or safety check was suppressed. The pre-flash
  dmesg invocation did not yield a usable log, so warning comparisons use
  the preserved B4/B2 captures rather than assuming they came from this boot.

## Post-unlock verification

At 638 seconds of uptime, user0 was `RUNNING_UNLOCKED`. Both
credential-encrypted and device-encrypted user0 directories were readable;
`/data` was still F2FS with `inlinecrypt`, and SELinux was enforcing.
A new live hash of `boot_a` matched the frozen candidate. All 689 loaded
module names still matched the pre-flash inventory. The retained kernel
log had no fatal-scan matches and no new warning sites compared with the
first capture. Some early records had already aged out of the ring buffer.
Evidence: `device-a/android-post-unlock.txt`, `device-a/dmesg-post-unlock.txt`
and `device-a/modules-post-unlock-diff.txt`.

The owner reported that everything was working well in use. At approximately
24 minutes of uptime, a follow-up independently verified the same `boot_a`
hash, user0 still `RUNNING_UNLOCKED`, and no fatal-scan matches in the
retained log (`device-a/android-use-check.txt` and
`device-a/dmesg-use-check.txt`). No individual hardware test outcomes were
specified. Suspend counters still showed zero successes and zero failures,
so unplugged suspend/resume remains unverified. Battery life was not
measured. The canary remains installed and is not fully stability-qualified.

The sealed images, raw local diagnostics and gate reports are kept outside
the source checkout under `out/coloros201-nov24-security.g3HCtv/flash-qualification/`.
`CANARY-SHA256SUMS` was verified locally. If rollback is required, verify the
same firmware, unlocked Canoe bootloader, slot A, idle snapshot state and
the exact B4 rollback hash above; restore only the raw `boot_a` from
`device-a/rollback-boot_a.img`, then recheck the full partition read-back
and boot. No accompanying image or slot switch is needed for this rollback.
