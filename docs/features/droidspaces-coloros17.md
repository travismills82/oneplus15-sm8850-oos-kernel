# Droidspaces experiment: ColorOS 17.0.0.102 / Linux 6.12.115

Date: 2026-11-03. Status: experimental boot-only candidate built and offline
qualified; initial Android/FBE and live checker PASS, full distro/hardware tests pending.

Later owner-authorized physical update: **boot_a write/readback, Android/FBE and
the installed Droidspaces requirements checker PASS**. The exact candidate is
left installed; distro-session and extended hardware tests remain pending.
See [the physical record](../validation/firmware-17.0.0.102/coloros1700102-droidspaces-canary-20261103.md).
The offline-phase no-flash statements below describe the original build task;
they were superseded only by the owner's subsequent explicit flash request.
The original build task authorized no phone installation, reboot, slot change,
partition write, push or publication. Later flash and source-push requests are
recorded separately. Container testing on ARM64 QEMU is not physical OnePlus
qualification.

## Exact baseline and isolation

- Branch: `experiment/coloros17-linux115-droidspaces`.
- Base: `d6552d3625e0656aeac303b878347bef208b4315`.
- Worktree: `out/coloros17-linux115-droidspaces`.
- Evidence/build root: `out/droidspaces-build` (not checked in).
- Qualified rollback boot SHA256:
  `3136cd36cfbb82be4423b46e0849527ea3bc26045ca925ddbd78611d591148f5`.
- Qualified rollback Image SHA256:
  `d9beaa4aafd4ec20f632d63bcebc79f8eeb3c12d18bc6ffc61bffcd89f079f3c`.
- Original tested release: `6.12.115-android16-6-o-gf116d2166542-4k`.
- Current production architecture is **boot only with original ColorOS stock
  DLKM**, including stock KGSL and GEAS. It is not the earlier unified no-GEAS
  graphics experiment. Those earlier packages are not used as build/oracle input.
- KernelSU 32716, API 6, pinned `28db74f6f4692e97006ce9ebd8b19b9cc614b5b7`;
  SUSFS v2.3.0, pinned `68f62fae75ada3959f816841a3c64299383521ed`.
  Both remain built in and unchanged; uname-spoof support remains enabled.
- The unrelated dirty root checkout is preserved. `main` and the original
  physically tested payload bytes remain unchanged.

Local `out/` and `reference-cache/` paths in this report are relative to
`/home/travis/Android/stock_kernel`, not to the nested experimental worktree.

The previous slot-B system is not altered. No device was visible to `adb devices
-l` during this audit; current physical slot/firmware/health are not freshly
claimed. Physical baseline evidence is the preserved KSU/SUSFS-uname canary
report under `kernel-test-backups/ksu-susfs-uname-canary-20261101/RESULT.md`.

## Runtime and documentation provenance

The requested [MGHazz fork](https://github.com/MGHazz/Droidspaces) was inspected
at `14a8dba3bedfd42cfc9b6f10ce06b72e688c7523` (2026-04-01). It is older than
the current official backend, and its old CLI does not offer `check --json`.

The current [official source](https://github.com/ravindu644/Droidspaces-OSS) was
reviewed at `b01a81197f57115da465af6ef196f65a2bd10c27` (2026-11-03), v7.1.0.
Read `src/cli/check.c`, the [kernel guide](https://www.droidspaces.org/docs/kernel-configuration.html)
and [Android installation guide](https://www.droidspaces.org/docs/installation-android.html).
The official exact-source CI artifact came from
[run 41623224506](https://github.com/ravindu644/Droidspaces-OSS/actions/runs/41623224506).

- CI artifact ZIP SHA256:
  `992906ac60a7d40d711b342439237103077205d8c4a9b9ac354cd1d6e74cda72`;
  matched GitHub's artifact digest, then the included tar matched `SHA256SUMS`.
- Actual static ARM64 checker SHA256:
  `72e1f67080a306894b1219d53ce0c8d6c491d39795ed32e12128425503aee820`.
- This is the real official checker, not a script that infers PASS from Kconfig.
- Its JSON `requirements_met` field is the acceptance signal: **the command
  itself exits zero even when mandatory capabilities are missing**.
- The full checker makes transient bridge/veth probes and maintains a boot cache;
  it is not a strictly read-only operation on Android. Here it ran only inside
  disposable ARM64 guests with no host disk or physical NIC attached.

An attempted local backend cross-build failed because the host cross-libc
headers were incomplete. No compiler/security flags were weakened. The
hash-verified official exact-source CI binary was used instead.

## Configuration and implementation

The boot-only build's authoritative source is
`kernel_platform/common/arch/arm64/configs/gki_defconfig`, processed by the
normal `//common:kernel_aarch64` Kleaf pipeline. Do not edit generated `.config`.

| Capability | Qualified baseline | Experiment | Reason |
|---|---|---|---|
| Mount/UTS/network namespaces | y | unchanged | Already operational in VM |
| PID namespaces | n | y | Mandatory isolated PID init |
| IPC namespaces / SysV IPC | absent / n | y / y | Mandatory IPC isolation and IPC userspace |
| POSIX message queues | n | y | Linux distribution IPC |
| User namespaces | n | y | Requested user isolation / sandbox capability |
| cgroups v1/v2, memory, CPU, freezer, accounting | y | unchanged | Existing controllers retained |
| Seccomp/filter, capabilities | y | unchanged | Existing mechanisms retained |
| Loop, ext4, OverlayFS, FUSE, TUN, VETH, bridge | y | unchanged | Existing storage/network stack |
| IPv4 xtables/NAT/MASQUERADE | y | unchanged | Existing legacy-iptables network backend |
| xtables ADDRTYPE match | n | y | Droidspaces IPv4 NAT rule support |
| devtmpfs | n | y | Container guest `/dev` support |
| devtmpfs automount | absent | n | Do not change Android's `/dev` setup |
| procfs, sysfs, devpts/PTYS, epoll/signalfd | y | unchanged | Existing interfaces |
| PIDs/device cgroup controllers | n | unchanged | Optional, not needed for base runtime |
| MACVLAN, NAT66, nftables, bridge netfilter | n | unchanged | Optional modes, no gratuitous KMI expansion |
| FHANDLE | n | unchanged | Legacy file-handle API not required by base checker |

Persistent edits add `CONFIG_SYSVIPC=y`, `CONFIG_POSIX_MQUEUE=y`,
`CONFIG_USER_NS=y`, `CONFIG_NETFILTER_XT_MATCH_ADDRTYPE=y`, `CONFIG_DEVTMPFS=y`
and remove the explicit PID_NS disable (its real Kconfig default is y).
IPC_NS defaults y once IPC is available. Kconfig also selects SYSVIPC_COMPAT,
SYSVIPC_SYSCTL and POSIX_MQUEUE_SYSCTL. No other effective options may change;
`tools/testing/droidspaces/audit-config.py` enforces this exact delta.

Obsolete config names such as MEMCG_SWAP, CGROUP_NS and
DEVPTS_MULTIPLE_INSTANCES are not blindly appended. Modern 6.12 supplies those
facilities through its real Kconfig/dependency implementation. Optional nested
Docker/Podman, process-count limits, MACVLAN and NAT66 are **not qualified** by
this minimal change.

### Genuine KABI preservation for task IPC state

Naively enabling SysV IPC adds 24 bytes near `task_struct::nameidata`, changing
vendor-visible offsets. The official Droidspaces patch instead places state in
the pre-`se` alignment gap, but uses packed unions; it was reviewed, not copied.
Our naturally aligned Android-KABI adaptation uses the existing gap:

| Item | Qualified | Experiment requirement |
|---|---:|---:|
| task_struct size | 5184 | 5184 |
| rt_priority | 160 | 160 |
| sysvsem | padding | 168, size 8 |
| sysvshm | padding | 176, size 16 |
| se | 192 | 192 |
| Existing dumpable reserve 3 | 3440 | 3440, untouched |

`ANDROID_KABI_IGNORE` is the documented gendwarfksyms mechanism for new fields
in actual padding. Compile-time checks enforce natural alignment, the original
`se` offset and full fit inside that gap. No live fields, vendor reserves,
existing member offsets, runtime object sizes, or CRC records are overwritten.
Other architectures/non-gendwarfksyms builds retain the original IPC placement.

An initial compiled experiment proved why C CRCs alone were insufficient: all
existing C exports and 17 C layouts matched, but bindgen's newly inserted
anonymous unions renumbered Rust types. Stock `rust_binder.ko` then had 23 CRC
mismatches and two unresolved Rust `Default` exports. That failed attempt is
retained under `out/droidspaces-build/attempt-01` and is **not** a candidate.

The Rust binding view retains the previous padding representation of the gap;
all actual C fields remain present, aligned and fully initialized by existing
IPC lifecycle code. Rust has no direct users/constructors of this private state.
The two new unused manual `Default` implementations are omitted by narrowly
gated bindgen options for sysv_sem/sysv_shm; their true C definitions remain
visible. This prevents renumbering unrelated rb_node/rb_root exports. No
generated Rust files, export names, CRC values or ELF versions are hand-edited.
Acceptance still requires the final complete compiled export/oracle comparisons.

Enabling IPC_NS also exposes two genuine modular Binder dependencies. Add only
`EXPORT_SYMBOL_GPL(init_ipc_ns)` and `EXPORT_SYMBOL_GPL(put_ipc_ns)`, with explicit
export includes. Their existing implementations, refcounting and teardown are
unchanged. KMI lists and ABI references are **not** updated; normal strict checks
and retained-stock signing/provider policy must still pass.

### KernelSU/SUSFS and Android policy

The official Android guide currently calls SUSFS **unsupported**, not officially
supported just because a kernel option exists. It specifically requires turning
off **Hide sus mounts for all processes** when attempting Droidspaces. In this
pinned SUSFS source the non-SU global mount-hiding static key defaults false.
No runtime flag or system-wide protection was changed here. Users who enabled
that setting must turn it off themselves for testing; do not promise all other
SUSFS modes are compatible. Test with the actual uname initially, not a spoofed
version that could mislead version checks.

KernelSU remains built in. Android root, inherited app seccomp filters, SELinux
policy, mount namespaces and cgroup ownership can still restrict the runtime
independently of Kconfig. Use the documented Droidspaces daemon/root flow and
only reviewed app-specific policy; do not disable SELinux or seccomp globally.
Rootful containers are not a VM-strength isolation boundary. Use trusted ARM64
rootfs images and deny raw GPU/USB/block-device access by default.

Source review of `src/dev/dev.c` confirms that normal mode uses a private tmpfs
`/dev`. Do **not** opt into `--hw-access` for the base test: it mounts the shared
devtmpfs device population. Also leave `--gpu` off: that upstream mode mirrors
GPU/DMA devices and Android Binder devices, not just a harmless rendering flag.
Stock Rust Binder was built without the new IPC namespace configuration; this
experiment does not qualify Binder/Waydroid namespace isolation or passthrough.
Enabling USER_NS/IPC also expands kernel attack surface; no security-hardening
claim is made merely from enabling containers. Existing security fixes and
protections remain unchanged, and trusted workloads/resource limits are needed.

The built-in command line remains:

```
console=ttynull stack_depot_disable=on cgroup_disable=pressure kasan.stacktrace=off kvm-arm.mode=protected bootconfig
```

`CMDLINE_EXTEND=y`. Disabling cgroup pressure reporting does not disable the
CPU/memory controllers (proved in VM). The boot header keeps empty cmdline and
zero ramdisk; no vendor_boot/init_boot/bootconfig edits are made. Additional live
vendor boot arguments and Android policy still need a device check.

## Offline validation evidence (before the device test)

Build/test logs and JSON are under `out/droidspaces-build`. The exact commands
use this experiment's own Bazel output root/cache and pinned toolchain. Common
build, ABI dump and strict list check:

```sh
cd out/coloros17-linux115-droidspaces/kernel_platform
tools/bazel --output_user_root=/home/travis/Android/stock_kernel/out/droidspaces-build/bazel \
  build --jobs=4 --config=local \
  --cache_dir=/home/travis/Android/stock_kernel/out/droidspaces-build/kbuild-cache \
  --kbuild_symtypes //common:kernel_aarch64 //common:kernel_aarch64_abi_dump \
  //common:kernel_aarch64_abi_kmi_symbol_checks
```

The enforced comparison uses the same flags with
`run //common:kernel_aarch64_abi_diff`. No update target is run. The distribution
wrapper's missing libcap/libcap-ng selftest inputs are not bypassed: direct ABI
comparison is recorded separately. Canoe module/DLKM images are not release
payloads for this boot-only architecture.

| Actual official checker probe | Qualified Image in ARM64 VM | Final experiment |
|---|---|---|
| mandatory PID namespace | FAIL | PASS |
| mandatory IPC namespace | FAIL | PASS |
| all other mandatory checks | PASS | PASS |
| devtmpfs / user namespaces | FAIL / FAIL | PASS / PASS |
| ext4 loop persistence / OverlayFS | PASS / PASS | PASS / PASS |
| optional MACVLAN / NAT66 / PID controller | absent | intentionally absent |

The test fixture uses no host block devices. Its 16 MiB ext4 image and all
network probes exist only in guest RAM. The final VM harness ends with a serial
marker and the host stops its own QEMU process; it does not exit guest PID 1
and manufacture a panic. Earlier fixture corrections (Android BusyBox loop
paths, absent poweroff applet) are preserved in logs and are not kernel results.

The final exact Image, using the same final guest fixture as the control,
returns `requirements_met=1` (control: 0). All mandatory checks and all
recommended probes except optional MACVLAN, IPv6 NAT and PID limits return 1.
Full JSON is retained in `vm/candidate-final.log`. Its header records the
exact Image/initramfs/QEMU hashes. Additional actual ARM64 functional tests pass:

- PID init=1; private mounts/hostname; mount, IPC, UTS, net and cgroup unshare.
- User-namespace UID/GID mappings; denial of parent-namespace UTS modification.
- A real seccomp filter denies the test syscall with EACCES.
- POSIX queue send/receive and invisibility across IPC namespaces.
- SysV shared memory fork/cleanup, namespace separation and 100 SEM_UNDO
  fork/exit cycles with the parent semaphore count restored each time.
- cgroup v2 memory/CPU limit files can be configured and the scoped group removed.
- IPv4 NAT table and ADDRTYPE revision-1 match are available.
- Loop-backed ext4 write/unmount/remount persistence, OverlayFS copy-up and
  explicit devtmpfs mounting.

No panic/Oops/BUG/WARNING/KCFI failure occurs. OverlayFS logs a fallback because
the test's initramfs upper directory lacks xattrs; the same fallback occurs in
the qualified control and copy-up passes. This is not proof of all Android
backing-filesystem combinations. Checker FUSE/TUN/bridge/veth probes are not
equivalent to an actual Android distro session, DNS or end-to-end NAT traffic.

### Final build, ABI and retained-stock results

`common-build-06.log` completes successfully in 1,217.723 seconds using pinned
Clang/LLD 19.0.1 r536225, Android Rust 1.82.0.p2 and bindgen 0.69.5. Tool binary
hashes and source snapshot are in the candidate manifest.

| Gate | Final result |
|---|---|
| Common kernel/module compilation | PASS |
| Strict KMI symbol-list target | PASS |
| Common ABI generation and parsing | PASS |
| Existing 10,668 complete Module.symvers records | all unchanged |
| Existing 10,163 vmlinux export records | all unchanged |
| Shared KCFI IDs | 2,525 unchanged; zero mismatches |
| 17 compiled BTF layout roots | existing sizes/offsets/types unchanged |
| Enforced GKI comparison | **FAIL**, exit 4; 45 owner roots |
| Qualified 44 GKI owner roots | all byte-for-byte unchanged |
| New GKI root | task_struct's two C-private padding additions only |
| Compare qualified generated ABI to final generated ABI | only that same task_struct addition |
| Existing driver/core host tests | 202 PASS |
| New focused source guards | 9 PASS |
| Applicable unchanged KSU/SUSFS source checks | 10 PASS |
| Final effective config allowlist | PASS |
| Stock module oracle | PASS, original 1,070 binaries only |
| Actual compiled provider/signing policy | PASS |
| AVB verification and boot-only payload scope | PASS |
| Physical OnePlus / Android distro tests | **NOT TESTED** |

The 17 size/offset controls are task_struct 5,184; thread_info 48; cred 184;
inode 704; file 216; file_operations 264; super_block 1,536; vfsmount 64;
mount 368; address_space 248; vm_area_struct 256; mm_struct 1,216; nsproxy 72;
pid_namespace 136; user_namespace 632; signal_struct 1,120; sighand_struct 2,080
bytes. Every pre-existing named member remains unchanged. The two private task
members are naturally aligned at 168 and 176; `se` stays at 192.

The 44→45 GKI count is **not** a GKI PASS. `enforced-gki-delta.json` compares
the complete unabridged small reports and verifies that no previous owner
changed or disappeared. `generated-baseline-diff-final.txt` independently shows
only the new C-private IPC unions. No symbol list, allowed-break file, checked-in
ABI reference, CRC table or module version metadata was edited.

Eight new natural exports are additive. Six are existing user-ID conversion
implementations selected by USER_NS; two are the real IPC exports needed by
modular Binder. No original export changes or disappears:

| New export | Natural CRC | Export class |
|---|---|---|
| from_kgid | 0x00fe9181 | EXPORT_SYMBOL |
| from_kgid_munged | 0x00fe9181 | EXPORT_SYMBOL |
| from_kuid | 0x97bd1b3b | EXPORT_SYMBOL |
| from_kuid_munged | 0x97bd1b3b | EXPORT_SYMBOL |
| make_kgid | 0x7a3939cb | EXPORT_SYMBOL |
| make_kuid | 0x4f90a136 | EXPORT_SYMBOL |
| init_ipc_ns | 0x59e1383f | EXPORT_SYMBOL_GPL |
| put_ipc_ns | 0x7c5bd82c | EXPORT_SYMBOL_GPL |

Final original-stock oracle:

```
Modules:                  1,070
Active compatible:        1,032
Dormant:                     38
Hard imports:      60,701 / 60,701
CRC records:       61,777 / 61,777
Missing providers:            0
CRC mismatches:               0
Namespace failures:           0
Active blockers:              0
Production-provider mismatches: 0
```

The oracle uses the unchanged original system_dlkm, vendor_dlkm and vendor_boot
trees and actual normal/recovery load lists. No rebuilt provider is substituted.
Its 3,220 provider ambiguities remain compatible; an independent vmlinux-first
model checks 54,785 core-provider edges and 54,986 unsigned import edges against
the **linked** 10,381 permitted imports and 633 protected exports. Zero policy
failures/collisions occur; in-memory bad-CRC/empty-permit negative controls fail
as expected. The 103 signed stock module records retain their original signing
contract. Exact stock DER certificate SHA256
`7f2e5f38e627e01318d0e2a3fbc6604a2d755b3aaae486f8ecbfcb10662cece3`
is linked into the kernel and the VM boot log loads its original key ID
`b0a5f478e751921ce5edace14f00fd3945fa5d82`.

Two old KSU test assertions intentionally require an entirely unchanged
defconfig/sched.h and cannot be claimed PASS for this authorized feature change.
They were not silently weakened: nine new explicit-delta/layout/source guards
replace that purpose, while the ten applicable original KSU tests run unchanged.
Host negative fixtures initially timed out on remote Ubuntu debug-symbol
lookup. The complete 202-test rerun with `DEBUGINFOD_URLS=` passes; ASan/UBSan
and all negative assertions remain enabled. This is host-test environment only.

### Frozen experimental artifact

- Source base: `d6552d3625e0656aeac303b878347bef208b4315` plus five uncommitted
  kernel-input files; snapshot common tree
  `6d074b67a27e4ccd1582be8178ab0bf8284870aa`.
- Source patch SHA256:
  `c840d276215490de9b140d36de1d4d10c057888692627c8cd75c647f38d1847f`.
- Kernel release: `6.12.115-android16-6-o-gd6552d3625e0-4k`.
  The release suffix alone does not identify the uncommitted snapshot.
- Candidate: `out/droidspaces-build/candidate/boot.img`, 100,663,296 bytes.
- Boot SHA256:
  `0cb2cdff300f522beccb4cbdedb0549579161bb48e30f756ea020c7bf5e643cc`.
- Image SHA256:
  `a952292f1067c8a4507cb60346e6de82d69104f6a22cbf0093ffe94bb69489a1`.
- Module.symvers SHA256:
  `e01ecec95a9603acdb9333f6c2672d573f39e02f0f98c93e50996358ce1b4aa8`.
- Effective config SHA256:
  `dcb3585a692d6ba12bfbe01aa647654962a8c962c68593aaa0fc7552af7f3536`.
- Generated filtered ABI SHA256:
  `428290ec9a27db0ca4cbda137e171ee06c270b8cf3d57493579968656ce01e42`.
- Source/toolchain, VM-input and all frozen artifact hashes:
  `out/droidspaces-build/candidate/manifest.json` and associated `SHA256SUMS`.

The raw Image grows from 42,977,792 to 43,047,424 bytes (+69,632). The signed
boot payload occupies 43,053,056 bytes before AVB metadata/padding, safely
within the existing 100,663,296-byte boot partition; no resizing is needed.
Header v4, empty header cmdline, zero ramdisk and zero inner signature remain
unchanged. DTB/DTBO inputs and their stock ownership are unchanged.

Packaging preserves the exact qualified outer AVB contract, even where its
property strings identify the Android-16 GKI boot build rather than Android-17
userspace: SHA256_RSA4096, original development/test key, original salt,
flags 0, rollback index/location 0, OS property 16, boot security patch
2026-10-01, fingerprint
`qti/canoe/canoe:16/BP2A.250605.015/1792254911768:user/release-keys`.
`avbtool verify_image` passes and the unpacked kernel matches the frozen Image.
This is offline verification, not a fresh claim of bootloader/device acceptance.

Only **boot.img** is packaged. No DLKM, vendor_boot, init_boot, vbmeta, recovery,
module-load list, firmware blob or Android userspace is modified. At the end of
the offline phase main remained at the original d6552d source point, with no
commit, push, tag, publication or physical device action.

## Files and review summary

Five tracked kernel inputs change: gki_defconfig, sched.h, ipc/msgutil.c,
ipc/namespace.c, rust/Makefile (48 insertions, 3 deletions). Additional new files
are this audit, the physical test plan and scoped test/packaging tools under
`tools/testing/droidspaces/`. Local build-scaffold symlinks are not source
changes for a commit and must not be added. `git diff --check`, Python syntax,
shell syntax and the focused source checks pass. No kernel implementation fix,
security backport or driver directory is reverted/replaced.

## Physical qualification / recommendation

Use [the separate test plan](../validation/firmware-17.0.0.102/coloros1700102-droidspaces-test-plan.md).
Do not promote this experiment into a stable release based on a QEMU check.
Android/FBE, live Droidspaces startup, actual Linux init/network/DNS and
USB-disconnected suspend/resume require a separately authorized physical cycle.
The initial offline recommendation was **ready for a separately authorized
experimental boot-only device test**, with disclosed GKI/SUSFS limitations;
the later initial boot/checker results are in the linked physical record.
This is still not ready for stable
inclusion or a claim of full Ubuntu/Debian/Fedora/Alpine runtime support until
the real Android/container matrix passes. Resuming the older matched unified
GPU/DLKM architecture would require its own rebase/provider/physical gates;
this boot-only result is not automatically qualification of that different stack.

## Source publication record

After physical testing, the owner separately authorized pushing this work to
`main`. The historical no-commit/no-push statements above describe the build
and canary phases; they are not a claim that the source remains unpublished.

The source is split into logical commits without rewriting existing history:

- `151a48c9976da93eb3a5308c88048523d229e34a`: task-layout, Rust binding and
  genuine modular Binder IPC-export preservation.
- `4f960022774c379000abdb40d230ab563bc86dd7`: persistent container feature config.
- A following documentation/tools commit records the audit, exact artifact,
  initial physical results and remaining tests.

The committed common tree at `4f960022774c379000abdb40d230ab563bc86dd7` is exactly
`6d074b67a27e4ccd1582be8178ab0bf8284870aa`, matching the source snapshot used for
the already tested boot image. No rebuild/repackaging or release-asset
replacement is involved. The original frozen manifest remains an unchanged
historical pre-commit record. KernelSU-patched init_boot stays untouched.
Source publication is not stable-release promotion; full distro/hardware tests
remain pending and enforced GKI remains FAIL with the documented 45 owners.
