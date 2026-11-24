# ColorOS .201 Linux 6.12.119 extended runtime analysis

The installed B2 kernel completed **20 h 42 min 52 s** without an observed
system restart. It reached encrypted user0, mounted F2FS with inline
encryption, and completed thousands of suspend cycles. The retained logs
show a real suspend-entry workload and several old vendor diagnostics;
they do not show a new fatal kernel failure. The firmware, slot and running
image were left intact. This is not an assertion of a clean, complete
overnight log or a measured optimization win.

## Exact baseline

- OnePlus 15 CPH2747 / Canoe, ColorOS `CPH2747_17.0.1.201(EX01)`, active boot_a.
- Running `6.12.119-android16-6-o-ga9059fb3d6d6-4k`, source
  `a9059fb3d6d64434c533383ddf5dbac270995d5d`. The experiment branches from
  its documentation-only descendant `4bee10123e11ee6a74944ec51bc15518b99f2e98`.
- Full installed B2 boot SHA256:
  `29002f26c6b5bb12dd59dd1887b4b24dbcf31297bd2118277eaed93dfd90946e`.
  Frozen B2 and the installed partition matched. Prior tested .119
  rollback: `aabd0c59d8cb6c9b27448c4c947fda90471e1fbb194837803f2e5b1b824448e4`.
- Decompressed live configuration SHA256:
  `1e7ab7814e8b12a852aa2e0365998b95ba3f5a548852cb910cfec12460f8b9a3`,
  matching the frozen B2 build. Android Clang/LLD 19.0.1 (r536225),
  Android Rust 1.82.0-dev and the pinned Kleaf toolchain are retained.
- 689 live module names matched the B2 snapshot. That is not the number
  of oracle records: 1,069 stock records include 1,031 compatible active
  and 38 dormant records. The locally linked inventory has 646 rows.
- KernelSU pin `28db74f6f4692e97006ce9ebd8b19b9cc614b5b7`, API 6/32716;
  SUSFS v2.4.0 pin `f83ae68add82d1c564e9428ba81dd537fa721924`.
  Droidspaces v7.2.0 prereqs and previous isolated Alpine ARM64 work are
  preserved; a running phone container or manager package version was not
  established by these probes. Root diagnostics use the existing authorization.

## Collection and privacy

At 12:06 local time, November 24, 2026, wireless ADB collected the raw
diagnostics before build work. No reboot, flash, setting write, log clear
or explicit statistics reset was requested of the device. Privileged
hardware reads can have side effects: stock KGSL disassembly confirms that
the initial `gpubusy` read can clear its two short-lived reporting fields
depending on GPU state; the condition at the read was not recorded.
Android's `--checkin` can consume a pending previous completed checkin
report if one exists, though it does not reset ongoing BatteryStats.
The acquisition cannot establish whether such a record existed. Neither
probe was repeated. Use plain/history BatteryStats, cumulative KGSL power
statistics, and saved PowerStats protobufs for subsequent captures.

Raw logs, identifiers, collection status, file hashes and analysis
copies are private under
`out/coloros17001201-battery-evidence/runtime-20261124-120613.AZLFab/`.
Toybox does not implement `dmesg --raw`; its unmodified normal and
priority-prefixed `-r` outputs were saved separately. ANSI escapes were
only stripped from analysis copies. Pstore returned no readable records.
PowerStats framework metadata and binary history were collected even
though direct HAL/suspend Binder dumps and debugfs workqueue trace details
were not exposed. The collection did not enable extra tracepoints.

See the [log triage](kernel-log-findings.md), [battery intervals](overnight-battery-analysis.md),
[wakeup attribution](wakeup-source-analysis.md), [CPU/GPU audit](cpu-gpu-efficiency-analysis.md),
[driver ownership](builtin-driver-update-audit.md), [storage and reclaim](filesystem-memory-analysis.md),
[optimization ledger](optimization-ledger.tsv), [test plan](performance-and-battery-test-plan.md),
and [qualification](qualification-results.md).
