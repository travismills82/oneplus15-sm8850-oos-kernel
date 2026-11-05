# Droidspaces exact-running-image publication checkpoint — 2026-11-05

Owner request: push the kernel commits and publish a release of the same
Droidspaces-enabled kernel running on the connected phone. This task authorizes
GitHub publication, not another physical flash or rebuild. Publication is a
boot-only **canary/prerelease**, not a stable promotion.

## Source preservation

Source commits already on main before this checkpoint:

- `151a48c9976da93eb3a5308c88048523d229e34a`: KABI preservation for task IPC
  state and Rust bindings.
- `4f960022774c379000abdb40d230ab563bc86dd7`: minimal Droidspaces configuration
  and actual IPC namespace exports.
- `0949edcfb86b90ec6a24d51b79d7673075a4324c`: tests and validation records.

The common tree is `6d074b67a27e4ccd1582be8178ab0bf8284870aa`. Release commits
change only packaging, validation and documentation. No kernel implementation,
configuration, module, ABI reference, toolchain or AVB payload is rebuilt or
changed. The unrelated dirty root checkout and untracked build symlinks are not
staged. No existing commits are squashed or rewritten.

## Fresh device verification

Read-only capture is retained locally under
`out/droidspaces-release-20261105/` in the root repository. Device properties
and uname may be spoofed; raw `/proc/version`, `/proc/bootconfig`, firmware
manifest, actual `/proc/config.gz` and full partition read-back are authoritative.

| Check | Observation |
|---|---|
| Raw kernel | `6.12.115-android16-6-o-gd6552d3625e0-4k` |
| Physical firmware manifest | `CPH2747_17.0.0.102(EX01)` |
| Active slot | A; unchanged by this task |
| Raw bootloader/AVB state | unlocked / ORANGE |
| Android/user0 | boot_completed=1, RUNNING_UNLOCKED |
| SELinux | Enforcing |
| `/data` | F2FS, inlinecrypt |
| Uptime at read-only check | 152,373.58 seconds, approximately 42 hours |
| Full boot_a SHA256 | `0cb2cdff300f522beccb4cbdedb0549579161bb48e30f756ea020c7bf5e643cc` |
| Live config SHA256 | `dcb3585a692d6ba12bfbe01aa647654962a8c962c68593aaa0fc7552af7f3536` |

The 96 MiB host read-back is identical to the frozen candidate and release
payload. Raw Image SHA256 is
`a952292f1067c8a4507cb60346e6de82d69104f6a22cbf0093ffe94bb69489a1`.
The five stock companion partition hashes match the release manifest exactly.
KernelSU-patched init_boot hashes remain unchanged:

- A: `f5af640655953413949dde1d25fca160fa9e2e3151d5f6fd3277abf3426d2539`.
- B: `8910c3ce57893273120434866ef29a436771ae931290e83156807153c8936002`.

Uptime is an observation, not a controlled 42-hour workload or a substitute for
peripheral, full distro and USB-disconnected suspend tests. The installed
requirements checker PASS is from the Nov 3 physical test; it was not rerun
as a supposedly read-only probe because it creates transient network/cache
state. No reboot, flash, slot change or partition write occurs in this task.

## Release-only validation

The new `droidspaces` installer profile is isolated from the previous
KernelSU/SUSFS profile. It requires the fresh CPH2747 firmware identity plus
the exact five stock companion hashes, unlocked/ORANGE state, active-slot
mapping, 96 MiB boot capacity and no OTA/snapshot state. A durable complete
boot backup is mandatory before writing. Only active boot is written; failed
write/read-back attempts restore and verify that backup.

- Seven installer test methods cover both profiles, both slots, dry-run,
  rejection and partial-write/read-back rollback cases using regular-file host
  fixtures. No physical block device is used.
- Three metadata test methods cover exact source ancestry/tree, canary status,
  unchanged module/GKI/AVB claims, firmware and artifact rejection paths.
- ZIP generation repeated from the independently read-back device payload is
  byte-identical: SHA256
  `67858b4dc623ce4944fabcfc865faca2507ec08739d323814cb4b2991571c6cb`.
- AVB footer/signature and boot hash verification succeed with the existing
  development boot key. This is not an OEM signing-authority claim.
- The release verifier checks every asset hash/size, checksum coverage, exact
  ZIP contents/modes, helper text, embedded boot identity, source ancestry and
  unchanged common tree. Published downloads must pass the same verifier.
- Existing controlled-stack release validation remains intact. This boot-only
  canary does not claim the full controlled-stack stable/ZIP physical gates.

The previously recorded build/module results are not falsely rerun or
transferred from a different artifact: they apply to these exact frozen bytes.
Common build, strict symbol-list check and ABI generation PASS; 1,070 modules,
1,032 compatible active, 38 dormant, 60,701 hard imports and 61,777 CRC records
match, with zero missing/CRC/namespace/provider/blocker failures. Shared KCFI
IDs (2,525) and inspected layouts (17) remain compatible.

Enforced GKI is still **FAIL: 45 owners**, comprising 44 unchanged prior owners
and one reviewed C-private IPC-padding owner. No ABI-reference update was used.
Known stock HBP and recorded eBPF/BHB policy warnings remain disclosed. No
security-completeness claim or universal device qualification is made.

## Scope and remaining gates

Assets: exact `boot.img`, guarded TWRP ZIP, two sidecars, SHA256SUMS, manifest
and README. No APK/rootfs, replacement module, DLKM, vbmeta or init_boot.
Kernel prerequisites for Droidspaces are built in; userspace is separate.

The exact ZIP has **HOST TESTS ONLY**. Initial boot/FBE and live checker passed,
but full distro sessions, extended hardware, repeated boots and unplugged
suspend remain **NOT TESTED**. Prior original KernelSU boot SHA256
`3136cd36cfbb82be4423b46e0849527ea3bc26045ca925ddbd78611d591148f5`
remains the tested boot-only rollback, with current KernelSU init_boot retained.

Recommendation: publish the exact running bytes as a narrowly firmware-bound
canary/prerelease with these limitations; do not promote to stable.
