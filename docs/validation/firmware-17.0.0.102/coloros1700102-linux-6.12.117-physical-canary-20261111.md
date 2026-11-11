# ColorOS 17.0.0.102 Linux 6.12.117 boot-only physical canary

Date: 2026-11-11. Verdict: **initial Android/FBE, live feature checks and the
owner-reported hardware checks below PASS**. Additional hardware, recovery,
extended stability and unplugged-power qualification remain pending.
This is not a stable-release or GKI-compliance declaration.

The owner requested the .117 update, compilation and testing, then connected
the phone for the prepared boot-only test. Only active boot_a was written.
There was one normal Android reboot, no slot switch and no recovery reboot.
The passing .117 candidate remains installed for hands-on testing.

## Exact installed artifact

- Source: `9218fe3f104bda23fb7e638f3f4a7236794b598b`.
- Common tree: `2e69273bcb66991b801cdce148ff66a7958e08e1`.
- Branch: `experiment/coloros17-linux117-boot-only`.
- Actual `/proc/version`: `6.12.117-android16-6-o-g9218fe3f104b-4k`.
- Firmware: `CPH2747_17.0.0.102(EX01)`.
- Boot image/read-back size: 100,663,296 bytes.
- Boot SHA256: `3cef6c673b60f189d19c95113a72e49c924a2f2800d8726b3b43f0fe1e8d4c51`.
- Image SHA256: `2e9a03d847908563c07f56338b3e9a0a9a8af7486a63d99742479552789b9091`.
- Live/frozen config SHA256: `902bfd6eff25d941cf2a19791022ea3a8cdd338fd6801b793b62c54b81a83ded`.
- Boot source path: `/home/travis/Android/stock_kernel/out/coloros17-linux117-build/candidate/boot.img`.

No rebuild, repackage or installed-image replacement occurred during this
test. The original freeze manifest records the pre-test NOT TESTED state;
this later physical record supplements it without altering frozen payloads.

## Fresh preflight and scoped write

Raw DT identifies Canoe/infiniti, SoC ID 660 and machine CANOE. The raw
bootconfig confirms slot `_a`, unlocked device state and verified-boot ORANGE.
Spoofed green/locked Android properties were not trusted as physical state.
Before writing, Android was boot-complete, user0 RUNNING_UNLOCKED, `/data`
F2FS with inlinecrypt and SELinux Enforcing. OTA state was empty and no
snapshot entries existed. Battery was above the guard's 20% minimum.

Active mapping `/dev/block/by-name/boot_a` resolves to `/dev/block/sde12`,
capacity 100,663,296 bytes. The complete fresh host backup and independent
device SHA match the qualified .116 CAKE artifact:
`f7cb7d973ce43de176a27fe8b9ad5e694b35e9d042d6e5875cf50c339f43ec16`.
The same rollback was staged and hash-verified on the device. Recovery A
hash `42286f1cf94ffefa9cbea6259db518cc372a140709cc01ae5d21d9de95f5d7f8`
matches the available host-side TWRP image. Recovery access was not exercised
by an additional reboot during this session.

The guarded device script verifies serial, raw slot/lock state, firmware,
SoC, healthy user0/data, snapshot state, battery, exact mapping/capacity,
candidate/rollback/current-boot hashes and thirteen protected companion
hashes. Its dry-run passed and an invalid mode was rejected before writes.
An initial dry-run caught a shell `hash` alias collision; the helper was
renamed and the full dry-run repeated successfully. No failed guard wrote a
partition or weakened a validation check.

The explicit write used exactly 96 one-MiB blocks with fsync. Full device
SHA256 verification passed; a separate host read-back of all 100,663,296
bytes passed SHA256 and byte-for-byte cmp against the frozen candidate.
The normal reboot was initiated only after both read-backs passed.

## Companion partition invariants

All thirteen hashes match before writing, after writing, after reboot and
after user unlock: inactive boot_b; both init_boot, vbmeta, recovery,
vendor_boot and DTBO slots; active system_dlkm and vendor_dlkm.

The stock logical DLKM hashes are exactly:

| Partition | SHA256 |
| --- | --- |
| system_dlkm_a | `3c1538210e986317784a7bf0f29487858ea0ea15bd2b6804cbcffd93a85d3e5a` |
| vendor_dlkm_a | `82f90a9e0aba489a880bfa844ec13e0facb870c70752647568a290d45492dc47` |
| KernelSU init_boot_a | `f5af640655953413949dde1d25fca160fa9e2e3151d5f6fd3277abf3426d2539` |

No DLKM, init_boot, vendor_boot, DTBO, parent VBMeta, recovery, slot metadata,
userdata or encryption-metadata partition was written by the flash procedure.
No AVB flag/key, rollback index, bootloader state or module policy changed.
Normal Android boot and test staging under /data are not described as zero
filesystem writes. The actual root image is KernelSU, not Magisk.

## Observed gates

| Gate | Result and scope |
| --- | --- |
| Exact .117 kernel / config / active boot bytes | PASS |
| Android sys.boot_completed=1 | PASS |
| First PIN unlock / user0 RUNNING_UNLOCKED | PASS, observed at 311.57 and 321.35 seconds |
| CE/DE system storage readability | PASS |
| `/data` F2FS + inlinecrypt | PASS |
| Stock vendor/system dm-verity | PASS, both targets report `V 0` |
| SELinux | Enforcing |
| Loaded module identity | All 691 loaded modules' build-ID notes match exact stock .102 ELF records |
| KernelSU | Root operational; installed ksud 3.3.1-36-g28db74f6 reports UAPI 6 |
| SUSFS | v2.3.0 and existing enabled features, including SPOOF_UNAME, retained |
| Droidspaces installed checker | PASS, v7.2.0, JSON requirements_met=1 |
| FQ/FQ-CoDel/CAKE real traffic/lifecycle | PASS, each eight isolated create/traffic/stat/delete cycles |
| Parent networking policy isolation | Namespace IDs and production tc qdisc listings identical before/after |
| Wi-Fi traffic | PASS, three ICMP replies, zero packet loss |
| Hardware graphics | Stock Adreno 840 / OpenGL ES 3.2 renderer observed; no sustained graphics benchmark claimed |
| Radios | PASS, owner reports "all radios" stable; individual modes/procedures were not enumerated |
| Camera and video | PASS, owner hands-on report |
| Voice calls / call audio path | PASS, owner hands-on report |
| Fingerprint | PASS, owner hands-on report |
| USB file sharing | PASS, owner hands-on report |
| Standalone speaker/microphone recording | NOT SEPARATELY REPORTED; call operation is not a recording test |
| USB OTG storage read/eject | NOT CONFIRMED; USB sharing is not an OTG-storage test |
| TWRP decryption / repeated boots / actual distro workload | NOT TESTED in this cycle |
| USB-disconnected idle/deep suspend | NOT TESTED |

The checker is the owner's installed binary, SHA256
`54dac6cc9cdc18a9cc61eeaaf7587c25dadd65e20e544a4f470e4b77c9e5db07`;
it was not replaced with the VM binary. This checker performs transient feature
probes/cache maintenance; it is not claimed strictly read-only. Optional
macvlan, IPv6 NAT and pids limits remain absent, as before. No container was
created, no SUSFS configuration was changed and no new hardware privilege was
granted.

The exact prior VM-tested qdisc-smoke binary SHA256 is
`ffe7dbab625b945b10c6a73dab3165623e6c6ed2e177ac70cba7bd7f4ee4066c`.
It unshares a network namespace before any socket/interface manipulation,
uses only private loopback, verifies sixteen UDP datagrams and nonzero qdisc
statistics per cycle, then deletes the test qdisc. Its unavailable-kind
negative control passed. It does not change netd, production interfaces,
default qdisc, shaping rates, routes, clocks or voltages. No congestion or
latency improvement is inferred from feature availability.

## Warnings and diagnostics

The retained early dmesg starts at 2.051288 seconds, not time zero; the very
earliest boot output is not claimed captured. The early snapshot, 240-second
live collector and final log cover the prior 121/183-second report points
and extend through 346.567316 seconds. State/partition checks passed again
at 347.70 seconds. The fixed live collector exits 124 at its intended timeout,
not because the phone failed.

No panic, Oops, kernel BUG, KASAN/KCSAN, use-after-free/refcount failure,
CFI/KCFI failure, unknown symbol, version/load failure, RCU stall, lockup,
hung-task report, storage I/O error or dm-verity corruption signature was
observed in these retained logs. Pstore is empty. This is bounded observation,
not proof that every hardware/security path has been exercised.

Three early warning roots are disclosed:

| Time | Root | Assessment |
| --- | --- | --- |
| 2.146715 s | `irq_set_irq_wake`, manage.c:926; `hfi_core_ssr_irq_init` in stock msm_hfi_core; unbalanced IRQ 403 wake disable | Historically documented stock HFI IRQ-wake warning family. The core wake function is unchanged from .116 and live HFI build ID matches stock. Not isolated as a new .117 regression; exact same-boot .116 early trace was not retained in this session |
| 14.411634 s | Unprivileged-eBPF/Spectre-v2-BHB policy warning | Same configured policy/source family documented in .116; not hidden or repaired by this test |
| 15.197245 s | `frame_put+0x36c/0x44c`, stock oplus_hbp_core, hbp_frame.c:88; 2,048-byte field-spanning memcpy | Matches the documented .116 stock HBP root/offset; unchanged module, no warning suppression |

The later ring snapshot has already dropped the HFI splat; it remains in the
preserved early capture. No new WARN root appeared after startup in the
observed interval. Existing QCOM timekeeping/suspend limitations require a
separate unplugged test; USB-connected observation cannot clear them.

ADCI (TID 145), ZRAM (540) and HFI (1346) retain the previously documented
stock wait stacks at 155.47 seconds: `__scm_smc_call+0x220` through adci_fn,
`zram_qpace_comp+0x28`, and `hfi_core_dbg_listener+0xb0`. ADCI is not in
wait_to_die. Qualcomm's adciAccept path is documented as non-returning, but
the inherited detector checks names/wait channels, not live transaction or
queue state. Zero reports therefore do not independently prove absence of
Secure World, QPACE or HFI active stalls. The detector/code/sysctl was not
changed; global timeout remains enabled at 60 seconds.

A subsequent heartbeat at 481.31 seconds still confirms the same .117 kernel,
Android completion and user0 RUNNING_UNLOCKED. The last log snapshot reaches
511.713995 seconds with no new WARN/fatal/module/stall root. It also contains
the existing stock UFS `READ_10`/`WRITE_10 cost more than 100ms` telemetry,
reporting roughly uptime-sized durations (511,710 ms). The fresh pre-flash
.116 log already has the identical message family with durations around
19,246,790 ms at 19,246 s. This supports a pre-existing diagnostic issue, not
a newly established .117 regression; it does not by itself prove either real
storage stalls or harmless telemetry. No block I/O error/reset signature was
observed, and the unresolved UFS telemetry limitation remains disclosed.

## Static status and release limitations

### Owner hardware results and read-only reconnect check

The owner reported: "all radios camera and video are stable phone calls work
fingerprint and usb sharing works". These are owner-performed functional
results for the installed .117 canary, not agent-observed individual RF,
camera, call or fingerprint procedures. "All radios" is retained as reported;
it is not expanded into an independently verified NFC/GNSS/roaming/VPN test.

The subsequent read-only check at 982.95 seconds (about 16.4 minutes uptime)
confirmed the same .117 source identity, slot A, raw unlocked/ORANGE state,
Android completion, user0 RUNNING_UNLOCKED, F2FS inlinecrypt, SELinux Enforcing
and healthy `V 0` for both DLKM verity targets. The active boot hash still
matches the frozen candidate and all thirteen companion hashes match.
Pstore remains empty and the global hung-task timeout remains 60 seconds.

The reconnect dmesg spans 15.164471–983.113724 seconds; earlier startup records
are retained separately because the ring has wrapped. No new panic, Oops,
kernel BUG, memory/refcount failure, CFI/KCFI failure, module symbol/version
failure, RCU stall, lockup, hung-task report, storage I/O-error or verity
corruption signature was found in this snapshot. The only WARN splat retained
there is the already-recorded stock HBP warning at 15.197245 seconds; no new
WARN root appeared after the hands-on tests. Earlier warnings and unresolved
UFS telemetry remain disclosed above, not silently cleared by a later snapshot.

Evidence: `followup-hardware-state.txt`, `followup-hardware-hashes.txt` and
`followup-hardware-dmesg{,-clean}.txt` in the same private evidence directory.
No phone setting, source implementation or installed artifact was changed in
this reconnect check. Unplugged idle/suspend, OTG storage, standalone audio
recording, repeated boot, TWRP decrypt and sustained thermal/power workloads
remain separate, unperformed gates.

The [integration report](coloros1700102-linux-6.12.117-integration-20261111.md)
records the common build, strict symbol-list check, ABI generation, real
compiled layouts, complete stock-module oracle and loader/provider checks.
These remain separate from runtime testing. Enforced GKI remains **FAIL:
45 unchanged prior difference owners plus eight reviewed new owners**. No
ABI reference, symbol list or acceptance rule was updated. The historical
host suite remains 201/202 PASS with its disclosed obsolete whole-file futex
freeze assertion; seventeen focused .117 guards pass.

This boot-only candidate does not deploy fixes compiled only into replacement
module outputs; it is not a Qualcomm/DLKM security-complete claim. Neither
successful startup nor stock module matching establishes stable-release
qualification, sustained gaming, OTG, recovery decrypt or power behavior.

## Evidence and rollback

Private full evidence and fresh rollback:
`/home/travis/Android/kernel-test-backups/coloros17-linux117-canary-20261111-1634/`.
The preserved `boot_a-before.img` is exact working .116 CAKE. The after image
is the complete .117 read-back. Device write guard and preflight scripts are
preserved in local `out/coloros17-linux117-build/`; no device data is committed.

If a mandatory gate fails, retain diagnostics, enter the known bootloader or
matching TWRP route and restore ONLY the verified `boot_a-before.img` to
boot_a. Reverify the complete 96 MiB read-back and Android/user0/F2FS
inlinecrypt. Do not switch slots or replace DLKM/AVB/init_boot as a substitute.
No rollback has been needed. The current passing .117 image is left installed.

No kernel implementation, installed companion image, Git main, push, merge,
tag or release was changed during physical testing. Documentation alone was
updated locally. Selected owner hands-on results are now recorded above;
the next gates include unplugged idle/resume and the remaining specific
hardware/recovery tests before broader stability claims.
