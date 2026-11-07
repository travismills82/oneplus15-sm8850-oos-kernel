# ColorOS 17 Linux 6.12.116 boot-only candidate — 2026-11-07

Status at build freeze: **OFFLINE VALIDATION COMPLETE; PHYSICAL DEVICE NOT TESTED.**
The enforced GKI comparison remains **FAIL**, unchanged from the .115 baseline.
This is not a stable release or permission to flash.

Subsequent authorized physical testing used these exact unchanged bytes:
Android/FBE, the live feature checks and the listed owner hardware tests passed.
See the [physical canary record](coloros1700102-linux-6.12.116-physical-canary-20261107.md)
for the final reconnect sweep, pre-existing warnings and remaining controlled
power/extended-stability gates. The build-time statements below are retained
as a historical checkpoint, not a claim that no physical test occurred later.
The owner subsequently authorized pushing the source commits to main; that is
not authorization to publish a release or replace the tested payload.

## Scope and preservation

- Device/firmware contract: OnePlus 15 SM8850,
  `CPH2747_17.0.0.102(EX01)`; not a LineageOS or OxygenOS 16 build.
- New branch: `experiment/coloros17-linux116-boot-only`.
- Starting production source: `4307fd57bf2fd8dbcf58b34fb94bb4e985bd092f`.
- Built source: `baba3e32875ab06ed6d53127cbdbced08e980ce4`.
- Built common tree: `06ce7d4783272820dfc29c75917dd93d9d0f58b1`.
- Actual kernel release: `6.12.116-android16-6-o-gbaba3e32875a-4k`.
- The existing worktree directory, `out/coloros17-linux115-droidspaces`, is
  reused for its established build inputs/cache; its **branch and source** are
  now .116. The original .115 branch, main and release remain unchanged.
- Deliver only `boot.img`. Retain the exact stock ColorOS DLKM population,
  stock KGSL/GEAS, Bluetooth, PPP, RFKILL and SLHC module providers.
- Preserve KernelSU 32716/API 6, SUSFS 2.3.0/uname support, Droidspaces,
  all prior KABI/security repairs and watchdog behavior. No new spoofing policy.
- No ABI reference, symbol list, signing policy, DLKM, vbmeta, init_boot,
  vendor_boot, DT or userspace changes. No phone contact, flash, reboot or slot
  change. No push, main merge, release, force operation or history rewrite.
- The unrelated dirty historical root checkout and its index are untouched.
  The two existing untracked build-scaffold symlinks remain untracked.

## Exact upstream interval

Official Linux stable repository:
<https://git.kernel.org/pub/scm/linux/kernel/git/stable/linux.git>.
Release changelog:
<https://cdn.kernel.org/pub/linux/kernel/v6.x/ChangeLog-6.12.116>.

The complete interval is
`03bae623ec326c2738198739625832aab0eb120a` (.115) through
`5c40a1750f44dd6c7292d8c8d0b4ffff1734576e` (.116).
The v6.12.116 annotated tag object is
`d9f08ef087e621ee00e75aca63ee2e966f16eca7`.
This is a Linux stable update, not a claimed new ACK release-tag merge.

All four commits are retained individually, in upstream order, with original
authors, author dates, subjects, messages and full cherry-pick provenance:

| Original Linux stable commit | Local commit | Subject |
|---|---|---|
| `6cd611d2abdd8623a554e14145f82c1cb4e4467a` | `4ee958b31117f9cbfab2fad891c38890d18309fa` | xen-netback: Fix UAF when reading statistics data |
| `a6bb2f1fa62c56279bdcdadce28d1310c5e12b62` | `fdf28b98c40524d026b9be5353ffa979eecf124a` | xen-netback: defer xenvif_mcast_addr_list_free() |
| `b764e17313534794aeb99c901869f213e36a16ef` | `98c74d6e3008742af110af1389dc4e7e1a0fb918` | xen-blkback: Avoid UAF when reading I/O statistics |
| `5c40a1750f44dd6c7292d8c8d0b4ffff1734576e` | `baba3e32875ab06ed6d53127cbdbced08e980ce4` | Linux 6.12.116 |

Import used the existing `tools/format-linux-stable-mbox.py` helper and
`git am --directory=kernel_platform/common`. No conflicts, local adaptations,
omissions or squashing occurred. Independently recomputed normalized stable
patch IDs and author/date/subject comparisons match for all four commits;
evidence: `out/coloros17-linux116-build/upstream-commit-verification.json`.

The source delta is five files, 30 insertions and 10 deletions:

- `kernel_platform/common/Makefile` (SUBLEVEL 115 to 116).
- `kernel_platform/common/drivers/block/xen-blkback/common.h`.
- `kernel_platform/common/drivers/block/xen-blkback/xenbus.c`.
- `kernel_platform/common/drivers/net/xen-netback/interface.c`.
- `kernel_platform/common/drivers/net/xen-netback/xenbus.c`.

These Xen backend fixes are included, but `CONFIG_XEN=n` on this device.
Do not claim additional active Qualcomm hardware protection from disabled Xen
code, or a general security-completeness certification.

## Build and effective configuration

Working directory: `out/coloros17-linux115-droidspaces/kernel_platform`.
Actual established boot-only production command:

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

PASS in 209.070 seconds. The same pinned Clang r536225, linker, Rust 1.82.0.p2
and bindgen binary hashes match the original .115 manifest. There are zero
effective configuration changes across 6,573 symbols. The complete textual
config difference is the generated .115 versus .116 version comment.
Existing KCFI, MODVERSIONS, GENDWARFKSYMS, module signing and LTO settings are
unchanged. No protection was disabled to obtain this result.

Eight BTF same-name/multiple-ID notices are identical to the prior .115 build
(`inode`, `module`, `vm_area_struct`, `task_struct`, `cgroup`, `seq_file`,
`file`, `path`). They are disclosed, not suppressed or confused with a corrupt
STG reference. No new compiler/linker failure is observed.

The generic Canoe replacement-module distribution and device-module-inclusive
Canoe ABI target are **NOT RUN in this cycle**: this uses the same common
boot-only build as the tested .115 release, retaining stock modules. No Canoe
ABI or replacement-DLKM qualification is claimed. The full ABI-distribution
wrapper is not substituted for the direct enforced comparison below.

## Compiled compatibility and tests

All evidence in this section is newly generated for the actual .116 Image.

| Gate | Result |
|---|---|
| Common kernel build | PASS |
| Strict KMI symbol-list check | PASS |
| Common filtered ABI generation | PASS; byte-identical to .115 |
| Existing Module.symvers records | All 10,676 unchanged; zero additions/removals/CRC or namespace changes |
| Vmlinux exports | All 10,171 unchanged |
| Compiled BTF layout comparison | PASS; 17 inspected types, all existing sizes/members unchanged |
| Shared KCFI IDs | 2,535 identical; zero changed IDs |
| Stock-module oracle | PASS; 1,070 checked, 1,032 compatible active, 38 dormant |
| Hard imports | 60,701 / 60,701; six additional weak imports resolved |
| CRC records | 61,777 / 61,777 |
| Missing / CRC / namespace / active blockers | 0 / 0 / 0 / 0 |
| Real linked provider/loading policy | PASS; zero production-provider mismatches or policy failures |
| Protected export collisions | 0 |
| Host ColorOS driver tests | 182 PASS |
| Applicable KernelSU source guards | 10 PASS |
| Applicable Droidspaces source guards | 8 PASS plus exact stable-source scope/provenance proof |
| Droidspaces ARM64 VM requirements checker | PASS; requirements_met=1 |
| ARM64 namespace, SysV, seccomp and filesystem probes | PASS |
| AVB/footer/hash verification | PASS; existing qualified development boot key |
| Physical device, FBE, peripheral and suspend tests | NOT TESTED on .116 |

The stock oracle uses only the untouched .102 stock system_dlkm, vendor_dlkm
and vendor_boot trees and actual stock module-load lists. No rebuilt module is
substituted. All three output TSVs are preserved. The 3,220 existing ambiguous
provider records are not accepted blindly: the independent linked policy check
verifies vmlinux-first precedence over 54,785 edges, checks 54,986 unsigned
edges, 10,381 permitted imports and 633 protected exports, with negative controls.

Seventeen inspected layouts include task/thread, credentials, inode/file,
mount/filesystem, memory/address-space, namespace and signal structures.
All reviewed source/KABI state is unchanged outside the five upstream files.

Historical tests are not weakened: the original nine-test Droidspaces run is
retained with its one expected feature-only scope assertion failure, because
that assertion predates and excludes the five legitimate .116 files. The
remaining eight tests pass unchanged; a separate exact five-file and four-
commit proof covers this update. The two already-inapplicable pre-Droidspaces
KernelSU baseline assertions remain unchanged and are not falsely reported PASS.

VM uses private QEMU TCG, no network and no host disks; it does not test the
phone. The exact frozen Image boots and passes namespace isolation, 100-fork
SEM_UNDO, shared-memory fork cleanup, seccomp filtering, ext4 loop remount,
OverlayFS copy-up and devtmpfs tests. No BUG/WARN/Oops/panic is observed in that
guest. The tmpfs upper filesystem's OverlayFS xattr fallback is retained in
the log; copy-up succeeds. Full distro and hardware operation remain untested.
SUSFS global mount hiding remains an independently documented Droidspaces
incompatibility; no policy change is made.

### Enforced GKI — unchanged FAIL

The direct enforced command uses the identical Bazel/cache/config flags with
`run //common:kernel_aarch64_abi_diff`. It exits **4**. There are **45 owners**,
the same 44 prior reviewed deviations plus the reviewed C-private IPC-padding
owner from Droidspaces. The candidate and baseline small reports are
byte-identical, and the generated filtered `abi.stg` is byte-identical.
There are zero newly introduced/changed owners. This is not overall GKI PASS;
no ABI reference or acceptance policy was updated.

## Frozen image and AVB contract

Evidence root: `/home/travis/Android/stock_kernel/out/coloros17-linux116-build/`.
The new package is `candidate/boot.img`; size **100,663,296 bytes (96 MiB)**.
Its embedded Image is 43,047,424 bytes, equal in size to .115. The kernel ends
at 43,053,056 bytes before its AVB metadata, safely within the existing boot
capacity. No partition resizing is needed.

| Artifact | SHA256 |
|---|---|
| New boot.img | `0c39caaa3c2095b834f2687a10ddce8e445b32d3dfc9cca363edb708c5be288a` |
| New Image | `ff1439ed3a3ac2bce86e3a8ace594213256ab4fd4eaa2d7a8c384597690f3531` |
| Module.symvers | `e01ecec95a9603acdb9333f6c2672d573f39e02f0f98c93e50996358ce1b4aa8` |
| Final config | `92002eb1181dee6400f8173341767316a7a488c6ab675cbd248efbfdd8b6e1d7` |
| Generated filtered ABI | `428290ec9a27db0ca4cbda137e171ee06c270b8cf3d57493579968656ce01e42` |
| Preserved .115 rollback boot.img | `0cb2cdff300f522beccb4cbdedb0549579161bb48e30f756ea020c7bf5e643cc` |

Preserve the exact tested .102 packaging, not a new security declaration:

- Android v4 boot header, empty command line, no ramdisk, no DT in boot,
  zero boot-signature size. Header bytes are identical to .115 (kernel size is
  also identical); unpacked kernel SHA256 matches the frozen new Image.
- SHA256_RSA4096, same development boot key, flags 0, rollback index 0 and
  rollback-index location 0. No parent vbmeta change or OEM trust claim.
- Same salt `41ab2e909a5a828fb838332beb116deff4a812e2287349b358ed9a238671981c`.
- AVB OS version `16`, fingerprint
  `qti/canoe/canoe:16/BP2A.250605.015/1792254911768:user/release-keys`,
  security patch `2026-10-01`; the ColorOS 17 framework retains this qualified
  vendor/KeyMint contract. No SPL/fingerprint/rollback manipulation.

`candidate/manifest.json`, sidecar, SHA256SUMS, complete packaging commands,
unpack logs, AVB information and verification logs are retained. The one-off
`qualify-and-package.py` gates packaging on the actual compiled, oracle, VM,
configuration, source, toolchain and unchanged-GKI evidence. The historical
Droidspaces-specific packaging helper is neither edited nor bypassed as a
claim that its older source baseline still applies.

## Physical test handoff

The original .115 payload is preserved at its original path and a separate
hash-verified copy is staged as `out/coloros17-linux116-build/rollback-6.12.115.img`.
No installed image is assumed current from historical slot observations.
Before any separately authorized test, collect fresh firmware/slot/OTA/snapshot
and health evidence, verify rollback access, verify the exact boot hash and
capacity, back up current boot, write only the resolved active boot partition,
and verify full read-back. Do not touch init_boot or stock companion partitions.

Required .116 physical checks: Android boot, PIN/user0/FBE/CE/DE, F2FS with
inlinecrypt, KeyMint and TWRP decryption; KernelSU/SUSFS, Droidspaces checker and
container startup; Wi-Fi/Bluetooth/LTE, camera/audio/fingerprint/USB; watchdog
log scan and USB-disconnected suspend/resume. Actual tests, not compilation or
the VM, decide qualification. No flash or publication has occurred.

Recommendation: **ready to request a controlled .116 boot-only device test**,
with the unchanged enforced-GKI limitation disclosed. Not release-qualified.
