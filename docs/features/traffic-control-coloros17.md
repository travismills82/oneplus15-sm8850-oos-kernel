# CAKE, FQ and FQ-CoDel — ColorOS .102 Linux 6.12.116

Status at build freeze: **built and offline/ARM64 VM tested; not flashed or published**.
Target: OnePlus 15 SM8850, `CPH2747_17.0.0.102(EX01)`, boot.img only with
the existing stock DLKM, GPU/GEAS and KernelSU init_boot.

The owner subsequently explicitly authorized flashing this exact frozen image.
Android/FBE and all three actual on-device qdisc packet/lifecycle checks pass;
production interfaces and qdiscs remain unchanged. See the
[physical canary record](../validation/firmware-17.0.0.102/coloros1700102-linux-6.12.116-cake-physical-canary-20261107.md).
The sections below retain the build-time checkpoint; later physical testing
does not imply stable, full hardware, congestion-performance or power qualification.

## Minimal configuration change

FQ and FQ-CoDel were already built into the tested .116 kernel. Only CAKE was
missing. The authoritative production input is
`kernel_platform/common/arch/arm64/configs/gki_defconfig`, used by the current
common/GKI boot-only build. Do not edit generated `.config` or vendor module
fragments to add this feature.

```text
CONFIG_NET_SCH_FQ_CODEL=y
CONFIG_NET_SCH_CAKE=y
CONFIG_NET_SCH_FQ=y
```

The effective config audit proves exactly one change versus the physically
tested .116 baseline: `CONFIG_NET_SCH_CAKE=n -> y`. Existing `CONFIG_NET_SCHED=y`
satisfies the CAKE tristate's enclosing dependency. This tree's CAKE Kconfig
entry introduces no additional selects/dependencies. Existing ingress,
classifiers, actions and MIRRED remain unchanged.

`CONFIG_NET_SCH_DEFAULT` remains disabled. No default qdisc, sysctl, Android
netd policy, interface, bandwidth, offload, route or module-loading setting is
changed. Making CAKE available does not automatically enable shaping on Wi-Fi,
mobile data or tethering, nor prove a performance improvement.

No scheduler implementation is imported or edited: the existing upstream
`net/sched/sch_cake.c`, `sch_fq.c` and `sch_fq_codel.c` and their Makefile rules
are used. No NTFS, KernelSU/SUSFS, Droidspaces, watchdog, security, private ABI,
Qualcomm driver or proprietary firmware change is mixed into this feature.

## Source and artifact identity

- Isolated branch: `feature/coloros17-linux116-cake`.
- Source parent: `c2e2f95d3c3c4ece99591320ebee1dc69a6f2ed4`.
- Kernel change at build freeze: the then-uncommitted single authoritative
  CAKE config line above.
- Actual common source snapshot tree:
  `481a38e99f82a476cdfa1ec1023cd9d4232dcaae`.
- Actual compiled release: `6.12.116-android16-6-o-gc2e2f95d3c3c-4k`.

The release suffix names the build parent, not a committed feature patch.
The source snapshot tree, patch SHA256, final config and Image hashes in the
manifest identify the actual uncommitted configuration used. No commit or main
update is performed without separate authorization. The real index is not
used to compute the snapshot: a disposable temporary Git index is used.

After successful physical testing, the owner separately authorized committing
and pushing the work to main. Source commit
`737f8bbabba274fec2d38cb22df9f854daebfa9b` records only that CAKE config line.
Its common tree exactly equals the frozen `481a38e99f82a476cdfa1ec1023cd9d4232dcaae`
snapshot, establishing source equivalence without rebuilding, changing the
historical manifest or falsely changing the actual compiled release suffix.
Tests/documentation remain a separate commit. This is not release publication
or a waiver of the known GKI, power or performance qualification limitations.

Evidence/package root:
`/home/travis/Android/stock_kernel/out/coloros17-linux116-cake-build/`.
The existing worktree directory still contains `.115-droidspaces` in its name;
its actual branch/source are the .116 CAKE feature above.

| Artifact | SHA256 |
|---|---|
| `candidate/boot.img` (100,663,296 bytes / 96 MiB) | `f7cb7d973ce43de176a27fe8b9ad5e694b35e9d042d6e5875cf50c339f43ec16` |
| `frozen/Image` | `456436bea4d1d1f126b4ebfce9001253871f4b0edf1d2af07c8116f836428038` |
| `frozen/.config` | `da9bf33efeac03dd29e0cefb9fb2f6aaad272b8b2458886fb0e30c06840656c5` |
| `frozen/Module.symvers` | `e01ecec95a9603acdb9333f6c2672d573f39e02f0f98c93e50996358ce1b4aa8` |
| Generated filtered ABI | `428290ec9a27db0ca4cbda137e171ee06c270b8cf3d57493579968656ce01e42` |
| Preserved tested .116 rollback boot | `0c39caaa3c2095b834f2687a10ddce8e445b32d3dfc9cca363edb708c5be288a` |

The tested baseline stays at
`out/coloros17-linux116-build/candidate/boot.img` and its independent physical
read-back remains in the existing canary backup directory. Neither is rebuilt
or overwritten. At build freeze the installed phone had not been contacted
or changed; the later explicitly authorized canary has its own record above.

## Validation results

The normal established boot-only production command was rerun from
`kernel_platform`, preserving the pinned .116 toolchain and build flags:

```sh
tools/bazel \
  --output_user_root=/home/travis/Android/stock_kernel/out/droidspaces-build/bazel \
  build --jobs=4 --config=local \
  --cache_dir=/home/travis/Android/stock_kernel/out/droidspaces-build/kbuild-cache \
  --kbuild_symtypes --show_progress_rate_limit=15 \
  //common:kernel_aarch64 \
  //common:kernel_aarch64_abi_dump \
  //common:kernel_aarch64_abi_kmi_symbol_checks
```

PASS in 154.070 seconds. Kernel Clang/linker/Rust/bindgen binary hashes match
the original .116 manifest. The eight prior BTF duplicate-name notices remain;
no claim of a warning-free compiler/build is made. No generic Canoe DLKM
distribution or Canoe device-module ABI dump is built in this boot-only cycle;
those targets are not falsely reported PASS.

| Check | Result |
|---|---|
| Common build | PASS |
| Strict KMI symbol-list check | PASS |
| ABI generation | PASS, byte-identical generated filtered STG |
| Existing exported records | All 10,676 unchanged, zero new/removed/changed records |
| Core vmlinux exports | 10,171, unchanged |
| Inspected BTF layouts | 17 unchanged sizes/existing member layouts |
| Shared KCFI IDs | 2,535 unchanged |
| Full stock ColorOS oracle | 1,070 checked; 1,032 compatible active, 38 dormant |
| Hard imports | 60,701 / 60,701; six weak records also resolved |
| CRC records | 61,777 / 61,777 |
| Missing / CRC / namespace / blockers | 0 / 0 / 0 / 0 |
| Production provider and load policy | PASS, zero mismatches/policy failures/protected-export collisions |
| Existing ColorOS host tests | 182 PASS |
| CAKE-specific source guards | 8 PASS |
| Disposable ARM64 traffic-control VM | PASS, real traffic and qdisc counters |
| Existing Droidspaces ARM64 VM | PASS, including IPC/namespace/seccomp/filesystem probes |
| Boot container, AVB and scope validation | PASS |
| Physical .116+CAKE device test | NOT TESTED at build freeze; subsequent Android/FBE and qdisc functional canary PASS, see linked record |

The direct enforced `//common:kernel_aarch64_abi_diff` still exits **4 / FAIL**
with the same 45 owners as the tested .116 baseline. Both small reports and
generated filtered STG are byte-identical. No new ABI difference, reference
update, CRC override or weakened acceptance criterion is introduced.

`modules.builtin` contains nominal paths for `sch_cake`, `sch_fq` and
`sch_fq_codel`; this metadata is not a set of emitted loadable modules.
The linked `vmlinux` contains `cake_module_init`, its initcall and all three
qdisc operations tables. No actual matching `.ko` is emitted. The exact stock
module population/load metadata also contains no competing copies/requests
for these three schedulers. No DLKM/load-list/dependency-metadata update is
needed, and no replacement partition image is produced.

## Functional VM test, not just configuration availability

`tools/testing/tc/qdisc-smoke.c` first creates a private network namespace,
then uses only its loopback interface. It uses rtnetlink directly, so the test
does not silently depend on an Android `tc` userspace CAKE plugin. For each
available qdisc it performs eight create/16-datagram/content-check/stats/delete
cycles. It verifies the returned qdisc kind and nonzero byte/packet counters.
An unavailable deliberately named qdisc must return ENOENT rather than being
silently substituted.

The exact same initramfs is tested against both Images:

- Tested baseline .116: FQ PASS, FQ-CoDel PASS, CAKE ABSENT.
- CAKE candidate: FQ PASS, FQ-CoDel PASS, CAKE PASS.
- Both: negative control PASS, smoke return 0, no VM BUG/WARN/Oops/panic.

The local host GNU cross compiler lacked ARM64 libc headers; it was not used
to fabricate a passing result. The probe was built statically with the existing
Android NDK r27d/API34 compiler using `-Wall -Wextra -Werror`. This userspace
test compiler is separate from, and does not replace, the pinned kernel
toolchain. No host packages were installed. QEMU TCG has no host disks or
external network; the existing bounded ARM64 runner is reused. These build-time
VM probes did not touch the phone. The later authorized direct C probe used an
isolated on-phone namespace, as recorded separately; guest-init/PID1 must never
be run on Android. Production network interfaces were not changed.

The pre-existing Droidspaces VM checker again returns `requirements_met=1`;
namespace, SysV semaphore/shared-memory lifecycle, seccomp, loop/ext4 remount,
OverlayFS copy-up and devtmpfs probes also pass. Full phone distro sessions
and the documented SUSFS global-mount-hiding limitation are not claimed fixed.

## Package and next gate

`candidate/boot.img` is Android v4, no ramdisk, unchanged header command line
and the exact qualified .102 vendor/KeyMint AVB properties. The existing
development boot key, SHA256_RSA4096, salt, flags 0 and rollback metadata 0
are preserved. The unpacked kernel hash matches the frozen new Image.
Offline signature/hash success is not OEM signing authority or a physical
device PASS. The 96 MiB image fits the existing boot capacity.

Only boot.img is required for a future authorized canary. Keep stock DLKM,
vbmeta, vendor_boot, dtbo and KernelSU init_boot untouched. Do not flash under
this feature-addition task. The current .116 remains installed as the tested
rollback. Before a new physical test, perform fresh device/firmware/slot/
snapshot/hash checks, a durable boot backup, an explicit minimum write plan
and complete read-back. A changed Image needs its own qualification cycle.

CAKE must be selected deliberately with suitable userspace `tc` support and
link-specific parameters if desired. No production qdisc is selected or tuned
automatically. Throughput, shaping rate, fairness, latency under congestion,
offload interactions, battery effects and radio/OTG regressions require later
physical testing. Existing ColorOS HBP/eBPF/QCOM timekeeping warnings and the
GKI/controlled-power qualifications from the baseline remain disclosed.

Files changed during implementation: the one-line GKI source config, this
record, and isolated VM/source guards under `tools/testing/tc/`. During that
implementation/build stage no commit, push, main update, release or rebuild
of a qualified artifact occurred. The later authorized physical test is
documented separately; subsequent source publication is owner-authorized as
described above. No release or tested-image rebuild is part of the source push.
