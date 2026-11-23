# ColorOS .201 Linux 6.12.119 — battery efficiency experiment

This is a one-device boot-only experiment, not a claim of longer battery life.
The installed system and the published .119 release were not changed.

## Baseline and decision

- Live CPH2747 / OP611FL1, firmware `CPH2747_17.0.1.201(EX01)`, slot A,
  `6.12.119-android16-6-o-g97d4684f2e09-4k`. Full live boot partition SHA256:
  `aabd0c59d8cb6c9b27448c4c947fda90471e1fbb194837803f2e5b1b824448e4`.
- Protected source `97d4684f2e0964856950c34bfbbadd8bbdbfab49`. Frozen raw
  Image SHA256: `ba1eba8af48d49b03cce5395c08538bfeac5f53ae6ff5bd6c91ec2554fb1c72d`.
  Final config SHA256: `1e7ab7814e8b12a852aa2e0365998b95ba3f5a548852cb910cfec12460f8b9a3`.
  Existing Android Clang 19.0.1 (r536225), Kleaf Canoe perf, KMI6 and Rust
  configuration are the starting point. Same stock firmware, KCFI,
  MODVERSIONS, SELinux, FORTIFY and module-signing policy.
  The running `/proc/config.gz` decompressed to the exact compiled config
  hash. LLD is 19.0.1, Rust is Android 1.82.0-dev, and the baseline selects
  AutoFDO and `CONFIG_LTO_NONE=y`. Keep the toolchain, optimization policy,
  hardware-tag KASAN and scheduler diagnostics unchanged; neither compiler
  flag changes nor removing instrumentation have a measured benefit here.
  See `out/coloros17001201-battery-evidence/toolchain.sha256`.
- Isolated branch `optimization/coloros17-linux612119-battery-efficiency`.
  Runtime trace and battery snapshots live outside source control in
  `out/coloros17001201-battery-evidence`.
- First optimization: the real kernel command line is
  `cgroup.memory=nosocket,nokmem`. The old `count_shadow_nodes` counted
  node-wide metadata against root-cgroup-local pages. ACK
  [76ab8e637626](https://android.googlesource.com/kernel/common/+/76ab8e637626af71d488b1fdf6afb6ffd505d862)
  uses node-wide pages when the metadata list is not memcg-aware. The existing
  memory cap and accounting-enabled behavior are preserved. Applied as
  `6c266e436449f` with the original author, scope and attribution. This can
  reduce needless reclaim and page-cache refault work on Android under
  pressure; it is not expected to change deep idle by itself.
- Separately retained ACK [0a07471a4c824](https://android.googlesource.com/kernel/common/+/0a07471a4c8242872691d03079720196d4b7b7e9)
  as `b452ac5e72b64`. The frozen .119 baseline's ARM64
  `blk_finish_plug` clears `current->plug` before storing the cleared
  `PF_BLOCK_TS` flag, contrary to the scheduler's invariant. The minimal
  compiler barrier restores source ordering without scheduling, delaying or
  changing any I/O. This is a stability fix; it is not counted as an energy
  improvement. The reclaim-only image is preserved for a one-variable A/B test.

## What the measurement shows

Read-only telemetry confirms stock SCMI + WALT, active MGLRU, LZ4 on stock
ZRAM, stock Adreno 840/KGSL/GEAS and F2FS inlinecrypt. Tickless idle and
power-efficient workqueues are already selected in the final config.
A 129.69-second uncontrolled interval was awake at both ends: the gauge
reported a 14 mAh decrement, 2,481 file refaults and no additional shadow
node reclaim in that particular window. This is **not** a screen-off drain
result. The gauge and sampling interval are too coarse to extrapolate battery
life or rank device energy use. See the [power audit](power-management-audit.md).

## KernelSU, SUSFS and Droidspaces

| Component | Verified integration and disposition |
| --- | --- |
| KernelSU | Built-in official [tiann/KernelSU](https://github.com/tiann/KernelSU) at `28db74f6f4692e97006ce9ebd8b19b9cc614b5b7`, pinned 32716/API 6. Root diagnostics work on the running handset; this observation does not establish the installed manager or daemon package version. Public stable tag remains [v3.3.1](https://github.com/tiann/KernelSU/releases/tag/v3.3.1). Head `2a60b2893b8d` is 19 commits ahead of the pin. Reviewed kernel changes include SELinux-policy handling, symbol-resolution behavior, an xattr hook and platform-specific fixes, rather than a demonstrated 6.12 idle-power fix. The manager root-shell argument handling and ksud profile path-validation fixes are distinct userspace security review items, not kernel efficiency changes. Preserve the matched manager/kernel contract; no app or daemon was changed. |
| SUSFS | Built-in v2.4.0 from [simonpunk/susfs4ksu](https://gitlab.com/simonpunk/susfs4ksu/-/tree/gki-android16-6.12), version revision `f83ae68add82d1c564e9428ba81dd537fa721924`. The verified branch tip `9b0b13f06967` adds further source sync and optional behavior, not a tested efficiency fix for the pinned KernelSU policy/API and Droidspaces contract. Existing rate-limited diagnostics and feature defaults stay in place. No new hook or permission change in this candidate. |
| Droidspaces | Kernel prerequisites only; the APK/runtime/rootfs are separate. The authoritative [Droidspaces-OSS](https://github.com/ravindu644/Droidspaces-OSS) stable release is [v7.2.0](https://github.com/ravindu644/Droidspaces-OSS/releases/tag/v7.2.0), tag `cdadb9eb5d0f86aacffff60776333e6f6b73a30a`. Older project notes recorded a physical 7.2.0 checker result on another firmware. The current read-only package lookup did not establish an installed app version. Existing namespaces, cgroups, seccomp, IPC, OverlayFS, loop, VETH/bridge/netfilter and Rust Binder remain selected. No proof of extra idle work merely from enabling these options, and no userspace upgrade was attempted. Full Android distro sessions remain a physical test item. |

## Deferred or rejected

- Do not re-tune WALT/schedutil rates, cpufreq limits, OPPs, input boosts,
  interconnect floors or thermal trips without controlled traces, owner
  attribution and latency/temperature measurements. The observed caps are
  momentary effective limits, not proof of firmware or kernel misconfiguration.
- Do not convert KGSL, WALT, RPMh providers, ZRAM, UFS vendor host or charging
  drivers into built-ins. Most meaningful device-specific tuning would
  require the actual vendor providers, firmware or Android power policy.
- Keep kernel autosleep disabled for the first experiment. The ACK
  userspace-autosleep option is a separate notifier policy, not an idle-state
  toggle or a proven power win for this working ColorOS stack.
- Keep the existing CPU governor, network qdisc, CUBIC, storage policy,
  charging limits and security mitigations. No SDP 900 mA, undervolt, global
  workqueue/timer overrides, watchdog suppression or unsupported DLKM changes.
- Do not import the newer OnePlus OxygenOS .601 kernel/device updates into
  this ColorOS .201 experiment. Public CodeLinaro graphics release tags alone
  cannot establish the proprietary GEAS, display/HAL or firmware contracts.

See [the built-in audit](builtin-driver-audit.md), [the update ledger](upstream-update-ledger.tsv),
[storage audit](filesystem-optimization.md), [qualification](qualification-results.md),
and [the physical A/B plan](battery-test-plan.md).
