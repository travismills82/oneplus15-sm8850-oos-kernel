# ColorOS 17.0.0.102 — Linux 6.12.115 integration

## Scope and preservation

Status: **SOURCE UPDATE BUILT; DEPLOYMENT BLOCKED BY KABI**.

This is an **unqualified experimental source update**, not a release or flash
authorization. Target: OnePlus 15 SM8850, ColorOS
`CPH2745_17.0.0.102(EX01)`. The installed slot-B 6.12.114/four-module canary,
paired VBMeta, Magisk init_boot, and qualified production branch are unchanged.
GPU modernization remains required for the separate unified release; the
isolated GPU worktree and its uncommitted interface experiments are preserved.

| Item | Identity |
|---|---|
| Integration branch | `experiment/coloros17-unified-linux-6.12.115` |
| Starting source | `c5306af264f5459ae27e67d257842d96b0de4ef9` |
| Full interval source HEAD | `46fb47434d1067af6076f6491fc046ba6bf912ba` |
| Common-kernel tree | `04acfaad89d44e72f47ca2711c97612a34b46d1a` |
| Official stable .114 endpoint | `85d82e0d7c2dfd3e75d34f35b7ebbf41f45c9c4e` |
| Official stable .115 endpoint | `03bae623ec326c2738198739625832aab0eb120a` |
| Official .115 tag object | `2ab8c3a12beb08e530e7697f273635e60417617c` |
| Source subversion | `6.12.115` |
| Production/main | `efa7ca4647889e6fe92b5e8ee70ffbfe9fadfa9c`, unchanged |

The release endpoint and October 30, 2026 publication were checked against
the [official stable changelog](https://cdn.kernel.org/pub/linux/kernel/v6.x/ChangeLog-6.12.115)
and local official Linux stable Git objects. This is the full stable interval,
not a Makefile-only version bump and not a wholesale Android Common merge.

The original qualified boot remains SHA256
`1affe0774f9a483606d3ff44ac7e65214171f29ef439cae74ff49c6742a2fbb3`.
The existing canary vendor_dlkm and paired VBMeta are not rebuilt or installed
by this source-integration step. No init_boot, userdata, firmware, partition,
AVB trust, or slot change is performed.

## Complete stable history

All **246** official interval commits are accounted for:

- 221 unadapted newly imported commits;
- 10 individually imported commits with reviewed Android/ACK context adaptations;
- 9 individually preserved backports already in the starting source;
- 6 earlier individual fixes confirmed already present, rather than reapplied.

Thus 231 new individual commits were appended. No prior commit was amended,
squashed, reverted, or rewritten. New imports retain upstream author, author
date, subject and original message, plus the full stable commit trailer.
All newly imported commits occur in original relative dependency order.
Already-present patches keep their original earlier position in history.

The exact ordered ledger and original/local patch IDs are recorded in the
private evidence bundle
`/home/travis/Android/kernel-test-backups/coloros17-unified-20261031-rdEWAJ/`.
The existing .114 release and GPU evidence are not overwritten.
The complete [246-entry commit inventory](coloros1700102-linux-6.12.115-commits.tsv)
records both upstream and actual local commits, exact normalized patch IDs,
and each adaptation. Original authorship, parent order and all unadapted
patch IDs were independently verified successfully.

### Context adaptations

| Order | Upstream | Resolution |
|---|---|---|
| 52 | `7100c4a72891` | Insert identical IPv6 payload helpers despite comment-whitespace context; all other hunks unchanged. |
| 80 | `0c0cf2240bb5` | Use existing ACK `ring_buffer_cpu_meta` and its sub-buffer-base helper. |
| 116 | `6449399259b5` | Keep ACK filemap tracepoint exports while moving mapping initialization to the shared macro. |
| 140 | `8a241d7ca23b` | Keep the existing `io_local_work_pending()` helper, which also checks retry work, and add the upstream ownership check. |
| 181 | `648f70d84003` | Keep ACK PTE-install support while resetting walk action before both none-entry branches. |
| 196 | `ebe8e97fb99d` | Apply the same split work-reference/free callbacks beside the ACK issue wrapper. |
| 206 | `d8c618747a6a` | Keep ACK message-cache fields following the inserted RCU polling state. |
| 216 | `2323770abf6e` | Apply restart-error normalization in the existing factored issue path; retain ACK handler cleanup. |
| 220 | `d1437705599c` | Move file lookup before worker queueing while retaining the ACK issue wrapper. |
| 232 | `13fc051269d6` | Preserve existing key-type reserve declarations when inserting the upstream index ID. New ABI remains explicitly subject to validation. |

No context conflict was resolved with an indiscriminate ours/theirs choice.

## Configuration and source invariants

Generated common configuration has **zero CONFIG-value differences** from the
matched 6.12.114 build. Its only textual change is the generated version
comment. The same pinned Clang 19/r536225 toolchain and production build path
are used. Existing baseline `CONFIG_LTO_NONE=y` is not a new relaxation.
KCFI, GENDWARFKSYMS, MODVERSIONS, module signing/protected exports, SELinux,
inline encryption and hung-task detection remain enabled as configured.

No KMI list or ABI reference is updated. No NTFS3 option is enabled. Existing
fscrypt/F2FS, watchdog, module-loader and DWC3 compatibility implementations
are unchanged relative to the starting source. Existing KernelSU/SUSFS files
and the separately accepted conntrack CVE-2026-80668 fix remain intact.

## ABI-sensitive changes requiring evidence

Successful replay is not ABI qualification. The complete stable interval
includes changes that must be evaluated rather than hidden:

- `bsg_setup_queue()` gains a reply-buffer-size parameter. The UFS advanced
  RPMB overflow fix must remain; any legacy KMI wrapper must retain correct
  argument ABI and buffer allocation behavior.
- Futex shared-MM identities replace recyclable MM addresses with unique IDs.
  The direct upstream `mm_struct` field must not silently shift stock-module
  member offsets. Any compatibility adaptation must retain the security fix.
- Keyring type/domain identities add state to key-related structures. Existing
  module allocations and structure accesses need explicit review.
- Unicode RCU teardown, private regcache operations, io_uring polling state and
  separate perf accounting also change type graphs or allocation state.

These are review candidates, not claims that all are actual runtime breaks.
No CRC is forced, no Module.symvers is edited, and no incompatible interface
is accepted merely because the kernel compiles.

## Validation status

| Gate | Current result |
|---|---|
| Complete source interval replay | COMPLETE, 246 accounted / 231 newly imported |
| Upstream authorship / patch provenance | PASS, all 246 accounted and every unadapted patch ID verified |
| Source whitespace check | PASS |
| Existing driver/security host suite plus .115 regressions | PASS, 141 tests |
| Effective common config options | PRESERVED, zero option differences |
| Common kernel build / strict KMI / ABI dump | PASS, normal target exited 0 in 1406.206 seconds |
| Enforced common GKI comparison | FAIL, exit 4; new differences exist beyond the .114 baseline's 38 reviewed differences |
| Production Canoe distribution / ABI | NOT RUN for .115; held at the incompatible installed-module gate |
| Complete installed-provider module oracle | FAIL, all 1,070 records checked; 1,032 active blockers |
| Production vmlinux-first provider validation | FAIL, 26,171 version mismatch edges |
| New private-layout comparison | 11 types checked; 9 differ, DWC3/UFS HBA remain identical |
| Firmware-bound .115 boot.img | NOT PREPARED |
| Physical .115 validation | NOT TESTED; no flashing authorized |

The compiled ABI and actual provider/private-layout gates identified the
failures detailed below. There is no transfer of physical qualification from
the installed .114 canary to these new source bytes. Root-level compatibility
correction and successful revalidation are prerequisites for packaging or
requesting a controlled test.

## Actual build results and frozen raw artifacts

The normal commands succeeded at the stated source HEAD:

```sh
cd kernel_platform
tools/bazel --output_user_root="$E/bazel" build --jobs=4 \
    //common:kernel_aarch64 //common:kernel_aarch64_abi_dump
```

The strict KMI symbol-list violation action also succeeded. This checks symbol
policy, not equality of the existing exported symbol CRCs. Eight BTF-generation
multiple-type-ID warnings are from the same known baseline family; they are
not treated as a new STG parser failure or ignored compiler errors.

The actual generated kernel release is:
`6.12.115-android16-6-o-g46fb47434d10-4k`.

| Raw, unqualified artifact | SHA256 |
|---|---|
| Image, 42,646,016 bytes | `4035f7d5f59adaa8ee582ee6ca952131761dc1bde2c6d13669b84d21cd365746` |
| Module.symvers | `88870d27f6af815e289eee8352436c4344d2268071bf687d4cc5fcd238f6ede0` |
| Effective config | `abbb37e8fae9073bd5ca5bf539196f4b9baff60fa40714fe1506dc01592f1a27` |
| Filtered generated ABI | `f16f714b3b2b8e7b89c12487bc13e109d10b05038684e19f317186ff0c87d05b` |

These files and a complete manifest are preserved under
`E/frozen-linux-612115-46fb4/`. They are explicitly marked **UNQUALIFIED — NOT
FLASHABLE**. The effective config hash differs because its version comment
changed; its actual option/value set is identical to the previous common build.

The enforced command was run normally, without an ABI update mutator:

```sh
tools/bazel --output_user_root="$E/bazel" run --jobs=4 \
    //common:kernel_aarch64_abi_dist -- --destdir="$E/linux-612115-common-abi-dist"
```

Its build dependencies succeeded; the enforced comparison then exited **4**.
The parser worked. The result is not the old missing-libcap-input failure and
not simply the previously reviewed 38-difference exception. This command also
generated generic GKI boot/system_dlkm outputs as dependencies. Those are
**not ColorOS deployment artifacts and must not be flashed**. No firmware-bound
ColorOS boot image, matched DLKM package, or new parent VBMeta is prepared.

## Installed-provider contract failure

The oracle used the actual packaged/read-back population from the existing
canary, not the generic rebuilt Canoe modules. The four module hashes were
reverified against the installed-canary manifest before auditing:

- RMNET: `0ad99c698fcaffdf0acd2656b8e4c4a113ec41c173828f4dec40ea25e6d5e287`.
- QCE: `eea2b5252aa6838b1091af37fa901827254cf33e7707cd5113d7eaa0aead5f62`.
- QCrypto: `6606b6a4bcb9818271015f66658c201020a1a9761bbb37554020ad8e65fa0bfb`.
- WCD9xxx: `5e10de199ea0eb39cde0d4c771995851f9dfc6c284eb8a0b98fc595cbfe5bec4`.

The .115 source still exports 10,163 vmlinux symbols, with zero names added or
removed and zero export-class/namespace changes. However, **6,598 export CRC
records differ** from the .114 provider. This is not explained away as a
version-string difference. In particular:

```text
module_layout qualified: 0x797f2b3e
module_layout .115:      0x505b4886
```

Every one of the 1,070 module records disagrees with the new module_layout CRC.
The complete oracle and separate vmlinux-first provider validator report:

```text
module records checked:       1,070
active compatible:                0
active blockers:              1,032
dormant records:                 38
undefined ELF imports:        60,707
hard required imports:        60,701
weak imports:                     6
total CRC records:            61,777
CRC matches:                  35,606
CRC mismatches:               26,171
missing providers:                0
namespace failures:               0
production-provider mismatches: 26,171
```

The retained modules are not silently replaced, force-loaded, re-signed to
bypass policy, or matched against a convenient non-runtime SDK provider.
All oracle failures remain failures. No Module.symvers or CRC table was edited.

## New root groups requiring KABI preservation/review

Comparison against the **actual previously built .114 ABI**, rather than only
the older checked-in GKI reference, isolates seven new source root groups.
They produce ten C type changes and seven dependent Rust-bindgen type changes,
in addition to the BSG function-argument change. The thousands of propagated
CRC differences are not thousands of independent prototype changes.

| Group | Exact first stable commit | Compiled change / issue |
|---|---|---|
| Shared-MM futex identity | `16cc0fee39dffd9994b7bea676a886ed52206754` | mm_struct stays 1,216 bytes but 39 later members move by 8 bytes; futex_key renames the same-size union identity member. The new unique-ID security semantics must remain. |
| Perf per-user accounting | `7b5023f129af51bb0b487b29c79aea3322309ea5` | user_struct grows 144→152 bytes and later members move by 8 bytes. Review reserve consumption, not counter removal. |
| Address-independent keyring indexing | `13fc051269d670810d9e314061b61c6dd06116a0` | key grows 240→256, key_type 184→192, key_tag 24→32, and keyring_index_key 40→56 bytes. Cached opaque IDs and address-disclosure protection must remain. |
| Regcache population state | `5e43756123197e7bca4920a47d20d9eff0e203c3` | Private regcache_ops grows 72→80 bytes. Audit actual ownership/consumers before selecting a private-state or representation-compatible adaptation. |
| Deferred Unicode teardown | `69a0d6b31d05e856c7335509222f25321e3c69ca` | unicode_map grows 32→48 bytes. Preserve deferred RCU lifetime and prove allocation ownership. |
| io_uring deferred polling wakeups | `d8c618747a6aa90022e19ec614aa7157d0302108` | io_ring_ctx stays 1,536 bytes but new RCU state changes internal member offsets; same total size is not a layout proof. |
| UFS BSG reply sizing | `bfb0582708b3432919a8baaa3a65de5fdb1490ea` | bsg_setup_queue changes from six to seven arguments; its CRC becomes 0x4b303b9e instead of 0xf6c636b8. No installed module directly imports this API, but maintained GKI compatibility still requires review. Retain the advanced-RPMB overflow correction. |

This does not automatically classify every private type change as a genuine
stock runtime layout conflict. It does prove that the unadapted source is
not compatible with the installed module version contract. The next repair
must work at the originating roots, use runtime-safe ownership/layout/API
preservation, and naturally regenerate compatible CRCs. Updating the ABI
reference alone cannot make these stock modules load.

Control layouts confirm the previously repaired DWC3 remains **1,488 bytes**,
and UFS HBA remains **5,368 bytes**, with identical expanded layouts. Neither
is blindly reverted or re-repaired as a cause of this new failure.

Complete evidence includes `linux-612115-private-layouts.json`,
`linux-612115-vmlinux-export-delta.tsv` (all 6,598 changed records),
`linux-612115-versus-114-abi.short/.flat`, the enforced ABI reports, and the
full module-contract/provider reports under `E`. Host regression tests cover
the actual extracted futex-ID, page-walk reset and restart-error helpers with
ASan/UBSan and negative controls; these do not prove ARM memory ordering,
module ABI, or device runtime behavior.

## Handoff

**BLOCKED FOR PACKAGING/FLASHING — .115 KABI compatibility preservation is
required before any physical canary.** No stable fix is reverted, no validation
criterion is relaxed, and no ABI reference is updated. The complete .115
source history and raw artifacts are preserved for the next focused repair.
Production Canoe and matched-driver compilation should follow correction of
these module-boundary failures, not produce an apparently installable package
around a known incompatible core.

## Focused KABI repair — input and distribution audit (2026-10-31)

This section records the subsequent repair work; it does **not** change the
failed raw-.115 results above into a pass. Evidence is preserved separately at:

`/home/travis/Android/kernel-test-backups/coloros17-612115-kabi-repair-20261031-trcyzT/`

### Reference integrity

The frozen `79c418a771a8717edb4a3ed2b814fb9e89ec4a7c` .114 vmlinux export
map is byte-identical to the physically qualified .114 export map, SHA256
`fa6d325e5e5ce232ca7622176829ce7c4bcf7cf7ea47070bf6f48b265da02efb`.
This is not a comparison against an unrelated intermediate SDK.

The read-only audit found:

- zero effective CONFIG-value differences;
- identical Clang, LLD and Rust compiler binary hashes;
- no production build-script, SoC target, manifest, KMI-list or ABI-reference
  changes in the .114→.115 integration;
- identical `common/gki` input tree
  `df04beac18d0fa02d78ad76efcdba0c7b2326f14`;
- unchanged MODVERSIONS, GENDWARFKSYMS stable processing, signing, protected
  exports, KCFI and established LTO settings;
- the same exact 1,070-location installed-canary module population, including
  the four already qualified replacement modules, with a full file-hash
  inventory (`actual-module-population.tsv`);
- a verified .114 SDK manifest: no hand-edited symbol-version table is used.

The 15 generated-header differences are version/build-path metadata plus the
real MM offset change: `MM_CONTEXT_ID` is 968 in .114 and 976 in raw .115.
The generated CONFIG header differs only in its version comment. Environment
differences are the source-derived timestamp/epoch and the explicitly enabled
`KBUILD_SYMTYPES=1` diagnostic output; compiler flags are unchanged.

An unchanged-source .115 control rebuild used the documented Kleaf local
cache mode, with no Kconfig or compiler-security change:

```sh
tools/bazel --output_user_root="$E/bazel" build --jobs=4 \
  --config=local --cache_dir="$R/kbuild-cache" --kbuild_symtypes \
  //common:kernel_aarch64 //common:kernel_aarch64_abi_dump
```

It completed successfully in 1,392.574 seconds. Both symbol-version files
and both ABI dumps are byte-identical to the original raw-.115 build:
`Module.symvers`, `vmlinux.symvers`, `abi.stg`, and `abi-full.stg`.
Thus the type-output/cache execution mode does not explain or repair the
CRC failures. The control outputs are frozen under `control-unchanged-115/`.

### Exact mismatch distribution

There are **2,817 unique imported symbols** behind the **26,171 failing CRC
records**. `module_layout` accounts for 1,070 records, including the 38
dormant locations; all 1,032 active records remain blocked in raw .115.
The corresponding 6,598 changed vmlinux exports include exports not imported
by this firmware and must not be confused with the 2,817-import count.

All 2,817 are attributable to the reviewed roots in the actual per-translation-
unit GENDWARFKSYMS type expansions. The pruned STG alone omitted some symbol
type information; the complete symtypes audit leaves **zero unattributed
mismatched symbols**. Exact symbol, consumer, provider, required/generated
CRC and originating symtypes file records are retained in the evidence.

| Type-root dependency cohort | Unique imported symbols | CRC mismatch records |
|---|---:|---:|
| MM + keyring | 13 | 157 |
| MM + keyring + perf | 2 | 11 |
| MM + keyring + perf + Unicode | 2,743 | 25,420 |
| MM + keyring + perf + Unicode + io_uring | 24 | 196 |
| MM + keyring + perf + Unicode + regcache | 33 | 383 |
| Keyring alone | 2 | 4 |
| **Total** | **2,817** | **26,171** |

These are disjoint cohorts, not sums of overlapping root reachability.
BSG has no installed direct importer but still has an independently changed
maintained function prototype. Repairing one root may leave a CRC different
because another root remains in the same type graph.

Actual BTF/DWARF controls confirm `struct module` (1,600 bytes), `task_struct`
(5,184) and `net_device` (2,688) have unchanged existing member offsets and
sizes. The `module_layout` failure is propagated type-graph fallout, not a
change in the module object's physical layout. This does not excuse the real
MM/key/accounting member-offset changes.

### Incremental repair ledger

Work is one root at a time. No ABI reference, KMI symbol list, CRC table,
installed partition or qualified artifact is modified by this repair phase.

| Root | Proposed preservation | Current status |
|---|---|---|
| Futex MM identity | Place the atomic identity in proven existing tail padding; retain unique-ID initialization/publication and same-width futex identity semantics. | Not applied; compiled layout/CRC proof required. |
| Perf accounting | Consume a compatible existing user_struct reserve for the independent perf counter. | Not applied. |
| Keyring identities | Preserve public prefixes with core-owned private identity state and cached IDs; retain SipHash/address-independent ordering and unregister-safe lifetime. | Ownership/consumer audit in progress; no source change. |
| Regcache population | Keep the existing operations table intact; hold the optional populate callback in a core-private backend registry. | Compiled root proof PASS; commit `410359e2ca1107485c017f09f23d98c932615e2b`. |
| Unicode RCU | Keep the public map unchanged inside an allocation-owned private RCU wrapper. | Compiled root proof PASS; commit `173b52b15d776`; full module gate remains failed through other roots. |
| io_uring polling RCU | Use proven tail padding without moving existing fields; retain reference-held deferred wakeup and pending-state synchronization. | Compiled root proof PASS; commit `238ad2a9f8cbf`; overall module gate still FAIL. |
| BSG reply sizing | Preserve the six-argument entry point with a correctly typed wrapper, and use a sized helper for the 104-byte UFS RPMB reply. | Compiled argument/KCFI proof PASS; commit `4d57854484314513f8b687b909279cbeef265174`; final shared-type CRC remains pending. |

The first regcache adaptation leaves cache init/exit outside the map lock and
population inside it, retaining the .115 stable lock-order corrections.
All three backend objects are core-owned; no backend currently initializes
the optional callback. The maple IRQ-safe teardown is untouched. No callback
cast, allocation-size deception, version override or source rollback is used.
The extracted-C ASan/UBSan tests exercise a test-only populated backend,
initialization failure and population failure; these are not physical tests.

Regcache's normal rebuild/dump completed in 126.239 seconds, with strict KMI
symbol-list checking PASS. Actual BTF and DWARF now reproduce the qualified
`regcache_ops` (72 bytes) and `regmap` (688 bytes) exactly, with zero existing
member offset/size changes. All 34 emitted AArch64 KCFI type words match;
the same eight direct-only helpers have no type word in either build.
The helper preceding-instruction bytes are not misidentified as KCFI data.
All 146 host tests pass, including the five new extracted-C regressions.

The full production-precedence oracle remains **FAIL: 2,817 unique symbols,
26,171 mismatch records**, with zero missing providers or namespace failures.
The regcache cohort still reaches the unrepaired MM/key/perf/Unicode graph;
an unchanged total does not invalidate the restored leaf, nor does restoring
the leaf qualify the kernel. The frozen source patch SHA256 is
`634674340da2e2b385f97f8693906d893b525e70f46cadf2dc6904786c342b21`,
which equals the common-source diff recorded by commit `410359e2ca110`.

BSG's compatibility wrapper preserves the exact qualified six-argument
declaration and forwards to the correctly typed sized implementation with
`SCSI_SENSE_BUFFERSIZE` (96). UFS alone requests its actual 104-byte
`ufs_rpmb_reply`. The sized allocation and per-job advertised reply length
remain intact. The new host tests cover argument forwarding, both buffer
sizes, and an ASan-detected deliberately undersized allocation negative
control. No function-pointer cast or forced version is used.

BSG's common build/strict symbol-list check/ABI dump completed successfully
in 159.572 seconds. Its six-argument type expansion and KCFI ID `0x99ebd488`
match the qualified function contract; the new sized helper is not retained
as an additional production export. With the other roots still present,
the public function's natural CRC is `0x4be95c53`, not yet the qualified
`0xf6c636b8`. The module oracle remains FAIL at 2,817 symbols / 26,171
records, zero missing providers/namespaces. There is no stock BSG importer;
this repair fixes a maintained API, not a count-selected oracle exception.

Unicode's source adaptation retains a 48-byte core allocation: a 32-byte
public map prefix followed by the same 16-byte RCU state. The public map
declaration is restored verbatim. Allocation failure, setup failure,
NULL unload, delayed table release and base-pointer freeing are covered by
the four new focused host tests. Source ownership audit finds the map is
allocated by `utf8_load`; no caller-owned static/embedded map is passed to
the unloading path. Its normal common rebuild/strict symbol-list check/ABI
dump completed in 160.055 seconds. Actual BTF and DWARF reproduce the old
32-byte map and every old member offset exactly. The private 48-byte wrapper
contains that map at offset zero and the 16-byte callback head at offset 32.
All ten pre-existing UTF8 KCFI words match; the additional internal RCU
callback is the required stable lifetime fix. All 154 host tests pass.
The complete oracle remains FAIL at 2,817 symbols / 26,171 records, with
zero missing providers and namespace failures. No count-selected module
replacement or oracle exception is introduced.

io_uring's deferred-wakeup functions are unchanged byte-for-byte in source.
Only the two new fields move into the established tail padding using
`ANDROID_KABI_IGNORE`, without a byte-size override. The new extracted-C
tests verify concurrent coalescing, reference-held deferred wakeups, repeat
wakeups, and detection of a deliberately omitted lifetime reference. These
are host control-flow tests, not an RCU weak-memory or device proof. The
normal common build/strict symbol-list check/ABI dump completed successfully
in 168.179 seconds. Actual BTF confirms size 1,536, zero existing member
offset/size changes, the RCU head at byte 1,512 and pending atomic at 1,528.
The two fields consume 20 of the old 24 padding bytes. Existing alignment
requirements are unchanged. All 344 emitted `io_` KCFI type words match the
unadapted .115 implementation, with identical direct-only function sets.
The oracle still fails at 2,817 symbols / 26,171 records, zero missing
providers and namespace failures; MM, perf and keyring roots remain.

**io_uring provenance correction:** the description of the newly created
`238ad2a9f8cbf` source commit mistakenly gives
`30982202276ac3007acb1ea221545389391702f3` as the upstream identity.
The authoritative original commit body in local import
`d97c442fb26f4a4482de70b19af4f661eb886dd3` identifies upstream
**`b88902d8745a701d24275d012f0830e3628e2a22`**, with stable backport
`d8c618747a6aa90022e19ec614aa7157d0302108`. This correction supersedes
that one description field. The stable SHA and implementation were correct;
no source or preserved history is rewritten to correct the description.

**Overall KABI/module compatibility remains FAIL pending the incremental
compiled/oracle results. GPU modernization remains mandatory and quarantined
until its remaining provider/GEAS contracts are proven. No .115 release
candidate images or physical tests are authorized by this report.**
