# Droidspaces boot-only physical canary — 2026-11-03

Owner authorized flashing the previously frozen experiment for hands-on
Droidspaces testing. Only active boot_a was changed. No rebuild or repackaging,
slot switch, root-manager replacement, DLKM/vbmeta/init_boot change, commit,
push or release occurred.

## Identity and exact bytes

- Running kernel: `6.12.115-android16-6-o-gd6552d3625e0-4k`, confirmed through
  `/proc/version`, not the owner's spoofed uname values.
- Boot SHA256: `0cb2cdff300f522beccb4cbdedb0549579161bb48e30f756ea020c7bf5e643cc`.
- Image SHA256: `a952292f1067c8a4507cb60346e6de82d69104f6a22cbf0093ffe94bb69489a1`.
- Running config SHA256: `dcb3585a692d6ba12bfbe01aa647654962a8c962c68593aaa0fc7552af7f3536`,
  exactly the frozen build configuration.
- Source base `d6552d3625e0656aeac303b878347bef208b4315` plus frozen common tree
  `6d074b67a27e4ccd1582be8178ab0bf8284870aa`.
- Active slot A was discovered live. Slot B was not selected or written.
- Live firmware manifest: `CPH2747_17.0.0.102(EX01)`; the earlier qualification
  used a CPH2745 label. The actual five relevant .102 partition hashes all
  match the qualified stock contract, and both boot slots initially contained
  the exact known-working qualified kernel. No arbitrary cross-firmware
  compatibility assumption was used.

Android getprop reported green/locked and uname a different release. Raw
bootconfig independently confirmed unlocked/ORANGE and slot A; these spoofed
identity settings were not changed.

## Write and preservation gates

Full boot backup, candidate hash/size, offline AVB verification, partition
mapping/capacity, empty OTA/snapshot state and dry-run passed before writing.
Independent complete 96 MiB host readback matches the candidate hash.
Protected hashes compare identically before writing, after writing and after
Android returns: boot_b; both init_boot, vbmeta, recovery, vendor_boot and DTBO
slots; active system_dlkm and vendor_dlkm.

The owner's init_boot is **KernelSU-patched**, not Magisk-patched. Current
init_boot_a stays `f5af640655953413949dde1d25fca160fa9e2e3151d5f6fd3277abf3426d2539`;
init_boot_b stays `8910c3ce57893273120434866ef29a436771ae931290e83156807153c8936002`.
No historic Magisk image is included in rollback operations.

Fresh rollback boot SHA256:
`3136cd36cfbb82be4423b46e0849527ea3bc26045ca925ddbd78611d591148f5`.
Host evidence and complete images:
`/home/travis/Android/kernel-test-backups/droidspaces-canary-20261103-224513/`.
Only restore the matching boot_a backup if needed; do not change init_boot,
switch slots as a substitute, or wipe userdata/metadata.

## Observed results

| Gate | Result |
|---|---|
| Exact boot write/full independent readback | PASS |
| Android startup / sys.boot_completed=1 | PASS |
| user0 RUNNING_UNLOCKED | PASS |
| F2FS /data with inlinecrypt | PASS |
| SELinux Enforcing | PASS |
| Exact running kernel configuration | PASS |
| Original module providers loaded | PASS, startup observation |
| Installed Droidspaces requirements checker | PASS, requirements_met=1 |
| Actual Linux distro session | NOT TESTED by agent |
| Full peripherals / USB-disconnected suspend | NOT TESTED in this cycle |

The installed backend reports v7.1.0 in checker JSON, SHA256
`9ac190cd99cf0546f7026d385b845bdea05a0b52d5e1aca6cff21b0036ff52df`.
It is the owner's installed app binary, distinct from the official CI binary
used in the earlier ARM64 VM. No app/backend was replaced. `check --json`
passes all mandatory probes, including PID/IPC namespaces, plus user namespace,
devtmpfs, cgroups, ext4/loop, OverlayFS, FUSE/TUN and bridge/veth availability.
MACVLAN, IPv6 NAT and PID limits remain intentionally absent. Checker exit zero
alone was not treated as proof; the JSON result was verified.

No container, hardware pass-through mode, SUSFS rule, uname spoof setting,
root grant, security policy or performance limit was configured by the agent.
Container creation/init, outbound networking/DNS, persistence, stop/restart and
sleep/wake remain for the owner's hands-on test.

## Diagnostics and limitations

The first recorded health scan reached 190 seconds, passing the old 121/183s
blocked-task report points. No panic, Oops, kernel BUG, module CRC/unknown-symbol
failure, CFI failure, RCU stall or blocked-task report was observed.
The final identity/log capture reached 302 seconds with Android boot-complete,
root available and none of those fatal/module/stall signatures.

The stock OPlus HBP `frame_put` field-spanning-write WARN recurs at 12.877832s.
The same function/source WARN exists in the preserved qualified baseline at
12.736144s; its module is unchanged. It is disclosed, not suppressed or claimed
fixed. An unprivileged-eBPF/BHB warning is also present; BPF policy/source/config
were not changed by this experiment. The corresponding runtime warning was not
independently proven in the old Android log, so no stronger baseline claim is
made. No security-completeness claim follows from successful boot.

Offline enforced GKI still **FAILS** with 45 owners: the prior 44 unchanged plus
one reviewed C-private IPC-padding root. Exact retained-stock oracle and
provider checks passed before flash; they are not the same as GKI compliance.
Official Droidspaces SUSFS compatibility limitations remain applicable even
though this installation's requirements checker passes.

Verdict: **initial Android/FBE and live requirements-checker PASS; candidate
left installed for owner Droidspaces testing**. This is not stable release or
full distro/hardware qualification. See the [functional test plan](coloros1700102-droidspaces-test-plan.md)
and [source/static audit](../../features/droidspaces-coloros17.md).
