# Qualification log

This branch does not change the installed phone. B2 remains the physical
baseline and first rollback. It was directly reverified over wireless
ADB. Diagnostic data and artifacts must not be committed or uploaded.

## Implemented source change

ACK [b8373bdc9f443](https://android.googlesource.com/kernel/common/+/b8373bdc9f4437d69ee8fffa00c5548ec5a78fb7)
(upstream linux-pm `0589f51fe4871a50b045c5b4a96ed203ddd90b54`)
is local commit `1d894b7d240ee00bb9bb59bff761db8c8820e231`.
`freeze_task()` checks the already-frozen bit in the same lock
before the heavier eligibility test. Remaining lock and state
transitions are unchanged. It does not alter the governor, device
callbacks, suspend cancellation or cgroup policy.

The host test compiles the actual old and new control-flow bodies
with mock scheduling primitives and checks sixteen combinations of
frozen/eligible/direct/kernel-thread state. The same return value
and state/wakeup actions are observed; the eight already-frozen
cases avoid the eligibility check. This is operation-count evidence,
not a latency or energy prediction. ARM64 offline testing is separate.

ACK [ee891cdd90e7](https://android.googlesource.com/kernel/common/+/ee891cdd90e709f14cd3229e8b3bb929d8e7d028)
(upstream workqueue `4f338225cc2a9af081ea1eb20b28895e25b23ed0`)
is local commit `cb7fe4cf4ab9ddde03f7d80a7b6ca59c1e0d1288`.
The freeze/thaw walks still hold the global pool mutex and handle
each freezable queue exactly as before. For other queues, adjusting
max-active under their mutex is a no-op because the effective value
already matches the saved value. The new early test uses the same
`WQ_FREEZABLE` condition as the adjacent busy check; no policy or
workqueue default changes. Both changes leave the real wake event
and the decision to abort intact.

## Evidence

- B2 control guest: three real cgroup v1 freeze/thaw cycles and two
  `pm_test=freezer` attempts passed. A wrong initial initramfs lacked a
  shell; the final test uses the preserved Alpine ARM64 initramfs.
  These are guest tests only; they do not exercise Qualcomm firmware.
- Three charge-history, three meter and four telemetry endpoint fixture
  tests passed. The inherited production C workingset tests pass with
  memcg both enabled and disabled.
- The older KernelSU/SUSFS source fixture passes 10/12; its two
  whole-file assertions against an older defconfig and task header
  fail identically on B2. Current B2 defconfig, task header, native
  hooks and reference files are byte-identical in this experiment.
- Fresh build inputs reproduce the prior Canoe perf symlinks, target
  variant and stock-signing certificate in the new worktree. Initial
  target/link resolution failures are retained separately in local logs.
- The raw capture manifest `runtime-20261124-120613.AZLFab/SHA256SUMS`
  checked all 93 files. Manifest SHA256:
  `7451c0d7f746470efb85635bf62d96108792b7322368ae7dc939b95825281b62`.

## B3 — isolated freezer-only candidate

- Common build, strict KMI, generated ABI and independent corrected
  ColorOS ABI: **PASS**. Corrected STG comparison against both the
  approved original .119 reference and exact B2 returned exit 0.
- Official Google GKI: **FAIL / exit 4**. Official small report
  byte-identical to B2; corrected report against the unchanged
  Google reference also unchanged.
- Exact config, 10,173 vmlinux exports and 10,678 combined exports
  conserved. Stock oracle: 1,031 active, 38 dormant; 60,835 hard
  imports and 61,910 CRC records; zero required-provider, namespace,
  CRC or production-policy failures.
- Compiled private layouts/KCFI: 39 layout roots, 2,533 named KCFI
  values and eight linked prefixes unchanged.
- ARM64 guest smoke: namespaces, persistent ext4/F2FS remounts and
  OverlayFS, three cgroup-v1 freeze/thaw cycles and two PM freezer
  tests passed. Droidspaces v7.2.0 checked requirements and started,
  used and stopped Alpine in a private guest.
- Offline AVB and exact unpacked Image: passed. Experimental boot image
  `out/coloros17001201-runtime-freezer/candidate-1d894b7d240ee/boot.img`,
  SHA256 `90a486a125aa4870a024c9ed0ef78e07626f117f82fdbc6d76bce029299f0ec5`.
  Raw Image SHA256 `c70de4d2808f892f023f5268433498cedfba3333c2460c5a6c92a210d7f65753`.
  Retained for the first one-variable physical comparison. Not flashed.

## B4 — freezer and workqueue candidate

The exact staged B4 build repeated the gates; it does not inherit a
PASS merely from B3.

| Gate | Result |
| --- | --- |
| Common build, strict KMI and ABI generation | PASS. Actual generated configuration SHA256 unchanged: `1e7ab7814e8b12a852aa2e0365998b95ba3f5a548852cb910cfec12460f8b9a3`. |
| Independent corrected ColorOS STG | PASS / exit 0 versus both approved .119 and exact B2, no differences. |
| Official enforced Google GKI | FAIL / exit 4, official small report byte-identical to B2. Corrected report against unchanged Google reference likewise identical. |
| Full stock module oracle | PASS: 1,031 compatible active, 38 dormant, 60,835 hard imports, 61,910 matching CRCs, zero CRC, missing-provider, namespace or policy failures. |
| Exports, compiled KCFI and layouts | PASS: 10,173 matching vmlinux exports and 10,678 combined export records; 39 layout roots, 2,533 KCFI IDs and eight Rust/C prefixes unchanged. |
| ARM64 test workload | PASS: namespace/IPC/cgroup isolation, seccomp, scratch filesystems and OverlayFS, three actual cgroup-v1 freezer cycles, two PM freezer suspend-path probes, and Droidspaces v7.2.0 Alpine start, file operation and clean stop. |
| Host control flow and memory | PASS: preserved freezable queue behavior across three mixed-list fixture cycles, four versus eight modeled per-queue lock acquisitions per cycle; actual compiled freeze predicate and workingset regression tests passed. |
| Offline AVB and packaging | PASS: 96 MiB Android v4 boot image, exact unpacked raw Image, matching ColorOS .201 boot properties and development signing. No companion artifacts. |
| Handset, actual efficiency and low-power behavior | NOT TESTED on B3/B4. The installed image is B2, not either experiment. |

- B4 source: `cb7fe4cf4ab9ddde03f7d80a7b6ca59c1e0d1288`,
  release `6.12.119-android16-6-o-gcb7fe4cf4ab9-4k`.
- B4 image: `out/coloros17001201-runtime-freezer/candidate-cb7fe4cf4ab9d/boot.img`,
  SHA256 `7ed5ecc5ab9c692752761f8aae2d92690d981aa6eaebdb115f518181912dd4d3`.
  Raw Image SHA256 `ec8275a423e80291be34e1622bcf8baedec3054571afb1f69f73b31bcb222e8e`.
- Exact receipts sit beside the candidates under
  `out/coloros17001201-runtime-freezer/qualification-*`. The legacy ABI
  reference, qualification tools and firmware inputs were not changed.
- Use B3 against B2 first; compare B4 against B3 to isolate the second
  change. Do not publish a battery gain or release either binary based on
  the VM or host operation-count probes.
