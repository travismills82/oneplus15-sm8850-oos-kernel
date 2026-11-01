# KernelSU + SUSFS experiment: ColorOS 17 / Linux 6.12.115

Branch: `feature/coloros17-linux115-kernelsu-susfs`.
Base: `f116d21665429dc9d6ba88d7b004f4de53796bf8`, the repaired boot-only .115
release source. Firmware scope remains `CPH2745_17.0.0.102(EX01)`, OnePlus 15
SM8850 / Canoe. This does not extend compatibility to another firmware.

## Source provenance

| Component | Authoritative source | Pinned revision |
|---|---|---|
| KernelSU | https://github.com/tiann/KernelSU | `28db74f6f4692e97006ce9ebd8b19b9cc614b5b7` |
| SUSFS Android 16 / 6.12 | https://gitlab.com/simonpunk/susfs4ksu, `gki-android16-6.12` | `68f62fae75ada3959f816841a3c64299383521ed` |

The paired SUSFS snapshot reports v2.3.0. It is experimental upstream code,
not a claim of production stability. Its KernelSU integration patch passed
`git apply --check` against the exact pinned official KernelSU snapshot before
import. KernelSU-Next, SukiSU and unrelated root forks are not substituted.

Kernel-only source is vendored into `common/drivers/kernelsu/`, with the
upstream UAPI files resolved from the original `include/uapi` symlink. No nested
Git database, manager APK, prebuilt driver, toolchain or private key is vendored.
The upstream GPL-2.0 license is retained. The bundled nolibc runner is source,
not a precompiled executable.

The original KernelSU Git-based version expression is replaced with its exact
pinned value: `30000 + 2716 = 32716`. The count was verified on the complete
upstream ancestry. This avoids network access and misleading fallback version
16 inside a vendored/hermetic Kleaf build. Manager package/signature checks and
per-app policy remain enabled. Root is not automatically granted to adb shell.

## Configuration and payload contract

The authoritative input is `common/arch/arm64/configs/gki_defconfig`; no generated
`.config` is hand-edited. KernelSU and SUSFS default off in their Kconfig and
are explicitly selected as built-ins by this branch's arm64 defconfig:

```text
CONFIG_KSU=y
CONFIG_KSU_SUSFS=y
CONFIG_KSU_SUSFS_SPOOF_UNAME=y
# CONFIG_KSU_SUSFS_HIDE_KSU_SUSFS_SYMBOLS is not set
# CONFIG_KSU_SUSFS_SPOOF_CMDLINE_OR_BOOTCONFIG is not set
# CONFIG_KSU_SUSFS_PRIORITY_UID_FILTER is not set
```

SUS_PATH, SUS_MOUNT, SUS_KSTAT, OPEN_REDIRECT, SUS_MAP and logging retain the
upstream enabled defaults. Uname spoofing was initially disabled and was
subsequently enabled at the owner's request; the initial artifact and physical
test below remain records of the original, uname-disabled bytes. The normalized
source defconfig now omits its old explicit disable because this bool already
defaults to y under KSU_SUSFS. The generated config must explicitly contain y.
The priority-UID filter, bootconfig spoofing and symbol hiding remain disabled.
No SUSFS userspace module, hide rules or root manager is installed on a phone
as part of this task. Runtime userspace configuration is a separate operation.

The intended deliverable remains **boot.img only plus untouched stock DLKM**.
KernelSU/SUSFS are built-in, not new `kernelsu.ko` or `susfs.ko` dependencies.
Stock GPU/GEAS, Bluetooth, PPP, RFKILL and SLHC providers remain modular and
unchanged. No GEAS blacklist, module-shadow patch, NTFS3, driver update or
kernel subversion change is introduced.

Module signatures, trusted certificates, protected exports/import lists,
MODVERSIONS, GENDWARFKSYMS, CFI/LTO and SELinux enforcement are not disabled.
The generic SUSFS README's instruction to delete protected-export lists is
explicitly **not followed**. No ABI reference or symbol list is updated.

## Native hook adaptations

The upstream SUSFS patch replaces KernelSU's dynamic syscall/kprobe bridge with
its paired native exec/open/stat/setuid/reboot/input hooks. No second set of
manual hooks is added.

Two upstream core-patch contexts differ from this repaired .115 tree:

1. `clone_mnt()` retains the stable `MNT_UMOUNT` clearing bit; the SUSFS mount
   marker is applied after the complete existing mask. The stable fix is not
   reverted to make patch context match.
2. This tree has the older OPlus-aware three-argument `smap_gather_stats()` and
   its full/partial rollup retry algorithm. SUS_MAP filtering is performed in
   that helper, preserving the entire lock/retry iteration rather than importing
   a different newer `show_smaps_rollup()` implementation.

Two small source corrections were also required. The unused credential helpers
are guarded by their sole caller feature, PRIORITY_UID_FILTER, rather than
disabling `-Werror`. The remote-memory map filter is evaluated only after GUP
has populated the VMA, releasing any acquired page before rejecting access;
the VM_IO fallback is filtered after its own valid lookup. The upstream early
test checked an explicitly NULL VMA and therefore could not work on this tree.
No existing GUP, mmap lock, stack-expansion or error-handling path is removed.

No existing public structure gets a new member. Upstream SUSFS uses reserved
high bits in existing thread-info/mapping/mount flags. ARM64 source values were
reviewed separately from `task_struct::flags` and `atomic_flags`; the prior
OPlus/core task-flag separation remains unchanged. Source bit audits do not
alone prove opaque stock-module runtime behavior; compiled compatibility and
physical tests remain required.

## Build and qualification

Fresh worktree preparation reuses the installed pinned Clang r536225 (19.0.1),
the manifest toolchain directories and the existing OPlus DT build scaffold.
The native OPlus selector generator chooses Canoe/perf with no added feature
overrides. Generated links/selectors are local build inputs, not new firmware
DT payloads or source edits. No DTB/DTBO is deployed.

The first analysis attempts identified missing generated OPlus/workspace links.
After restoring those, the defconfig check correctly rejected redundant explicit
defaults and option ordering. The source defconfig was normalized without
disabling its check. A normal common build, strict KMI and ABI dump are then
requested. Results will be recorded below only after the commands finish.

Build root: `out/ksu-susfs-build/`, separate from the feature worktree.

```sh
cd kernel_platform
tools/bazel --output_user_root=<build-root>/bazel build --jobs=4 \
  --config=local --cache_dir=<build-root>/kbuild-cache --kbuild_symtypes \
  //common:kernel_aarch64 //common:kernel_aarch64_abi_dump \
  //common:kernel_aarch64_abi_kmi_symbol_checks
```

## Completed offline results

| Gate | Result |
|---|---|
| Common kernel build, strict KMI symbol check, ABI generation | PASS |
| Effective config vs config extracted from published boot | Only eight KernelSU/SUSFS options changed from n to y |
| Complete `Module.symvers` | Byte-identical to repaired .115 baseline |
| All 10,163 vmlinux export records | Byte-identical to repaired .115 baseline |
| Filtered generated ABI | Byte-identical to repaired .115 baseline |
| Twelve named compiled BTF layouts vs published kernel | Identical |
| Original firmware module binaries hash-verified | 1,070 / 1,070 |
| Exact stock oracle | PASS: 1,032 compatible, 38 dormant |
| Hard imports / CRC records | 60,701 / 60,701; 61,777 / 61,777 |
| Missing / CRC / namespace / active blockers | 0 / 0 / 0 / 0 |
| vmlinux-first provider edges / mismatches | 54,785 / 0 |
| Unsigned import policy edges checked / failures | 54,986 / 0 |
| Actual linked permitted imports / protected exports | 10,381 / 633; no protected collisions |
| Validator in-memory negative controls | Corrupt-CRC and empty-permit-list fixtures correctly reject |
| Existing host tests / focused source checks | 202 PASS / 12 PASS |
| `git diff --check` | PASS |
| Android-v4 boot scope, embedded Image and AVB verification | PASS |
| Enforced GKI comparator | FAIL / exit 4: same 44 inherited owner blocks |
| Full ABI distribution wrapper | BLOCKED by absent libcap/libcap-ng selftest inputs |
| Generic Canoe distribution / device-module-inclusive ABI rebuild | NOT RUN; not the boot-only production payload |
| Device boot, root Manager, SUSFS runtime, Magisk coexistence | NOT TESTED |

The full distribution wrapper's missing selftest libraries were not substituted
with unpinned sources. The normal underlying enforced comparator
`//common:kernel_aarch64_abi_diff` was run directly, as in the existing project
qualification procedure. It uses the unchanged reference and enforcement and
returns the real stgdiff status. It is not a replacement PASS: the same 38 prior
plus six .115 representation owners remain. No new ABI difference is introduced.

The exact filtered ABI SHA256 is
`0ce8ff105a7b5a0375b31bc182cf56624ab60d59a4ff824eda89a4c2400b19ed`,
matching the pre-KernelSU repair report. The twelve BTF comparisons cover
`task_struct`, `thread_info`, `cred`, `inode`, `file`, `file_operations`,
`super_block`, `vfsmount`, `mount`, `address_space`, `vm_area_struct`, and
`mm_struct`. Baseline BTF and config were recovered from the exact published
boot image, not assumed from a similarly named source tree.

The normal compiler retains the eight previously documented duplicate-DWARF
type warning families; they are not new parser failures. Global KCFI remains
enabled with permissive mode disabled. The baseline's `LTO_NONE=y` setting is
unchanged, not represented as newly enabled LTO. Upstream KernelSU's local
`__nocfi` annotations in symbol resolution and SELinux compatibility helpers
remain part of that pinned driver; the dynamic syscall bridge is not compiled.
No whole-kernel CFI disable, callback cast workaround or CRC forcing is used.
This is not a claim of complete root-framework security or runtime validation.

The oracle's mandatory project-certificate argument uses the existing public
stock certificate because there are **no controlled/replacement modules** in
this model; it does not introduce a new signing authority. All input binaries
match the committed stock inventory. The independent validator reads the actual
linked vmlinux policy pointer arrays, distinguishes signed GKI from unsigned
vendor providers, and checks module_layout as a version-only record, not an
ordinary symbol-resolution import. It does not edit module version data.

## Frozen experimental boot image

The exact source remains uncommitted in the isolated feature worktree; `HEAD`
alone does not identify these changes. A complete common-source patch and Git
tree snapshot are recorded separately:

| Item | Value |
|---|---|
| Common-source snapshot tree | `40ee2736bc26b4dc61fe3683a32c9dabf6fb5ee2` |
| Common-source patch SHA256 | `088487293954d6ebf44f47c6b479fb4f9d00e1ca56c82e71d1f846b1b63d43e7` |
| Kernel release, normal build-derived suffix | `6.12.115-android16-6-o-gf116d2166542-4k` |
| Image bytes | 42,977,792; +331,776 vs published baseline |
| Image SHA256 | `5b39b6e62ffeeaf0cd3270a000be19de0701d63a6fc955f54477c8ff5d687ca3` |
| Config SHA256 | `36e6b83e9b5e164dbd329dd1402cb722bd04c96c58c6fd5c28a6e2cd8817ced9` |
| Module.symvers SHA256 | `6a6413d41ce692c0476a0ccd81e38a446fe1490400f3924c79424c597a390da1` |
| Boot bytes | 100,663,296, within the unchanged boot partition capacity |
| Boot SHA256 | `889072fec1140bdd56f9ec6390d914f310ef8bdf949f6353accbdf2b69326470` |

Candidate: `out/ksu-susfs-build/candidate/boot.img` (relative to the parent
`stock_kernel` workspace, not inside this feature checkout). Its manifest,
boot checksum and SHA256SUMS accompany it. The common source patch, prepared
build outputs, matching vmlinux, logs and policy/layout evidence remain under
that build root for future debugging. No binary or key is staged for Git.

Packaging preserves the qualified .102 Android-v4 container: zero ramdisk,
empty header cmdline, no DT payload, unchanged QTI fingerprint, OS version 16,
security patch 2026-10-01, SHA256_RSA4096, flags/index/location zero, and the same
qualified development/test boot key. It is not OEM-signed locked-device support.
No DLKM, vbmeta, vendor_boot, init_boot, recovery or userdata change is packaged.
KernelSU and SUSFS symbols are present in vmlinux; neither is emitted as a .ko.

Decision: **OFFLINE BUILD AND STOCK-MODULE COMPATIBILITY PASS; EXPERIMENTAL,
AWAITING SEPARATELY AUTHORIZED PHYSICAL QUALIFICATION**. Do not replace the
qualified image or call this a stable/root-functionality-tested release yet.

The published baseline boot image is preserved separately by its exact hash
`7eba8fe7265c316265a274c893ae7886ba4598942862fb642d0b34fc61bd8e6e`.
Previous local caches were removed before this task, so old physical tests
are not treated as fresh tests of the new KernelSU/SUSFS build.

## Device-test boundary

No device flash, reboot, partition write, root-manager install, Magisk removal,
commit, push or release is authorized by this implementation request. Main and
the published tested image remain unchanged.

Future testing needs explicit approval and must cover encrypted Android/user0,
F2FS inlinecrypt, KeyMint/TWRP, stock module loading, GPU/GEAS, networking,
watchdog health and suspend. KernelSU Manager must match the official source/API;
do not assume a fork's Manager or SUSFS userspace package is interchangeable.
Magisk currently residing in init_boot is not silently removed or migrated;
root-manager/module coexistence needs an explicit controlled test plan.

## Authorized physical availability test — 2026-11-01

The owner subsequently authorized flashing and testing the frozen image above.
Only active `boot_b` was written. A complete backup of the existing qualified
boot image was hash-verified first; a separate full partition read-back exactly
matched candidate SHA256
`889072fec1140bdd56f9ec6390d914f310ef8bdf949f6353accbdf2b69326470`.
Before/after hashes prove unchanged inactive boot, both recoveries, init_boot,
vbmeta, vendor_boot, DTBO and stock system/vendor DLKM. No slot or trust-state
change was made.

Android boot completed, user0 reached `RUNNING_UNLOCKED`, /data was F2FS with
inlinecrypt, and SELinux remained enforcing. The unprivileged KernelSU GET_INFO
query returned version 32716, UAPI 6. The running config hash matches the frozen
config above. Stock Bluetooth/PPP/RFKILL/SLHC/GPU/GEAS providers were present.

The installed Manager v3.3.1 (32680/UAPI 5) reports Working but requests an
update for this UAPI-6 kernel. No APK or root permission was changed. To avoid
granting Android shell root, the read-only SUSFS getters were tested from the
existing TWRP root environment, which uses the installed boot kernel. The exact
candidate release and config were verified in that environment. All getters
succeeded: SUSFS `v2.3.0`, `GKI`, with SUS_PATH, SUS_MOUNT, SUS_KSTAT, ENABLE_LOG,
OPEN_REDIRECT and SUS_MAP. Thus runtime SUSFS availability is confirmed; full
Manager/root-policy functionality remains a separate qualification gate.

Private physical evidence and the verified rollback image are preserved under
`/home/travis/Android/kernel-test-backups/ksu-susfs-canary-20261101/`.
Recovery logs contain an Oplus hbp frame_put warning; no causality conclusion
or warning suppression is made. This short availability test does not qualify
extended hardware behavior or change the inherited enforced GKI FAIL status.
No commit, push or publication occurred.

## Uname support and matching Manager follow-up

The owner requested `CONFIG_KSU_SUSFS_SPOOF_UNAME=y`. This enables only the
existing SUSFS uname hook, which can override uname's release and version
strings after a privileged userspace request. It does not change Android
account names/UIDs, module vermagic, actual kernel code, patch metadata or ABI.
No spoof strings or runtime policy are configured by this build change.

The isolated rebuild uses the same pinned toolchain and Kleaf targets. New
outputs are kept under `out/ksu-susfs-build/uname-spoof/` in the parent workspace
without replacing the previously flashed image. The final config differs in
exactly this one bool. No new physical flash is authorized by this request.

The existing kernel already contains KernelSU (`CONFIG_KSU=y`); a second LKM
installation or init_boot patch is not needed. Its KernelSU UAPI is 6, while
the installed Manager reports UAPI 5. This is the root-management interface
version, not Android's API level or the kernel's KMI generation.

A matching official signed APK was recovered from the successful upstream
[Manager workflow for the exact pinned commit](https://github.com/tiann/KernelSU/actions/runs/41311436762),
artifact `manager` (ID 13085128531), not the intermediate `manager-gradle`.
Its SHA256 is `aeb9a8eac801509d406ca8613a9bac17b389805aa8b9e976fb7421ada786b868`.
It is package `me.weishu.kernelsu`, versionCode 32716,
versionName `v3.3.1-36-g28db74f6`. APK verification succeeds and the signing
certificate SHA256 is
`c371061b19d8c7d7d6133c6a9bafe198fa944e50c1b31c9d8daa8d7f1fc2d2d6`,
matching both the installed app certificate and the kernel's expected Manager
certificate. The compiled ARM64 JNI `getManagerUAPIVersion` returns 6.
The GitHub artifact ZIP digest also matches the upstream API digest.

The APK is staged at `/home/travis/Downloads/KernelSU-32716-API6-official.apk`
for an ordinary in-place app update. Do not uninstall the existing app, flash
a generic KernelSU kernel, or use Direct install/LKM patching merely to update
the Manager. No APK installation, root grant, device flash, commit or push was
performed during this follow-up.

### Completed uname-enabled build

| Item | Result |
|---|---|
| Effective config | `CONFIG_KSU_SUSFS_SPOOF_UNAME=y`; exactly one bool differs from the tested build |
| Common build / strict KMI / ABI generation | PASS |
| Complete Module.symvers and filtered ABI | Byte-identical to the tested uname-disabled build |
| Twelve compiled BTF layouts | Identical |
| Stock module oracle | PASS: 1,070 checked, 1,032 compatible, 38 dormant |
| Hard imports / CRC records | 60,701 / 60,701; 61,777 / 61,777 |
| Missing / CRC / namespace / blockers / production mismatches | 0 / 0 / 0 / 0 / 0 |
| Linked provider policy and negative controls | PASS |
| Enforced GKI comparison | FAIL / exit 4; identical pre-existing report roots and filtered ABI |
| Existing / focused host tests | 202 / 12 PASS on final runs |
| Boot scope and AVB signature/hash verification | PASS |
| New physical test | NOT PERFORMED; installed image unchanged |

The first host-suite run hit the existing WLAN negative-control mutant's
10-second process timeout. That test passed unchanged in isolation. The full
unchanged suite then passed with host core-file generation disabled for the
intentional crashing negative controls (202 tests, 28.681s). All initial failure
and retry logs are preserved; no test assertion or kernel protection was relaxed.

New candidate:
`/home/travis/Android/stock_kernel/out/ksu-susfs-build/uname-spoof/candidate/boot.img`.

- Boot bytes: 100,663,296.
- Boot SHA256: `3136cd36cfbb82be4423b46e0849527ea3bc26045ca925ddbd78611d591148f5`.
- Image SHA256: `d9beaa4aafd4ec20f632d63bcebc79f8eeb3c12d18bc6ffc61bffcd89f079f3c`.
- Config SHA256: `8a1b910d9ad4ba7490c7bf06b01d7d2ddf79deda064bda4c277553a025fb0d5f`.
- Common-source snapshot tree: `be76448748b0d005af30e20235c5447b1e878dd0`.
- Common-source patch SHA256: `c6f1873c194647ce6d419b1887d744f8de216aaffb84f946f52bb88ed6f9be53`.

The complete manifest, independent unpacking, frozen symbols/config, original
and new source patches, and packaging logs are beside this new candidate.
The previously tested boot image remains unchanged at SHA256
`889072fec1140bdd56f9ec6390d914f310ef8bdf949f6353accbdf2b69326470`.
The new option only enables configuration of uname spoofing; it does not apply
a false version automatically. The new bytes require separate flash approval
and their own physical check before claiming runtime spoofing works.

### Authorized flash of the uname-enabled candidate — 2026-11-01

The owner subsequently requested flashing this new candidate. Only active
boot_b was written after a complete live backup of the working uname-disabled
image. Independent full read-back matches new boot SHA256
`3136cd36cfbb82be4423b46e0849527ea3bc26045ca925ddbd78611d591148f5`.
All protected before/after partition hashes match, including inactive boot,
init_boot, vbmeta, recoveries, vendor_boot, DTBO and stock DLKM.

The installed kernel's root SUSFS getters in recovery returned v2.3.0/GKI
and explicitly advertised CONFIG_KSU_SUSFS_SPOOF_UNAME. No spoof values were
set. Android then reached boot_completed=1 and user0 RUNNING_UNLOCKED with
F2FS inlinecrypt and SELinux enforcing. Its running config hash matches the
uname-enabled frozen config above. The phone remains on the new kernel, slot
B. No Manager APK, root grant or other partition was changed. This establishes
availability and encrypted boot, not arbitrary uname-setting behavior or
extended hardware/stability qualification.

Private evidence, full read-back and rollback are under
`/home/travis/Android/kernel-test-backups/ksu-susfs-uname-canary-20261101/`.
The pre-existing recovery hbp frame_put warning remains recorded. No GKI PASS,
commit, publication or stable promotion is claimed.

## Authorized source and canary publication

The owner subsequently authorized commits to main and a release containing the
tested boot image, a boot-only recovery ZIP and matching official Manager APK.
The kernel source was committed without rebuilding the tested artifact:

- `f003bfa5bf637efb9889b9d9c24fc00a152d5f9e`: initial native integration,
  exactly matching the original common-source tree 40ee2736bc26.
- `bb4992ba6b0a7814b4eec02d4201a175478d6e42`: uname support and focused tests,
  exactly matching tested common-source tree be76448748b0.

A separate release-tooling/documentation commit adds a ColorOS-specific
installer rather than modifying the historical OxygenOS installation contract.
It requires exact stock system/vendor DLKM, vbmeta, vendor_boot and DTBO hashes,
read-only firmware identity, raw unlocked/ORANGE state, consistent slot/capacity
and idle snapshot metadata. It saves a complete durable boot backup, writes only
active boot, verifies the entire readback and restores on write/readback failure.
Both slots, dry-run, 16 rejection/failure paths, rollback and recovery-shell
syntax are covered offline (19 execution scenarios plus syntax). Fixture blocks
are regular files and mocked recovery commands; they are not a physical ZIP test.

The canary channel and known GKI/hardware/root-lifecycle limitations remain
disclosed. Kernel/APK payloads, private logs, keys and build caches are release
assets/local evidence, not Git source files. No stable history is rewritten or
squashed and no additional physical flash is performed for publication.
